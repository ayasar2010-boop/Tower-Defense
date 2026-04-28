#include "enemy.h"
#include "audio.h"
#include "hud.h"
#include "particle.h"
#include "util.h"
#include <math.h>
#include <string.h>

void SpawnEnemy(Game *g, EnemyType type, float hpMult) {
    if (hpMult <= 0.0f)
        hpMult = 1.0f;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g->enemies[i].active)
            continue;
        Enemy *e = &g->enemies[i];
        memset(e, 0, sizeof(Enemy));
        e->type = type;
        e->currentWaypoint = 1; /* Waypoints[0] spawn noktası, hedef [1]'den başlar */
        e->position = g->waypoints[0];
        e->slowFactor = 1.0f;
        e->active = true;
        e->isFlying = false;
        e->engagedFriendlyIdx = -1;
        switch (type) {
        case ENEMY_NORMAL:
            e->maxHp = ENEMY_BASE_HP * 1.0f * hpMult;
            e->speed = ENEMY_BASE_SPEED * 1.0f;
            e->radius = (float)ISO_HALF_W * 0.50f; /* ~6 */
            e->radius = (float)ISO_HALF_W * 0.35f; /* ~7px, Normal insan/yaratık */
            e->color = RED;
            break;
        case ENEMY_FAST:
            e->maxHp = ENEMY_BASE_HP * 0.6f * hpMult;
            e->speed = ENEMY_BASE_SPEED * 1.8f;
            e->radius = (float)ISO_HALF_W * 0.38f; /* ~4.5 */
            e->radius = (float)ISO_HALF_W * 0.25f; /* ~5px, Hızlı goblin boyutu */
            e->color = ORANGE;
            break;
        case ENEMY_TANK:
            e->maxHp = ENEMY_BASE_HP * 3.0f * hpMult;
            e->speed = ENEMY_BASE_SPEED * 0.6f;
            e->radius = (float)ISO_HALF_W * 0.70f; /* ~8.4 */
            e->radius = (float)ISO_HALF_W * 0.60f; /* ~12px, İri kıyım troll */
            e->color = DARKPURPLE;
            break;
        case ENEMY_BOSS:
            e->maxHp = ENEMY_BASE_HP * 20.0f * hpMult;
            e->speed = ENEMY_BASE_SPEED * 0.4f;
            e->radius = (float)ISO_HALF_W * 1.20f; /* ~14 */
            e->radius = (float)ISO_HALF_W * 1.50f; /* ~30px, Devasa Boss */
            e->color = MAROON;
            e->isBoss = true;
            e->bossPhase = 1;
            e->abilityTimer = 3.0f;
            e->isCasting = false;
            e->castTimer = 0.0f;
            e->targetAbilityPos = (Vector2){0, 0};
            break;
        default:
            break;
        }
        e->currentHp = e->maxHp;
        return;
    }
}

/* ============================================================
 * T20 — CreateProjectile
 * ============================================================ */

/* Boş mermi yuvası bulur ve verilen parameterlerle başlatır. */

