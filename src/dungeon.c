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

    hero->skillCount = 4;
    hero->selectedSkill = -1;

    switch (cls) {
        case HERO_WARRIOR:
            hero->stats = (HeroStats){700,700, 80, 80,2.0f, 40,20,100, 35, 1.5f, 0.10f,2.0f};
            hero->bodyColor   = SKYBLUE;
            hero->accentColor = (Color){200, 240, 255, 255};
            {
                Skill *s = &hero->skills[0]; strncpy(s->name, "Q: Kilic Darbesi", 31); s->name[31]='\0'; s->manaCost=10; s->cooldown=4.0f; s->currentCooldown=0; s->value=1.5f; s->radius=80.0f; s->color=ORANGE;
                s = &hero->skills[1]; strncpy(s->name, "W: Komuta Cigligi", 31); s->name[31]='\0'; s->manaCost=20; s->cooldown=8.0f; s->currentCooldown=0; s->value=0.20f; s->radius=0.0f; s->color=GREEN;
                s = &hero->skills[2]; strncpy(s->name, "E: Sis Perdesi", 31); s->name[31]='\0'; s->manaCost=30; s->cooldown=12.0f; s->currentCooldown=0; s->value=0.60f; s->radius=100.0f; s->color=SKYBLUE;
                s = &hero->skills[3]; strncpy(s->name, "R: Destek Cagir", 31); s->name[31]='\0'; s->manaCost=50; s->cooldown=ALLY_CALL_COOLDOWN; s->currentCooldown=0; s->value=0.0f; s->radius=0.0f; s->color=GOLD;
            }
            break;
        case HERO_MAGE:
            hero->stats = (HeroStats){350,350,300,300,8.0f, 15, 5, 90,120, 0.8f, 0.05f,3.0f};
            hero->bodyColor   = VIOLET;
            hero->accentColor = (Color){210, 180, 255, 255};
            {
                Skill *s = &hero->skills[0]; strncpy(s->name, "Q: Ates Dalgasi", 31); s->name[31]='\0'; s->manaCost=25; s->cooldown=3.0f; s->currentCooldown=0; s->value=2.0f; s->radius=90.0f; s->color=ORANGE;
                s = &hero->skills[1]; strncpy(s->name, "W: Mana Kalkani", 31); s->name[31]='\0'; s->manaCost=50; s->cooldown=10.0f; s->currentCooldown=0; s->value=0.30f; s->radius=0.0f; s->color=GREEN;
                s = &hero->skills[2]; strncpy(s->name, "E: Buz Patlamasi", 31); s->name[31]='\0'; s->manaCost=60; s->cooldown=15.0f; s->currentCooldown=0; s->value=0.80f; s->radius=150.0f; s->color=SKYBLUE;
                s = &hero->skills[3]; strncpy(s->name, "R: Meteor", 31); s->name[31]='\0'; s->manaCost=100; s->cooldown=60.0f; s->currentCooldown=0; s->value=3.0f; s->radius=200.0f; s->color=GOLD;
            }
            break;
        case HERO_ARCHER:
        default:
            hero->stats = (HeroStats){450,450,150,150,4.0f, 30,10,130,160, 2.0f, 0.20f,1.8f};
            hero->bodyColor   = LIME;
            hero->accentColor = (Color){180, 255, 150, 255};
            {
                Skill *s = &hero->skills[0]; strncpy(s->name, "Q: Ok Yelpazesi", 31); s->name[31]='\0'; s->manaCost=15; s->cooldown=2.0f; s->currentCooldown=0; s->value=1.8f; s->radius=120.0f; s->color=ORANGE;
                s = &hero->skills[1]; strncpy(s->name, "W: Ceviklik", 31); s->name[31]='\0'; s->manaCost=20; s->cooldown=8.0f; s->currentCooldown=0; s->value=0.15f; s->radius=0.0f; s->color=GREEN;
                s = &hero->skills[2]; strncpy(s->name, "E: Zehir Bulutu", 31); s->name[31]='\0'; s->manaCost=30; s->cooldown=10.0f; s->currentCooldown=0; s->value=0.50f; s->radius=80.0f; s->color=SKYBLUE;
                s = &hero->skills[3]; strncpy(s->name, "R: Ok Yagmuru", 31); s->name[31]='\0'; s->manaCost=80; s->cooldown=45.0f; s->currentCooldown=0; s->value=2.5f; s->radius=250.0f; s->color=GOLD;
            }
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

    dungeon->interactableCount = 0;
    /* 1. odayı atla, diğerlerine rastgele etkileşim nesneleri koy */
    for (int i = 1; i < dungeon->roomCount && dungeon->interactableCount < MAX_INTERACTABLES; i++) {
        if (GetRandomValue(0, 100) < 60) { /* %60 ihtimalle */
            Interactable *itr = &dungeon->interactables[dungeon->interactableCount++];
            itr->active = true;
            itr->used   = false;
            itr->type   = GetRandomValue(0, 2); /* CHEST=0, SHRINE=1, WELL=2 */
            itr->position = (Vector2){
                (float)(dungeon->rooms[i].x + dungeon->rooms[i].w / 2) * DUNGEON_TILE_SIZE,
                (float)(dungeon->rooms[i].y + dungeon->rooms[i].h / 2) * DUNGEON_TILE_SIZE
            };
            if (itr->type == INTERACTABLE_CHEST) strncpy(itr->tooltip, "[E] Hazine Sandigi", 31);
            else if (itr->type == INTERACTABLE_SHRINE) strncpy(itr->tooltip, "[E] Guc Sunagi", 31);
            else if (itr->type == INTERACTABLE_WELL) strncpy(itr->tooltip, "[E] Iyilesme Kuyusu", 31);
        }
    }

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

/* ── Yardımcı Fonksiyonlar ────────────────────────────────────────── */

static void AddItemToInventory(Inventory *inv, ItemType type, const char* name, int amount) {
    /* Aynısından varsa üstüne ekle */
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (inv->slots[i].type == type) {
            inv->slots[i].amount += amount;
            return;
        }
    }
    /* Yoksa ilk boş slota yerleştir */
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (inv->slots[i].type == ITEM_NONE) {
            inv->slots[i].type = type;
            inv->slots[i].amount = amount;
            strncpy(inv->slots[i].name, name, 31);
            return;
        }
    }
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

