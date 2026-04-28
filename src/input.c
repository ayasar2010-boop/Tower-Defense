#include "audio.h"
#include "hud.h"
#include "map.h"
#include "tower.h"
#include "types.h"
#include "util.h"
#include "wave.h"
#include <math.h>
#include <string.h>

bool GameSave(Game *g, int slot);
bool GameLoad(Game *g, int slot);

void HandleInput(Game *g) {
    int *km = g->settings.keymap; /* T58 — tuş haritası kısayolu */

    /* Kule tipi seç — bina seçimini sıfırla */
    if (IsKeyPressed(km[KA_TOWER_BASIC])) {
        g->selectedTowerType = TOWER_BASIC;
        g->selectedBuildingType = -1;
    }
    if (IsKeyPressed(km[KA_TOWER_SNIPER])) {
        g->selectedTowerType = TOWER_SNIPER;
        g->selectedBuildingType = -1;
    }
    if (IsKeyPressed(km[KA_TOWER_SPLASH])) {
        g->selectedTowerType = TOWER_SPLASH;
        g->selectedBuildingType = -1;
    }

    /* Oyun hızı */
    if (IsKeyPressed(km[KA_SPEED]))
        g->gameSpeed = (g->gameSpeed < 1.5f) ? 2.0f : 1.0f;

    /* Hızlı kayıt/yükleme: F5 = kaydet (slot 0), F9 = yükle (slot 0) */
    if (IsKeyPressed(KEY_F5))
        GameSave(g, 0);
    if (IsKeyPressed(KEY_F9))
        GameLoad(g, 0);

    /* Grid göster/gizle */
    if (IsKeyPressed(km[KA_GRID]))
        g->showGrid = !g->showGrid;

    /* T63 — Hero kamera takibi toggle */
    if (IsKeyPressed(km[KA_HERO_FOLLOW]))
        g->camera.heroFollow = !g->camera.heroFollow;

    /* ESC: önce kule/bina seçimi iptal, yoksa duraklat */
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (g->selectedTowerType != (TowerType)-1 || g->selectedBuildingType >= 0) {
            g->selectedTowerType = (TowerType)-1;
            g->selectedBuildingType = -1;
        } else {
            g->contextMenuOpen = false;
            g->state = STATE_PAUSED;
        }
    }
    if (IsKeyPressed(km[KA_PAUSE])) {
        g->contextMenuOpen = false;
        g->state = STATE_PAUSED;
    }

    /* Bina seçimi */
    if (IsKeyPressed(km[KA_BLDG_BARRACKS])) {
        g->selectedBuildingType = BUILDING_BARRACKS;
        g->selectedTowerType = (TowerType)-1;
    }
    if (IsKeyPressed(km[KA_BLDG_MARKET])) {
        g->selectedBuildingType = BUILDING_MARKET;
        g->selectedTowerType = (TowerType)-1;
    }
    if (IsKeyPressed(km[KA_BLDG_TOWNCENTER])) {
        g->selectedBuildingType = BUILDING_TOWN_CENTER;
        g->selectedTowerType = (TowerType)-1;
    }

    /* T65 — RTS Seçim Kutusu: sol tık basılı sürükleme */
    Vector2 screenMp = GetMousePosition();
    Vector2 worldMp = GetScreenToWorld2D(screenMp, g->camera.cam);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        g->isSelecting = true;
        g->selectionStart = screenMp;
        g->selectionEnd = screenMp;
    }
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && g->isSelecting) {
        g->selectionEnd = screenMp;
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && g->isSelecting) {
        g->isSelecting = false;
        float dx = g->selectionEnd.x - g->selectionStart.x;
        float dy = g->selectionEnd.y - g->selectionStart.y;
        float dragDist = sqrtf(dx * dx + dy * dy);

        if (dragDist > 8.0f) {
            /* Büyük sürükleme → birim seçimi */
            /* Seçim dikdörtgenini dünya koordinatına çevir */
            Vector2 worldStart = GetScreenToWorld2D(g->selectionStart, g->camera.cam);
            Vector2 worldEnd = GetScreenToWorld2D(g->selectionEnd, g->camera.cam);
            float minX = fminf(worldStart.x, worldEnd.x);
            float maxX = fmaxf(worldStart.x, worldEnd.x);
            float minY = fminf(worldStart.y, worldEnd.y);
            float maxY = fmaxf(worldStart.y, worldEnd.y);
            for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
                FriendlyUnit *fu = &g->friendlyUnits[i];
                if (!fu->active) {
                    fu->selected = false;
                    continue;
                }
                fu->selected = (fu->position.x >= minX && fu->position.x <= maxX &&
                                fu->position.y >= minY && fu->position.y <= maxY);
            }
        } else {
            /* Küçük tık → kule/bina yerleştirme */
            /* Tüm seçimleri kaldır */
            for (int i = 0; i < MAX_FRIENDLY_UNITS; i++)
                g->friendlyUnits[i].selected = false;

            /* Birim yerleştirme modu */
            if (g->pendingPlacementCount > 0 && !g->contextMenuOpen) {
                int gx, gy;
                if (WorldToGrid(worldMp, &gx, &gy) &&
                    (g->grid[gy][gx] == CELL_BUILDABLE || g->grid[gy][gx] == CELL_PATH)) {
                    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
                        if (g->friendlyUnits[i].active)
                            continue;
                        InitFriendlyUnit(&g->friendlyUnits[i], g->pendingPlacementType,
                                         GridToWorld(gx, gy));
                        g->pendingPlacementCount--;
                        break;
                    }
                }
            }
            /* Bağlam menüsü */
            else if (g->contextMenuOpen) {
                if (g->contextMenuTowerIdx >= 0) {
                    // Kameraya göre menünün ekrandaki yerini senkronize et
                    g->contextMenuPos = GetWorldToScreen2D(
                        g->towers[g->contextMenuTowerIdx].position, g->camera.cam);
                }
                Rectangle menuRect = {g->contextMenuPos.x - 2, g->contextMenuPos.y - 2, 144, 80};
                if (CheckCollisionPointRec(screenMp, menuRect) && g->contextMenuTowerIdx >= 0) {
                    Tower *t = &g->towers[g->contextMenuTowerIdx];
                    Rectangle upgradeBtn = {g->contextMenuPos.x, g->contextMenuPos.y, 140, 34};
                    Rectangle sellBtn = {g->contextMenuPos.x, g->contextMenuPos.y + 38, 140, 34};
                    if (t->active && CheckCollisionPointRec(screenMp, upgradeBtn) && t->level < 3) {
                        int cost = GetTowerCost(t->type) * t->level;
                        if (g->gold >= cost) {
                            g->gold -= cost;
                            t->level++;
                            t->damage *= 1.3f;
                            t->range *= 1.1f;
                            PlaySFX(&g->audio, SFX_TOWER_UPGRADE);
                        }
                    } else if (t->active && CheckCollisionPointRec(screenMp, sellBtn)) {
                        g->gold += GetTowerCost(t->type) / 2;
                        PlaySFX(&g->audio, SFX_TOWER_SELL);
                        g->grid[t->gridY][t->gridX] = CELL_BUILDABLE;
                        t->active = false;
                    }
                } else {
                    g->contextMenuOpen = false; // Menü dışına tıklandıysa kapat
                }
                g->contextMenuOpen = false;
            }
            /* T67 — Bina yerleştirme (rural alana) */
            else if (g->selectedBuildingType >= 0) {
                int gx, gy;
                if (WorldToGrid(worldMp, &gx, &gy) && g->grid[gy][gx] == CELL_RURAL) {
                    int cost = 0;
                    switch (g->selectedBuildingType) {
                    case BUILDING_BARRACKS:
                        cost = 100;
                        break;
                    case BUILDING_MARKET:
                        cost = 120;
                        break;
                    case BUILDING_TOWN_CENTER:
                        cost = 200;
                        break;
                    default:
                        break;
                    }
                    /* Town Center maks 1 adet kontrolü */
                    bool canBuild = (g->gold >= cost);
                    if (g->selectedBuildingType == BUILDING_TOWN_CENTER) {
                        for (int i = 0; i < g->buildingCount; i++)
                            if (g->buildings[i].active &&
                                g->buildings[i].type == BUILDING_TOWN_CENTER)
                                canBuild = false;
                    }
                    if (canBuild && g->buildingCount < MAX_BUILDINGS) {
                        Building *b = &g->buildings[g->buildingCount++];
                        memset(b, 0, sizeof(Building));
                        b->gridX = gx;
                        b->gridY = gy;
                        b->type = (BuildingType)g->selectedBuildingType;
                        b->active = true;
                        g->gold -= cost;
                        g->grid[gy][gx] = CELL_TOWER; /* İşgal edildi */
                    }
                }
            }
            /* Kule yerleştirme */
            else if ((int)g->selectedTowerType >= 0 &&
                     (int)g->selectedTowerType < TOWER_TYPE_COUNT) {
                int gx, gy;
                if (WorldToGrid(worldMp, &gx, &gy) && CanPlaceTower(g, gx, gy)) {
                    for (int i = 0; i < MAX_TOWERS; i++) {
                        if (g->towers[i].active)
                            continue;
                        Tower *t = &g->towers[i];
                        memset(t, 0, sizeof(Tower));
                        t->gridX = gx;
                        t->gridY = gy;
                        t->position = GridToWorld(gx, gy);
                        t->type = g->selectedTowerType;
                        t->level = 1;
                        t->targetEnemyIndex = -1;
                        t->active = true;
                        switch (t->type) {
                        case TOWER_BASIC:
                            t->range = 200;
                            t->damage = 20;
                            t->fireRate = 2.0f;
                            t->range = 250;
                            t->damage = 20;
                            t->fireRate = 2.0f;
                            t->splashRadius = 0;
                            t->color = BLUE;
                            break;
                        case TOWER_SNIPER:
                            t->range = 400;
                            t->damage = 80;
                            t->fireRate = 0.5f;
                            t->range = 450;
                            t->damage = 80;
                            t->fireRate = 0.5f;
                            t->splashRadius = 0;
                            t->color = DARKGREEN;
                            break;
                        case TOWER_SPLASH:
                            t->range = 160;
                            t->damage = 30;
                            t->fireRate = 1.0f;
                            t->splashRadius = 70;
                            t->color = MAROON;
                            break;
                            t->range = 180;
                            t->damage = 30;
                            t->fireRate = 1.0f;
                            t->splashRadius = 100;
                            t->color = MAROON;
                            break;
                        default:
                            break;
                        }
                        if (g->hero.heroClass == HERO_WARRIOR)
                            t->damage *= 1.15f;
                        else if (g->hero.heroClass == HERO_MAGE)
                            t->range *= 1.20f;
                        else if (g->hero.heroClass == HERO_ARCHER)
                            t->fireRate *= 1.25f;
                        g->gold -= GetTowerCost(t->type);
                        g->grid[gy][gx] = CELL_TOWER;
                        ApplySynergyBonuses(g, i);
                        PlaySFX(&g->audio, SFX_TOWER_PLACE);
                        break;
                    }
                }
            }
        }
    }

    /* Sağ tık — bağlam menüsü veya seçili birimlere hareket emri */
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        bool hitTower = false;
        for (int i = 0; i < MAX_TOWERS; i++) {
            Tower *t = &g->towers[i];
            if (!t->active)
                continue;
            if (Vec2Distance(worldMp, t->position) < CELL_SIZE / 2.0f) {
                if (Vec2Distance(worldMp, t->position) <
                    CELL_SIZE * 1.5f) { /* Kuleler devasa olduğu için tıklama alanı genişletildi */
                    g->contextMenuOpen = true;
                    g->contextMenuTowerIdx = i;
                    g->contextMenuPos = screenMp;
                    g->contextMenuPos = GetWorldToScreen2D(
                        t->position, g->camera.cam); /* Screen koordinatını doğru kaydet */
                    hitTower = true;
                    break;
                }
            }
            if (!hitTower) {
                g->contextMenuOpen = false;
                /* Seçili birimler varsa hareket emri ver */
                bool anySelected = false;
                for (int i = 0; i < MAX_FRIENDLY_UNITS; i++)
                    if (g->friendlyUnits[i].active && g->friendlyUnits[i].selected) {
                        anySelected = true;
                        break;
                    }
                if (anySelected) {
                    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
                        FriendlyUnit *fu = &g->friendlyUnits[i];
                        if (!fu->active || !fu->selected)
                            continue;
                        fu->order = FUNIT_MOVE;
                        fu->moveTarget = worldMp;
                    }
                }
            }
        }
    }
}

/* ============================================================
 * UpdateMenu / UpdatePause / UpdateWaveClear
 * ============================================================ */