void UpdateEnemies(Game *g, float dt) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g->enemies[i];
        if (!e->active)
            continue;

        /* T55 - Boss Yetenek ve Faz Mantığı */
        if (e->isBoss) {
            float hpRatio = e->currentHp / e->maxHp;
            if (e->bossPhase == 1 && hpRatio <= 0.6f) {
                e->bossPhase = 2;
                e->color = MAGENTA;
                TriggerScreenShake(g, 5.0f);
            } else if (e->bossPhase == 2 && hpRatio <= 0.25f) {
                e->bossPhase = 3;
                e->color = PINK;
                e->speed *= 1.5f; /* Phase 3: Çılgınlık */
                TriggerScreenShake(g, 8.0f);
            }

            if (e->isCasting) {
                e->castTimer -= dt;
                if (e->castTimer <= 0.0f) {
                    /* Yeteneği ateşle */
                    e->isCasting = false;
                    if (e->bossPhase == 1) {
                        /* Radyal Mermi */
                        for (int a = 0; a < 8; a++) {
                            float angle = a * 45.0f * DEG2RAD;
                            Vector2 dir = {cosf(angle), sinf(angle)};
                            for (int pidx = 0; pidx < MAX_PROJECTILES; pidx++) {
                                if (g->projectiles[pidx].active)
                                    continue;
                                Projectile *p = &g->projectiles[pidx];
                                p->position = e->position;
                                p->velocity = Vec2Scale(dir, 150.0f);
                                p->damage = 20.0f;
                                p->splashRadius = 0.0f;
                                p->speed = 150.0f;
                                p->targetIndex = -1;
                                p->sourceType = TOWER_BASIC; /* dummy */
                                p->color = MAGENTA;
                                p->active = true;
                                p->isEnemyProjectile = true;
                                p->trailIndex = 0;
                                for (int j = 0; j < TRAIL_LENGTH; j++)
                                    p->trailPositions[j] = p->position;
                                break;
                            }
                        }
                        e->abilityTimer = 4.0f;
                    } else if (e->bossPhase == 2) {
                        /* Meteor: targetAbilityPos noktasına patlama */
                        TriggerScreenShake(g, 6.0f);
                        SpawnParticles(g, e->targetAbilityPos, ORANGE, 30, 200.0f, 1.0f);
                        /* Kuleleri stunla */
                        for (int tidx = 0; tidx < MAX_TOWERS; tidx++) {
                            if (!g->towers[tidx].active)
                                continue;
                            if (Vec2Distance(g->towers[tidx].position, e->targetAbilityPos) <
                                100.0f) {
                                g->towers[tidx].fireCooldown += 3.0f; /* 3 sn stun */
                            }
                        }
                        /* Dost birimlere hasar */
                        for (int fidx = 0; fidx < MAX_FRIENDLY_UNITS; fidx++) {
                            if (!g->friendlyUnits[fidx].active)
                                continue;
                            if (Vec2Distance(g->friendlyUnits[fidx].position, e->targetAbilityPos) <
                                100.0f) {
                                g->friendlyUnits[fidx].hp -= 50.0f;
                            }
                        }
                        /* Hero'ya hasar (eğer sahadaysa) */
                        if (g->hero.alive &&
                            Vec2Distance(g->hero.position, e->targetAbilityPos) < 100.0f) {
                            g->hero.stats.hp -= 50.0f;
                        }
                        e->abilityTimer = 6.0f;
                    } else if (e->bossPhase == 3) {
                        /* Minion Çağırma */
                        SpawnEnemy(g, ENEMY_TANK, 0.6f);
                        SpawnEnemy(g, ENEMY_FAST, 1.0f);
                        SpawnEnemy(g, ENEMY_FAST, 1.0f);
                        e->abilityTimer = 4.0f;
                    }
                }
                /* Cast yaparken hareket etme */
                continue;
            } else {
                e->abilityTimer -= dt;
                if (e->abilityTimer <= 0.0f) {
                    e->isCasting = true;
                    if (e->bossPhase == 1) {
                        e->castTimer = 1.0f;
                    } else if (e->bossPhase == 2) {
                        e->castTimer = 1.5f;
                        /* Rastgele bir kule seç */
                        e->targetAbilityPos = e->position; /* default */
                        int activeTowers[MAX_TOWERS];
                        int tCount = 0;
                        for (int tidx = 0; tidx < MAX_TOWERS; tidx++) {
                            if (g->towers[tidx].active)
                                activeTowers[tCount++] = tidx;
                        }
                        if (tCount > 0) {
                            e->targetAbilityPos =
                                g->towers[activeTowers[GetRandomValue(0, tCount - 1)]].position;
                        } else if (g->hero.alive) {
                            e->targetAbilityPos = g->hero.position;
                        }
                    } else if (e->bossPhase == 3) {
                        e->castTimer = 0.8f;
                    }
                }
            }
        }

        /* Son waypoint gecildi: can kaybet */
        if (e->currentWaypoint >= g->waypointCount) {
            if (e->engagedFriendlyIdx >= 0) {
                g->friendlyUnits[e->engagedFriendlyIdx].engagedEnemyIdx = -1;
                e->engagedFriendlyIdx = -1;
            }
            g->lives--;
            if (g->lives < 0)
                g->lives = 0;
            e->active = false;
            PlaySFX(&g->audio, SFX_LIVE_LOST);
            continue;
        }

        /* Ucmayan dusman: dost birim bloklama kontrolu */
        if (!e->isFlying) {
            /* Mevcut engaged birim hala aktif mi? */
            if (e->engagedFriendlyIdx >= 0 && !g->friendlyUnits[e->engagedFriendlyIdx].active) {
                e->engagedFriendlyIdx = -1;
            }

            if (e->engagedFriendlyIdx >= 0) {
                /* Savasiyor: dur ve dost birime saldır */
                FriendlyUnit *fu = &g->friendlyUnits[e->engagedFriendlyIdx];
                fu->hp -= e->speed * 0.04f * dt; /* Hasar: hiza orantili */
                continue;                        /* Hareket etme */
            }

            /* Blok aramayi sadece enemy kendi waypoint'ine gidiyorken yap */
            /* Yakin dost birim bul — 1:1 kurali */
            int blockSlot = -1;
            float blockDist = FUNIT_BLOCK_DIST;
            for (int j = 0; j < MAX_FRIENDLY_UNITS; j++) {
                FriendlyUnit *fu = &g->friendlyUnits[j];
                if (!fu->active || fu->hp <= 0.0f)
                    continue;
                if (fu->engagedEnemyIdx != -1)
                    continue; /* Zaten mesgul */
                float d = Vec2Distance(e->position, fu->position);
                if (d < blockDist) {
                    blockDist = d;
                    blockSlot = j;
                }
            }
            if (blockSlot >= 0) {
                /* Engage: ikisi birbirini kilitler */
                e->engagedFriendlyIdx = blockSlot;
                g->friendlyUnits[blockSlot].engagedEnemyIdx = i;
                continue; /* Bu frame hareket etme */
            }
        }

        /* Normal hareket */
        Vector2 target = g->waypoints[e->currentWaypoint];
        e->direction = Vec2Normalize(Vec2Subtract(target, e->position));

        float effectiveSpeed = e->speed;
        if (e->slowTimer > 0.0f) {
            effectiveSpeed *= e->slowFactor;
            e->slowTimer -= dt;
            if (e->slowTimer < 0.0f)
                e->slowTimer = 0.0f;
        }

        e->position = Vec2Add(e->position, Vec2Scale(e->direction, effectiveSpeed * dt));

        if (Vec2Distance(e->position, target) < WAYPOINT_REACH_DIST)
            e->currentWaypoint++;
    }
}

