#include "hud.h"
#include "enemy.h"
#include "guardian.h"
#include "i18n.h"
#include "rarity.h"
#include "logger.h"
#include "map.h"
#include "particle.h"
#include "projectile.h"
#include "tower.h"
#include "ui.h"
#include "util.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void ResetGameplay(Game *g);

void TriggerScreenShake(Game *g, float intensity) {
    if (intensity > g->screenShake.intensity)
        g->screenShake.intensity = intensity;
    g->screenShake.decay = intensity * 6.0f;
}

/* Her frame sarsıntı ofseti güncellenir (rawDt ile çalışır — gameSpeed etkilemez). */
void UpdateScreenShake(Game *g, float rawDt) {
    ScreenShake *s = &g->screenShake;
    if (s->intensity <= 0.0f) {
        s->offsetX = 0.0f;
        s->offsetY = 0.0f;
        return;
    }
    s->intensity -= s->decay * rawDt;
    if (s->intensity < 0.0f)
        s->intensity = 0.0f;
    float f = s->intensity;
    /* Rastgele ofset: [-f, f] aralığında */
    s->offsetX = ((float)(GetRandomValue(-1000, 1000) / 1000.0f)) * f;
    s->offsetY = ((float)(GetRandomValue(-1000, 1000) / 1000.0f)) * f;
}

/* ============================================================
 * T57 — Floating Damage Numbers
 * ============================================================ */

/* Hasar / iyileşme sayısını ekranda uçarak gösteren FloatingText oluşturur. */

void SpawnFloatingText(Game *g, Vector2 pos, float value, bool isCrit, bool isHeal) {
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        if (g->floatingTexts[i].active)
            continue;
        FloatingText *ft = &g->floatingTexts[i];
        ft->position = pos;
        ft->value = value;
        ft->lifetime = 1.0f;
        ft->maxLifetime = 1.0f;
        ft->isCrit = isCrit;
        ft->active = true;
        if (isHeal)
            ft->color = (Color){50, 255, 120, 255};
        else if (isCrit)
            ft->color = (Color){255, 215, 0, 255};
        else
            ft->color = (Color){255, 255, 255, 255};
        break;
    }
}

/* FloatingText'leri yukarı kaydırır ve solar. */
void UpdateFloatingTexts(Game *g, float dt) {
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        FloatingText *ft = &g->floatingTexts[i];
        if (!ft->active)
            continue;
        ft->position.y -= 40.0f * dt;
        ft->lifetime -= dt;
        if (ft->lifetime <= 0.0f)
            ft->active = false;
    }
}

/* FloatingText'leri opaklık ve boyut artışıyla çizer. */
void DrawFloatingTexts(Game *g) {
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        FloatingText *ft = &g->floatingTexts[i];
        if (!ft->active)
            continue;
        float ratio = ft->lifetime / ft->maxLifetime;
        unsigned char alpha = (unsigned char)(255.0f * ratio);
        Color col = ft->color;
        col.a = alpha;
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f", ft->value);
        if (ft->isCrit) {
            /* Kritik: büyük + "!" */
            char cbuf[20];
            snprintf(cbuf, sizeof(cbuf), "%.0f!", ft->value);
            DrawText(cbuf, (int)ft->position.x - MeasureText(cbuf, 22) / 2, (int)ft->position.y, 22,
                     col);
        } else {
            DrawText(buf, (int)ft->position.x - MeasureText(buf, 16) / 2, (int)ft->position.y, 16,
                     col);
        }
    }
}

/* ============================================================
 * T59 — Tower Synergy System
 * ============================================================ */

/* İki kule arasındaki sinerji bonusunu tip çiftine göre uygular ve çifti kaydeder. */

void UpdateGameCamera(Game *g, float dt) {
    GameCamera *gc = &g->camera;
    float panSpeed = 400.0f / gc->zoom; /* Yakın zoomda yavaş, uzakta hızlı */

    /* WASD ile kamera kaydırma (STATE_DUNGEON dışında) */
    if (g->state == STATE_PLAYING || g->state == STATE_PREP_PHASE) {
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
            gc->cam.target.y -= panSpeed * dt;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
            gc->cam.target.y += panSpeed * dt;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
            gc->cam.target.x -= panSpeed * dt;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
            gc->cam.target.x += panSpeed * dt;
    }

    /* Ekran kenarına fare gelince kaydırma (20px bant) */
    Vector2 mp = GetMousePosition();
    float edgeBand = 20.0f;
    if (mp.x < edgeBand)
        gc->cam.target.x -= panSpeed * dt;
    if (mp.x > SCREEN_WIDTH - edgeBand)
        gc->cam.target.x += panSpeed * dt;
    if (mp.y < edgeBand)
        gc->cam.target.y -= panSpeed * dt;
    if (mp.y > SCREEN_HEIGHT - edgeBand)
        gc->cam.target.y += panSpeed * dt;

    /* Scroll ile zoom: 1.2× – 3.0× */
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        gc->zoom += wheel * 0.15f;
        if (gc->zoom < 1.2f)
            gc->zoom = 1.2f;
        if (gc->zoom > 3.0f)
            gc->zoom = 3.0f;
        gc->cam.zoom = gc->zoom;
    }

    /* Hero takibi */
    if (gc->heroFollow && g->hero.alive) {
        gc->cam.target = g->hero.position;
    }

    /* Kamera offset = ekran merkezi */
    gc->cam.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};

    /* Izometrik harita sinirlarina klample */
    float isoMinX = (float)(ISO_ORIGIN_X - GRID_ROWS * ISO_HALF_W);
    float isoMaxX = (float)(ISO_ORIGIN_X + GRID_COLS * ISO_HALF_W);
    float isoMinY = (float)ISO_ORIGIN_Y;
    float isoMaxY = (float)(ISO_ORIGIN_Y + (GRID_COLS + GRID_ROWS) * ISO_HALF_H);
    float halfViewW = (SCREEN_WIDTH / 2.0f) / gc->zoom;
    float halfViewH = (SCREEN_HEIGHT / 2.0f) / gc->zoom;
    if (gc->cam.target.x < isoMinX + halfViewW)
        gc->cam.target.x = isoMinX + halfViewW;
    if (gc->cam.target.y < isoMinY + halfViewH)
        gc->cam.target.y = isoMinY + halfViewH;
    if (gc->cam.target.x > isoMaxX - halfViewW)
        gc->cam.target.x = isoMaxX - halfViewW;
    if (gc->cam.target.y > isoMaxY - halfViewH)
        gc->cam.target.y = isoMaxY - halfViewH;
}

/* ============================================================
 * T27 — UpdateWaves
 * ============================================================ */

/* Dalganın spawn zamanlayıcısını yönetir, düşman üretir,
 * dalga bitişini ve geçiş durumunu kontrol eder. */

void InitFriendlyUnit(FriendlyUnit *fu, FUnitType type, Vector2 pos) {
    memset(fu, 0, sizeof(FriendlyUnit));
    fu->type = type;
    fu->order = FUNIT_HOLD;
    fu->position = pos;
    fu->active = true;
    fu->engagedEnemyIdx = -1;
    switch (type) {
    case FUNIT_ARCHER:
        fu->maxHp = 90;
        fu->hp = 90;
        fu->atk = 18;
        fu->attackRange = 120;
        fu->attackSpeed = 1.5f;
        break;
    case FUNIT_WARRIOR:
        fu->maxHp = 220;
        fu->hp = 220;
        fu->atk = 28;
        fu->attackRange = 36;
        fu->attackSpeed = 0.9f;
        break;
    case FUNIT_MAGE:
        fu->maxHp = 70;
        fu->hp = 70;
        fu->atk = 40;
        fu->attackRange = 110;
        fu->attackSpeed = 0.5f;
        break;
    case FUNIT_KNIGHT:
        fu->maxHp = 320;
        fu->hp = 320;
        fu->atk = 22;
        fu->attackRange = 44;
        fu->attackSpeed = 1.0f;
        break;
    case FUNIT_VILLAGER:
        fu->maxHp = 50;
        fu->hp = 50;
        fu->atk = 0;
        fu->attackRange = 0;
        fu->attackSpeed = 0;
        fu->vstate = VSTATE_IDLE;
        fu->outpostIdx = -1;
        break;
    case FUNIT_HERO:
    default:
        fu->maxHp = 250;
        fu->hp = 250;
        fu->atk = 30;
        fu->attackRange = 55;
        fu->attackSpeed = 1.2f;
        break;
    }
}

/* Hero sinifina gore path sonunda ozel birim olusturur */

void SpawnHeroUnit(Game *g) {
    /* Son waypointten 1 tile oncesi */
    int wc = g->waypointCount;
    if (wc < 2)
        return;
    Vector2 pos = g->waypoints[wc - 2]; /* son waypoint oncesi */

    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
        if (g->friendlyUnits[i].active)
            continue;
        InitFriendlyUnit(&g->friendlyUnits[i], FUNIT_HERO, pos);
        /* Hero sinifi bonuslari */
        if (g->hero.heroClass == HERO_WARRIOR) {
            g->friendlyUnits[i].maxHp = 400;
            g->friendlyUnits[i].hp = 400;
            g->friendlyUnits[i].atk = 42;
            g->friendlyUnits[i].attackRange = 44;
        } else if (g->hero.heroClass == HERO_MAGE) {
            g->friendlyUnits[i].maxHp = 180;
            g->friendlyUnits[i].hp = 180;
            g->friendlyUnits[i].atk = 60;
            g->friendlyUnits[i].attackRange = 120;
        } else { /* ARCHER */
            g->friendlyUnits[i].maxHp = 220;
            g->friendlyUnits[i].hp = 220;
            g->friendlyUnits[i].atk = 30;
            g->friendlyUnits[i].attackRange = 100;
        }
        break;
    }
}

static void UpdateVillagers(Game *g, float dt); /* ileri bildirim */

/* Dost birimleri guncelle: hold/attack AI + engaged takibi */

void UpdateFriendlyUnits(Game *g, float dt) {
    /* T83 — köylüler ayrı FSM'den güncellenir */
    UpdateVillagers(g, dt);

    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
        FriendlyUnit *fu = &g->friendlyUnits[i];
        if (!fu->active)
            continue;
        /* Köylüler kendi FSM'inden işlendi */
        if (fu->type == FUNIT_VILLAGER) continue;

        /* Oldu mu? */
        if (fu->hp <= 0.0f) {
            /* Engaged enemy'yi serbest birak */
            if (fu->engagedEnemyIdx >= 0) {
                g->enemies[fu->engagedEnemyIdx].engagedFriendlyIdx = -1;
                fu->engagedEnemyIdx = -1;
            }
            fu->active = false;
            continue;
        }

        /* Engaged enemy hala aktif mi? */
        if (fu->engagedEnemyIdx >= 0 && !g->enemies[fu->engagedEnemyIdx].active)
            fu->engagedEnemyIdx = -1;

        /* En yakin aktif dusmani bul */
        int bestIdx = -1;
        float bestDist = fu->attackRange;
        for (int j = 0; j < MAX_ENEMIES; j++) {
            Enemy *e = &g->enemies[j];
            if (!e->active || e->isFlying)
                continue;
            float d = Vec2Distance(fu->position, e->position);
            if (d < bestDist) {
                bestDist = d;
                bestIdx = j;
            }
        }

        /* MOVE modu: hedef noktaya yuru, varinca HOLD'a don */
        if (fu->order == FUNIT_MOVE) {
            float dist = Vec2Distance(fu->position, fu->moveTarget);
            if (dist < 6.0f) {
                fu->order = FUNIT_HOLD;
            } else {
                Vector2 dir = Vec2Normalize(Vec2Subtract(fu->moveTarget, fu->position));
                float spd = 100.0f;
                fu->position.x += dir.x * spd * dt;
                fu->position.y += dir.y * spd * dt;
            }
        }

        /* T85 — PATROL modu: moveTarget ↔ patrolTarget arasında gidip gel */
        if (fu->order == FUNIT_PATROL) {
            Vector2 dest = fu->patrolReturn ? fu->moveTarget : fu->patrolTarget;
            float dist = Vec2Distance(fu->position, dest);
            if (dist < 8.0f) {
                fu->patrolReturn = !fu->patrolReturn;
            } else {
                Vector2 dir = Vec2Normalize(Vec2Subtract(dest, fu->position));
                fu->position.x += dir.x * 80.0f * dt;
                fu->position.y += dir.y * 80.0f * dt;
            }
        }

        /* ATTACK modu: hedefe dog_ru yuru */
        if (fu->order == FUNIT_ATTACK && bestIdx < 0) {
            /* Menzil disindaki en yakin dusmana yuru */
            float walkDist = 9999;
            int walkIdx = -1;
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (!g->enemies[j].active || g->enemies[j].isFlying)
                    continue;
                float d = Vec2Distance(fu->position, g->enemies[j].position);
                if (d < walkDist) {
                    walkDist = d;
                    walkIdx = j;
                }
            }
            if (walkIdx >= 0) {
                Vector2 dir =
                    Vec2Normalize(Vec2Subtract(g->enemies[walkIdx].position, fu->position));
                fu->position.x += dir.x * 90.0f * dt;
                fu->position.y += dir.y * 90.0f * dt;
            }
        }

        /* Saldiri */
        if (fu->attackCooldown > 0.0f)
            fu->attackCooldown -= dt;
        if (bestIdx >= 0 && fu->attackCooldown <= 0.0f) {
            fu->attackCooldown = 1.0f / fu->attackSpeed;
            g->enemies[bestIdx].currentHp -= fu->atk;
            if (g->enemies[bestIdx].currentHp <= 0.0f) {
                g->enemies[bestIdx].active = false;
                /* Engaged iliskisini temizle */
                if (fu->engagedEnemyIdx == bestIdx)
                    fu->engagedEnemyIdx = -1;
                g->gold += KILL_REWARD;
                g->score += 10;
                g->enemiesKilled++;
                EarnProsperity(&g->homeCity, 5);
            }
        }
    }
}

