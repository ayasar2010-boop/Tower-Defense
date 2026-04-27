#include "dungeon.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

/* ── Yardımcılar ──────────────────────────────────────────────────── */

static float DLen(Vector2 a, Vector2 b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return sqrtf(dx*dx + dy*dy);
}

/* ── Oda + Tile Üretimi ───────────────────────────────────────────── */

/* Tile haritasını duvarlarla doldur, sonra odaları kazı */
static void BuildTileMap(DungeonMode *d) {
    /* Her şeyi duvar yap */
    for (int r = 0; r < DUNGEON_ROWS; r++)
        for (int c = 0; c < DUNGEON_COLS; c++)
            d->tiles[r][c] = DTILE_WALL;

    /* Sabit 6 oda düzeni — merkeze yakın, aralarında koridor */
    DungeonRoom rooms[MAX_ROOMS] = {
        {1,  1,  5, 4},   /* sol üst */
        {7,  1,  5, 4},   /* orta üst */
        {13, 1,  6, 4},   /* sağ üst */
        {1,  7,  5, 4},   /* sol alt */
        {7,  7,  5, 4},   /* orta alt */
        {13, 7,  6, 4},   /* sağ alt */
    };
    d->roomCount = MAX_ROOMS;
    memcpy(d->rooms, rooms, sizeof(rooms));

    /* Odaları zemine çevir */
    for (int i = 0; i < d->roomCount; i++) {
        DungeonRoom *ro = &d->rooms[i];
        for (int r = ro->y; r < ro->y + ro->h && r < DUNGEON_ROWS; r++)
            for (int c = ro->x; c < ro->x + ro->w && c < DUNGEON_COLS; c++)
                d->tiles[r][c] = DTILE_FLOOR;
    }

    /* Odalar arasında yatay/dikey koridorlar */
    /* üst satır koridoru: oda 0–1, 1–2 */
    for (int c = 6; c <= 7;  c++) { d->tiles[3][c] = DTILE_FLOOR; d->tiles[4][c] = DTILE_FLOOR; }
    for (int c = 12; c <= 13; c++){ d->tiles[3][c] = DTILE_FLOOR; d->tiles[4][c] = DTILE_FLOOR; }
    /* alt satır koridoru: oda 3–4, 4–5 */
    for (int c = 6; c <= 7;  c++) { d->tiles[8][c] = DTILE_FLOOR; d->tiles[9][c] = DTILE_FLOOR; }
    for (int c = 12; c <= 13; c++){ d->tiles[8][c] = DTILE_FLOOR; d->tiles[9][c] = DTILE_FLOOR; }
    /* dikey koridorlar sol/orta/sağ */
    for (int r = 5; r <= 7; r++) {
        d->tiles[r][3]  = DTILE_FLOOR;
        d->tiles[r][9]  = DTILE_FLOOR;
        d->tiles[r][15] = DTILE_FLOOR;
    }
}

/* Bir odaya 2-3 mob ve loot spawn et */
static void PopulateRoom(DungeonMode *d, int roomIdx) {
    DungeonRoom *ro = &d->rooms[roomIdx];
    /* mob merkezi: oda içi */
    int mobCount = 2 + roomIdx % 2;
    for (int m = 0; m < mobCount; m++) {
        for (int i = 0; i < MAX_DUNGEON_MOBS; i++) {
            if (d->mobs[i].active) continue;
            d->mobs[i].position = (Vector2){
                (float)((ro->x + 1 + m) * DUNGEON_TILE_SIZE + 24),
                (float)((ro->y + 1)     * DUNGEON_TILE_SIZE + 24)
            };
            d->mobs[i].hp      = 40.0f + roomIdx * 15.0f;
            d->mobs[i].maxHp   = d->mobs[i].hp;
            d->mobs[i].speed   = 55.0f;
            d->mobs[i].active  = true;
            break;
        }
    }
    /* Oda ortasına loot */
    for (int i = 0; i < MAX_LOOT_ITEMS; i++) {
        if (d->loots[i].active) continue;
        d->loots[i].position = (Vector2){
            (float)((ro->x + ro->w/2) * DUNGEON_TILE_SIZE + 24),
            (float)((ro->y + ro->h/2) * DUNGEON_TILE_SIZE + 24)
        };
        d->loots[i].type   = (LootType)(roomIdx % 3);
        d->loots[i].value  = 10 + roomIdx * 5;
        d->loots[i].active = true;
        break;
    }
}

