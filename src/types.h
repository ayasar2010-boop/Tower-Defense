/* types.h — T54: Tüm enum, struct ve sabitler için ortak başlık.
 * Bu dosya #include chain'in zirvesidir; döngüsel bağımlılık olmaması için
 * sadece sistem başlıkları + ayrı modül başlıkları include eder. */

#ifndef TYPES_H
#define TYPES_H

#include "daycycle.h"
#include "dungeon.h"
#include "homecity.h"
#include "i18n.h"
#include "save.h"
#include "settings.h"
#include "siege.h"
#include <raylib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ============================================================
 * SABİTLER
 * ============================================================ */
#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720
#define GRID_COLS     120
#define GRID_ROWS     80
#define CELL_SIZE     20
#define GRID_OFFSET_X 0
#define GRID_OFFSET_Y 0
#define ISO_HALF_W    CELL_SIZE
#define ISO_HALF_H    (CELL_SIZE / 2)
#define ISO_ORIGIN_X  (GRID_ROWS * ISO_HALF_W)
#define ISO_ORIGIN_Y  20

#define MAX_ENEMIES         50
#define MAX_TOWERS          30
#define MAX_PROJECTILES     100
#define MAX_PARTICLES       200
#define MAX_WAYPOINTS       20
#define MAX_WAVES           20
#define MAX_LEVELS          5
#define MAX_DIALOGUE_LINES  8
#define MAX_TIP_TEXTS       6
#define MAX_FLOATING_TEXTS  40
#define TRAIL_LENGTH        8
#define SYNERGY_RADIUS      180.0f
#define MAX_SYNERGY_PAIRS   60

#define ENEMY_BASE_SPEED  80.0f
#define ENEMY_BASE_HP     100.0f

#define TOWER_COST_BASIC   50
#define TOWER_COST_SNIPER  100
#define TOWER_COST_SPLASH  150

#define STARTING_GOLD    200
#define STARTING_LIVES   20
#define KILL_REWARD      15

#define WAYPOINT_REACH_DIST  6.0f
#define MAX_FRIENDLY_UNITS   20
#define FUNIT_BLOCK_DIST     28.0f

#define MAX_BUILDINGS        10
#define MAX_OUTPOSTS          5
#define MAX_COURIER_UNITS     4
#define DEFSIM_SNAPSHOT_INTERVAL 10.0f
#define DEFSIM_CRITICAL_LIVES    5
#define PREP_PHASE_DURATION  25.0f
#define MAX_GUARDIANS         8
#define GUARDIAN_AGGRO_RANGE 80.0f

/* ============================================================
 * ENUM'LAR
 * ============================================================ */

/* T92 — Nadirlik sistemi (5 kademe) */
typedef enum {
    RARITY_COMMON    = 0,
    RARITY_UNCOMMON  = 1,
    RARITY_RARE      = 2,
    RARITY_EPIC      = 3,
    RARITY_MYTHICAL  = 4,
    RARITY_COUNT
} ItemRarity;

/* T97 — Görev (Quest) sistemi */
#define SHRINE_KILL_RANGE 80.0f

typedef enum {
    QUEST_INACTIVE = 0,
    QUEST_ACTIVE,
    QUEST_COMPLETED
} QuestState;

typedef enum {
    QUEST_CARAVAN_RESCUE = 0, /* 3 kervan arabası bul        */
    QUEST_SHRINE_RITUAL,      /* Sunakta 15 düşman öldür     */
    QUEST_PERFECT_DEFENSE,    /* 3 dalgayı can kaybetmeden   */
    QUEST_TYPE_COUNT
} QuestType;

typedef struct {
    QuestType   type;
    QuestState  state;
    const char *name;
    const char *description;
    int         progress;
    int         target;
} Quest;

typedef struct {
    Quest  quests[QUEST_TYPE_COUNT];
    int    perfectWaveStreak;
    int    livesAtWaveStart;
    float  notifyTimer;
    char   notifyText[64];
} QuestManager;