static bool CastSkillCost(Hero *hero, int skillIdx) {
    if (skillIdx >= hero->skillCount) return false;
    Skill *s = &hero->skills[skillIdx];
    if (s->currentCooldown > 0.0f) return false;
    if (hero->stats.mana < s->manaCost) return false;
    
    hero->stats.mana -= s->manaCost;
    s->currentCooldown = s->cooldown;
    return true;
}

/* Q — Warrior: Koni(Cleave), Mage: Fireball(Ates topu - tek hedef veya mermi), Archer: Delip gecen ok */
static void SkillQ(Hero *hero, DungeonMode *dungeon) {
    if (!CastSkillCost(hero, 0)) return;
    Skill *s = &hero->skills[0];

    Vector2 mouse = GetMousePosition();
    float dx = mouse.x - hero->position.x;
    float dy = mouse.y - hero->position.y;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 1.0f) return;
    float nx = dx / len, ny = dy / len;

    if (hero->heroClass == HERO_WARRIOR) {
        /* Warrior Cleave: Koni şeklinde hasar */
        hero->swingAngle = atan2f(dy, dx) * RAD2DEG;
        hero->swingTimer = 0.30f;
        float coneRange = s->radius;
        for (int i = 0; i < MAX_DUNGEON_MOBS; i++) {
            DungeonMob *m = &dungeon->mobs[i];
            if (!m->active) continue;
            float mx = m->position.x - hero->position.x;
            float my = m->position.y - hero->position.y;
            float dist = sqrtf(mx*mx + my*my);
            if (dist > coneRange) continue;
            float dot = (mx / dist) * nx + (my / dist) * ny;
            if (dot < 0.5f) continue;
            float dmg = hero->stats.atk * s->value;
            m->hp -= dmg;
            if (m->hp <= 0.0f) { m->active = false; AddHeroXP(hero, 20 + hero->level * 5); }
        }
    } else if (hero->heroClass == HERO_MAGE) {
        /* Mage Fireball: İlk hedefe hasar */
        hero->swingAngle = atan2f(dy, dx) * RAD2DEG;
        hero->swingTimer = 0.20f;
        DungeonMob *target = NULL;
        float bestDist = s->radius;
        for (int i = 0; i < MAX_DUNGEON_MOBS; i++) {
            DungeonMob *m = &dungeon->mobs[i];
            if (!m->active) continue;
            float mx = m->position.x - hero->position.x;
            float my = m->position.y - hero->position.y;
            float dist = sqrtf(mx*mx + my*my);
            if (dist < bestDist) {
                float dot = (mx / dist) * nx + (my / dist) * ny;
                if (dot > 0.8f) { bestDist = dist; target = m; }
            }
        }
        if (target) {
            target->hp -= hero->stats.atk * s->value;
            if (target->hp <= 0.0f) { target->active = false; AddHeroXP(hero, 20 + hero->level * 5); }
        }
    } else if (hero->heroClass == HERO_ARCHER) {
        /* Archer Piercing Arrow: Düz çizgi boyunca herkese hasar */
        hero->swingAngle = atan2f(dy, dx) * RAD2DEG;
        hero->swingTimer = 0.15f;
        for (int i = 0; i < MAX_DUNGEON_MOBS; i++) {
            DungeonMob *m = &dungeon->mobs[i];
            if (!m->active) continue;
            float mx = m->position.x - hero->position.x;
            float my = m->position.y - hero->position.y;
            float dist = sqrtf(mx*mx + my*my);
            if (dist > s->radius) continue;
            float dot = (mx / dist) * nx + (my / dist) * ny;
            if (dot > 0.9f) {
                m->hp -= hero->stats.atk * s->value;
                if (m->hp <= 0.0f) { m->active = false; AddHeroXP(hero, 20 + hero->level * 5); }
            }
        }
    }
}