/* T83 — Köylü FSM güncelle */
static void UpdateVillagers(Game *g, float dt) {
    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
        FriendlyUnit *v = &g->friendlyUnits[i];
        if (!v->active || v->type != FUNIT_VILLAGER) continue;
        if (v->hp <= 0.0f) { v->active = false; continue; }

        switch (v->vstate) {
        case VSTATE_IDLE: {
            /* Boş outpost slot ara; yoksa en yakın RURAL hücreye git */
            int slot = -1;
            for (int k = 0; k < MAX_OUTPOSTS; k++) {
                if (!g->outposts[k].active) { slot = k; break; }
            }
            if (slot < 0) break; /* tüm slotlar dolu */
            /* Rastgele bir CELL_RURAL hedef seç */
            for (int r = 0; r < GRID_ROWS; r++) {
                for (int c = 0; c < GRID_COLS; c++) {
                    if (g->grid[r][c] == CELL_RURAL) {
                        v->nodePos = GridToWorld(c, r);
                        v->outpostIdx = slot;
                        v->vstate = VSTATE_MOVE_TO_NODE;
                        goto next_villager;
                    }
                }
            }
            break;
        }
        case VSTATE_MOVE_TO_NODE: {
            float d = Vec2Distance(v->position, v->nodePos);
            if (d < 8.0f) {
                v->vstate = VSTATE_BUILD_CAMP;
                v->gatherTimer = 3.0f; /* inşa süresi */
            } else {
                Vector2 dir = Vec2Normalize(Vec2Subtract(v->nodePos, v->position));
                v->position.x += dir.x * 70.0f * dt;
                v->position.y += dir.y * 70.0f * dt;
            }
            break;
        }
        case VSTATE_BUILD_CAMP: {
            v->gatherTimer -= dt;
            if (v->gatherTimer <= 0.0f && v->outpostIdx >= 0) {
                Outpost *op = &g->outposts[v->outpostIdx];
                op->position = v->nodePos;
                op->level    = 1;
                op->resources = 0.0f;
                op->active   = true;
                if (v->outpostIdx >= g->outpostCount)
                    g->outpostCount = v->outpostIdx + 1;
                v->vstate = VSTATE_GATHERING;
                v->gatherTimer = 0.0f;
            }
            break;
        }
        case VSTATE_GATHERING: {
            v->gatherTimer += dt;
            if (v->outpostIdx >= 0)
                g->outposts[v->outpostIdx].resources += 8.0f * dt;
            if (v->gatherTimer >= 6.0f) { /* 6 saniye topla */
                v->resourceCarried = 50.0f;
                /* Köye dön: ilk waypoint'e git */
                if (g->waypointCount > 0)
                    v->moveTarget = g->waypoints[0];
                v->vstate = VSTATE_TRANSPORT;
            }
            break;
        }
        case VSTATE_TRANSPORT: {
            float d = Vec2Distance(v->position, v->moveTarget);
            if (d < 12.0f) {
                /* Köye teslim et */
                g->gold += (int)(v->resourceCarried);
                v->resourceCarried = 0.0f;
                v->vstate = VSTATE_IDLE;
                v->gatherTimer = 0.0f;
            } else {
                Vector2 dir = Vec2Normalize(Vec2Subtract(v->moveTarget, v->position));
                v->position.x += dir.x * 80.0f * dt;
                v->position.y += dir.y * 80.0f * dt;
            }
            break;
        }
        }
        next_villager:;
    }
}

/* T85 — Bina güncelle: Auto-Train (Kışlak) */
void UpdateBuildings(Game *g, float dt) {
    for (int i = 0; i < g->buildingCount; i++) {
        Building *b = &g->buildings[i];
        if (!b->active || b->type != BUILDING_BARRACKS || !b->autoTrain) continue;
        b->trainCooldown -= dt;
        if (b->trainCooldown <= 0.0f) {
            int cost = 50;
            if (g->gold < cost) continue;
            /* Boş friendly unit slotu ara */
            for (int j = 0; j < MAX_FRIENDLY_UNITS; j++) {
                if (g->friendlyUnits[j].active) continue;
                g->gold -= cost;
                Vector2 bpos = GridToWorld(b->gridX, b->gridY);
                InitFriendlyUnit(&g->friendlyUnits[j], FUNIT_WARRIOR, bpos);
                g->friendlyUnits[j].order = FUNIT_PATROL;
                g->friendlyUnits[j].patrolTarget = b->rallyPoint;
                g->friendlyUnits[j].patrolReturn  = false;
                b->trainCooldown = 15.0f;
                break;
            }
        }
    }
}

/* T83 — Outpost'ları çiz */
void DrawOutposts(Game *g) {
    for (int i = 0; i < g->outpostCount; i++) {
        Outpost *op = &g->outposts[i];
        if (!op->active) continue;
        Color c = (op->level >= 1) ? (Color){180, 130, 60, 220} : (Color){120, 100, 60, 160};
        DrawRectangle((int)op->position.x - 10, (int)op->position.y - 10, 20, 20, c);
        DrawRectangleLinesEx((Rectangle){op->position.x - 10, op->position.y - 10, 20, 20},
                             1.5f, (Color){220, 180, 80, 255});
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f", op->resources);
        DrawText(buf, (int)op->position.x - 8, (int)op->position.y - 20, 10,
                 (Color){255, 240, 140, 220});
    }
}

/* Dost birimleri ciz: renk + HP bar + durum ikonu */

void DrawFriendlyUnits(Game *g) {
    Color typeColors[FUNIT_TYPE_COUNT] = {
        (Color){50, 200, 255, 230}, /* ARCHER  — mavi */
        (Color){60, 180, 60, 230},  /* WARRIOR — yesil */
        (Color){200, 80, 255, 230}, /* MAGE    — mor */
        (Color){240, 160, 30, 230}, /* KNIGHT  — turuncu */
        (Color){255, 255, 80, 230}, /* HERO    — sari */
        (Color){210, 170, 90, 230}, /* VILLAGER — bej */
    };
    const char *labels[FUNIT_TYPE_COUNT] = {"Ok", "Sv", "By", "Sv", "Hr", "Ky"};

    /* Bekleyen yerlestirme gostergesi */
    if (g->pendingPlacementCount > 0) {
        Vector2 mp = GetMousePosition();
        DrawCircleLines((int)mp.x, (int)mp.y, 14, typeColors[g->pendingPlacementType]);
        DrawText(TextFormat("Yerlestirilecek: %d", g->pendingPlacementCount), (int)mp.x + 16,
                 (int)mp.y - 8, 13, WHITE);
    }

    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
        FriendlyUnit *fu = &g->friendlyUnits[i];
        if (!fu->active)
            continue;
        Color col = typeColors[fu->type];
        float radius = (fu->type == FUNIT_HERO) ? 8.0f : 5.0f; /* Orantılı küçük boyutlar */

        /* Gövde */
        DrawCircle((int)fu->position.x, (int)fu->position.y, (int)radius, col);
        DrawCircleLines((int)fu->position.x, (int)fu->position.y, radius,
                        fu->order == FUNIT_HOLD ? WHITE
                        : fu->order == FUNIT_MOVE ? (Color){0,255,255,255} : ORANGE);

        /* Tip etiketi */
        DrawText(labels[fu->type],
                 (int)fu->position.x - 6, (int)fu->position.y - 6, 10, BLACK);
        if (g->camera.zoom > 1.5f) {
            DrawText(labels[fu->type], (int)fu->position.x - 6, (int)fu->position.y - 14, 10,
                     WHITE);
        }

        /* HP bari */
        float hr = fu->hp / fu->maxHp;
        DrawRectangle((int)fu->position.x - 10, (int)fu->position.y - (int)(radius + 6), 20, 3,
                      (Color){60,0,0,180});
        DrawRectangle((int)fu->position.x - 10, (int)fu->position.y - (int)(radius + 6),
                      (int)(20 * hr), 3, (Color){30,200,30,220});

        /* Hold/Attack durum ikonu */
        if (fu->order == FUNIT_HOLD)
            DrawText("H", (int)fu->position.x + (int)radius, (int)fu->position.y - 12, 10,
                     (Color){200,200,255,200});
        else
            DrawText("A", (int)fu->position.x + (int)radius, (int)fu->position.y - 12, 10,
                     (Color){255,160,30,220});

        /* Secili birim beyaz halka */
        if (fu->selected)
            DrawCircleLines((int)fu->position.x, (int)fu->position.y, radius + 4.0f, WHITE);
    }
}

/* ============================================================
 * T11 — SpawnEnemy
 * ============================================================ */

/* Boş bir düşman yuvası bulur, belirtilen tipte + HP çarpanıyla düşman oluşturur. */
void UpdateDialogue(Game *g, float dt) {
    DialogueSystem *d = &g->dialogue;
    if (!d->active)
        return;

    const char *curText = d->lines[d->currentLine].text;
    int textLen = (int)strlen(curText);

    /* Harf harf yaz: her 0.025s bir karakter */
    d->charTimer += dt;
    if (d->charTimer >= 0.025f) {
        d->charTimer = 0.0f;
        if (d->visibleChars < textLen) {
            d->visibleChars++;
            /* TODO: PlaySound(g->assets.sfxDialogueBlip) — her birkaç karakterde bir */
        }
    }

    /* SPACE veya sol tık: hızlandır veya sonraki satıra geç */
    bool advance = IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    if (advance) {
        if (d->visibleChars < textLen) {
            d->visibleChars = textLen; /* Tüm metni göster */
        } else {
            d->currentLine++;
            d->visibleChars = 0;
            d->charTimer = 0.0f;
            if (d->currentLine >= d->lineCount)
                d->active = false;
        }
    }
}

/* ============================================================
 * UpdateWorldMap — Level node seçimi
 * ============================================================ */

/* Açık level node'larına tıklandığında brifing ekranına geçer.
 * "Ana Menü" butonu menüye döner. */
void UpdateWorldMap(Game *g) {
    /* ESC veya Ana Menü butonu */
    Rectangle back = {20, (float)SCREEN_HEIGHT - 60, 160, 40};
    if (IsButtonClicked(back) || IsKeyPressed(KEY_ESCAPE)) {
        g->state = STATE_MENU;
        return;
    }

    Vector2 mp = GetMousePosition();
    for (int i = 0; i < MAX_LEVELS; i++) {
        LevelData *lv = &g->levels[i];
        if (!lv->unlocked)
            continue;
        float dx = mp.x - lv->nodeX, dy = mp.y - lv->nodeY;
        if ((dx * dx + dy * dy) <= 30.0f * 30.0f && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            g->currentLevel = i;
            /* Brifing diyaloğunu yükle */
            g->dialogue.lineCount = lv->briefingCount;
            g->dialogue.currentLine = 0;
            g->dialogue.visibleChars = 0;
            g->dialogue.charTimer = 0.0f;
            g->dialogue.active = (lv->briefingCount > 0);
            for (int j = 0; j < lv->briefingCount; j++)
                g->dialogue.lines[j] = lv->briefing[j];
            /* TODO: PlaySound(g->assets.sfxButtonClick) */
            g->state = STATE_LEVEL_BRIEFING;
            return;
        }
    }
}

/* ============================================================
 * UpdateLevelBriefing — Diyalog bitince BAŞLA butonunu aktif eder
 * ============================================================ */

void UpdateLevelBriefing(Game *g, float dt) {
    if (g->dialogue.active) {
        UpdateDialogue(g, dt);
        return;
    }
    /* Diyalog bitti — BAŞLA butonunu bekle */
    Rectangle btn = {SCREEN_WIDTH / 2.0f - 100, (float)SCREEN_HEIGHT - 100, 200, 50};
    if (IsButtonClicked(btn)) {
        const char *tips[MAX_TIP_TEXTS] = {
            "Ipucu: Sniper kuleler Tank dusmanlara karsi cok etkilidir.",
            "Ipucu: Splash kuleler grup dusmanlarini hizla temizler.",
            "Ipucu: Kule yükseltmek fiyatina deger — hasar %30 artar.",
            "Ipucu: [F] tusu ile oyun hizini 2x yapabilirsiniz.",
            "Ipucu: Satilan kule altinin %50'sini geri verir.",
            "Ipucu: Hizli dusmanlar gelmeden once yolu kapatmayi unutmayin."};
        g->loading.progress = 0.0f;
        g->loading.timer = 0.0f;
        g->loading.duration = 1.8f;
        g->loading.nextState = STATE_PLAYING;
        g->loading.tipText = tips[g->currentLevel % MAX_TIP_TEXTS];
        /* TODO: StopMusic(g->assets.bgmMenu); PlayMusic(g->assets.bgmBattle[g->currentLevel]) */
        g->state = STATE_LOADING;
    }
}