typedef enum {
    CELL_EMPTY = 0, CELL_PATH = 1, CELL_BUILDABLE = 2,
    CELL_TOWER = 3, CELL_RURAL = 4, CELL_VILLAGE = 5
} CellType;

typedef enum {
    TERRAIN_NONE = 0, TERRAIN_MOUNTAIN = 1, TERRAIN_ROCK = 2,
    TERRAIN_RIVER = 3, TERRAIN_FIELD = 4, TERRAIN_TREE = 5,
    TERRAIN_BUSH = 6,
    /* T87 — Gameplay terrain */
    TERRAIN_MUD = 7,        /* -%30 hız */
    TERRAIN_TALL_GRASS = 8, /* Gizlenme: fog dışında görünmez */
    TERRAIN_STONY = 9,      /* +%20 hasar azaltma */
    TERRAIN_COUNT
} TerrainType;

typedef enum { TOWER_BASIC, TOWER_SNIPER, TOWER_SPLASH, TOWER_TYPE_COUNT } TowerType;

typedef enum { ENEMY_NORMAL, ENEMY_FAST, ENEMY_TANK, ENEMY_BOSS, ENEMY_TYPE_COUNT } EnemyType;

typedef enum {
    STATE_MENU,
    STATE_CLASS_SELECT,
    STATE_WORLD_MAP,
    STATE_LEVEL_BRIEFING,
    STATE_LOADING,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_WAVE_CLEAR,
    STATE_PREP_PHASE,
    STATE_DUNGEON,
    STATE_LEVEL_COMPLETE,
    STATE_GAMEOVER,
    STATE_VICTORY,
    STATE_SETTINGS
} GameState;

typedef enum {
    SFX_SHOOT_BASIC = 0, SFX_SHOOT_SNIPER, SFX_SHOOT_SPLASH,
    SFX_ENEMY_DEATH, SFX_TANK_DEATH, SFX_TOWER_PLACE, SFX_TOWER_UPGRADE,
    SFX_TOWER_SELL, SFX_WAVE_START, SFX_LIVE_LOST, SFX_BUTTON_CLICK,
    SFX_VICTORY, SFX_DEFEAT, SFX_LEVEL_UP, SFX_COUNT
} SFXType;

typedef enum {
    FUNIT_ARCHER = 0, FUNIT_WARRIOR = 1, FUNIT_MAGE = 2,
    FUNIT_KNIGHT = 3, FUNIT_HERO = 4, FUNIT_VILLAGER = 5, FUNIT_TYPE_COUNT
} FUnitType;

typedef enum { FUNIT_HOLD, FUNIT_ATTACK, FUNIT_MOVE, FUNIT_PATROL } FUnitOrder;

/* T83 — Köylü FSM durumları */
typedef enum {
    VSTATE_IDLE = 0, VSTATE_MOVE_TO_NODE, VSTATE_BUILD_CAMP,
    VSTATE_GATHERING, VSTATE_TRANSPORT
} VillagerState;

typedef enum {
    BUILDING_BARRACKS, BUILDING_MARKET, BUILDING_BARRICADE,
    BUILDING_TOWN_CENTER, BUILDING_BLACKSMITH,
    BUILDING_DUNGEON_ENTRANCE, /* T101 — Zindan Kapısı */
    BUILDING_COURIER_POST,     /* T101 — Ulak Postası   */
    BUILDING_TYPE_COUNT
} BuildingType;

/* ============================================================
 * STRUCT'LAR
 * ============================================================ */
typedef struct {
    Vector2      position;
    FUnitType    type;
    FUnitOrder   order;
    Vector2      moveTarget;
    float        hp, maxHp;
    float        atk;
    float        attackRange;
    float        attackCooldown;
    float        attackSpeed;
    int          engagedEnemyIdx;
    bool         active;
    bool         selected;
    /* T85 — patrol için ikinci hedef nokta */
    Vector2      patrolTarget;
    bool         patrolReturn;
    /* T83 — köylü FSM alanları */
    VillagerState vstate;
    float         gatherTimer;
    float         resourceCarried;
    int           outpostIdx;
    Vector2       nodePos;
} FriendlyUnit;