/* Sınıfa göre hero istatistiklerini başlatır (T52). */
void InitHero(Hero *hero, HeroClass cls) {
    hero->heroClass   = cls;
    hero->state       = HERO_STATE_IDLE;
    hero->level       = 1;
    hero->xp          = 0;
    hero->xpToNext    = 100;
    hero->attackTimer = 0.0f;
    hero->invulnTimer = 0.0f;
    hero->comboTimer  = 0.0f;
    hero->comboCount  = 0;
    hero->bodyAngle   = 0.0f;
    hero->animTimer   = 0.0f;
    hero->alive       = true;
    hero->isMoving    = false;
    hero->targetPos   = (Vector2){0, 0};

    /* Q/W/E/R max cooldownlar (saniye) */
    hero->skillMaxCD[0] = 4.0f;   /* Q — Kılıç Darbesi */
    hero->skillMaxCD[1] = 8.0f;   /* W — Komuta Çığlığı */
    hero->skillMaxCD[2] = 12.0f;  /* E — Sis Perdesi */
    hero->skillMaxCD[3] = ALLY_CALL_COOLDOWN; /* R — Destek Çağır */
    for (int i = 0; i < 4; i++) hero->skillCooldown[i] = 0.0f;

    switch (cls) {
        case HERO_WARRIOR:
            hero->stats = (HeroStats){700,700, 80, 80,2.0f, 40,20,100, 35, 1.5f, 0.10f,2.0f};
            hero->bodyColor   = SKYBLUE;
            hero->accentColor = (Color){200, 240, 255, 255};
            break;
        case HERO_MAGE:
            hero->stats = (HeroStats){350,350,300,300,8.0f, 15, 5, 90,120, 0.8f, 0.05f,3.0f};
            hero->bodyColor   = VIOLET;
            hero->accentColor = (Color){210, 180, 255, 255};
            break;
        case HERO_ARCHER:
        default:
            hero->stats = (HeroStats){450,450,150,150,4.0f, 30,10,130,160, 2.0f, 0.20f,1.8f};
            hero->bodyColor   = LIME;
            hero->accentColor = (Color){180, 255, 150, 255};
            break;
    }
}

/* XP ekler, gerekirse level atlatır. Her level: stat artışı + tam iyileşme. */
void AddHeroXP(Hero *hero, int amount) {
    if (!hero->alive || hero->level >= 20) return;
    hero->xp += amount;
    while (hero->xp >= hero->xpToNext && hero->level < 20) {
        hero->xp -= hero->xpToNext;
        hero->level++;
        hero->xpToNext = (int)(100 * 1.35f * (hero->level - 1) + 100);
        /* Stat artışı sınıfa göre */
        hero->stats.maxHp   += (hero->heroClass == HERO_WARRIOR) ? 40 : 20;
        hero->stats.maxMana += (hero->heroClass == HERO_MAGE)    ? 25 : 10;
        hero->stats.atk     += (hero->heroClass == HERO_ARCHER)  ? 5  : 3;
        hero->stats.def     += (hero->heroClass == HERO_WARRIOR) ? 4  : 2;
        hero->stats.hp       = hero->stats.maxHp;
        hero->stats.mana     = hero->stats.maxMana;
    }
}

void InitDungeon(DungeonMode *dungeon, Hero *hero) {
    memset(dungeon, 0, sizeof(DungeonMode));
    BuildTileMap(dungeon);
    for (int i = 0; i < dungeon->roomCount; i++)
        PopulateRoom(dungeon, i);

    /* Hero sınıfı korunuyor, sadece pozisyon sıfırlanıyor */
    hero->position    = (Vector2){
        (float)(dungeon->rooms[0].x * DUNGEON_TILE_SIZE + DUNGEON_TILE_SIZE),
        (float)(dungeon->rooms[0].y * DUNGEON_TILE_SIZE + DUNGEON_TILE_SIZE)
    };
    hero->attackTimer = 0.0f;
    hero->alive       = true;
    hero->state       = HERO_STATE_IDLE;

    dungeon->isDungeonActive = true;
}

/* ── Hero Güncelleme ─────────────────────────────────────────────── */