/* ============================================================
 * UpdateLoading — Sahte progress bar, sonra level başlat
 * ============================================================ */

void UpdateLoading(Game *g, float dt) {
    g->loading.timer += dt;
    g->loading.progress = g->loading.timer / g->loading.duration;
    if (g->loading.progress < 1.0f)
        return;

    g->loading.progress = 1.0f;
    /* Gameplay sıfırla, kampanya verisi koru */
    ResetGameplay(g);
    g->state = STATE_PLAYING;
    g->waveActive = true;
    /* TODO: LoadLevelAssets(g, g->currentLevel) — seviyeye özel asset yükle */
    /* TODO: InitLevelMap(g, g->currentLevel) — seviyeye özel harita & waypoint */
}

/* ============================================================
 * UpdateLevelComplete — Yıldız ekranı, sonraki level kilidi
 * ============================================================ */

void UpdateLevelComplete(Game *g, float dt) {
    if (g->dialogue.active) {
        UpdateDialogue(g, dt);
        return;
    }
    Rectangle btn = {SCREEN_WIDTH / 2.0f - 100, (float)SCREEN_HEIGHT - 120, 200, 55};
    if (IsButtonClicked(btn)) {
        /* TODO: PlaySound(g->assets.sfxButtonClick) */
        if (g->currentLevel + 1 < MAX_LEVELS)
            g->levels[g->currentLevel + 1].unlocked = true;
        /* Son level tamamlandıysa Victory */
        if (g->currentLevel + 1 >= MAX_LEVELS)
            g->state = STATE_VICTORY;
        else
            g->state = STATE_WORLD_MAP;
    }
}

/* ============================================================
 * HandleInput
 * ============================================================ */