/* ============================================================
 * T17 — UpdateTowers
 * ============================================================ */

/* Kulelerin ateş zamanlayıcısını günceller, hedef seçer ve mermi oluşturur. */

    void DrawEnemies(Game * g) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            Enemy *e = &g->enemies[i];
            if (!e->active)
                continue;

            /* T70 — Fogdaki dusmanlar cizilmez */
            {
                int ec, er;
                if (WorldToGrid(e->position, &ec, &er) && !g->fogVisible[er][ec])
                    continue;
            }

            /* Şekil: Normal=daire, Fast=üçgen, Tank=kare
             * TODO (sprite): DrawTextureRec(g->assets.enemyNormal/Fast/Tank,
             *                  frameRect, e->position, WHITE)
             * frameRect: e->animFrame * frameW ile yürüyüş animasyon karesi seçilir */
            switch (e->type) {
            case ENEMY_BOSS:
            case ENEMY_NORMAL:
                if (e->isBoss) {
                    /* Nabız efekti */
                    float pulse = 1.0f + sinf((float)GetTime() * 4.0f) * 0.2f;
                    if (e->isCasting) {
                        pulse += 0.5f; /* Büyü hazırlığı: ekstra parlama */
                        DrawCircleGradient((int)e->position.x, (int)e->position.y,
                                           e->radius * 2.0f * pulse, WHITE, BLANK);
                    } else {
                        DrawCircleGradient((int)e->position.x, (int)e->position.y,
                                           e->radius * 1.5f * pulse, e->color, BLANK);
                    }
                }
                DrawCircleV(e->position, e->radius, e->color);
                DrawCircleLines((int)e->position.x, (int)e->position.y, e->radius, WHITE);
                break;
            case ENEMY_FAST: {
                /* Küçük yukarı bakan üçgen */
                float r = e->radius;
                Vector2 v1 = {e->position.x, e->position.y - r};
                Vector2 v2 = {e->position.x - r, e->position.y + r};
                Vector2 v3 = {e->position.x + r, e->position.y + r};
                DrawTriangle(v1, v2, v3, e->color);
                break;
            }
            case ENEMY_TANK: {
                float r = e->radius;
                DrawRectangle((int)(e->position.x - r), (int)(e->position.y - r), (int)(r * 2),
                              (int)(r * 2), e->color);
                DrawRectangleLines((int)(e->position.x - r), (int)(e->position.y - r), (int)(r * 2),
                                   (int)(r * 2), WHITE);
                break;
            }
            default:
                break;
            }

            /* T55 — Boss yetenek görsel efektleri (Meteor uyarı dairesi vb.) */
            if (e->isBoss && e->isCasting && e->bossPhase == 2) {
                float blink = (int)(GetTime() * 10) % 2 == 0 ? 0.8f : 0.4f;
                DrawCircleLines((int)e->targetAbilityPos.x, (int)e->targetAbilityPos.y, 100.0f,
                                Fade(RED, blink));
                DrawCircle((int)e->targetAbilityPos.x, (int)e->targetAbilityPos.y, 100.0f,
                           Fade(RED, blink * 0.3f));
            }

            /* HP bar — sadece hasar almışsa göster (Boss ise HUD'a havâle et) */
            if (!e->isBoss && e->currentHp < e->maxHp) {
                if (!e->isBoss && e->currentHp < e->maxHp && g->camera.zoom > 1.0f) {
                    float barW = e->radius * 2.5f;
                    float barH = 4.0f;
                    float barX = e->position.x - barW / 2.0f;
                    float barY = e->position.y - e->radius - 8.0f;
                    float ratio = e->currentHp / e->maxHp;
                    DrawRectangle((int)barX, (int)barY, (int)barW, (int)barH, DARKGRAY);
                    Color hpColor =
                        (ratio > 0.5f)
                            ? (Color){(unsigned char)(255 * (1.0f - ratio) * 2), 200, 0, 255}
                            : (Color){220, (unsigned char)(200 * ratio * 2), 0, 255};
                    DrawRectangle((int)barX, (int)barY, (int)(barW * ratio), (int)barH, hpColor);
                }
            }
        }

    }

    /* ============================================================
     * T18 — DrawTowers
     * ============================================================ */

    /* Kuleleri kare govde + yonlendirilmis namlu cizgisi ile cizer. */