/* T83 — Outpost (dışarıdaki kamp yapısı) */
typedef struct {
    Vector2 position;
    int     level;        /* 0=yok, 1=kamp */
    float   resources;    /* birikmiş kaynak */
    float   buildTimer;
    bool    active;
} Outpost;

/* T101 — Diegetic UI: Ulak birimi (atlı haberci) */
typedef struct {
    Vector2     position;
    Vector2     destination;
    float       speed;
    float       angle;
    float       lifetime;
    char        message[48];   /* iletilen mesaj (opsiyonel) */
    Color       color;
    bool        active;
} CourierUnit;

/* T102 — Background Defense Sim: Dungeon'dayken savunma durumu */
typedef struct {
    float   totalTowerDPS;        /* tüm aktif kulelerin toplam DPS'i */
    int     totalSoldierCount;    /* aktif asker sayısı */
    float   snapshotTimer;        /* 10s'de bir güncelle */
    float   threatLevel;          /* mevcut düşman tehdidi (0–1) */
    int     livesLostWhileAway;   /* dungeon'dayken kaybedilen can */
    bool    criticalWarning;      /* savunma kritik eşiğin altında mı */
    float   critWarnTimer;        /* uyarı animasyon timer */
    float   lastSnapshotDPS;      /* son snapshot DPS değeri */
    int     lastSnapshotSoldiers; /* son snapshot asker sayısı */
} BackgroundDefSim;

typedef struct {
    Vector2   position;
    Vector2   direction;
    float     speed;
    float     maxHp;
    float     currentHp;
    float     slowTimer;
    float     slowFactor;
    int       currentWaypoint;
    EnemyType type;
    Color     color;
    float     radius;
    bool      active;
    bool      isFlying;
    int       engagedFriendlyIdx;
    bool      isBoss;
    int       bossPhase;
    float     abilityTimer;
    bool      isCasting;
    float     castTimer;
    Vector2   targetAbilityPos;
    /* T87 — Terrain efektleri */
    bool      inTallGrass;
    float     terrainArmor; /* 0.0-1.0: hasar azaltma oranı */
    /* T98 — Director AI: elit varyant */
    bool      isElite;
} Enemy;

/* T98 — Director AI: performans takibi + elit zorluk */
typedef struct {
    float performanceScore;    /* EMA, 0.0=düşük → 1.0=çok iyi */
    float eliteChance;         /* Mevcut dalga için 0.0–0.5 spawn şansı */
    int   consecutiveFlawless; /* Can kaybetmeden geçilen ardışık dalga */
    bool  eliteModeActive;
    float waveStartTime;       /* GetTime() ile kaydedilen dalga başlangıcı */
    int   livesAtWaveStart;    /* Director kendi snapshot'ı */
} DirectorAI;

typedef struct {
    Vector2   position;
    int       gridX, gridY;
    TowerType type;
    float     range;
    float     damage;
    float     fireRate;
    float     fireCooldown;
    float     splashRadius;
    int       level;
    float     rotation;
    int       targetEnemyIndex;
    Color     color;
    bool      active;
    float     flashTimer;
    bool      synergyApplied;
} Tower;

typedef struct {
    Vector2   position;
    Vector2   velocity;
    float     damage;
    float     splashRadius;
    float     speed;
    int       targetIndex;
    TowerType sourceType;
    Color     color;
    bool      active;
    bool      isEnemyProjectile;
    Vector2   trailPositions[TRAIL_LENGTH];
    int       trailIndex;
} Projectile;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float   lifetime;
    float   maxLifetime;
    Color   color;
    float   size;
    bool    active;
} Particle;

typedef struct {
    Sound sounds[SFX_COUNT];
    bool  loaded[SFX_COUNT];
    bool  deviceReady;
    float masterVolume;
} AudioManager;

typedef struct { int idxA, idxB; } SynergyPair;

typedef struct {
    float intensity;
    float decay;
    float offsetX;
    float offsetY;
} ScreenShake;

typedef struct {
    Camera2D cam;
    float    zoom;
    bool     heroFollow;
} GameCamera;