/* Klavye ve fare girdisini işler: kule seçimi, yerleştirme, yükseltme ve durum değişiklikleri. */

        void DrawPlacementPreview(Game * g) {
            Vector2 worldMp = GetScreenToWorld2D(GetMousePosition(), g->camera.cam);
            int gx, gy;
            if (!WorldToGrid(worldMp, &gx, &gy))
                return;

            bool canPlace = CanPlaceTower(g, gx, gy);
            Color previewColor = canPlace ? (Color){0, 255, 0, 80} : (Color){255, 0, 0, 80};
            Color edgeColor = canPlace ? GREEN : RED;
            Vector2 ctr = GridToWorld(gx, gy);
            Vector2 ptop = {ctr.x, ctr.y - (float)ISO_HALF_H};
            Vector2 prght = {ctr.x + (float)ISO_HALF_W, ctr.y};
            Vector2 pbot = {ctr.x, ctr.y + (float)ISO_HALF_H};
            Vector2 pleft = {ctr.x - (float)ISO_HALF_W, ctr.y};
            DrawTriangle(ptop, pleft, prght, previewColor);
            DrawTriangle(prght, pleft, pbot, previewColor);
            DrawLineV(ptop, prght, edgeColor);
            DrawLineV(prght, pbot, edgeColor);
            DrawLineV(pbot, pleft, edgeColor);
            DrawLineV(pleft, ptop, edgeColor);

            /* Menzil dairesi */
            float range = 150;
            switch (g->selectedTowerType) {
            case TOWER_BASIC:
                range = 150;
                break;
            case TOWER_SNIPER:
                range = 300;
                break;
            case TOWER_SPLASH:
                range = 120;
                break;
            default:
                break;
            }
            Vector2 center = GridToWorld(gx, gy);
            DrawCircleLines((int)center.x, (int)center.y, range, (Color){255, 255, 255, 60});
        }

        /* ============================================================
         * T30 — DrawHUD
         * ============================================================ */

        /* Üst bilgi panelini ve kule seçim butonlarını çizer. */
        void DrawHUD(Game * g) {
            /* Üst panel — obsidyen gradient */
            DrawGradientPanel((Rectangle){0, 0, SCREEN_WIDTH, GRID_OFFSET_Y}, UI_PANEL_TOP,
                              (Color){8, 10, 18, 255});
            DrawOrnateBorder((Rectangle){0, 0, SCREEN_WIDTH, GRID_OFFSET_Y}, UI_GOLD, UIS(1.2f));

            /* Sol: Can / Altın / Skor — ResourceText göstergeler */
            DrawResourceText((Vector2){10, 8}, g->lives, "CAN", (Color){200, 50, 50, 220});
            DrawResourceText((Vector2){130, 8}, g->gold, "ALTIN", (Color){200, 160, 0, 220});
            DrawResourceText((Vector2){290, 8}, g->score, "SKOR", (Color){80, 180, 220, 220});
            /* T86 — Market geliri göstergesi */
            if (g->managers.marketIncome > 0) {
                char incbuf[24];
                snprintf(incbuf, sizeof(incbuf), "+%d/tick", g->managers.marketIncome);
                DrawText(incbuf, 220, 26, 10, (Color){180, 220, 80, 200});
            }

            /* Sağ: Dalga / Hız / Kill */
            char buf[128];
            snprintf(buf, sizeof(buf), "Dalga: %d/%d   Hiz: %.0fx   Kill: %d", g->currentWave + 1,
                     g->totalWaves, g->gameSpeed, g->enemiesKilled);
            DrawBodyText(buf, (Vector2){(float)(SCREEN_WIDTH - UISI(280)), 8.0f}, 13.0f,
                         (Color){200, 200, 220, 200});

            /* Alt satır: Kule seçim butonları */
            const char *labels[3] = {"[1] Basic 50g", "[2] Sniper 100g", "[3] Splash 150g"};
            Color colors[3] = {BLUE, DARKGREEN, MAROON};
            for (int i = 0; i < 3; i++) {
                Rectangle btn = {10.0f + i * 190.0f, 46.0f, 180.0f, 26.0f};
                Color bg = colors[i];
                if (g->selectedTowerType == (TowerType)i) {
                    bg.r = (unsigned char)fminf(bg.r + 80, 255);
                    bg.g = (unsigned char)fminf(bg.g + 80, 255);
                    bg.b = (unsigned char)fminf(bg.b + 80, 255);
                    DrawRectangleLinesEx(btn, 2, YELLOW);
                }
                DrawRectangleRec(btn, bg);
                DrawText(labels[i], (int)btn.x + 5, (int)btn.y + 5, 16, WHITE);
            }

            /* Dalgayı başlat bilgisi */
            if (!g->waveActive && g->currentWave < g->totalWaves)
                DrawText("[ SPACE ] - Sonraki Dalgayi Baslat", SCREEN_WIDTH / 2 - 180, 50, 18,
                         YELLOW);

            /* B03 — Dalga ilerleme çubuğu */
            if (g->waveActive && g->currentWave < g->totalWaves) {
                GameWave *w = &g->waves[g->currentWave];
                int activeCount = 0;
                for (int i = 0; i < MAX_ENEMIES; i++)
                    if (g->enemies[i].active)
                        activeCount++;
                float progress =
                    (w->enemyCount > 0) ? (float)w->spawnedCount / w->enemyCount : 0.0f;
                int bx = SCREEN_WIDTH / 2 - 200, by = 35, bw = 400, bh = 8;
                DrawRectangle(bx, by, bw, bh, (Color){40, 40, 40, 200});
                DrawRectangle(bx, by, (int)(bw * progress), bh, (Color){255, 180, 0, 230});
                DrawRectangleLines(bx, by, bw, bh, (Color){160, 160, 160, 120});
                char pbuf[48];
                snprintf(pbuf, sizeof(pbuf), "%d/%d  [%d aktif]", w->spawnedCount, w->enemyCount,
                         activeCount);
                DrawText(pbuf, bx + bw / 2 - MeasureText(pbuf, 13) / 2, by - 15, 13,
                         (Color){200, 200, 200, 200});
            }

            /* T62 — Dalga adı banner (dalga başlangıcında 3 saniye görünür) */
            if (g->waveNameTimer > 0.0f && g->waveNameText[0] != '\0') {
                float alpha = (g->waveNameTimer > 0.5f) ? 1.0f : g->waveNameTimer / 0.5f;
                int ba = (int)(200 * alpha);
                int fa = (int)(255 * alpha);
                /* Dalga numarası + adı */
                char bannerBuf[64];
                bool isBoss = (g->currentWave < MAX_WAVES) && g->waves[g->currentWave].isBossWave;
                snprintf(bannerBuf, sizeof(bannerBuf), "Dalga %d: %s", g->currentWave + 1,
                         g->waveNameText);
                int bw = MeasureText(bannerBuf, isBoss ? 28 : 22) + 30;
                int bx = SCREEN_WIDTH / 2 - bw / 2;
                int by = SCREEN_HEIGHT / 2 - 30;
                DrawRectangle(bx, by, bw, isBoss ? 44 : 34, (Color){0, 0, 0, ba});
                DrawRectangleLines(bx, by, bw, isBoss ? 44 : 34,
                                   isBoss ? (Color){255, 200, 0, fa} : (Color){160, 160, 160, fa});
                DrawText(bannerBuf, bx + 15, by + (isBoss ? 8 : 6), isBoss ? 28 : 22,
                         isBoss ? (Color){255, 220, 0, fa} : (Color){255, 255, 255, fa});
            }

            /* T55 — Boss HP Bar (Eğer haritada aktif boss varsa) */
            for (int i = 0; i < MAX_ENEMIES; i++) {
                Enemy *e = &g->enemies[i];
                if (e->active && e->isBoss) {
                    int bbw = 500;
                    int bbh = 20;
                    int bbx = SCREEN_WIDTH / 2 - bbw / 2;
                    int bby = 65;

                    /* Nabız border */
                    float pulse = 1.0f + sinf((float)GetTime() * 5.0f) * 0.5f;
                    DrawRectangle(bbx - 4, bby - 4, bbw + 8, bbh + 8,
                                  (Color){30, 0, 0, (unsigned char)(200 * pulse)});

                    DrawRectangle(bbx, bby, bbw, bbh, (Color){40, 40, 40, 240});
                    float bossRatio = e->currentHp / e->maxHp;
                    DrawRectangle(bbx, bby, (int)(bbw * bossRatio), bbh, MAROON);
                    DrawRectangleLines(bbx, bby, bbw, bbh, GOLD);

                    char tbuf[64];
                    snprintf(tbuf, sizeof(tbuf), "BOSS: %s (Faz %d) - %.0f/%.0f",
                             g->waves[g->currentWave].waveName, e->bossPhase, e->currentHp,
                             e->maxHp);
                    DrawText(tbuf, bbx + bbw / 2 - MeasureText(tbuf, 16) / 2, bby + 2, 16, WHITE);
                    break; /* Tek boss varsayımı */
                }
            }

            /* T41 — Home City UI */
            DrawHomeCityUI(&g->homeCity, SCREEN_WIDTH, SCREEN_HEIGHT);
        }

        /* ============================================================
         * T33 — DrawMenu
         * ============================================================ */

        void DrawMenu(Game * g) {
            (void)g;
            /* Arka plan */
            DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){6, 8, 18, 255},
                                   (Color){18, 22, 40, 255});

            /* Merkez panel */
            Rectangle panel = {(float)SCREEN_WIDTH / 2 - 220, (float)SCREEN_HEIGHT / 2 - 160, 440,
                               320};
            DrawEpicPanel(panel, NULL);

            /* Başlık */
            DrawEpicTitle(
                "RULER",
                (Vector2){(float)SCREEN_WIDTH / 2 - UIS(72), (float)SCREEN_HEIGHT / 2 - 130},
                72.0f);

            /* Alt başlık */
            DrawBodyText(
                "Kralligini Koru",
                (Vector2){(float)SCREEN_WIDTH / 2 - UIS(90), (float)SCREEN_HEIGHT / 2 - 42}, 22.0f,
                UI_IVORY);

            Rectangle btn  = {(float)SCREEN_WIDTH / 2 - 100, (float)SCREEN_HEIGHT / 2 + 10, 200, 56};
            Rectangle sBtn = {(float)SCREEN_WIDTH / 2 - 80,  (float)SCREEN_HEIGHT / 2 + 74, 160, 40};
            DrawButton(btn,  T("BTN_START"),    GREEN,    BLACK);
            DrawButton(sBtn, T("BTN_SETTINGS"), DARKGRAY, WHITE);
        }

        /* ============================================================
         * T34 — DrawPauseOverlay
         * ============================================================ */

        void DrawPauseOverlay(Game * g) {
            (void)g;
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 170});

            Rectangle panel = {(float)SCREEN_WIDTH / 2 - 180, (float)SCREEN_HEIGHT / 2 - 120, 360,
                               260};
            DrawEpicPanel(panel, T("PAUSE_TITLE"));

            Rectangle resume = {(float)SCREEN_WIDTH / 2 - 90, (float)SCREEN_HEIGHT / 2 - 10, 180,
                                50};
            Rectangle quit = {(float)SCREEN_WIDTH / 2 - 90, (float)SCREEN_HEIGHT / 2 + 72, 180, 50};
            DrawButton(resume, T("BTN_RESUME"), DARKGREEN, WHITE);
            DrawButton(quit,   T("BTN_BACK"),   DARKGRAY,  WHITE);
        }

        /* ============================================================
         * T57/T58 — DrawSettingsMenu / UpdateSettingsMenu + Key Rebinding
         * ============================================================ */

        static int g_settingsSel  = 0; /* ana sayfa seçili satır (0-4) */
        static int g_settingsPage = 0; /* 0=ana ayarlar, 1=tuş atamaları */
        static int g_rebindSel    = 0; /* tuş atamaları sayfasında seçili eylem */
        static int g_listeningFor = -1; /* >= 0: hangi KeyAction bekleniyor */

        static void DrawSlider(float x, float y, float w, float h,
                               const char *label, float value, bool selected) {
            Color bg   = selected ? (Color){60, 80, 120, 200} : (Color){30, 35, 55, 180};
            Color fill = (Color){80, 180, 100, 255};
            DrawRectangleRec((Rectangle){x, y, w, h}, bg);
            if (selected)
                DrawRectangleLinesEx((Rectangle){x, y, w, h}, 2, (Color){140, 200, 255, 255});
            DrawTextEx(g_ui.body, label, (Vector2){x + 10, y + h * 0.25f}, UIS(18.0f), 1, WHITE);
            float bx = x + w * 0.55f, bw = w * 0.38f;
            DrawRectangle((int)bx, (int)(y + h * 0.35f), (int)bw, (int)(h * 0.3f),
                          (Color){20, 20, 30, 220});
            DrawRectangle((int)bx, (int)(y + h * 0.35f), (int)(bw * value), (int)(h * 0.3f), fill);
            char pct[8];
            snprintf(pct, sizeof(pct), "%d%%", (int)(value * 100));
            DrawTextEx(g_ui.body, pct, (Vector2){bx + bw + 8, y + h * 0.25f}, UIS(16.0f), 1, UI_IVORY);
        }

        static void DrawToggleRow(float x, float y, float w, float h,
                                  const char *label, bool value, bool selected) {
            Color bg = selected ? (Color){60, 80, 120, 200} : (Color){30, 35, 55, 180};
            DrawRectangleRec((Rectangle){x, y, w, h}, bg);
            if (selected)
                DrawRectangleLinesEx((Rectangle){x, y, w, h}, 2, (Color){140, 200, 255, 255});
            DrawTextEx(g_ui.body, label, (Vector2){x + 10, y + h * 0.25f}, UIS(18.0f), 1, WHITE);
            const char *val = value ? T("TOGGLE_ON") : T("TOGGLE_OFF");
            Color vc = value ? (Color){80, 200, 100, 255} : (Color){180, 80, 80, 255};
            DrawTextEx(g_ui.body, val, (Vector2){x + w * 0.7f, y + h * 0.25f}, UIS(18.0f), 1, vc);
        }

        /* Dil satırı: TR ↔ EN gösterir */
        static void DrawLangRow(float x, float y, float w, float h, bool isEN, bool selected) {
            Color bg = selected ? (Color){60, 80, 120, 200} : (Color){30, 35, 55, 180};
            DrawRectangleRec((Rectangle){x, y, w, h}, bg);
            if (selected)
                DrawRectangleLinesEx((Rectangle){x, y, w, h}, 2, (Color){140, 200, 255, 255});
            DrawTextEx(g_ui.body, "Dil / Language", (Vector2){x + 10, y + h * 0.25f}, UIS(18.0f), 1, WHITE);
            const char *val = isEN ? "EN" : "TR";
            Color vc = (Color){80, 200, 255, 255};
            DrawTextEx(g_ui.body, val, (Vector2){x + w * 0.7f, y + h * 0.25f}, UIS(18.0f), 1, vc);
        }

        static void DrawNavRow(float x, float y, float w, float h,
                               const char *label, bool selected) {
            Color bg = selected ? (Color){60, 80, 120, 200} : (Color){30, 35, 55, 180};
            DrawRectangleRec((Rectangle){x, y, w, h}, bg);
            if (selected)
                DrawRectangleLinesEx((Rectangle){x, y, w, h}, 2, (Color){140, 200, 255, 255});
            DrawTextEx(g_ui.body, label, (Vector2){x + 10, y + h * 0.25f}, UIS(18.0f), 1, WHITE);
            DrawTextEx(g_ui.body, ">", (Vector2){x + w - 24, y + h * 0.25f}, UIS(18.0f), 1,
                       (Color){180, 220, 255, 220});
        }

        /* T58 — Tuş atamaları sayfasını çizer */
        static void DrawKeybindPage(Game *g) {
            float pw = 520, ph = 500;
            float px = SCREEN_WIDTH / 2.0f - pw / 2.0f;
            float py = SCREEN_HEIGHT / 2.0f - ph / 2.0f;
            DrawEpicPanel((Rectangle){px, py, pw, ph}, T("KEYBINDS_TITLE"));

            float rh = 34, gap = 4;
            float rx = px + 16, rw = pw - 32;
            float ry = py + 56;

            for (int i = 0; i < KA_COUNT; i++) {
                bool sel = (g_rebindSel == i);
                bool listening = (g_listeningFor == i);
                Color bg = listening ? (Color){80, 40, 10, 220}
                         : sel       ? (Color){60, 80, 120, 200}
                                     : (Color){30, 35, 55, 160};
                float iy = ry + i * (rh + gap);
                DrawRectangleRec((Rectangle){rx, iy, rw, rh}, bg);
                if (sel || listening)
                    DrawRectangleLinesEx((Rectangle){rx, iy, rw, rh}, 2,
                        listening ? (Color){255, 160, 40, 255} : (Color){140, 200, 255, 255});

                DrawTextEx(g_ui.body, KeyActionLabel((KeyAction)i),
                           (Vector2){rx + 10, iy + rh * 0.22f}, UIS(16.0f), 1, WHITE);

                const char *kname = listening ? T("KEYBINDS_LISTEN") : KeyName(g->settings.keymap[i]);
                Color kc = listening ? (Color){255, 200, 80, 255} : (Color){120, 220, 140, 255};
                float tw = MeasureTextEx(g_ui.body, kname, UIS(16.0f), 1).x;
                DrawTextEx(g_ui.body, kname,
                           (Vector2){rx + rw - tw - 14, iy + rh * 0.22f}, UIS(16.0f), 1, kc);
            }

            float by = py + ph - 52;
            Rectangle bBack = {px + pw / 2.0f - 95, by, 190, 40};
            DrawButton(bBack, T("BTN_BACK_ESC"), DARKGRAY, WHITE);

            DrawBodyText(T("KEYBINDS_HINT"),
                         (Vector2){px + 10, py + ph - 20}, 13.0f, (Color){130, 130, 150, 200});
        }

        void DrawSettingsMenu(Game *g) {
            DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                                   (Color){6, 8, 18, 255}, (Color){18, 22, 40, 255});

            if (g_settingsPage == 1) {
                DrawKeybindPage(g);
                return;
            }

            float pw = 480, ph = 444;
            float px = SCREEN_WIDTH / 2.0f - pw / 2.0f;
            float py = SCREEN_HEIGHT / 2.0f - ph / 2.0f;
            DrawEpicPanel((Rectangle){px, py, pw, ph}, T("SETTINGS_TITLE"));

            float rh = 46, gap = 8;
            float rx = px + 20, rw = pw - 40;
            float ry = py + 60;
            bool isEN = (g->settings.language == LANG_EN);

            DrawToggleRow(rx, ry,                rw, rh, T("SET_FULLSCREEN"), g->settings.fullscreen,   g_settingsSel == 0);
            DrawSlider   (rx, ry + (rh+gap),     rw, rh, T("SET_MASTER_VOL"), g->settings.masterVolume, g_settingsSel == 1);
            DrawSlider   (rx, ry + (rh+gap)*2,   rw, rh, T("SET_SFX_VOL"),    g->settings.sfxVolume,    g_settingsSel == 2);
            DrawSlider   (rx, ry + (rh+gap)*3,   rw, rh, T("SET_MUSIC_VOL"),  g->settings.musicVolume,  g_settingsSel == 3);
            DrawLangRow  (rx, ry + (rh+gap)*4,   rw, rh, isEN,                                             g_settingsSel == 4);
            DrawNavRow   (rx, ry + (rh+gap)*5,   rw, rh, T("SET_KEYBINDS"),                             g_settingsSel == 5);

            float by = py + ph - 70;
            Rectangle bSave = {px + 30,       by, 190, 44};
            Rectangle bBack = {px + pw - 220, by, 190, 44};
            DrawButton(bSave, T("BTN_SAVE"), DARKGREEN, WHITE);
            DrawButton(bBack, T("BTN_BACK"), DARKGRAY,  WHITE);

            DrawBodyText(T("SETTINGS_HINT"),
                         (Vector2){px + 10, py + ph - 22}, 14.0f, (Color){140, 140, 160, 200});
        }

        static void ApplySettings(Game *g) {
            g->audio.masterVolume = g->settings.masterVolume;
            if (g->settings.fullscreen != IsWindowFullscreen())
                ToggleFullscreen();
            I18nLoad(g->settings.language); /* T59 — dil değişince yeniden yükle */
        }

        /* T58 — Varsayılan keymap'ten tek eylemi sıfırlar */
        static void ResetKeybindToDefault(Settings *s, int action) {
            Settings def;
            DefaultSettings(&def);
            s->keymap[action] = def.keymap[action];
        }

        void UpdateSettingsMenu(Game *g) {
            /* --- Tuş atamaları sayfası --- */
            if (g_settingsPage == 1) {
                if (g_listeningFor >= 0) {
                    /* Bekleme modu: herhangi bir tuşa basılınca ata */
                    int pressed = GetKeyPressed();
                    if (pressed == KEY_ESCAPE) {
                        g_listeningFor = -1; /* iptal */
                    } else if (pressed != 0) {
                        g->settings.keymap[g_listeningFor] = pressed;
                        SaveSettings(&g->settings);
                        g_listeningFor = -1;
                    }
                    return;
                }
                if (IsKeyPressed(KEY_UP))
                    g_rebindSel = (g_rebindSel - 1 + KA_COUNT) % KA_COUNT;
                if (IsKeyPressed(KEY_DOWN))
                    g_rebindSel = (g_rebindSel + 1) % KA_COUNT;
                if (IsKeyPressed(KEY_ENTER))
                    g_listeningFor = g_rebindSel;
                if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))
                    ResetKeybindToDefault(&g->settings, g_rebindSel);

                float pw = 520, ph = 500;
                float px = SCREEN_WIDTH / 2.0f - pw / 2.0f;
                float py = SCREEN_HEIGHT / 2.0f - ph / 2.0f;
                Rectangle bBack = {px + pw / 2.0f - 95, py + ph - 52, 190, 40};
                if (IsButtonClicked(bBack) || IsKeyPressed(KEY_ESCAPE))
                    g_settingsPage = 0;
                return;
            }

            /* --- Ana ayarlar sayfası --- */
            int itemCount = 6;
            float step = 0.05f;

            if (IsKeyPressed(KEY_UP))   g_settingsSel = (g_settingsSel - 1 + itemCount) % itemCount;
            if (IsKeyPressed(KEY_DOWN)) g_settingsSel = (g_settingsSel + 1) % itemCount;

            float *vol = NULL;
            if      (g_settingsSel == 1) vol = &g->settings.masterVolume;
            else if (g_settingsSel == 2) vol = &g->settings.sfxVolume;
            else if (g_settingsSel == 3) vol = &g->settings.musicVolume;

            if (vol) {
                if (IsKeyDown(KEY_LEFT))  *vol = (*vol - step < 0.0f) ? 0.0f : *vol - step;
                if (IsKeyDown(KEY_RIGHT)) *vol = (*vol + step > 1.0f) ? 1.0f : *vol + step;
            }
            if (g_settingsSel == 0 && (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)))
                g->settings.fullscreen = !g->settings.fullscreen;
            /* T59 — dil toggle: TR ↔ EN */
            if (g_settingsSel == 4 && (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
                                        IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_RIGHT))) {
                g->settings.language = (g->settings.language == LANG_TR) ? LANG_EN : LANG_TR;
                I18nLoad(g->settings.language);
            }
            if (g_settingsSel == 5 && (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)))
                g_settingsPage = 1;

            float pw = 480, ph = 444;
            float px = SCREEN_WIDTH / 2.0f - pw / 2.0f;
            float py = SCREEN_HEIGHT / 2.0f - ph / 2.0f;
            float by = py + ph - 70;
            Rectangle bSave = {px + 30,       by, 190, 44};
            Rectangle bBack = {px + pw - 220, by, 190, 44};

            bool save = IsButtonClicked(bSave);
            bool back = IsButtonClicked(bBack) || IsKeyPressed(KEY_ESCAPE);

            if (save || back) {
                if (save) {
                    ApplySettings(g);
                    SaveSettings(&g->settings);
                    LOG_INFO("Ayarlar kaydedildi");
                }
                g_settingsPage = 0;
                g->state = g->preSettingsState;
            }
        }

        /* ============================================================
         * T35 — DrawGameOver
         * ============================================================ */

        void DrawGameOver(Game * g) {
            DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){18, 4, 4, 255},
                                   (Color){6, 2, 2, 255});

            Rectangle panel = {(float)SCREEN_WIDTH / 2 - 240, (float)SCREEN_HEIGHT / 2 - 150, 480,
                               310};
            DrawEpicPanel(panel, NULL);

            DrawShadowText(
                g_ui.title, T("GAMEOVER_TITLE"),
                (Vector2){(float)SCREEN_WIDTH / 2 - UIS(100), (float)SCREEN_HEIGHT / 2 - 130},
                UIS(64.0f), 2.0f, (Color){220, 50, 50, 255});

            char buf[64];
            snprintf(buf, sizeof(buf), T("GAMEOVER_STATS"), g->score, g->enemiesKilled);
            DrawBodyText(
                buf, (Vector2){(float)SCREEN_WIDTH / 2 - UIS(115), (float)SCREEN_HEIGHT / 2 - 20},
                20.0f, UI_IVORY);

            Rectangle btn = {(float)SCREEN_WIDTH / 2 - 110, (float)SCREEN_HEIGHT / 2 + 50, 220, 56};
            DrawButton(btn, T("BTN_PLAY_AGAIN"), DARKGRAY, WHITE);
        }

        /* ============================================================
         * T36 — DrawVictory
         * ============================================================ */

        void DrawVictory(Game * g) {
            DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){4, 18, 8, 255},
                                   (Color){2, 8, 4, 255});

            Rectangle panel = {(float)SCREEN_WIDTH / 2 - 260, (float)SCREEN_HEIGHT / 2 - 150, 520,
                               310};
            DrawEpicPanel(panel, NULL);

            DrawEpicTitle(
                T("VICTORY_TITLE"),
                (Vector2){(float)SCREEN_WIDTH / 2 - UIS(72), (float)SCREEN_HEIGHT / 2 - 130},
                72.0f);

            char buf[64];
            snprintf(buf, sizeof(buf), "Final Skor: %d", g->score);
            DrawBodyText(
                buf, (Vector2){(float)SCREEN_WIDTH / 2 - UIS(100), (float)SCREEN_HEIGHT / 2 - 20},
                24.0f, UI_IVORY);

            Rectangle btn = {(float)SCREEN_WIDTH / 2 - 110, (float)SCREEN_HEIGHT / 2 + 50, 220, 56};
            DrawButton(btn, T("BTN_PLAY_AGAIN"), DARKGREEN, BLACK);
        }

        /* ============================================================
         * T70 — Fog of War Hesaplama
         * ============================================================ */

        /* Fog grid'ini her frame kuleler + dost birimler + hero'dan yeniden hesaplar. */
        void UpdateFogOfWar(Game * g) {
            /* Tüm görünürlüğü sıfırla */
            memset(g->fogVisible, 0, sizeof(g->fogVisible));

            /* Köy her zaman görünür */
            for (int r = 0; r < GRID_ROWS; r++)
                for (int c = 0; c < GRID_COLS; c++)
                    if (g->grid[r][c] == CELL_VILLAGE)
                        g->fogVisible[r][c] = true;

            /* Kulelerden görüş açığı (kule menzili / CELL_SIZE = görüş yarıçapı hücre) */
            for (int i = 0; i < MAX_TOWERS; i++) {
                Tower *t = &g->towers[i];
                if (!t->active)
                    continue;
                int cr = t->gridY, cc = t->gridX;
                int vr = (int)(t->range / CELL_SIZE) + 1;
                for (int dr = -vr; dr <= vr; dr++) {
                    for (int dc = -vr; dc <= vr; dc++) {
                        int nr = cr + dr, nc = cc + dc;
                        if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS)
                            continue;
                        if (dr * dr + dc * dc <= vr * vr) {
                            g->fogVisible[nr][nc] = true;
                            g->fogExplored[nr][nc] = true;
                        }
                    }
                }
            }

            /* Dost birimlerden gorunum (yaklasik 7 hucre) */
            for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
                FriendlyUnit *fu = &g->friendlyUnits[i];
                if (!fu->active)
                    continue;
                int cc, cr;
                if (!WorldToGrid(fu->position, &cc, &cr))
                    continue;
                int vr = 7;
                for (int dr = -vr; dr <= vr; dr++) {
                    for (int dc = -vr; dc <= vr; dc++) {
                        int nr = cr + dr, nc = cc + dc;
                        if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS)
                            continue;
                        if (dr * dr + dc * dc <= vr * vr) {
                            g->fogVisible[nr][nc] = true;
                            g->fogExplored[nr][nc] = true;
                        }
                    }
                }
            }

            /* Hero'dan gorunum (yaklasik 10 hucre) */
            if (g->hero.alive) {
                int cc, cr;
                if (WorldToGrid(g->hero.position, &cc, &cr)) {
                    int vr = 10;
                    for (int dr = -vr; dr <= vr; dr++) {
                        for (int dc = -vr; dc <= vr; dc++) {
                            int nr = cr + dr, nc = cc + dc;
                            if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS)
                                continue;
                            if (dr * dr + dc * dc <= vr * vr) {
                                g->fogVisible[nr][nc] = true;
                                g->fogExplored[nr][nc] = true;
                            }
                        }
                    }
                }
            }
        }

        /* Fog overlay: karanlık hücrelere izometrik diamond sis çizer. */
        void DrawFogOverlay(Game * g) {
            for (int r = 0; r < GRID_ROWS; r++) {
                for (int c = 0; c < GRID_COLS; c++) {
                    if (g->fogVisible[r][c])
                        continue;
                    Color fc =
                        g->fogExplored[r][c] ? (Color){7, 7, 10, 140} : (Color){7, 7, 10, 200};
                    Vector2 ctr = GridToWorld(c, r);
                    Vector2 ftop = {ctr.x, ctr.y - (float)ISO_HALF_H};
                    Vector2 frght = {ctr.x + (float)ISO_HALF_W, ctr.y};
                    Vector2 fbot = {ctr.x, ctr.y + (float)ISO_HALF_H};
                    Vector2 fleft = {ctr.x - (float)ISO_HALF_W, ctr.y};
                    DrawTriangle(ftop, fleft, frght, fc);
                    DrawTriangle(frght, fleft, fbot, fc);
                }
            }
        }

        /* ============================================================
         * T70 — Minimap (ekranın sol altında, 200×120 px)
         * ============================================================ */

        void DrawMinimap(Game * g) {
            int mmW = 200, mmH = 130;
            int mmX = 10, mmY = SCREEN_HEIGHT - mmH - 10;
            float scaleX = (float)mmW / GRID_COLS;
            float scaleY = (float)mmH / GRID_ROWS;

            /* Arka plan */
            DrawRectangle(mmX - 2, mmY - 2, mmW + 4, mmH + 4, (Color){200, 175, 80, 200});
            DrawRectangle(mmX, mmY, mmW, mmH, (Color){7, 7, 10, 220});

            /* Grid hucreleri — ortografik minimap */
            for (int r = 0; r < GRID_ROWS; r++) {
                for (int c = 0; c < GRID_COLS; c++) {
                    int px = mmX + (int)(c * scaleX);
                    int py = mmY + (int)(r * scaleY);
                    int pw = (int)scaleX + 1;
                    int ph = (int)scaleY + 1;
                    Color mc;
                    if (!g->fogExplored[r][c])
                        continue; /* Keşfedilmemiş: çizme */
                    switch (g->grid[r][c]) {
                    case CELL_PATH:
                        mc = (Color){160, 130, 90, 200};
                        break;
                    case CELL_BUILDABLE:
                        mc = (Color){50, 80, 50, 150};
                        break;
                    case CELL_TOWER:
                        mc = (Color){60, 120, 200, 255};
                        break;
                    case CELL_VILLAGE:
                        mc = (Color){200, 160, 80, 255};
                        break;
                    default:
                        mc = (Color){25, 40, 25, 100};
                        break;
                    }
                    DrawRectangle(px, py, pw, ph, mc);
                }
            }

            /* Dusmanlar (kirmizi noktalar — sadece gorununce) */
            for (int i = 0; i < MAX_ENEMIES; i++) {
                Enemy *e = &g->enemies[i];
                if (!e->active)
                    continue;
                int ec, er;
                if (WorldToGrid(e->position, &ec, &er) && g->fogVisible[er][ec]) {
                    int ex = mmX + (int)(ec * scaleX);
                    int ey = mmY + (int)(er * scaleY);
                    DrawCircle(ex, ey, 2, RED);
                }
            }

            /* Kamera hedefini grid'e cevir ve minimap'te goster */
            int camGx, camGy;
            if (WorldToGrid(g->camera.cam.target, &camGx, &camGy)) {
                int cx = mmX + (int)(camGx * scaleX);
                int cy = mmY + (int)(camGy * scaleY);
                int cvw =
                    (int)(SCREEN_WIDTH / g->camera.zoom * scaleX / (2.0f * (float)ISO_HALF_W));
                int cvh =
                    (int)(SCREEN_HEIGHT / g->camera.zoom * scaleY / (2.0f * (float)ISO_HALF_H));
                DrawRectangleLines(cx - cvw, cy - cvh, cvw * 2, cvh * 2, WHITE);
            }
        }

        /* ============================================================
         * DrawGame — Ana Çizim Sırası (T08 + Katmanlar)
         * ============================================================ */


        /* T101 — Dungeon Entrance kapısı çiz */
        static void DrawDungeonEntrance(Game *g) {
            if (!g->dungeonEntranceBuilt) return;
            Vector2 p = g->dungeonEntrancePos;
            float t = (float)GetTime();
            /* Dış hale */
            float r = 22.0f + sinf(t * 2.2f) * 4.0f;
            DrawCircleV(p, r, Fade((Color){80, 40, 160, 255}, 0.35f + sinf(t * 2.2f) * 0.15f));
            /* İç parlayan merkez */
            DrawCircleV(p, 12.0f, Fade((Color){160, 80, 255, 255}, 0.7f));
            DrawCircleV(p, 7.0f, (Color){220, 180, 255, 240});
            /* Çerçeve */
            DrawCircleLines((int)p.x, (int)p.y, r, (Color){140, 80, 220, 200});
            /* Etiket */
            const char *lbl = "ZINDAN";
            DrawText(lbl, (int)p.x - MeasureText(lbl, 10) / 2, (int)p.y + (int)r + 3, 10,
                     (Color){200, 160, 255, 220});
        }

        /* T101 — Ulak birimlerini çiz */
        static void DrawCourierUnits(Game *g) {
            for (int i = 0; i < MAX_COURIER_UNITS; i++) {
                CourierUnit *cu = &g->courierUnits[i];
                if (!cu->active) continue;
                /* At gövdesi */
                DrawCircleV(cu->position, 6.0f, cu->color);
                /* Hareket yönü oku */
                Vector2 tip = {cu->position.x + cosf(cu->angle) * 10.0f,
                               cu->position.y + sinf(cu->angle) * 10.0f};
                DrawLineV(cu->position, tip, WHITE);
                /* Mesaj balonu (varsa) */
                if (cu->message[0] != '\0') {
                    int tw = MeasureText(cu->message, 10);
                    DrawRectangle((int)cu->position.x - tw / 2 - 2, (int)cu->position.y - 22,
                                  tw + 4, 14, (Color){20, 20, 20, 180});
                    DrawText(cu->message, (int)cu->position.x - tw / 2, (int)cu->position.y - 20,
                             10, YELLOW);
                }
            }
        }

        void DrawGame(Game * g) {
            DrawMap(g);
            DrawPathArrows(g);
            DrawPlacementPreview(g);
            DrawTowerSynergies(g);
            DrawTowers(g);
            DrawFriendlyUnits(g);
            DrawOutposts(g);
            DrawGuardians(g);
            DrawEnemies(g);
            DrawProjectiles(g);
            DrawParticles(g);
            DrawFloatingTexts(g);
            DrawDungeonEntrance(g); /* T101 */
            DrawCourierUnits(g);    /* T101 */
            DrawFogOverlay(g); /* T70 — Fog en üstte */

            /* T65 — Seçim kutusu çizimi (dünya koordinatı) */
            if (g->isSelecting) {
                Vector2 ws = GetScreenToWorld2D(g->selectionStart, g->camera.cam);
                Vector2 we = GetScreenToWorld2D(g->selectionEnd, g->camera.cam);
                float sx = fminf(ws.x, we.x), sy = fminf(ws.y, we.y);
                float sw = fabsf(we.x - ws.x), sh = fabsf(we.y - ws.y);
                DrawRectangle((int)sx, (int)sy, (int)sw, (int)sh, (Color){100, 255, 100, 40});
                DrawRectangleLines((int)sx, (int)sy, (int)sw, (int)sh, (Color){180, 255, 180, 180});
            }

            DrawContextMenu(g);
        }

        /* ============================================================
         * DrawDialogueBox — Typewriter efektli konuşma kutusu
         * ============================================================ */

        /* Ekranın alt bölümünde konuşmacı portesi + kayan metin çizer.
         * TODO: portreAsset Texture2D yüklenince DrawTexture ile değiştirilir. */
        void DrawDialogueBox(Game * g) {
            DialogueSystem *d = &g->dialogue;
            if (!d->active || d->currentLine >= d->lineCount)
                return;

            DialogueLine *line = &d->lines[d->currentLine];

            int bx = 40, by = SCREEN_HEIGHT - 210, bw = SCREEN_WIDTH - 80, bh = 160;
            DrawRectangle(bx, by, bw, bh, (Color){8, 8, 18, 215});
            DrawRectangleLinesEx((Rectangle){(float)bx, (float)by, (float)bw, (float)bh}, 2,
                                 (Color){180, 160, 80, 210});

            /* Portreye yer — TODO: DrawTexture(portraitTexture, bx+8, by+8, WHITE) */
            DrawRectangle(bx + 8, by + 8, 80, 80, (Color){35, 35, 55, 210});
            DrawRectangleLines(bx + 8, by + 8, 80, 80, (Color){100, 100, 140, 180});
            DrawText("?", bx + 34, by + 28, 32, GRAY); /* Portrenin yerinde placeholder */
            DrawText("[portré]", bx + 14, by + 66, 11, (Color){90, 90, 90, 180});

            /* Konuşmacı adı */
            DrawText(line->speaker, bx + 102, by + 10, 20, line->speakerColor);

            /* Typewriter metni — visibleChars'a kadar kes */
            char buf[512];
            int len = (int)strlen(line->text);
            int vis = (d->visibleChars < len) ? d->visibleChars : len;
            if (vis > 511)
                vis = 511; // prevent buffer overflow
            memcpy(buf, line->text, (size_t)vis);
            buf[vis] = '\0';
            DrawText(buf, bx + 102, by + 38, 16, WHITE);

            /* Metin tamamsa ilerleme oku */
            if (d->visibleChars >= len)
                DrawText(">> [SPACE / Tik]", bx + bw - 160, by + bh - 22, 14,
                         (Color){220, 200, 60, 220});
        }

        /* ============================================================
         * DrawWorldMap — Kingdom Rush tarzı level seçim haritası
         * ============================================================ */

        /* Level node'larını yıldız durumu, kilit/açık ile çizer.
         * TODO: DrawTexture(g->assets.worldMapBg, 0, 0, WHITE) ile arka plan sprite'ı */
        void DrawWorldMap(Game * g) {
            /* Arka plan — TODO: DrawTexture(g->assets.worldMapBg, 0, 0, WHITE) */
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){20, 35, 60, 255});
            /* Basit dekoratif ızgara — atmosfer */
            for (int x = 0; x < SCREEN_WIDTH; x += 64)
                DrawLine(x, 0, x, SCREEN_HEIGHT, (Color){255, 255, 255, 8});
            for (int y = 0; y < SCREEN_HEIGHT; y += 64)
                DrawLine(0, y, SCREEN_WIDTH, y, (Color){255, 255, 255, 8});

            const char *title = "DUNYA HARITASI";
            DrawText(title, SCREEN_WIDTH / 2 - MeasureText(title, 40) / 2, 24, 40, GOLD);
            DrawText("Bir seviye secin", SCREEN_WIDTH / 2 - MeasureText("Bir seviye secin", 20) / 2,
                     72, 20, LIGHTGRAY);

            /* Level node'ları arasındaki yol çizgisi */
            for (int i = 0; i + 1 < MAX_LEVELS; i++) {
                LevelData *a = &g->levels[i], *b = &g->levels[i + 1];
                Color lc = b->unlocked ? (Color){160, 130, 60, 210} : (Color){60, 60, 60, 120};
                /* Kesik çizgi: kilitli segmentler */
                if (!b->unlocked) {
                    for (int s = 0; s < 8; s++) {
                        float t0 = s / 8.0f, t1 = (s + 0.5f) / 8.0f;
                        Vector2 p0 = {a->nodeX + (b->nodeX - a->nodeX) * t0,
                                      a->nodeY + (b->nodeY - a->nodeY) * t0};
                        Vector2 p1 = {a->nodeX + (b->nodeX - a->nodeX) * t1,
                                      a->nodeY + (b->nodeY - a->nodeY) * t1};
                        DrawLineV(p0, p1, lc);
                    }
                } else {
                    DrawLine(a->nodeX, a->nodeY, b->nodeX, b->nodeY, lc);
                }
            }

            Vector2 mp = GetMousePosition();
            for (int i = 0; i < MAX_LEVELS; i++) {
                LevelData *lv = &g->levels[i];
                bool hover =
                    Vec2Distance(mp, (Vector2){(float)lv->nodeX, (float)lv->nodeY}) < 32.0f;
                Color nc = lv->unlocked ? (hover ? YELLOW : GOLD) : (Color){70, 70, 70, 200};

                /* Dış halka (seçim efekti) */
                if (hover && lv->unlocked)
                    DrawCircleLines(lv->nodeX, lv->nodeY, 36, (Color){255, 220, 0, 180});

                DrawCircle(lv->nodeX, lv->nodeY, 28, nc);
                DrawCircleLines(lv->nodeX, lv->nodeY, 28,
                                lv->unlocked ? WHITE : (Color){90, 90, 90, 200});

                /* Yıldızlar (üstte) */
                for (int s = 0; s < 3; s++) {
                    Color sc = (s < lv->stars) ? GOLD : (Color){50, 50, 50, 180};
                    DrawCircle(lv->nodeX - 18 + s * 18, lv->nodeY - 38, 6, sc);
                }

                /* Level numarası */
                char num[4];
                snprintf(num, sizeof(num), "%d", i + 1);
                DrawText(num, lv->nodeX - MeasureText(num, 22) / 2, lv->nodeY - 11, 22,
                         lv->unlocked ? BLACK : DARKGRAY);

                /* Level adı (altında) */
                DrawText(lv->name, lv->nodeX - MeasureText(lv->name, 13) / 2, lv->nodeY + 34, 13,
                         lv->unlocked ? WHITE : DARKGRAY);

                /* Kilitli simgesi */
                if (!lv->unlocked) {
                    DrawText("[X]", lv->nodeX - 14, lv->nodeY - 11, 20, DARKGRAY);
                }
            }

            /* Ana Menü geri butonu */
            Rectangle back = {20, (float)SCREEN_HEIGHT - 60, 160, 40};
            DrawButton(back, "< Ana Menu", DARKGRAY, WHITE);
        }

        /* ============================================================
         * DrawLevelBriefing — Level öncesi senaryo ekranı
         * ============================================================ */

        /* TODO: DrawTexture(g->assets.worldMapBg, ...) ile harita arkaplanı */
        void DrawLevelBriefing(Game * g) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){8, 12, 25, 255});

            LevelData *lv = &g->levels[g->currentLevel];

            /* Başlık */
            DrawText(lv->name, SCREEN_WIDTH / 2 - MeasureText(lv->name, 38) / 2, 28, 38, GOLD);

            /* Harita açıklaması */
            DrawText(lv->description, 50, 88, 18, (Color){200, 200, 220, 230});

            /* Düşman lore */
            DrawText(lv->enemyLore, 50, 116, 15, (Color){180, 140, 140, 200});

            /* Asset placeholder bilgi bandı */
            DrawRectangle(0, 145, SCREEN_WIDTH, 22, (Color){60, 30, 30, 180});
            DrawText("TODO: harita arkaplan sprite + müzik", SCREEN_WIDTH / 2 - 180, 148, 14,
                     (Color){200, 100, 100, 220});

            /* Diyalog kutusu */
            DrawDialogueBox(g);

            /* BAŞLA butonu (diyalog bittiyse) */
            if (!g->dialogue.active) {
                Rectangle btn = {SCREEN_WIDTH / 2.0f - 100, (float)SCREEN_HEIGHT - 100, 200, 50};
                DrawButton(btn, "BASLA!", GREEN, BLACK);
            }

            DrawText("[SPACE / Tik] ileri", SCREEN_WIDTH - 230, SCREEN_HEIGHT - 26, 15,
                     (Color){120, 120, 120, 200});
        }

        /* ============================================================
         * DrawLoading — Sahte progress bar + ipucu + spinner
         * ============================================================ */

        /* TODO: DrawTexture(levelArtwork, ...) ile level görseli arkaplan */
        void DrawLoading(Game * g) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){8, 8, 18, 255});

            const char *title = "RULER";
            DrawText(title, SCREEN_WIDTH / 2 - MeasureText(title, 60) / 2, 150, 60, GOLD);

            if (g->currentLevel >= 0 && g->currentLevel < MAX_LEVELS) {
                const char *lvName = g->levels[g->currentLevel].name;
                DrawText(lvName, SCREEN_WIDTH / 2 - MeasureText(lvName, 30) / 2, 228, 30, WHITE);
            }

            /* Progress bar */
            int bx = SCREEN_WIDTH / 2 - 260, by = SCREEN_HEIGHT / 2, bw = 520, bh = 22;
            DrawRectangle(bx, by, bw, bh, (Color){35, 35, 35, 200});
            DrawRectangle(bx, by, (int)(bw * g->loading.progress), bh, (Color){70, 200, 90, 230});
            DrawRectangleLines(bx, by, bw, bh, (Color){140, 140, 140, 180});
            char pct[16];
            snprintf(pct, sizeof(pct), "%d%%", (int)(g->loading.progress * 100));
            DrawText(pct, SCREEN_WIDTH / 2 - MeasureText(pct, 18) / 2, by + 30, 18, LIGHTGRAY);

            /* İpucu metni */
            if (g->loading.tipText)
                DrawText(g->loading.tipText,
                         SCREEN_WIDTH / 2 - MeasureText(g->loading.tipText, 16) / 2, by + 70, 16,
                         (Color){180, 180, 180, 200});

            /* Dönen spinner (animasyon placeholder) */
            float angle = g->loading.timer * 180.0f;
            int sx = SCREEN_WIDTH / 2, sy = by + 120, sr = 18;
            DrawCircleLines(sx, sy, (float)sr, (Color){60, 100, 180, 160});
            float ar = angle * DEG2RAD;
            DrawCircle((int)(sx + cosf(ar) * sr), (int)(sy + sinf(ar) * sr), 4,
                       (Color){100, 180, 255, 220});
            /* TODO: Spinner yerine dönen emblem sprite animasyonu */
        }

        /* ============================================================
         * T90 — DrawDetailTooltip: Nadirlik renkli detay kutusu
         * ============================================================ */

        /* T92 — 5 kademe: rarity.c fonksiyonları kullanılıyor */

        static const char *T90_FLAVOR[] = {
            "Gökyüzü kıran bir bıçak.", "Çelik gibi sert, irade gibi katı.",
            "Gizemli güçler barındırır.", "Hasarı anında onarır.",
            "Kalkan kıran darbe.", "Rüzgâr gibi çeviktir.",
            "Ejderin nefesinden doğdu.", "Toprağı hisseden bir ayak.",
        };

        /* x,y — tooltip'in sol-üst köşesi */
        static void DrawDetailTooltip(int x, int y, const char *name, int rarity,
                                      const char *stats, const char *flavor) {
            if (rarity < 0) rarity = RARITY_COMMON;
            if (rarity >= RARITY_COUNT) rarity = RARITY_COUNT - 1;
            Color rc = RarityColor((ItemRarity)rarity);

            int pw = 240, ph = 96;
            if (x + pw > SCREEN_WIDTH)  x = SCREEN_WIDTH  - pw - 4;
            if (y + ph > SCREEN_HEIGHT) y = SCREEN_HEIGHT - ph - 4;

            DrawRectangle(x, y, pw, ph, (Color){10, 10, 20, 235});
            DrawRectangleLines(x, y, pw, ph, rc);
            DrawRectangle(x, y, pw, 18, (Color){rc.r, rc.g, rc.b, 60});
            char hdr[48];
            snprintf(hdr, sizeof(hdr), "[ %s ]", RarityLabel((ItemRarity)rarity));
            DrawText(hdr, x + pw / 2 - MeasureText(hdr, 11) / 2, y + 4, 11, rc);
            DrawText(name, x + 8, y + 24, 15, WHITE);
            if (stats && stats[0])
                DrawText(stats, x + 8, y + 44, 12, (Color){200, 220, 180, 220});
            /* T92 — flavor: önce isim tablosuna bak, yoksa rarity flavor */
            const char *flv = (flavor && flavor[0])
                              ? flavor
                              : RarityFlavor(name, (ItemRarity)rarity);
            if (flv && flv[0])
                DrawText(flv, x + 8, y + 72, 11, (Color){160, 150, 130, 200});
        }

        /* ============================================================
         * T89 — GenerateLootChest / DrawLootChest
         * ============================================================ */

        /* Level biterken çağrılır; dalga sayısı + yıldıra göre ödül üretir */
        void GenerateLootChest(Game *g) {
            LootChest *lc = &g->lootChest;
            memset(lc, 0, sizeof(LootChest));
            lc->visible    = true;
            lc->animDone   = false;
            lc->openTimer  = 0.0f;

            /* Temel altın: wave sayısı * 20 + yıldız bonusu */
            lc->itemGold = (g->currentWave + 1) * 20 + g->levelStars * 30;
            lc->itemCount = 0;

            /* Sabit eşya listesi: dalga sayısına göre rastgele seç */
            static const char *ITEM_POOL[][3] = {
                /* isim, rarity(0-3 as string) — dummy encoding */
                {"Keskin Kılıç",   "2", ""},
                {"Demir Zırh",     "1", ""},
                {"Büyülü Rün",     "3", ""},
                {"Şifa İksiri",    "0", ""},
                {"Komutan Kalkanı","2", ""},
                {"Hız Tılsımı",    "1", ""},
                {"Ejder Kolyesi",  "3", ""},
                {"Savaşçı Çizmesi","0", ""},
            };
            int poolSize = 8;
            int wave = g->currentWave + 1;
            /* Her 3 dalgada bir ekstra eşya, max 6 */
            int count = 2 + wave / 3;
            if (count > MAX_CHEST_ITEMS) count = MAX_CHEST_ITEMS;

            for (int i = 0; i < count; i++) {
                int idx = (wave * 7 + i * 13) % poolSize;
                strncpy(lc->itemNames[i], ITEM_POOL[idx][0], 31);
                /* T92 — RollRarity: dalga * yıldız bazlı */
                int seed = wave * (g->levelStars + 1);
                ItemRarity r = RollRarity(seed, i);
                lc->itemRarity[i] = (int)r;
                lc->itemCount++;
            }
        }

        /* Sandık animasyonu + eşya listesi çizer */
        static void DrawLootChest(Game *g, int cx, int cy) {
            LootChest *lc = &g->lootChest;

            /* Animasyon: openTimer 0→1, 0.9 saniyede */
            if (!lc->animDone) {
                lc->openTimer += GetFrameTime() * 1.1f;
                if (lc->openTimer >= 1.0f) { lc->openTimer = 1.0f; lc->animDone = true; }
            }

            float t = lc->openTimer;
            /* Sandık gövdesi */
            int bw = 90, bh = 54;
            DrawRectangle(cx - bw/2, cy - bh/2, bw, bh, (Color){110, 72, 20, 255});
            DrawRectangleLines(cx - bw/2, cy - bh/2, bw, bh, (Color){200, 155, 40, 255});
            /* Kilit levha */
            DrawRectangle(cx - 8, cy - 6, 16, 14, (Color){180, 140, 30, 255});
            /* Kapak: t=0 kapalı (0°), t=1 açık (-80°) */
            float lidAngle = -80.0f * t;
            float lidH = (float)bh * 0.45f;
            /* Kapak köşe noktaları (basit dikdörtgen döndürme) */
            float rad = lidAngle * 3.14159f / 180.0f;
            float x0 = (float)(cx - bw/2), y0 = (float)(cy - bh/2);
            float x1 = (float)(cx + bw/2), y1 = (float)(cy - bh/2);
            float hx = cosf(rad) * 0.0f - sinf(rad) * (-lidH);
            float hy = sinf(rad) * 0.0f + cosf(rad) * (-lidH);
            DrawTriangle((Vector2){x0, y0}, (Vector2){x1, y1},
                         (Vector2){x1 + hx, y1 + hy}, (Color){140, 90, 25, 220});
            DrawTriangle((Vector2){x0, y0}, (Vector2){x1 + hx, y1 + hy},
                         (Vector2){x0 + hx, y0 + hy}, (Color){140, 90, 25, 220});
            /* Kapak çerçeve */
            DrawLineEx((Vector2){x0, y0}, (Vector2){x0 + hx, y0 + hy}, 2.0f,
                       (Color){200, 155, 40, 255});
            DrawLineEx((Vector2){x1, y1}, (Vector2){x1 + hx, y1 + hy}, 2.0f,
                       (Color){200, 155, 40, 255});

            /* Açılınca parıltı */
            if (t > 0.7f) {
                float glow = (t - 0.7f) / 0.3f;
                DrawCircle(cx, cy - bh/2 - 10, 22.0f * glow, (Color){255, 230, 80, (unsigned char)(120 * glow)});
            }

            /* Nadirlik renk tablosu */
            /* T92 — 5 kademe rarity */
            static const Color RARITY_COLORS[RARITY_COUNT] = {
                {180, 180, 180, 255}, /* Common    */
                {80,  200, 80,  255}, /* Uncommon  */
                {80,  140, 230, 255}, /* Rare      */
                {180, 60,  240, 255}, /* Epic      */
                {255, 200, 40,  255}, /* Mythical  */
            };
            static const char *RARITY_NAMES[RARITY_COUNT] = {
                "Yaygın","Nadir","Seyrek","Destansı","Efsanevi"
            };

            /* Eşya listesi: sandığın sağında */
            if (lc->animDone) {
                int lx = cx + bw/2 + 20;
                int ly = cy - lc->itemCount * 13;
                Vector2 mp = GetMousePosition();
                int hoveredItem = -1;
                for (int i = 0; i < lc->itemCount; i++) {
                    Color rc = RARITY_COLORS[lc->itemRarity[i]];
                    char line[64];
                    snprintf(line, sizeof(line), "[%s] %s",
                             RARITY_NAMES[lc->itemRarity[i]], lc->itemNames[i]);
                    int iy = ly + i * 26;
                    DrawText(line, lx, iy, 14, rc);
                    /* T90 — hover tespiti */
                    Rectangle row = {(float)lx, (float)iy, 220.0f, 22.0f};
                    if (CheckCollisionPointRec(mp, row)) hoveredItem = i;
                }
                /* T90 — Tooltip: üzerine gelinen eşya */
                if (hoveredItem >= 0) {
                    int fi = hoveredItem % 8;
                    DrawDetailTooltip((int)mp.x + 12, (int)mp.y - 50,
                                      lc->itemNames[hoveredItem],
                                      lc->itemRarity[hoveredItem],
                                      "+5 Hasar  +3 Zırh",
                                      T90_FLAVOR[fi]);
                }
                /* Altın ödülü */
                char gbuf[32];
                snprintf(gbuf, sizeof(gbuf), "+%d Altin", lc->itemGold);
                DrawText(gbuf, cx - MeasureText(gbuf, 16) / 2, cy + bh/2 + 14, 16, GOLD);
            }
        }

        /* ============================================================
         * DrawLevelComplete — Yıldız / ödül ekranı
         * ============================================================ */

        void DrawLevelComplete(Game * g) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){4, 18, 4, 245});

            const char *t1 = "SEVIYE TAMAMLANDI!";
            DrawText(t1, SCREEN_WIDTH / 2 - MeasureText(t1, 52) / 2, 100, 52, GOLD);

            /* Level adı */
            if (g->currentLevel >= 0 && g->currentLevel < MAX_LEVELS)
                DrawText(g->levels[g->currentLevel].name,
                         SCREEN_WIDTH / 2 - MeasureText(g->levels[g->currentLevel].name, 26) / 2,
                         162, 26, WHITE);

            /* Yıldızlar */
            int stars = g->levelStars;
            for (int s = 0; s < 3; s++) {
                int cx = SCREEN_WIDTH / 2 - 80 + s * 80;
                Color fc = (s < stars) ? GOLD : (Color){50, 50, 50, 180};
                DrawCircle(cx, 240, 28, fc);
                DrawCircleLines(cx, 240, 28, (s < stars) ? YELLOW : GRAY);
                DrawText("*", cx - 8, 228, 28, (s < stars) ? BLACK : DARKGRAY);
            }

            /* Skor satırı */
            char buf[80];
            snprintf(buf, sizeof(buf), "Skor: %d   Kill: %d   Can: %d", g->score, g->enemiesKilled,
                     g->lives);
            DrawText(buf, SCREEN_WIDTH / 2 - MeasureText(buf, 22) / 2, 298, 22, WHITE);

            /* Completion diyaloğu */
            DrawDialogueBox(g);

            /* T89 — Loot Chest */
            if (g->lootChest.visible)
                DrawLootChest(g, SCREEN_WIDTH / 2, 430);

            /* Devam butonu (diyalog bittiyse) */
            if (!g->dialogue.active) {
                bool isLast = (g->currentLevel + 1 >= MAX_LEVELS);
                Rectangle btn = {SCREEN_WIDTH / 2.0f - 100, (float)SCREEN_HEIGHT - 120, 200, 55};
                DrawButton(btn, isLast ? "KAMPANYA BITTI!" : "Haritaya Don",
                           isLast ? GOLD : DARKGREEN, isLast ? BLACK : WHITE);
                /* Sandık altınını topla */
                if (IsButtonClicked(btn) && g->lootChest.itemGold > 0) {
                    g->gold += g->lootChest.itemGold;
                    g->lootChest.itemGold = 0;
                }
            }
        }

        /* ============================================================
         * B02 — DrawPathArrows: Yol üzerinde yön okları
         * ============================================================ */

        /* Waypoint segmentleri ortasına küçük üçgen oklar çizer. */

        void DrawPathArrows(Game * g) {
            for (int i = 0; i + 1 < g->waypointCount; i++) {
                Vector2 from = g->waypoints[i];
                Vector2 to = g->waypoints[i + 1];
                Vector2 mid = {(from.x + to.x) / 2.0f, (from.y + to.y) / 2.0f};
                Vector2 dir = Vec2Normalize(Vec2Subtract(to, from));
                Vector2 perp = {-dir.y, dir.x};
                float s = 9.0f;
                Vector2 tip = {mid.x + dir.x * s, mid.y + dir.y * s};
                Vector2 left = {mid.x - dir.x * s + perp.x * s * 0.55f,
                                mid.y - dir.y * s + perp.y * s * 0.55f};
                Vector2 right = {mid.x - dir.x * s - perp.x * s * 0.55f,
                                 mid.y - dir.y * s - perp.y * s * 0.55f};
                DrawTriangle(tip, left, right, (Color){255, 255, 255, 90});
            }
        }

        /* ============================================================
         * B01 — DrawContextMenu: Kule sağ tık menüsü
         * ============================================================ */

        /* Seçili kulenin yanında yükselt/sat seçeneklerini çizer. */

        void DrawContextMenu(Game * g) {
            if (!g->contextMenuOpen || g->contextMenuTowerIdx < 0)
                return;
            Tower *t = &g->towers[g->contextMenuTowerIdx];
            if (!t->active) {
                g->contextMenuOpen = false;
                return;
            }

            /* Kameranın anlık durumuna göre ekrandaki menü lokasyonunu sürekli tazele */
            g->contextMenuPos = GetWorldToScreen2D(t->position, g->camera.cam);
            float mx = g->contextMenuPos.x;
            float my = g->contextMenuPos.y;

            DrawRectangle((int)mx - 2, (int)my - 2, 144, 80, (Color){15, 15, 25, 230});
            DrawRectangleLines((int)mx - 2, (int)my - 2, 144, 80, (Color){200, 200, 200, 160});

            Rectangle upgradeBtn = {mx, my, 140, 34};
            Rectangle sellBtn = {mx, my + 38, 140, 34};

            /* Yükselt butonu */
            char upgLabel[32];
            if (t->level >= 3) {
                snprintf(upgLabel, sizeof(upgLabel), "Maks Seviye");
            } else {
                int cost = GetTowerCost(t->type) * t->level;
                snprintf(upgLabel, sizeof(upgLabel), "Yukselt (%dg)", cost);
            }
            bool canUpgrade = (t->level < 3 && g->gold >= GetTowerCost(t->type) * t->level);
            DrawButton(upgradeBtn, upgLabel, canUpgrade ? DARKGREEN : DARKGRAY, WHITE);

            /* Sat butonu */
            char sellLabel[32];
            snprintf(sellLabel, sizeof(sellLabel), "Sat (+%dg)", GetTowerCost(t->type) / 2);
            DrawButton(sellBtn, sellLabel, (Color){120, 35, 35, 255}, WHITE);

            /* T90 — Kule detay tooltip */
            {
                static const char *TOWER_NAMES[TOWER_TYPE_COUNT] = {"Temel Kule","Keskin Nişancı","Patlama Kulesi"};
                static const char *TOWER_FLAVOR[TOWER_TYPE_COUNT] = {
                    "Güvenilir, sıradan ama vazgeçilmez.",
                    "Uzaktan seçer, asla ıskalamaz.",
                    "Kalabalığı dağıtır, savaşı şekillendirir.",
                };
                char tstats[48];
                snprintf(tstats, sizeof(tstats), "Hasar:%.0f  Menzil:%.0f  Lv:%d",
                         t->damage, t->range, t->level);
                int rarity = (t->type == TOWER_SNIPER) ? 1 : (t->type == TOWER_SPLASH) ? 2 : 0;
                DrawDetailTooltip((int)mx + 148, (int)my, TOWER_NAMES[t->type], rarity,
                                  tstats, TOWER_FLAVOR[t->type]);
            }
        }

        /* ============================================================
         * BUTTON YARDIMCILARI
         * ============================================================ */

        bool IsButtonClicked(Rectangle btn) {
            Vector2 mp = GetMousePosition();
            return IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mp, btn);
        }

        /* Epic UI DrawButton: aynı imza, gradient+bevel+glow görsel.
         * BtnAnim hash'i için ptr adresini index olarak kullanır. */
        void DrawButton(Rectangle btn, const char *label, Color bg, Color fg) {
            (void)bg;
            (void)fg;
/* Her unique çağrı için sabit bir BtnAnim tut (max 32 buton) */
#define MAX_BTN_SLOTS 32
            static BtnAnim btnAnims[MAX_BTN_SLOTS] = {0};
            static Rectangle btnKeys[MAX_BTN_SLOTS] = {0};
            static int btnSlotCount = 0;

            /* Slot bul veya oluştur (x+y+w+h parmak izi) */
            int slot = -1;
            for (int i = 0; i < btnSlotCount; i++) {
                if (btnKeys[i].x == btn.x && btnKeys[i].y == btn.y &&
                    btnKeys[i].width == btn.width && btnKeys[i].height == btn.height) {
                    slot = i;
                    break;
                }
            }
            if (slot < 0 && btnSlotCount < MAX_BTN_SLOTS) {
                slot = btnSlotCount++;
                btnKeys[slot] = btn;
                btnAnims[slot] = (BtnAnim){1.0f, 0.0f};
            }
            if (slot < 0) {
                /* Overflow fallback: basit çizim */
                DrawRectangleRec(btn, (Color){60, 45, 0, 200});
                DrawRectangleLinesEx(btn, 1.5f, UI_GOLD);
                DrawText(label, (int)(btn.x + btn.width / 2 - MeasureText(label, 18) / 2),
                         (int)(btn.y + btn.height / 2 - 9), 18, UI_IVORY);
                return;
            }

            float dt = GetFrameTime();
            DrawEpicButton(btn, label, &btnAnims[slot], dt);
        }

        /* ============================================================
         * T43 — UpdatePrepPhase / DrawPrepPhase
         * ============================================================ */