/* Tile geçilebilir mi kontrol eder */
static bool IsTileWalkable(DungeonMode *dungeon, float wx, float wy) {
    int tc = (int)(wx / DUNGEON_TILE_SIZE);
    int tr = (int)(wy / DUNGEON_TILE_SIZE);
    if (tr < 0 || tr >= DUNGEON_ROWS || tc < 0 || tc >= DUNGEON_COLS) return false;
    return dungeon->tiles[tr][tc] != DTILE_WALL;
}

/* Yakın moblara hasar uygular */
static void HeroMeleeAttack(Hero *hero, DungeonMode *dungeon) {
    hero->attackTimer = 1.0f / hero->stats.attackSpeed;
    hero->state = HERO_STATE_ATTACKING;
    for (int i = 0; i < MAX_DUNGEON_MOBS; i++) {
        DungeonMob *m = &dungeon->mobs[i];
        if (!m->active) continue;
        if (DLen(hero->position, m->position) <= hero->stats.attackRange) {
            bool isCrit = ((float)GetRandomValue(0, 999) / 1000.0f) < hero->stats.critChance;
            float dmg = hero->stats.atk * (isCrit ? hero->stats.critMult : 1.0f);
            m->hp -= dmg;
            if (m->hp <= 0.0f) {
                m->active = false;
                AddHeroXP(hero, 20 + hero->level * 5);
            }
        }
    }
}

/* Q — Kılıç Darbesi: mouse yönünde 120° koni hasar + sallama görseli */
static void SkillQ(Hero *hero, DungeonMode *dungeon) {
    if (hero->skillCooldown[0] > 0.0f) return;
    hero->skillCooldown[0] = hero->skillMaxCD[0];
    Vector2 mouse = GetMousePosition();
    float dx = mouse.x - hero->position.x;
    float dy = mouse.y - hero->position.y;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 1.0f) return;
    float nx = dx / len, ny = dy / len;

    /* Görsel: koni yönü ve timer kaydet */
    hero->swingAngle = atan2f(dy, dx) * RAD2DEG;
    hero->swingTimer = 0.30f;

    float coneRange = 80.0f;
    for (int i = 0; i < MAX_DUNGEON_MOBS; i++) {
        DungeonMob *m = &dungeon->mobs[i];
        if (!m->active) continue;
        float mx = m->position.x - hero->position.x;
        float my = m->position.y - hero->position.y;
        float dist = sqrtf(mx*mx + my*my);
        if (dist > coneRange) continue;
        /* 120° koni: dot product > cos(60°) ≈ 0.5 */
        float dot = (mx / dist) * nx + (my / dist) * ny;
        if (dot < 0.5f) continue;
        float dmg = hero->stats.atk * 1.5f;
        m->hp -= dmg;
        if (m->hp <= 0.0f) {
            m->active = false;
            AddHeroXP(hero, 20 + hero->level * 5);
        }
    }
}

/* W — Komuta Çığlığı: hero'nun HP'sini %20 yeniler */
static void SkillW(Hero *hero) {
    if (hero->skillCooldown[1] > 0.0f) return;
    hero->skillCooldown[1] = hero->skillMaxCD[1];
    hero->stats.hp += hero->stats.maxHp * 0.20f;
    if (hero->stats.hp > hero->stats.maxHp) hero->stats.hp = hero->stats.maxHp;
}

/* E — Sis Perdesi: 100px yarıçaptaki moblara 3s yavaşlatma */
static void SkillE(Hero *hero, DungeonMode *dungeon) {
    if (hero->skillCooldown[2] > 0.0f) return;
    hero->skillCooldown[2] = hero->skillMaxCD[2];
    for (int i = 0; i < MAX_DUNGEON_MOBS; i++) {
        DungeonMob *m = &dungeon->mobs[i];
        if (!m->active) continue;
        if (DLen(hero->position, m->position) <= 100.0f)
            m->speed *= 0.4f; /* %60 yavaşlatma — 3s sonra UpdateDungeon'da düzeltilir */
    }
}

