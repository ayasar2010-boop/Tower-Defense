/* types.h — T54: Tüm enum, struct ve sabitler için ortak başlık.
 * Bu dosya #include chain'in zirvesidir; döngüsel bağımlılık olmaması için
 * sadece sistem başlıkları + ayrı modül başlıkları include eder. */

#ifndef TYPES_H
#define TYPES_H

#include "dungeon.h"
#include "homecity.h"
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
#define PREP_PHASE_DURATION  25.0f

/* ============================================================
 * ENUM'LAR
 * ============================================================ */
typedef enum {
    CELL_EMPTY = 0, CELL_PATH = 1, CELL_BUILDABLE = 2,
    CELL_TOWER = 3, CELL_RURAL = 4, CELL_VILLAGE = 5
} CellType;

typedef enum {
    TERRAIN_NONE = 0, TERRAIN_MOUNTAIN = 1, TERRAIN_ROCK = 2,
    TERRAIN_RIVER = 3, TERRAIN_FIELD = 4, TERRAIN_TREE = 5,
    TERRAIN_BUSH = 6, TERRAIN_COUNT
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
    FUNIT_KNIGHT = 3, FUNIT_HERO = 4, FUNIT_TYPE_COUNT
} FUnitType;

typedef enum { FUNIT_HOLD, FUNIT_ATTACK, FUNIT_MOVE } FUnitOrder;

typedef enum {
    BUILDING_BARRACKS, BUILDING_MARKET, BUILDING_BARRICADE,
    BUILDING_TOWN_CENTER, BUILDING_TYPE_COUNT
} BuildingType;

/* ============================================================
 * STRUCT'LAR
 * ============================================================ */
typedef struct {
    Vector2   position;
    FUnitType type;
    FUnitOrder order;
    Vector2   moveTarget;
    float     hp, maxHp;
    float     atk;
    float     attackRange;
    float     attackCooldown;
    float     attackSpeed;
    int       engagedEnemyIdx;
    bool      active;
    bool      selected;
} FriendlyUnit;

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
} Enemy;

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
} Building;

typedef struct {
    float       progress;
    float       timer;
    float       duration;
    GameState   nextState;
    const char *tipText;
} LoadingScreen;

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
