#include "tower.h"
#include "projectile.h"
#include "util.h"
#include <math.h>

int FindNearestEnemy(Game *g, Vector2 pos, float range) {
    float minDist = range + 1.0f;
    int bestIdx = -1;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g->enemies[i].active)
            continue;
        /* T70 — Fogdaki dusmanlara vurulamaz */
        {
            int ec, er;
            if (WorldToGrid(g->enemies[i].position, &ec, &er) && !g->fogVisible[er][ec])
                continue;
        }
        float d = Vec2Distance(pos, g->enemies[i].position);
        if (d <= range && d < minDist) {
            minDist = d;
            bestIdx = i;
        }
    }
    return bestIdx;
}

/* ============================================================
 * T12 — UpdateEnemies
 * ============================================================ */

/* Her frame düşmanları hareket ettirir, yavaşlama efektini uygular,
 * waypoint geçişini ve son waypoint'e ulaşmayı kontrol eder. */

void UpdateTowers(Game *g, float dt) {
    for (int i = 0; i < MAX_TOWERS; i++) {
        Tower *t = &g->towers[i];
        if (!t->active)
            continue;

        /* B04: flash zamanlayıcısını azalt */
        if (t->flashTimer > 0.0f) {
            t->flashTimer -= dt;
            if (t->flashTimer < 0.0f)
                t->flashTimer = 0.0f;
        }

        /* Hedef seç */
        if (t->targetEnemyIndex >= 0) {
            Enemy *e = &g->enemies[t->targetEnemyIndex];
            if (!e->active || Vec2Distance(t->position, e->position) > t->range)
                t->targetEnemyIndex = -1;
        }
        if (t->targetEnemyIndex < 0)
            t->targetEnemyIndex = FindNearestEnemy(g, t->position, t->range);

        /* Ateş et */
        t->fireCooldown -= dt;
        if (t->fireCooldown <= 0.0f && t->targetEnemyIndex >= 0) {
            /* Namlu yönünü hedefe çevir */
            Vector2 toTarget = Vec2Subtract(g->enemies[t->targetEnemyIndex].position, t->position);
            t->rotation = atan2f(toTarget.y, toTarget.x) * RAD2DEG;
            CreateProjectile(g, t->position, t->targetEnemyIndex, t->damage, t->splashRadius,
                             t->type);
            t->fireCooldown = 1.0f / t->fireRate;
            t->flashTimer = 0.1f; /* B04 */
        }
    }
}

/* ============================================================
 * T21 — UpdateProjectiles
 * ============================================================ */

/* Mermileri hedeflerine doğru hareket ettirir, isabet ve splash hasarı uygular. */

static void ApplySynergyPair(Game *g, int a, int b) {
    Tower *ta = &g->towers[a];
    Tower *tb = &g->towers[b];

    /* Küçük tipten büyüğe sıralanmış çift anahtarı */
    TowerType lo = (ta->type <= tb->type) ? ta->type : tb->type;
    TowerType hi = (ta->type <= tb->type) ? tb->type : ta->type;

    if (lo == TOWER_BASIC && hi == TOWER_BASIC) {
        ta->fireRate *= 1.15f;
        tb->fireRate *= 1.15f;
    } else if (lo == TOWER_BASIC && hi == TOWER_SNIPER) {
        /* Sniper'a +10% hasar */
        Tower *sn = (ta->type == TOWER_SNIPER) ? ta : tb;
        sn->damage *= 1.10f;
    } else if (lo == TOWER_BASIC && hi == TOWER_SPLASH) {
        /* Splash yarıçapı +20% */
        Tower *sp = (ta->type == TOWER_SPLASH) ? ta : tb;
        sp->splashRadius *= 1.20f;
    } else if (lo == TOWER_SNIPER && hi == TOWER_SNIPER) {
        ta->range *= 1.15f;
        tb->range *= 1.15f;
    } else if (lo == TOWER_SNIPER && hi == TOWER_SPLASH) {
        Tower *sp = (ta->type == TOWER_SPLASH) ? ta : tb;
        Tower *sn = (ta->type == TOWER_SNIPER) ? ta : tb;
        sp->damage *= 1.20f;
        sn->fireRate *= 1.10f;
    } else if (lo == TOWER_SPLASH && hi == TOWER_SPLASH) {
        ta->splashRadius *= 1.25f;
        tb->splashRadius *= 1.25f;
    } else {
        return; /* Bilinmeyen çift — çifti kaydetme */
    }

    /* Çifti görsel çizim listesine ekle */
    if (g->synergyPairCount < MAX_SYNERGY_PAIRS) {
        g->synergyPairs[g->synergyPairCount].idxA = a;
        g->synergyPairs[g->synergyPairCount].idxB = b;
        g->synergyPairCount++;
    }
}