/* ============================================================
 * T93 — DrawForgeUI: Blacksmith NPC paneli
 * ============================================================ */

/* Demirci NPC replikleri — LORE.md bağlantısı */
static const char *FORGE_NPC_LINES[] = {
    "\"Ates soner, demir konusur. Ne getireceksin?\"",
    "\"Bu kalkani taniyorum... Eski bir ustanin isi.\"",
    "\"Guclendir, komutan. Zira dusman beklemiyor.\"",
    "\"Ejder celik istemiyorsan, siraya gir.\"",
    "\"Demir Cevheri getir, seni geri gondermeyeyim.\"",
};
#define FORGE_LINE_COUNT 5

/* T94 — Upgrade başarı ihtimali (0-100) */
static int ForgeSuccessChance(int curLevel) {
    if (curLevel < 3) return 100;
    if (curLevel == 3) return 70;
    if (curLevel == 4) return 50;
    if (curLevel == 5) return 30;
    return 15;
}

/* T94 — Slot bazlı stat ekleme (ApplyEquipStats'ı taklit eder, ama sadece delta) */
static void ForgeAddUpgradeStat(Hero *hero, EquipSlot slot) {
    switch (slot) {
    case EQUIP_WEAPON:  hero->stats.atk   += 1.5f; break;
    case EQUIP_ARMOR:   hero->stats.def   += 1.0f; break;
    case EQUIP_HEAD:    hero->stats.hp     = hero->stats.hp + 8.0f;
                        hero->stats.maxHp += 8.0f; break;
    case EQUIP_ACCESS:  hero->stats.speed += 0.2f; break;
    default: break;
    }
}