/* Önceki şehirlerden müttefik birlik çağırır */
void SpawnAllies(DungeonMode *dungeon, Vector2 heroPos) {
    static const char *names[] = {"Kuzey Koy", "Sahil Kale", "Dag Kalesi",
                                   "Vadi Kenti", "Gney Liman"};
    static Color colors[] = {BLUE, GREEN, ORANGE, PURPLE, YELLOW};
    int spawned = 0;
    for (int i = 0; i < MAX_ALLY_UNITS; i++) {
        if (dungeon->allies[i].active) continue;
        float angle = (float)i * (360.0f / MAX_ALLY_UNITS) * DEG2RAD;
        dungeon->allies[i].position = (Vector2){
            heroPos.x + cosf(angle) * 40.0f,
            heroPos.y + sinf(angle) * 40.0f
        };
        dungeon->allies[i].targetPos  = heroPos;
        dungeon->allies[i].hp         = 150.0f;
        dungeon->allies[i].maxHp      = 150.0f;
        dungeon->allies[i].atk        = 18.0f;
        dungeon->allies[i].speed      = 110.0f;
        dungeon->allies[i].attackTimer= 0.0f;
        dungeon->allies[i].attackRange= 40.0f;
        dungeon->allies[i].lifeTimer  = ALLY_DURATION;
        dungeon->allies[i].active     = true;
        dungeon->allies[i].color      = colors[i % 5];
        strncpy(dungeon->allies[i].cityName, names[i % 5], 15);
        spawned++;
    }
    (void)spawned;
}

/* Sağ tık pathfinding + Q/W/E/R skill sistemi */
void UpdateHero(Hero *hero, DungeonMode *dungeon, float dt) {
    if (!hero->alive) return;

    /* Mana yenilenmesi */
    hero->stats.mana += hero->stats.manaRegen * dt;
    if (hero->stats.mana > hero->stats.maxMana)
        hero->stats.mana = hero->stats.maxMana;

    /* Dokunulmazlık ve animasyon */
    if (hero->invulnTimer > 0.0f) hero->invulnTimer -= dt;
    hero->animTimer += dt;

    /* Skill cooldown'ları ve swing görseli azalt */
    for (int i = 0; i < 4; i++)
        if (hero->skillCooldown[i] > 0.0f) hero->skillCooldown[i] -= dt;
    if (hero->swingTimer > 0.0f) hero->swingTimer -= dt;

    /* Sağ tık → hareket hedefi belirle */
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        hero->targetPos = GetMousePosition();
        hero->isMoving  = true;
    }

    /* Pathfinding: hedefe doğru ilerle */
    if (hero->isMoving) {
        float dx  = hero->targetPos.x - hero->position.x;
        float dy  = hero->targetPos.y - hero->position.y;
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist < 4.0f) {
            hero->isMoving = false;
            hero->state    = HERO_STATE_IDLE;
        } else {
            hero->state = HERO_STATE_MOVING;
            float step = hero->stats.speed * dt;
            float nx = (dx / dist) * step;
            float ny = (dy / dist) * step;
            hero->bodyAngle = atan2f(dy, dx) * RAD2DEG;

            /* Çarpışma: önce X sonra Y dene (kayma) */
            Vector2 nextX = {hero->position.x + nx, hero->position.y};
            Vector2 nextY = {hero->position.x,       hero->position.y + ny};
            Vector2 nextXY= {hero->position.x + nx, hero->position.y + ny};

            if (IsTileWalkable(dungeon, nextXY.x, nextXY.y))
                hero->position = nextXY;
            else if (IsTileWalkable(dungeon, nextX.x, nextX.y))
                hero->position = nextX;
            else if (IsTileWalkable(dungeon, nextY.x, nextY.y))
                hero->position = nextY;
            else
                hero->isMoving = false; /* Sıkıştı, dur */
        }
    } else {
        hero->state = HERO_STATE_IDLE;
    }

    /* Sol tık → yakın saldırı */
    if (hero->attackTimer > 0.0f) hero->attackTimer -= dt;
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hero->attackTimer <= 0.0f)
        HeroMeleeAttack(hero, dungeon);

    /* Q/W/E/R skill'leri */
    if (IsKeyPressed(KEY_Q)) SkillQ(hero, dungeon);
    if (IsKeyPressed(KEY_W)) SkillW(hero);
    if (IsKeyPressed(KEY_E)) SkillE(hero, dungeon);
    if (IsKeyPressed(KEY_R)) {
        if (hero->skillCooldown[3] <= 0.0f) {
            hero->skillCooldown[3] = hero->skillMaxCD[3];
            dungeon->allyCooldown  = ALLY_CALL_COOLDOWN;
            SpawnAllies(dungeon, hero->position);
        }
    }

    /* Loot toplama */
    for (int i = 0; i < MAX_LOOT_ITEMS; i++) {
        DungeonLoot *lt = &dungeon->loots[i];
        if (!lt->active) continue;
        if (DLen(hero->position, lt->position) < 20.0f) {
            switch (lt->type) {
                case LOOT_POTION:
                    dungeon->inventory.potions++;
                    hero->stats.hp += hero->stats.maxHp * 0.25f;
                    if (hero->stats.hp > hero->stats.maxHp) hero->stats.hp = hero->stats.maxHp;
                    break;
                case LOOT_RUNE: dungeon->inventory.runes++;                      break;
                case LOOT_GEAR: dungeon->inventory.gear++;                       break;
                case LOOT_GOLD: dungeon->inventory.bonusGold += lt->value;       break;
            }
            lt->active = false;
        }
    }

    if (hero->stats.hp <= 0.0f) { hero->alive = false; hero->state = HERO_STATE_DEAD; }
}