typedef struct {
    Vector2 position;
    float   value;
    float   lifetime;
    float   maxLifetime;
    Color   color;
    bool    isCrit;
    bool    active;
} FloatingText;

typedef struct {
    EnemyType enemyTypes[3];
    int       enemyCounts[3];
    int       typeCount;
    int       enemyCount;
    float     spawnInterval;
    float     spawnTimer;
    int       spawnedCount;
    float     preDelay;
    bool      started;
    float     enemyHpMultiplier;
    bool      isBossWave;
    int       bonusGold;
    char      waveName[32];
} GameWave;

typedef struct {
    bool loaded;
} Assets;

typedef struct {
    const char *speaker;
    const char *portrait;
    const char *text;
    Color       speakerColor;
} DialogueLine;

typedef struct {
    DialogueLine lines[MAX_DIALOGUE_LINES];
    int          lineCount;
    int          currentLine;
    bool         active;
    float        charTimer;
    int          visibleChars;
} DialogueSystem;

typedef struct {
    const char  *name;
    const char  *description;
    const char  *enemyLore;
    const char  *mapBgAsset;
    int          nodeX, nodeY;
    DialogueLine briefing[MAX_DIALOGUE_LINES];
    int          briefingCount;
    DialogueLine completion[MAX_DIALOGUE_LINES];
    int          completionCount;
    int          star3Lives;
    int          star2Lives;
    int          stars;
    bool         unlocked;
} LevelData;

typedef struct {
    int          gridX, gridY;
    BuildingType type;
    bool         active;
    /* T85 — Auto-Train & Patrol */
    bool         autoTrain;
    float        trainCooldown;   /* kalan süre */
    Vector2      rallyPoint;
} Building;

/* T88 — Kaynak Muhafızı */
typedef struct {
    Vector2 position;
    Vector2 patrolCenter;
    float   patrolAngle;
    float   hp, maxHp;
    float   radius;
    int     level;          /* 1=Zayıf, 2=Orta, 3=Güçlü */
    float   attackDamage;
    float   attackCooldown;
    float   attackRange;
    int     lootGold;
    bool    aggro;
    bool    active;
} ResourceGuardian;

typedef struct {
    float       progress;
    float       timer;
    float       duration;
    GameState   nextState;
    const char *tipText;
} LoadingScreen;

/* T89 — Loot Chest */
#define MAX_CHEST_ITEMS 6

typedef struct {
    float openTimer;     /* 0.0 → 1.0, animasyon süresi */
    bool  animDone;
    bool  visible;
    char  itemNames[MAX_CHEST_ITEMS][32];
    int   itemRarity[MAX_CHEST_ITEMS]; /* 0=Common,1=Uncommon,2=Rare,3=Epic */
    int   itemGold;      /* toplam altın ödülü */
    int   itemCount;
} LootChest;

/* T86 — Manager tick-rate sabitleri */
#define MGR_ECO_HZ   4.0f
#define MGR_AI_HZ    5.0f
#define MGR_TICK_ECO (1.0f / MGR_ECO_HZ)
#define MGR_TICK_AI  (1.0f / MGR_AI_HZ)

typedef struct {
    float ecoTimer;      /* Ekonomi tick birikimcisi */
    float aiTimer;       /* AI karar tick birikimcisi */
    int   marketIncome;  /* Son eko-tick market geliri */
    int   activeUnits;   /* Son AI-tick aktif birim sayısı */
    int   engagedUnits;  /* Son AI-tick çarpışan birim sayısı */
} Managers;

/* ============================================================
 * GAME — Ana oyun durumu (tüm alt sistemleri içerir)
 * ============================================================ */