/* T94 — Stat geri al (başarısızlık -1 level düşünce bir önceki bonus çıkarılır) */
static void ForgeRemoveUpgradeStat(Hero *hero, EquipSlot slot) {
    switch (slot) {
    case EQUIP_WEAPON:  hero->stats.atk   -= 1.5f; break;
    case EQUIP_ARMOR:   hero->stats.def   -= 1.0f; break;
    case EQUIP_HEAD:    hero->stats.hp     = hero->stats.hp - 8.0f;
                        hero->stats.maxHp -= 8.0f; break;
    case EQUIP_ACCESS:  hero->stats.speed -= 0.2f; break;
    default: break;
    }
}

void DrawForgeUI(Game *g) {
    static char feedbackMsg[48] = {0};
    static float feedbackTimer  = 0.0f;
    static Color feedbackCol    = {255, 255, 255, 255};

    float dt = GetFrameTime();
    if (feedbackTimer > 0.0f) feedbackTimer -= dt;

    /* Glow timers */
    for (int s = 0; s < EQUIP_SLOT_COUNT; s++)
        if (g->hero.equip[s].upgradeGlow > 0.0f)
            g->hero.equip[s].upgradeGlow -= dt;

    int pw = 580, ph = 480;
    int px = SCREEN_WIDTH  / 2 - pw / 2;
    int py = SCREEN_HEIGHT / 2 - ph / 2;

    /* Koyu örtü */
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 170});

    /* Panel */
    DrawRectangle(px, py, pw, ph, (Color){18, 10, 6, 248});
    DrawRectangleLines(px, py, pw, ph, (Color){255, 100, 30, 210});
    DrawRectangleLines(px + 2, py + 2, pw - 4, ph - 4, (Color){160, 55, 10, 100});

    /* Başlık */
    float t = (float)GetTime();
    const char *title = "DEMIRCI";
    DrawText(title, px + pw / 2 - MeasureText(title, 28) / 2, py + 12, 28,
             (Color){255, 140, 40, 255});

    /* NPC portret */
    int ax = px + 32, ay = py + 52;
    DrawRectangle(ax, ay + 20, 38, 14, (Color){90, 80, 72, 255});
    DrawRectangle(ax - 5, ay + 14, 48, 9, (Color){110, 100, 90, 255});
    DrawRectangle(ax + 13, ay + 34, 12, 22, (Color){72, 64, 56, 255});
    DrawRectangle(ax + 46, ay + 6, 6, 36, (Color){130, 95, 55, 255});
    DrawRectangle(ax + 38, ay - 2, 20, 16, (Color){150, 145, 140, 255});
    for (int s = 0; s < 5; s++) {
        float sx = ax + 38 + cosf(t * 3.2f + s * 1.3f) * 13;
        float sy = ay + 8  + sinf(t * 4.1f + s * 1.0f) * 7 - 3;
        DrawCircle((int)sx, (int)sy, 1.8f, (Color){255, (unsigned char)(180 + s * 15), 20, 200});
    }
    DrawText("Gareth", ax, ay + 62, 11, (Color){190, 150, 90, 210});

    /* Diyalog */
    int dlgX = px + 100, dlgY = py + 52, dlgW = pw - 118, dlgH = 54;
    DrawRectangle(dlgX, dlgY, dlgW, dlgH, (Color){10, 7, 4, 220});
    DrawRectangleLines(dlgX, dlgY, dlgW, dlgH, (Color){130, 80, 35, 150});
    int lineIdx = ((int)(t * 0.22) + g->currentWave) % FORGE_LINE_COUNT;
    DrawText(FORGE_NPC_LINES[lineIdx], dlgX + 6, dlgY + 8, 11, (Color){225, 195, 150, 235});

    /* Ayıraç */
    DrawLine(px + 16, py + 120, px + pw - 16, py + 120, (Color){90, 55, 18, 160});

    /* Malzeme sayacı */
    char matBuf[64];
    snprintf(matBuf, sizeof(matBuf), "Demir Cevheri: %d    Buyulu Toz: %d",
             g->ironOre, g->magicDust);
    DrawText(matBuf, px + 20, py + 128, 13, (Color){210, 170, 80, 240});

    /* Ayıraç 2 */
    DrawLine(px + 16, py + 148, px + pw - 16, py + 148, (Color){90, 55, 18, 120});

    /* Ekipman yükseltme listesi */
    static const char *SLOT_NAMES[EQUIP_SLOT_COUNT] = {"Silah", "Zirh", "Baslik", "Aksesuar"};
    static const char *STAT_HINTS[EQUIP_SLOT_COUNT]  = {"+1.5 ATK", "+1.0 DEF", "+8 HP", "+0.2 HIZ"};
    Vector2 mp = GetMousePosition();

    for (int s = 0; s < EQUIP_SLOT_COUNT; s++) {
        EquippedItem *eq = &g->hero.equip[s];
        int ry = py + 158 + s * 66;

        /* Satır arka planı */
        Color rowBg = eq->occupied ? (Color){28, 18, 10, 200} : (Color){15, 12, 10, 160};
        DrawRectangle(px + 14, ry, pw - 28, 58, rowBg);
        DrawRectangleLines(px + 14, ry, pw - 28, 58, (Color){80, 50, 20, 160});

        /* Slot etiketi */
        DrawText(SLOT_NAMES[s], px + 22, ry + 6, 12, (Color){160, 140, 100, 200});

        if (!eq->occupied) {
            DrawText("(bos)", px + 22, ry + 26, 11, (Color){90, 80, 60, 160});
            continue;
        }

        /* Eşya adı + upgrade level */
        char upgBuf[48];
        int uLv = eq->upgradeLevel;
        snprintf(upgBuf, sizeof(upgBuf), "%s  +%d", eq->name, uLv);

        /* Glow rengi */
        float glow = eq->upgradeGlow;
        Color nameCol = (glow > 0.0f)
            ? (Color){255, (unsigned char)(200 + (int)(sinf(t * 8.0f) * 40)), 30, 255}
            : (Color){220, 190, 130, 255};
        if (glow > 0.0f) {
            /* Parıltı dairesi */
            float r = 8.0f + sinf(t * 6.0f) * 4.0f;
            DrawCircle(px + 22 + MeasureText(upgBuf, 14) / 2, ry + 26, r,
                       (Color){255, 200, 50, (unsigned char)(80 * glow)});
        }
        DrawText(upgBuf, px + 22, ry + 22, 14, nameCol);

        /* Malzeme maliyeti */
        int oreCost  = uLv + 1;
        int dustCost = (uLv >= 3) ? 2 : 1;
        int chance   = ForgeSuccessChance(uLv);
        char costBuf[48];
        snprintf(costBuf, sizeof(costBuf), "Maliyet: %d cevher  %d toz  (%d%%)",
                 oreCost, dustCost, chance);
        DrawText(costBuf, px + 22, ry + 40, 10, (Color){150, 130, 90, 200});

        /* Stat ipucu */
        DrawText(STAT_HINTS[s], px + pw - 150, ry + 10, 11, (Color){100, 180, 100, 200});

        /* Yükselt butonu */
        Rectangle upBtn = {(float)(px + pw - 110), (float)(ry + 28), 94, 24};
        bool canUpgrade = (g->ironOre >= oreCost) && (g->magicDust >= dustCost);
        Color btnBg  = canUpgrade ? (Color){50, 30, 10, 220} : (Color){25, 18, 12, 180};
        Color btnBrd = canUpgrade ? (Color){255, 130, 40, 220} : (Color){80, 60, 40, 160};
        bool hovered = CheckCollisionPointRec(mp, upBtn);
        if (hovered && canUpgrade) btnBg = (Color){80, 45, 15, 240};
        DrawRectangleRec(upBtn, btnBg);
        DrawRectangleLinesEx(upBtn, 1.2f, btnBrd);
        DrawText("Yukselt", (int)(upBtn.x + 12), (int)(upBtn.y + 6), 12,
                 canUpgrade ? (Color){255, 210, 100, 255} : (Color){100, 85, 60, 180});

        if (hovered && canUpgrade && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            g->ironOre   -= oreCost;
            g->magicDust -= dustCost;

            /* Başarı kontrolü */
            int roll = GetRandomValue(1, 100);
            if (roll <= chance) {
                ForgeAddUpgradeStat(&g->hero, (EquipSlot)s);
                eq->upgradeLevel++;
                eq->upgradeGlow = 2.0f;
                snprintf(feedbackMsg, sizeof(feedbackMsg), "+%d! Basarili!", eq->upgradeLevel);
                feedbackCol = (Color){80, 240, 80, 255};
            } else {
                /* Başarısız: -1 level, min 0, önceki bonus geri alınır */
                if (eq->upgradeLevel > 0) {
                    ForgeRemoveUpgradeStat(&g->hero, (EquipSlot)s);
                    eq->upgradeLevel--;
                }
                snprintf(feedbackMsg, sizeof(feedbackMsg), "Basarisiz! Seviye: +%d", eq->upgradeLevel);
                feedbackCol = (Color){240, 80, 60, 255};
            }
            feedbackTimer = 3.0f;
        }
    }

    /* Geri bildirim mesajı */
    if (feedbackTimer > 0.0f && feedbackMsg[0]) {
        float alpha = feedbackTimer > 0.5f ? 1.0f : feedbackTimer * 2.0f;
        Color fc = feedbackCol; fc.a = (unsigned char)(255 * alpha);
        DrawText(feedbackMsg, px + pw / 2 - MeasureText(feedbackMsg, 16) / 2,
                 py + ph - 60, 16, fc);
    }

    /* Kapat butonu */
    Rectangle closeBtn = {(float)(px + pw / 2 - 65), (float)(py + ph - 38), 130, 28};
    DrawRectangleRec(closeBtn, (Color){55, 18, 8, 230});
    DrawRectangleLinesEx(closeBtn, 1.4f, (Color){255, 100, 30, 210});
    DrawText("[F] Kapat", (int)(closeBtn.x + 30), (int)(closeBtn.y + 7), 13,
             (Color){255, 175, 90, 245});
}

        /* Dalga arası inşa fazı: bina yerleştirme + geri sayım */