void UpdateDungeon(DungeonMode *dungeon, Hero *hero, float dt) {
    if (!dungeon->isDungeonActive) return;

    UpdateHero(hero, dungeon, dt);

    /* Mob yapay zekâsı: hero'ya yürü, yaklaşırsa hasar ver */
    for (int i = 0; i < MAX_DUNGEON_MOBS; i++) {
        DungeonMob *m = &dungeon->mobs[i];
        if (!m->active) continue;
        float dx   = hero->position.x - m->position.x;
        float dy   = hero->position.y - m->position.y;
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist > 0.5f && dist < 300.0f) {
            m->position.x += (dx / dist) * m->speed * dt;
            m->position.y += (dy / dist) * m->speed * dt;
        }
        if (dist < 14.0f && hero->alive && hero->invulnTimer <= 0.0f) {
            float dmg = 8.0f * dt - hero->stats.def * 0.1f * dt;
            if (dmg > 0.0f) {
                hero->stats.hp -= dmg;
                hero->invulnTimer = 0.15f;
            }
        }
    }

    /* Müttefik birlik yapay zekâsı */
    for (int i = 0; i < MAX_ALLY_UNITS; i++) {
        AllyUnit *a = &dungeon->allies[i];
        if (!a->active) continue;

        /* Süre bitince devre dışı bırak */
        a->lifeTimer -= dt;
        if (a->lifeTimer <= 0.0f || a->hp <= 0.0f) { a->active = false; continue; }

        /* En yakın aktif mobu hedef seç */
        DungeonMob *target = NULL;
        float bestDist = 200.0f;
        for (int j = 0; j < MAX_DUNGEON_MOBS; j++) {
            if (!dungeon->mobs[j].active) continue;
            float d = DLen(a->position, dungeon->mobs[j].position);
            if (d < bestDist) { bestDist = d; target = &dungeon->mobs[j]; }
        }

        if (target) {
            /* Hedefe yürü */
            float dx = target->position.x - a->position.x;
            float dy = target->position.y - a->position.y;
            float d  = sqrtf(dx*dx + dy*dy);
            if (d > a->attackRange) {
                a->position.x += (dx / d) * a->speed * dt;
                a->position.y += (dy / d) * a->speed * dt;
            }
            /* Menzildeyse saldır */
            a->attackTimer -= dt;
            if (a->attackTimer <= 0.0f && d <= a->attackRange) {
                a->attackTimer = 1.2f;
                target->hp -= a->atk;
                if (target->hp <= 0.0f) {
                    target->active = false;
                    AddHeroXP(hero, 10);
                }
            }
        } else {
            /* Mob yok — hero'nun yanına dön */
            float dx = hero->position.x - a->position.x;
            float dy = hero->position.y - a->position.y;
            float d  = sqrtf(dx*dx + dy*dy);
            if (d > 50.0f) {
                a->position.x += (dx / d) * a->speed * dt;
                a->position.y += (dy / d) * a->speed * dt;
            }
        }
    }

    /* Tüm odalar temizlendi mi? */
    int activeMobs = 0;
    for (int i = 0; i < MAX_DUNGEON_MOBS; i++)
        if (dungeon->mobs[i].active) activeMobs++;
    dungeon->allRoomsCleared = (activeMobs == 0);
}

/* ── Çizim ───────────────────────────────────────────────────────── */