/* Yeni yerleştirilen kule için sinerji komşularını tarar ve bonusları uygular. */
void ApplySynergyBonuses(Game *g, int newIdx) {
    Tower *newT = &g->towers[newIdx];
    for (int i = 0; i < MAX_TOWERS; i++) {
        if (i == newIdx || !g->towers[i].active)
            continue;
        Tower *other = &g->towers[i];
        if (Vec2Distance(newT->position, other->position) <= SYNERGY_RADIUS) {
            /* Her çift yalnızca bir kez işlenmeli */
            bool alreadyPaired = false;
            for (int p = 0; p < g->synergyPairCount; p++) {
                SynergyPair *sp = &g->synergyPairs[p];
                if ((sp->idxA == i && sp->idxB == newIdx) ||
                    (sp->idxA == newIdx && sp->idxB == i)) {
                    alreadyPaired = true;
                    break;
                }
            }
            if (!alreadyPaired)
                ApplySynergyPair(g, newIdx, i);
        }
    }
}

/* Sinerji bağlantısı olan kule çiftleri arasında soluk parlayan çizgi çizer. */
void DrawTowerSynergies(Game *g) {
    float t = (float)GetTime();
    for (int i = 0; i < g->synergyPairCount; i++) {
        SynergyPair *sp = &g->synergyPairs[i];
        Tower *ta = &g->towers[sp->idxA];
        Tower *tb = &g->towers[sp->idxB];
        if (!ta->active || !tb->active)
            continue;
        /* Sinüs dalgası ile pulsing opaklık */
        float alpha = 0.18f + sinf(t * 2.0f + i * 0.8f) * 0.10f;
        Color lineCol = {72, 145, 220, (unsigned char)(alpha * 255)};
        DrawLineEx(ta->position, tb->position, 1.5f, lineCol);
        /* Uç noktalarda küçük daireler */
        DrawCircleLines((int)ta->position.x, (int)ta->position.y, (float)CELL_SIZE * 0.55f,
                        lineCol);
        DrawCircleLines((int)tb->position.x, (int)tb->position.y, (float)CELL_SIZE * 0.55f,
                        lineCol);
    }
}

/* ============================================================
 * T63 — UpdateGameCamera
 * ============================================================ */

/* T69 — Kamera: WASD pan, kenar kaydırma, zoom, sınır kontrolü */

    void DrawTowers(Game * g) {
        for (int i = 0; i < MAX_TOWERS; i++) {
            Tower *t = &g->towers[i];
            if (!t->active)
                continue;

            float sizeScale = 1.5f + 0.3f * t->level;
            float hw = (float)ISO_HALF_W * sizeScale;
            float hh = (float)ISO_HALF_H * sizeScale;
            float towerH = hh * 4.5f;
                int px = (int)t->position.x;

                Color c = t->color;
                c.r = (unsigned char)(c.r * (0.7f + 0.1f * t->level));
                c.g = (unsigned char)(c.g * (0.7f + 0.1f * t->level));
                c.b = (unsigned char)(c.b * (0.7f + 0.1f * t->level));

                /* Zemin diamond (kule tabanı) */
                Vector2 dTop = {t->position.x, t->position.y - hh};
                Vector2 dRght = {t->position.x + hw, t->position.y};
                Vector2 dBot = {t->position.x, t->position.y + hh};
                Vector2 dLeft = {t->position.x - hw, t->position.y};
                Color baseC = c;
                baseC.r = (unsigned char)(baseC.r * 0.6f);
                baseC.g = (unsigned char)(baseC.g * 0.6f);
                baseC.b = (unsigned char)(baseC.b * 0.6f);
                DrawTriangle(dTop, dLeft, dRght, baseC);
                DrawTriangle(dRght, dLeft, dBot, baseC);

                /* Kule gövdesi — iso silindir: ön dikdörtgen + üst diamond */
                float tw = hw * 0.55f;
                Vector2 bodyBL = {t->position.x - tw, t->position.y};
                Vector2 bodyBR = {t->position.x + tw, t->position.y};
                Vector2 bodyTL = {t->position.x - tw, t->position.y - towerH};
                Vector2 bodyTR = {t->position.x + tw, t->position.y - towerH};
                DrawTriangle(bodyTL, bodyBL, bodyBR, c);
                DrawTriangle(bodyTL, bodyBR, bodyTR, c);
                DrawLineV(bodyTL, bodyTR, WHITE);
                DrawLineV(bodyBL, bodyTL, WHITE);
                DrawLineV(bodyBR, bodyTR, WHITE);

                /* Namlu */
                float angle = t->rotation * DEG2RAD;
                float barLen = tw * 2.8f;
                Vector2 barBase = {t->position.x, t->position.y - towerH * 0.6f};
                Vector2 barTip = {barBase.x + cosf(angle) * barLen,
                                  barBase.y + sinf(angle) * barLen};
                DrawLineEx(barBase, barTip, 2.5f, WHITE);

                /* Seviye noktaları */
                for (int lv = 0; lv < t->level; lv++)
                    DrawCircle(px - (int)tw + 3 + lv * 6, (int)(t->position.y - towerH + 4), 2,
                               YELLOW);

                /* B04: ateş flash */
                if (t->flashTimer > 0.0f) {
                    float alpha = t->flashTimer / 0.1f;
                    DrawCircle(px, (int)(t->position.y - towerH * 0.5f), hw * 1.2f,
                               (Color){255, 255, 255, (unsigned char)(90 * alpha)});
                }
            }
        }

        /* ============================================================
         * T22 — DrawProjectiles
         * ============================================================ */
