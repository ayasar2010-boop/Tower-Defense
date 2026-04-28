#include "hud.h"
#include "enemy.h"
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
    case FUNIT_HERO:
    default:
        fu->maxHp = 250;
        fu->hp = 250;
        fu->atk = 30;
        fu->attackRange = 55;
        fu->attackSpeed = 1.2f;
        break;
        fu->maxHp = 250;
        fu->hp = 250;
        fu->atk = 30;
        fu->attackRange = 65;
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

/* Dost birimleri guncelle: hold/attack AI + engaged takibi */

void UpdateFriendlyUnits(Game *g, float dt) {
    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
        FriendlyUnit *fu = &g->friendlyUnits[i];
        if (!fu->active)
            continue;

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

/* Dost birimleri ciz: renk + HP bar + durum ikonu */

void DrawFriendlyUnits(Game *g) {
    Color typeColors[FUNIT_TYPE_COUNT] = {
        (Color){50, 200, 255, 230}, /* ARCHER  — mavi */
        (Color){60, 180, 60, 230},  /* WARRIOR — yesil */
        (Color){200, 80, 255, 230}, /* MAGE    — mor */
        (Color){240, 160, 30, 230}, /* KNIGHT  — turuncu */
        (Color){255, 255, 80, 230}, /* HERO    — sari */
    };
    const char *labels[FUNIT_TYPE_COUNT] = {"Ok", "Sv", "By", "Sv", "Hr"};

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
            DrawButton(btn,  "OYUNA BASLA", GREEN,    BLACK);
            DrawButton(sBtn, "AYARLAR [S]", DARKGRAY, WHITE);
        }

        /* ============================================================
         * T34 — DrawPauseOverlay
         * ============================================================ */

        void DrawPauseOverlay(Game * g) {
            (void)g;
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 170});

            Rectangle panel = {(float)SCREEN_WIDTH / 2 - 180, (float)SCREEN_HEIGHT / 2 - 120, 360,
                               260};
            DrawEpicPanel(panel, "DURAKLATILDI");

            Rectangle resume = {(float)SCREEN_WIDTH / 2 - 90, (float)SCREEN_HEIGHT / 2 - 10, 180,
                                50};
            Rectangle quit = {(float)SCREEN_WIDTH / 2 - 90, (float)SCREEN_HEIGHT / 2 + 72, 180, 50};
            DrawButton(resume, "DEVAM ET", DARKGREEN, WHITE);
            DrawButton(quit, "CIK", DARKGRAY, WHITE);
        }

        /* ============================================================
         * T57 — DrawSettingsMenu / UpdateSettingsMenu
         * ============================================================ */

        static int g_settingsSel = 0; /* seçili satır: 0-5 */

        /* Hacimli slider çizer: sol kenarda etiket, sağda dolu bar */
        static void DrawSlider(float x, float y, float w, float h,
                               const char *label, float value, bool selected) {
            Color bg  = selected ? (Color){60, 80, 120, 200} : (Color){30, 35, 55, 180};
            Color fill = (Color){80, 180, 100, 255};
            DrawRectangleRec((Rectangle){x, y, w, h}, bg);
            if (selected)
                DrawRectangleLinesEx((Rectangle){x, y, w, h}, 2, (Color){140, 200, 255, 255});
            /* metin */
            DrawTextEx(g_ui.body, label, (Vector2){x + 10, y + h * 0.25f},
                       UIS(18.0f), 1, WHITE);
            /* bar arka planı */
            float bx = x + w * 0.55f;
            float bw = w * 0.38f;
            DrawRectangle((int)bx, (int)(y + h * 0.35f), (int)bw, (int)(h * 0.3f),
                          (Color){20, 20, 30, 220});
            /* dolum */
            DrawRectangle((int)bx, (int)(y + h * 0.35f), (int)(bw * value), (int)(h * 0.3f), fill);
            /* yüzde */
            char pct[8];
            snprintf(pct, sizeof(pct), "%d%%", (int)(value * 100));
            DrawTextEx(g_ui.body, pct,
                       (Vector2){bx + bw + 8, y + h * 0.25f}, UIS(16.0f), 1, UI_IVORY);
        }

        static void DrawToggleRow(float x, float y, float w, float h,
                                  const char *label, bool value, bool selected) {
            Color bg = selected ? (Color){60, 80, 120, 200} : (Color){30, 35, 55, 180};
            DrawRectangleRec((Rectangle){x, y, w, h}, bg);
            if (selected)
                DrawRectangleLinesEx((Rectangle){x, y, w, h}, 2, (Color){140, 200, 255, 255});
            DrawTextEx(g_ui.body, label, (Vector2){x + 10, y + h * 0.25f},
                       UIS(18.0f), 1, WHITE);
            const char *val = value ? "ACIK" : "KAPALI";
            Color vc = value ? (Color){80, 200, 100, 255} : (Color){180, 80, 80, 255};
            DrawTextEx(g_ui.body, val,
                       (Vector2){x + w * 0.7f, y + h * 0.25f}, UIS(18.0f), 1, vc);
        }

        void DrawSettingsMenu(Game *g) {
            DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                                   (Color){6, 8, 18, 255}, (Color){18, 22, 40, 255});

            float pw = 480, ph = 360;
            float px = SCREEN_WIDTH / 2.0f - pw / 2.0f;
            float py = SCREEN_HEIGHT / 2.0f - ph / 2.0f;
            DrawEpicPanel((Rectangle){px, py, pw, ph}, "AYARLAR");

            float rh = 46, gap = 8;
            float rx = px + 20, rw = pw - 40;
            float ry = py + 60;

            DrawToggleRow(rx, ry,           rw, rh, "Tam Ekran",      g->settings.fullscreen,   g_settingsSel == 0);
            DrawSlider   (rx, ry + (rh+gap),    rw, rh, "Ana Ses",    g->settings.masterVolume, g_settingsSel == 1);
            DrawSlider   (rx, ry + (rh+gap)*2,  rw, rh, "SFX Ses",   g->settings.sfxVolume,    g_settingsSel == 2);
            DrawSlider   (rx, ry + (rh+gap)*3,  rw, rh, "Muzik Ses", g->settings.musicVolume,  g_settingsSel == 3);

            /* Kaydet / Geri butonları */
            float by = py + ph - 70;
            Rectangle bSave = {px + 30,        by, 190, 44};
            Rectangle bBack = {px + pw - 220,  by, 190, 44};
            DrawButton(bSave, "KAYDET",  DARKGREEN, WHITE);
            DrawButton(bBack, "GERI",    DARKGRAY,  WHITE);

            DrawBodyText("Sol/Sag: deger degistir   |   Enter/Space: togla   |   ESC: geri",
                         (Vector2){px + 10, py + ph - 22}, 14.0f, (Color){140,140,160,200});
        }

        /* Ayarları AudioManager'a uygular */
        static void ApplySettings(Game *g) {
            g->audio.masterVolume = g->settings.masterVolume;
            if (g->settings.fullscreen != IsWindowFullscreen())
                ToggleFullscreen();
        }

        void UpdateSettingsMenu(Game *g) {
            int itemCount = 4;
            float step = 0.05f;

            if (IsKeyPressed(KEY_UP))   g_settingsSel = (g_settingsSel - 1 + itemCount) % itemCount;
            if (IsKeyPressed(KEY_DOWN)) g_settingsSel = (g_settingsSel + 1) % itemCount;

            float *vol = NULL;
            if      (g_settingsSel == 1) vol = &g->settings.masterVolume;
            else if (g_settingsSel == 2) vol = &g->settings.sfxVolume;
            else if (g_settingsSel == 3) vol = &g->settings.musicVolume;

            if (vol) {
                if (IsKeyDown(KEY_LEFT))  *vol = (*vol - step < 0.0f)   ? 0.0f   : *vol - step;
                if (IsKeyDown(KEY_RIGHT)) *vol = (*vol + step > 1.0f)   ? 1.0f   : *vol + step;
            }
            if (g_settingsSel == 0 && (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)))
                g->settings.fullscreen = !g->settings.fullscreen;

            /* Panel butonları */
            float pw = 480, ph = 360;
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
                g_ui.title, "YENILGI",
                (Vector2){(float)SCREEN_WIDTH / 2 - UIS(100), (float)SCREEN_HEIGHT / 2 - 130},
                UIS(64.0f), 2.0f, (Color){220, 50, 50, 255});

            char buf[64];
            snprintf(buf, sizeof(buf), "Skor: %d   Dusmanlar: %d", g->score, g->enemiesKilled);
            DrawBodyText(
                buf, (Vector2){(float)SCREEN_WIDTH / 2 - UIS(115), (float)SCREEN_HEIGHT / 2 - 20},
                20.0f, UI_IVORY);

            Rectangle btn = {(float)SCREEN_WIDTH / 2 - 110, (float)SCREEN_HEIGHT / 2 + 50, 220, 56};
            DrawButton(btn, "Yeniden Baslat [R]", DARKGRAY, WHITE);
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
                "ZAFER!",
                (Vector2){(float)SCREEN_WIDTH / 2 - UIS(72), (float)SCREEN_HEIGHT / 2 - 130},
                72.0f);

            char buf[64];
            snprintf(buf, sizeof(buf), "Final Skor: %d", g->score);
            DrawBodyText(
                buf, (Vector2){(float)SCREEN_WIDTH / 2 - UIS(100), (float)SCREEN_HEIGHT / 2 - 20},
                24.0f, UI_IVORY);

            Rectangle btn = {(float)SCREEN_WIDTH / 2 - 110, (float)SCREEN_HEIGHT / 2 + 50, 220, 56};
            DrawButton(btn, "Yeniden Baslat [R]", DARKGREEN, BLACK);
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


        void DrawGame(Game * g) {
            DrawMap(g);
            DrawPathArrows(g);
            DrawPlacementPreview(g);
            DrawTowerSynergies(g);
            DrawTowers(g);
            DrawFriendlyUnits(g);
            DrawEnemies(g);
            DrawProjectiles(g);
            DrawParticles(g);
            DrawFloatingTexts(g);
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

            /* Devam butonu (diyalog bittiyse) */
            if (!g->dialogue.active) {
                bool isLast = (g->currentLevel + 1 >= MAX_LEVELS);
                Rectangle btn = {SCREEN_WIDTH / 2.0f - 100, (float)SCREEN_HEIGHT - 120, 200, 55};
                DrawButton(btn, isLast ? "KAMPANYA BITTI!" : "Haritaya Don",
                           isLast ? GOLD : DARKGREEN, isLast ? BLACK : WHITE);
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

        /* Dalga arası inşa fazı: bina yerleştirme + geri sayım */