void DrawDungeon(DungeonMode *dungeon, Hero *hero) {
    if (!dungeon->isDungeonActive) return;

    /* Tile haritası */
    for (int r = 0; r < DUNGEON_ROWS; r++) {
        for (int c = 0; c < DUNGEON_COLS; c++) {
            int px = c * DUNGEON_TILE_SIZE;
            int py = r * DUNGEON_TILE_SIZE;
            switch (dungeon->tiles[r][c]) {
                case DTILE_WALL:
                    DrawRectangle(px, py, DUNGEON_TILE_SIZE, DUNGEON_TILE_SIZE,
                                  (Color){30, 25, 20, 255});
                    DrawRectangleLines(px, py, DUNGEON_TILE_SIZE, DUNGEON_TILE_SIZE,
                                       (Color){55, 45, 35, 255});
                    break;
                case DTILE_FLOOR:
                    DrawRectangle(px, py, DUNGEON_TILE_SIZE, DUNGEON_TILE_SIZE,
                                  (Color){60, 50, 40, 255});
                    DrawRectangleLines(px, py, DUNGEON_TILE_SIZE, DUNGEON_TILE_SIZE,
                                       (Color){75, 62, 50, 120});
                    break;
                case DTILE_DOOR:
                    DrawRectangle(px, py, DUNGEON_TILE_SIZE, DUNGEON_TILE_SIZE,
                                  (Color){100, 70, 30, 255});
                    break;
            }
        }
    }

    /* Loot */
    for (int i = 0; i < MAX_LOOT_ITEMS; i++) {
        DungeonLoot *lt = &dungeon->loots[i];
        if (!lt->active) continue;
        Color lc = (lt->type == LOOT_POTION) ? RED   :
                   (lt->type == LOOT_RUNE)   ? PURPLE:
                   (lt->type == LOOT_GEAR)   ? GRAY  : GOLD;
        DrawCircle((int)lt->position.x, (int)lt->position.y, 8, lc);
        DrawCircleLines((int)lt->position.x, (int)lt->position.y, 8, WHITE);
    }

    /* Moblar */
    for (int i = 0; i < MAX_DUNGEON_MOBS; i++) {
        DungeonMob *m = &dungeon->mobs[i];
        if (!m->active) continue;
        DrawCircle((int)m->position.x, (int)m->position.y, 11, (Color){180, 40, 40, 230});
        DrawCircleLines((int)m->position.x, (int)m->position.y, 11, RED);
        /* Can barı */
        float ratio = m->hp / m->maxHp;
        DrawRectangle((int)m->position.x - 12, (int)m->position.y - 18, 24, 4,
                      (Color){80,0,0,180});
        DrawRectangle((int)m->position.x - 12, (int)m->position.y - 18,
                      (int)(24 * ratio), 4, (Color){20,200,20,220});
    }

    /* Q kılıç sallama koni görseli */
    if (hero->alive && hero->swingTimer > 0.0f) {
        float alpha = hero->swingTimer / 0.30f;
        /* Dolgulu koni */
        DrawCircleSector(hero->position,
                         hero->stats.attackRange,
                         hero->swingAngle - 60.0f,
                         hero->swingAngle + 60.0f,
                         20,
                         Fade((Color){255, 210, 60, 180}, alpha));
        /* Parlak kenar çizgisi */
        DrawCircleSectorLines(hero->position,
                              hero->stats.attackRange,
                              hero->swingAngle - 60.0f,
                              hero->swingAngle + 60.0f,
                              20,
                              Fade((Color){255, 255, 200, 230}, alpha));
    }

    /* Hero */
    if (hero->alive) {
        /* Gövde rengi sınıfa göre */
        DrawCircle((int)hero->position.x, (int)hero->position.y, 13, hero->bodyColor);
        DrawCircleLines((int)hero->position.x, (int)hero->position.y, 13, hero->accentColor);
        /* Yürüme salınımı: hareket ediyorken küçük daireler */
        if (hero->state == HERO_STATE_MOVING) {
            float bob = sinf(hero->animTimer * 10.0f) * 2.0f;
            DrawCircle((int)hero->position.x, (int)(hero->position.y + bob), 4,
                       Fade(hero->accentColor, 0.4f));
        }
        /* HP barı */
        float hr = hero->stats.hp / hero->stats.maxHp;
        DrawRectangle((int)hero->position.x - 16, (int)hero->position.y - 22, 32, 4,
                      (Color){80,0,0,180});
        DrawRectangle((int)hero->position.x - 16, (int)hero->position.y - 22,
                      (int)(32 * hr), 4, (Color){20,220,20,230});
        /* Mana barı (altında, mavi) */
        float mr = (hero->stats.maxMana > 0) ? hero->stats.mana / hero->stats.maxMana : 0.0f;
        DrawRectangle((int)hero->position.x - 16, (int)hero->position.y - 17, 32, 3,
                      (Color){0,0,80,180});
        DrawRectangle((int)hero->position.x - 16, (int)hero->position.y - 17,
                      (int)(32 * mr), 3, (Color){60,120,255,230});
        /* Saldırı menzili (saldırırken) */
        if (hero->attackTimer > 0.1f)
            DrawCircleLines((int)hero->position.x, (int)hero->position.y,
                            (int)hero->stats.attackRange, (Color){255,255,100,120});
    }

    /* Müttefik birlikler */
    for (int i = 0; i < MAX_ALLY_UNITS; i++) {
        AllyUnit *a = &dungeon->allies[i];
        if (!a->active) continue;
        DrawCircle((int)a->position.x, (int)a->position.y, 9, a->color);
        DrawCircleLines((int)a->position.x, (int)a->position.y, 9, WHITE);
        /* Can barı */
        float ar = a->hp / a->maxHp;
        DrawRectangle((int)a->position.x - 10, (int)a->position.y - 16, 20, 3,
                      (Color){60,0,0,180});
        DrawRectangle((int)a->position.x - 10, (int)a->position.y - 16,
                      (int)(20 * ar), 3, a->color);
        /* Süre göstergesi: küçük yay */
        float lifeRatio = a->lifeTimer / ALLY_DURATION;
        DrawCircleSector(a->position, 5.0f, 0.0f, lifeRatio * 360.0f, 8,
                         Fade(WHITE, 0.6f));
    }

    /* ── HUD — üst panel ────────────────────────────────────────────── */
    DrawRectangle(0, 0, 1280, 52, (Color){0,0,0,210});
    const char *clsName = (hero->heroClass == HERO_WARRIOR) ? "SAVASCI" :
                          (hero->heroClass == HERO_MAGE)    ? "BUYUCU" : "OKCU";
    DrawText("DUNGEON  |  Sag Tik:Hareket  |  Sol Tik:Saldir  |  Q/W/E/R:Skill  |  ESC:Geri",
             20, 6, 12, (Color){200,200,200,230});
    char inv[140];
    snprintf(inv, sizeof(inv),
             "%s Lv.%d  HP:%.0f/%.0f  Mana:%.0f/%.0f  |  Iksir:%d Rune:%d Gear:%d  Altin:+%d",
             clsName, hero->level,
             hero->stats.hp, hero->stats.maxHp,
             hero->stats.mana, hero->stats.maxMana,
             dungeon->inventory.potions, dungeon->inventory.runes,
             dungeon->inventory.gear, dungeon->inventory.bonusGold);
    DrawText(inv, 20, 28, 12, GOLD);

    /* ── Skill CD paneli — alt ──────────────────────────────────────── */
    DrawRectangle(0, 720 - 48, 1280, 48, (Color){0,0,0,180});
    const char *skillNames[4] = {"Q:Darbe", "W:Iyiles", "E:Sis", "R:Destek"};
    Color        skillColors[4] = {ORANGE, GREEN, SKYBLUE, GOLD};
    for (int i = 0; i < 4; i++) {
        int bx = 20 + i * 120;
        int by = 720 - 44;
        float cd    = hero->skillCooldown[i];
        float maxcd = hero->skillMaxCD[i];
        Color bc = (cd <= 0.0f) ? skillColors[i] : (Color){60,60,60,200};
        DrawRectangle(bx, by, 100, 36, bc);
        DrawRectangleLines(bx, by, 100, 36, WHITE);
        DrawText(skillNames[i], bx + 4, by + 4, 12, WHITE);
        if (cd > 0.0f) {
            /* Dolum çubuğu */
            float fill = 1.0f - (cd / maxcd);
            DrawRectangle(bx, by + 28, (int)(100 * fill), 8, skillColors[i]);
            char cdtxt[8];
            snprintf(cdtxt, sizeof(cdtxt), "%.1fs", cd);
            DrawText(cdtxt, bx + 36, by + 14, 11, WHITE);
        } else {
            DrawText("HAZIR", bx + 28, by + 18, 10, WHITE);
        }
    }

    if (dungeon->allRoomsCleared) {
        const char *msg = "Tum odalar temizlendi!  ESC ile geri don.";
        DrawText(msg, 1280/2 - MeasureText(msg, 20)/2, 720/2 - 10, 20, GREEN);
    }
}
