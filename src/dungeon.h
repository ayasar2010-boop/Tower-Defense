#ifndef DUNGEON_H
#define DUNGEON_H

#include "raylib.h"
#include <stdbool.h>

/* T44 — Dungeon Mode (RPG / Farming) */
/* T52/T53 — Hero Class System */

#define MAX_DUNGEON_MOBS    20
#define MAX_LOOT_ITEMS      30
#define MAX_ROOMS            6
#define DUNGEON_TILE_SIZE   48
#define DUNGEON_COLS        20
#define DUNGEON_ROWS        12
#define MAX_ALLY_UNITS       5
#define ALLY_CALL_COOLDOWN 120.0f
#define ALLY_DURATION       30.0f

/* T52 — Kahraman sınıfı */
typedef enum {
    HERO_WARRIOR,   /* Yüksek HP, yakın dövüş, kulelere +15% hasar */
    HERO_MAGE,      /* Yüksek Mana, AoE, kulelere +20% menzil */
    HERO_ARCHER,    /* Dengeli, uzak saldırı, kulelere +25% atış hızı */
    HERO_CLASS_COUNT,
    HERO_CLASS_NONE = -1
} HeroClass;

/* T53 — Hero durum makinesi */
typedef enum {
    HERO_STATE_IDLE,
    HERO_STATE_MOVING,
    HERO_STATE_ATTACKING,
    HERO_STATE_HURT,
    HERO_STATE_DEAD
} HeroState;

/* T53 — Temel ve aktif istatistikler */
typedef struct {
    float hp, maxHp;
    float mana, maxMana;
    float manaRegen;   /* mana/saniye */
    float atk;
    float def;
    float speed;
    float attackRange;
    float attackSpeed; /* saldırı/saniye */
    float critChance;  /* 0.0-1.0 */
    float critMult;    /* varsayılan 2.0 */
} HeroStats;

/* Envanter */
typedef enum { LOOT_POTION, LOOT_RUNE, LOOT_GEAR, LOOT_GOLD } LootType;

typedef enum {
    ITEM_NONE = 0,
    ITEM_POTION,
    ITEM_RUNE,
    ITEM_GEAR
} ItemType;

typedef struct {
    ItemType type;
    int amount;
    char name[32];
} Item;

#define MAX_INVENTORY_SLOTS 12

typedef struct {
    Item slots[MAX_INVENTORY_SLOTS];
    int bonusGold;  /* Dungeon'dan kazanılan altın TD haritasına taşınır */
    bool isOpen;
} Inventory;

typedef struct {
    Vector2  position;
    LootType type;
    int      value;
    bool     active;
} DungeonLoot;

#define MAX_SKILLS 8

typedef struct {
    char name[32];
    char description[64];
    int manaCost;
    float cooldown;
    float currentCooldown;
    float duration;
    float value;
    float radius;
    int level;
    int maxLevel;
    bool isPassive;
    bool unlocked;
    Color color;
} Skill;

/* T53 — Genişletilmiş Hero struct */
typedef struct {
    Vector2   position;
    HeroClass heroClass;
    HeroState state;
    HeroStats stats;       /* aktif (buff/debuff sonrası) istatistikler */

    /* XP / Seviye */
    int   level;
    int   xp;
    int   xpToNext;

    /* Zamanlayıcılar */
    float attackTimer;
    float invulnTimer;    /* hasar sonrası dokunulmazlık */
    float comboTimer;
    int   comboCount;

    /* MOBA — sağ tık pathfinding */
    Vector2 targetPos;
    bool    isMoving;

    /* MOBA — Yetenek Sistemi */
    Skill skills[MAX_SKILLS];
    int skillCount;
    int selectedSkill;

    /* Q kılıç sallama koni görseli */
    float swingTimer;
    float swingAngle;

    /* Görsel */
    float bodyAngle;
    float animTimer;
    Color bodyColor;
    Color accentColor;

    bool  alive;
} Hero;

/* Önceki şehirlerden çağrılan müttefik birlik */
typedef struct {
    Vector2 position;
    Vector2 targetPos;
    float   hp, maxHp;
    float   atk;
    float   speed;
    float   attackTimer;
    float   attackRange;
    float   lifeTimer;   /* Kalan saha süresi */
    bool    active;
    Color   color;
    char    cityName[16];
} AllyUnit;

/* Düşman mob */
typedef struct {
    Vector2 position;
    float   hp, maxHp;
    float   speed;
    bool    active;
} DungeonMob;

/* Basit dikdörtgen oda */
typedef struct {
    int x, y, w, h;   /* tile koordinatları */
    bool cleared;
} DungeonRoom;

/* Zemin tile haritası */
typedef enum { DTILE_WALL = 0, DTILE_FLOOR = 1, DTILE_DOOR = 2 } DungeonTile;

#define MAX_INTERACTABLES 10

typedef enum {
    INTERACTABLE_CHEST,
    INTERACTABLE_SHRINE,
    INTERACTABLE_WELL
} InteractableType;

typedef struct {
    Vector2 position;
    InteractableType type;
    bool active;
    bool used;
    char tooltip[32];
} Interactable;

typedef struct {
    DungeonTile    tiles[DUNGEON_ROWS][DUNGEON_COLS];
    DungeonRoom    rooms[MAX_ROOMS];
    int            roomCount;
    DungeonMob     mobs[MAX_DUNGEON_MOBS];
    DungeonLoot    loots[MAX_LOOT_ITEMS];
    AllyUnit       allies[MAX_ALLY_UNITS];
    Interactable   interactables[MAX_INTERACTABLES];
    int            interactableCount;
    Inventory      inventory;
    float          allyCooldown;   /* R skill global cooldown */
    bool           isDungeonActive;
    bool           allRoomsCleared;
    /* Önceki şehir isimleri (R skill için kaynak) */
    char           cityNames[MAX_ALLY_UNITS][16];
    int            cityCount;
} DungeonMode;

void  InitHero(Hero *hero, HeroClass cls);
void  AddHeroXP(Hero *hero, int amount);
void  InitDungeon(DungeonMode *dungeon, Hero *hero);
void  UpdateDungeon(DungeonMode *dungeon, Hero *hero, float dt);
void  DrawDungeon(DungeonMode *dungeon, Hero *hero);
void  UpdateHero(Hero *hero, DungeonMode *dungeon, float dt);
void  SpawnAllies(DungeonMode *dungeon, Vector2 heroPos);

#endif