/* W — Warrior: Shield Wall (Can), Mage: Mana Shield (Mana -> HP), Archer: Agility (Hız) */
static void SkillW(Hero *hero) {
    if (!CastSkillCost(hero, 1)) return;
    Skill *s = &hero->skills[1];
    
    if (hero->heroClass == HERO_WARRIOR) {
        hero->stats.hp += hero->stats.maxHp * s->value;
        if (hero->stats.hp > hero->stats.maxHp) hero->stats.hp = hero->stats.maxHp;
    } else if (hero->heroClass == HERO_MAGE) {
        /* Geçici kalkan / hasar emilimi - Şimdilik direkt HP'ye ekleyelim */
        hero->stats.hp += hero->stats.maxHp * s->value;
        if (hero->stats.hp > hero->stats.maxHp) hero->stats.hp = hero->stats.maxHp;
    } else if (hero->heroClass == HERO_ARCHER) {
        /* Hız artışı için geçici buff. Basitlik için kalıcı az miktar heal + hızlı can yenileme */
        hero->stats.hp += hero->stats.maxHp * s->value;
        if (hero->stats.hp > hero->stats.maxHp) hero->stats.hp = hero->stats.maxHp;
    }
}

/* E — Warrior: Charge (İleri Atılma), Mage: Blink (Işınlanma), Archer: Zehir Bulutu (Yavaşlatma) */
static void SkillE(Hero *hero, DungeonMode *dungeon) {
    if (!CastSkillCost(hero, 2)) return;
    Skill *s = &hero->skills[2];

    Vector2 mouse = GetMousePosition();
    float dx = mouse.x - hero->position.x;
    float dy = mouse.y - hero->position.y;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 1.0f) len = 1.0f;
    float nx = dx / len, ny = dy / len;

    if (hero->heroClass == HERO_WARRIOR) {
        /* İleri atılma ve hedeflere hasar verme */
        hero->position.x += nx * 60.0f;
        hero->position.y += ny * 60.0f;
        for (int i = 0; i < MAX_DUNGEON_MOBS; i++) {
            DungeonMob *m = &dungeon->mobs[i];
            if (!m->active) continue;
            if (DLen(hero->position, m->position) <= s->radius) {
                m->hp -= hero->stats.atk * s->value;
                if (m->hp <= 0.0f) { m->active = false; AddHeroXP(hero, 15); }
            }
        }
    } else if (hero->heroClass == HERO_MAGE) {
        /* Blink: Teleport (Duvarlardan geçebilir, harita dışına çıkmayı engellemeliyiz) */
        float targetX = hero->position.x + nx * s->radius;
        float targetY = hero->position.y + ny * s->radius;
        if (IsTileWalkable(dungeon, targetX, targetY)) {
            hero->position.x = targetX;
            hero->position.y = targetY;
        }
    } else if (hero->heroClass == HERO_ARCHER) {
        /* Alan yavaşlatması */
        for (int i = 0; i < MAX_DUNGEON_MOBS; i++) {
            DungeonMob *m = &dungeon->mobs[i];
            if (!m->active) continue;
            if (DLen(hero->position, m->position) <= s->radius)
                m->speed *= (1.0f - s->value);
        }
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
    for (int i = 0; i < hero->skillCount; i++)
        if (hero->skills[i].currentCooldown > 0.0f) hero->skills[i].currentCooldown -= dt;
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

    /* Q/W/E/R skill'leri ve Harita Etkileşimleri (T60) */
    if (IsKeyPressed(KEY_Q)) SkillQ(hero, dungeon);
    if (IsKeyPressed(KEY_W)) SkillW(hero);

    bool interacted = false;
    for (int i = 0; i < dungeon->interactableCount; i++) {
        Interactable *itr = &dungeon->interactables[i];
        if (!itr->active || itr->used) continue;
        if (DLen(hero->position, itr->position) < 50.0f) {
            if (IsKeyPressed(KEY_E)) {
                itr->used = true;
                interacted = true;
                if (itr->type == INTERACTABLE_CHEST) {
                    AddHeroXP(hero, 150);
                    dungeon->inventory.bonusGold += 50;
                } else if (itr->type == INTERACTABLE_SHRINE) {
                    hero->stats.maxHp += 20.0f;
                    hero->stats.hp    += 20.0f;
                    hero->stats.atk   += 5.0f;
                } else if (itr->type == INTERACTABLE_WELL) {
                    hero->stats.hp    = hero->stats.maxHp;
                    hero->stats.mana  = hero->stats.maxMana;
                }
                break;
            }
        }
    }

    if (IsKeyPressed(KEY_E) && !interacted) SkillE(hero, dungeon);

    if (IsKeyPressed(KEY_R)) {
        if (CastSkillCost(hero, 3)) {
            dungeon->allyCooldown  = ALLY_CALL_COOLDOWN;
            Skill *s = &hero->skills[3];
            Vector2 mouse = GetMousePosition();

            if (hero->heroClass == HERO_WARRIOR) {
                /* Destek Çağır (Müttefik Birlikler) */
                SpawnAllies(dungeon, hero->position);
            } else if (hero->heroClass == HERO_MAGE) {
                /* Meteor: İmlecin olduğu alana devasa hasar */
                for (int i = 0; i < MAX_DUNGEON_MOBS; i++) {
                    DungeonMob *m = &dungeon->mobs[i];
                    if (!m->active) continue;
                    if (DLen(mouse, m->position) <= s->radius) {
                        m->hp -= hero->stats.atk * s->value;
                        if (m->hp <= 0.0f) { m->active = false; AddHeroXP(hero, 20 + hero->level * 5); }
                    }
                }
            } else if (hero->heroClass == HERO_ARCHER) {
                /* Ok Yağmuru: İmleç etrafına çok geniş alan hasarı */
                for (int i = 0; i < MAX_DUNGEON_MOBS; i++) {
                    DungeonMob *m = &dungeon->mobs[i];
                    if (!m->active) continue;
                    if (DLen(mouse, m->position) <= s->radius) {
                        m->hp -= hero->stats.atk * s->value;
                        if (m->hp <= 0.0f) { m->active = false; AddHeroXP(hero, 30); }
                    }
                }
            }
        }
    }

    /* Loot toplama */
    for (int i = 0; i < MAX_LOOT_ITEMS; i++) {
        DungeonLoot *lt = &dungeon->loots[i];
        if (!lt->active) continue;
        if (DLen(hero->position, lt->position) < 20.0f) {
            switch (lt->type) {
                case LOOT_POTION:
                    AddItemToInventory(&dungeon->inventory, ITEM_POTION, "Health Potion", 1);
                    break;
                case LOOT_RUNE:
                    AddItemToInventory(&dungeon->inventory, ITEM_RUNE, "Magic Rune", 1);
                    break;
                case LOOT_GEAR:
                    AddItemToInventory(&dungeon->inventory, ITEM_GEAR, "Iron Sword", 1);
                    break;
                case LOOT_GOLD:
                    dungeon->inventory.bonusGold += lt->value;
                    break;
            }
            lt->active = false;
        }
    }

    /* T61: Envanter Toggle ve Kısayollar (Shift + 1-6) */
    if (IsKeyPressed(KEY_I)) {
        dungeon->inventory.isOpen = !dungeon->inventory.isOpen;
    }

    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        int useIndex = -1;
        if (IsKeyPressed(KEY_ONE))   useIndex = 0;
        else if (IsKeyPressed(KEY_TWO))   useIndex = 1;
        else if (IsKeyPressed(KEY_THREE)) useIndex = 2;
        else if (IsKeyPressed(KEY_FOUR))  useIndex = 3;
        else if (IsKeyPressed(KEY_FIVE))  useIndex = 4;
        else if (IsKeyPressed(KEY_SIX))   useIndex = 5;

        if (useIndex >= 0 && useIndex < MAX_INVENTORY_SLOTS) {
            Item *it = &dungeon->inventory.slots[useIndex];
            if (it->type == ITEM_POTION && it->amount > 0) {
                it->amount--;
                if (it->amount == 0) it->type = ITEM_NONE;
                /* İksir kullanımı: %40 HP yeniler */
                hero->stats.hp += hero->stats.maxHp * 0.40f;
                if (hero->stats.hp > hero->stats.maxHp) hero->stats.hp = hero->stats.maxHp;
            }
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

    /* T60 — Harita Etkileşimleri (Interactables) */
    for (int i = 0; i < dungeon->interactableCount; i++) {
        Interactable *itr = &dungeon->interactables[i];
        if (!itr->active || itr->used) continue;
        
        Color ic = WHITE;
        if (itr->type == INTERACTABLE_CHEST) ic = GOLD;
        else if (itr->type == INTERACTABLE_SHRINE) ic = MAGENTA;
        else if (itr->type == INTERACTABLE_WELL) ic = SKYBLUE;

        DrawRectangle((int)itr->position.x - 12, (int)itr->position.y - 12, 24, 24, ic);
        DrawRectangleLines((int)itr->position.x - 12, (int)itr->position.y - 12, 24, 24, RAYWHITE);

        if (DLen(hero->position, itr->position) < 50.0f) {
            int tw = MeasureText(itr->tooltip, 14);
            DrawText(itr->tooltip, (int)itr->position.x - tw / 2, (int)itr->position.y - 30, 14, WHITE);
        }
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
             "%s Lv.%d  HP:%.0f/%.0f  Mana:%.0f/%.0f  |  Altin:+%d  |  [I] Envanter",
             clsName, hero->level,
             hero->stats.hp, hero->stats.maxHp,
             hero->stats.mana, hero->stats.maxMana,
             dungeon->inventory.bonusGold);
    DrawText(inv, 20, 28, 12, GOLD);

    /* ── Skill CD paneli — alt ──────────────────────────────────────── */
    DrawRectangle(0, 720 - 48, 1280, 48, (Color){0,0,0,180});
    for (int i = 0; i < hero->skillCount; i++) {
        Skill *s = &hero->skills[i];
        int bx = 20 + i * 150;
        int by = 720 - 44;
        float cd    = s->currentCooldown;
        float maxcd = s->cooldown;
        bool hasMana = hero->stats.mana >= s->manaCost;
        Color bc = (cd <= 0.0f && hasMana) ? s->color : (Color){60,60,60,200};
        DrawRectangle(bx, by, 140, 36, bc);
        DrawRectangleLines(bx, by, 140, 36, WHITE);
        DrawText(s->name, bx + 4, by + 4, 11, WHITE);
        if (cd > 0.0f) {
            float fill = 1.0f - (cd / maxcd);
            DrawRectangle(bx, by + 28, (int)(140 * fill), 8, s->color);
            char cdtxt[32];
            snprintf(cdtxt, sizeof(cdtxt), "%.1fs (M:%d)", cd, s->manaCost);
            DrawText(cdtxt, bx + 30, by + 16, 11, WHITE);
        } else if (!hasMana) {
            char cdtxt[32];
            snprintf(cdtxt, sizeof(cdtxt), "MANA YOK(%d)", s->manaCost);
            DrawText(cdtxt, bx + 10, by + 18, 10, RED);
        } else {
            char cdtxt[32];
            snprintf(cdtxt, sizeof(cdtxt), "HAZIR (M:%d)", s->manaCost);
            DrawText(cdtxt, bx + 25, by + 18, 10, WHITE);
        }
    }

    /* T61: Gelişmiş Envanter Arayüzü */
    if (dungeon->inventory.isOpen) {
        int invW = 220;
        int invH = 290;
        int invX = 1280 - invW - 20;
        int invY = 52 + 20;

        DrawRectangle(invX, invY, invW, invH, (Color){20, 20, 25, 230});
        DrawRectangleLines(invX, invY, invW, invH, (Color){100, 100, 120, 255});
        DrawText("ENVANTER", invX + invW/2 - MeasureText("ENVANTER", 14)/2, invY + 10, 14, GOLD);
        DrawText("Shift+1..6 Kullan", invX + invW/2 - MeasureText("Shift+1..6 Kullan", 10)/2, invY + 28, 10, LIGHTGRAY);
        
        int slotW = 50;
        int slotH = 50;
        int padding = 10;
        int startX = invX + 20;
        int startY = invY + 50;

        for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
            int row = i / 3;
            int col = i % 3;
            int sx = startX + col * (slotW + padding);
            int sy = startY + row * (slotH + padding);

            DrawRectangle(sx, sy, slotW, slotH, (Color){40, 40, 50, 200});
            DrawRectangleLines(sx, sy, slotW, slotH, (Color){80, 80, 100, 255});

            /* Hotkey İpucu */
            if (i < 6) {
                DrawText(TextFormat("S%d", i+1), sx + 2, sy + 2, 8, (Color){150,150,150,255});
            }

            Item *it = &dungeon->inventory.slots[i];
            if (it->type != ITEM_NONE && it->amount > 0) {
                Color c = WHITE;
                if (it->type == ITEM_POTION) c = RED;
                else if (it->type == ITEM_RUNE) c = PURPLE;
                else if (it->type == ITEM_GEAR) c = LIGHTGRAY;
                
                DrawCircle(sx + slotW/2, sy + slotH/2, 12, c);
                DrawText(TextFormat("x%d", it->amount), sx + slotW - 18, sy + slotH - 14, 10, WHITE);
            }

            /* Tooltip */
            if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){(float)sx, (float)sy, (float)slotW, (float)slotH})) {
                DrawRectangleLines(sx, sy, slotW, slotH, WHITE);
                if (it->type != ITEM_NONE) {
                    int tw = MeasureText(it->name, 12);
                    DrawRectangle(sx, sy - 20, tw + 10, 18, (Color){0,0,0,200});
                    DrawText(it->name, sx + 5, sy - 18, 12, WHITE);
                }
            }
        }
    }

    if (dungeon->allRoomsCleared) {
        const char *msg = "Tum odalar temizlendi!  ESC ile geri don.";
        DrawText(msg, 1280/2 - MeasureText(msg, 20)/2, 720/2 - 10, 20, GREEN);
    }
}