typedef struct {
    int  grid[GRID_ROWS][GRID_COLS];
    Vector2 waypoints[MAX_WAYPOINTS];
    int  waypointCount;

    Enemy       enemies[MAX_ENEMIES];
    Tower       towers[MAX_TOWERS];
    Projectile  projectiles[MAX_PROJECTILES];
    Particle    particles[MAX_PARTICLES];
    GameWave    waves[MAX_WAVES];

    int       currentWave;
    int       totalWaves;
    bool      waveActive;
    int       gold;
    int       lives;
    int       score;
    int       enemiesKilled;

    GameState  state;
    TowerType  selectedTowerType;
    float      gameSpeed;
    bool       showGrid;

    bool      contextMenuOpen;
    int       contextMenuTowerIdx;
    Vector2   contextMenuPos;

    Assets        assets;
    LevelData     levels[MAX_LEVELS];
    int           currentLevel;
    int           levelStars;
    DialogueSystem dialogue;
    LoadingScreen  loading;
    HomeCity       homeCity;
    SiegeMechanics siege;

    Building      buildings[MAX_BUILDINGS];
    int           buildingCount;
    float         prepTimer;
    BuildingType  selectedBuilding;

    DungeonMode   dungeon;
    Hero          hero;
    GameState     preDungeonState;

    FriendlyUnit  friendlyUnits[MAX_FRIENDLY_UNITS];
    int           pendingPlacementCount;
    FUnitType     pendingPlacementType;
    /* T83 — Outpost (kamp) dizisi */
    Outpost       outposts[MAX_OUTPOSTS];
    int           outpostCount;

    ScreenShake   screenShake;
    FloatingText  floatingTexts[MAX_FLOATING_TEXTS];
    SynergyPair   synergyPairs[MAX_SYNERGY_PAIRS];
    int           synergyPairCount;

    float         waveNameTimer;
    char          waveNameText[32];

    GameCamera    camera;
    AudioManager  audio;

    bool      isSelecting;
    Vector2   selectionStart;
    Vector2   selectionEnd;

    int       selectedBuildingType;
    float     townCenterTimer;

    bool          fogVisible[GRID_ROWS][GRID_COLS];
    bool          fogExplored[GRID_ROWS][GRID_COLS];
    unsigned char terrainLayer[GRID_ROWS][GRID_COLS];

    Settings  settings;
    GameState preSettingsState;

    Managers  managers; /* T86 — tick-rate manager */

    /* T88 — Kaynak Muhafızları */
    ResourceGuardian guardians[MAX_GUARDIANS];
    int              guardianCount;

    /* T89 — Loot Chest */
    LootChest lootChest;

    /* T93 — Blacksmith Forge UI durumu */
    bool forgeOpen;

    /* T94 — Yükseltme malzemeleri */
    int ironOre;
    int magicDust;

    /* T97 — Görev Yöneticisi */
    QuestManager questManager;

    /* T98 — Director AI */
    DirectorAI director;

    /* T100 — Gün/Gece döngüsü */
    DayCycle dayCycle;

    /* T101 — Diegetic UI: ulak birimleri + zindan kapısı konumu */
    CourierUnit  courierUnits[MAX_COURIER_UNITS];
    int          courierCount;
    Vector2      dungeonEntrancePos; /* dünya koordinatı */
    bool         dungeonEntranceBuilt;

    /* T102 — Background Defense Simulation */
    BackgroundDefSim defSim;
} Game;

/* ============================================================
 * T56 — SAVE DATA
 * ============================================================ */
typedef struct {
    int       gold, lives, score, enemiesKilled;
    int       currentWave, totalWaves;
    bool      waveActive;
    GameState state, preDungeonState;
    int       currentLevel;

    Tower        towers[MAX_TOWERS];
    GameWave     waves[MAX_WAVES];
    Building     buildings[MAX_BUILDINGS];
    int          buildingCount;
    float        prepTimer, townCenterTimer;

    FriendlyUnit friendlyUnits[MAX_FRIENDLY_UNITS];
    int          pendingPlacementCount;
    FUnitType    pendingPlacementType;

    HomeCity       homeCity;
    SiegeMechanics siege;
    Hero           hero;
    DungeonMode    dungeon;

    bool          fogExplored[GRID_ROWS][GRID_COLS];
    unsigned char terrainLayer[GRID_ROWS][GRID_COLS];
} SaveData;

#endif /* TYPES_H */
