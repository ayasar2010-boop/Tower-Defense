/* Ruler — C99 + Raylib
 * Derleme: gcc src/main.c -lraylib -lm -lpthread -ldl -lrt -o game
 * Windows: gcc src/main.c -lraylib -lopengl32 -lgdi32 -lwinmm -o game.exe
 */

#include <raylib.h>
#include <math.h>
#include "homecity.h"
#include "siege.h"
#include "dungeon.h"
#include "ui.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================
 * T03 — SABİTLER VE KONFİGÜRASYON
 * ============================================================ */

#define SCREEN_WIDTH            1280
#define SCREEN_HEIGHT           720
#define GRID_COLS               120
#define GRID_ROWS               80
#define CELL_SIZE               12
#define GRID_OFFSET_X           0
#define GRID_OFFSET_Y           0

#define MAX_ENEMIES             50
#define MAX_TOWERS              30
#define MAX_PROJECTILES         100
#define MAX_PARTICLES           200
#define MAX_WAYPOINTS           20
#define MAX_WAVES               20  /* T62 — 20 dalga sistemi */
#define MAX_LEVELS              5   /* Kampanya seviye sayısı */
#define MAX_DIALOGUE_LINES      8   /* Bir diyalog oturumundaki maks satır */
#define MAX_TIP_TEXTS           6   /* Loading screen ipucu havuzu */
#define MAX_FLOATING_TEXTS      40  /* T57 — uçan hasar sayıları */
#define TRAIL_LENGTH            8   /* T58 — mermi kuyruk uzunluğu */
#define SYNERGY_RADIUS          120.0f /* T59 — kule sinerji etki alanı */
#define MAX_SYNERGY_PAIRS       60  /* T59 — maksimum sinerji çifti */

#define ENEMY_BASE_SPEED        60.0f
#define ENEMY_BASE_HP           100.0f

#define TOWER_COST_BASIC        50
#define TOWER_COST_SNIPER       100
#define TOWER_COST_SPLASH       150

#define STARTING_GOLD           200
#define STARTING_LIVES          20
#define KILL_REWARD             15

/* Waypoint'e ulaşma eşiği — bu değerin altında bir sonraki waypoint'e geçilir.
 * 4.0f: 60 FPS'de bir frame'de maksimum hareket ~4px olduğundan kaçırma riski yok. */
#define WAYPOINT_REACH_DIST     4.0f
#define MAX_FRIENDLY_UNITS      20
#define FUNIT_BLOCK_DIST        28.0f  /* Düşmanı durdurmak için menzil */

/* ============================================================
 * T03 — ENUM TANIMLARI
 * ============================================================ */

typedef enum {
    CELL_EMPTY    = 0,
    CELL_PATH     = 1,
    CELL_BUILDABLE= 2,
    CELL_TOWER    = 3,
    CELL_RURAL    = 4,  /* T67 — Bina kurulabilir kırsal alan */
    CELL_VILLAGE  = 5   /* T68 — Korunan köy/kasaba */
} CellType;

typedef enum {
    TOWER_BASIC,
    TOWER_SNIPER,
    TOWER_SPLASH,
    TOWER_TYPE_COUNT
} TowerType;

typedef enum {
    ENEMY_NORMAL,
    ENEMY_FAST,
    ENEMY_TANK,
    ENEMY_BOSS,
    ENEMY_TYPE_COUNT
} EnemyType;

typedef enum {
    STATE_MENU,
    STATE_CLASS_SELECT,     /* T52 — Kahraman sınıf seçimi                    */
    STATE_WORLD_MAP,        /* Kingdom Rush: yıldızlı level haritası          */
    STATE_LEVEL_BRIEFING,   /* Level öncesi senaryo / diyalog ekranı           */
    STATE_LOADING,          /* Sahte loading bar + ipucu metni                 */
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_WAVE_CLEAR,
    STATE_PREP_PHASE,       /* T43 — dalga arası inşa fazı (25 sn)             */
    STATE_DUNGEON,          /* T44 — Dungeon RPG modu                          */
    STATE_LEVEL_COMPLETE,   /* Dalga temizlendi — yıldız + ödül ekranı         */
    STATE_GAMEOVER,
    STATE_VICTORY           /* Tüm kampanya tamamlandı                         */
} GameState;

/* T64 — Ses efekti tipleri */
typedef enum {
    SFX_SHOOT_BASIC = 0,
    SFX_SHOOT_SNIPER,
    SFX_SHOOT_SPLASH,
    SFX_ENEMY_DEATH,
    SFX_TANK_DEATH,
    SFX_TOWER_PLACE,
    SFX_TOWER_UPGRADE,
    SFX_TOWER_SELL,
    SFX_WAVE_START,
    SFX_LIVE_LOST,
    SFX_BUTTON_CLICK,
    SFX_VICTORY,
    SFX_DEFEAT,
    SFX_LEVEL_UP,
    SFX_COUNT
} SFXType;

/* ============================================================
 * T04 — STRUCT TANIMLARI
 * ============================================================ */

typedef enum {
    FUNIT_ARCHER  = 0,
    FUNIT_WARRIOR = 1,
    FUNIT_MAGE    = 2,
    FUNIT_KNIGHT  = 3,
    FUNIT_HERO    = 4,
    FUNIT_TYPE_COUNT
} FUnitType;

typedef enum {
    FUNIT_HOLD,   /* Yerinde dur, menzile gireni vur */
    FUNIT_ATTACK  /* A tusu: en yakin dusmana dogru ilerle */
} FUnitOrder;

typedef struct {
    Vector2    position;
    FUnitType  type;
    FUnitOrder order;
    float      hp, maxHp;
    float      atk;
    float      attackRange;
    float      attackCooldown;
    float      attackSpeed; /* saldiris/saniye */
    int        engagedEnemyIdx; /* -1 = serbest */
    bool       active;
    bool       selected;   /* T65 — RTS seçim kutusuyla seçilmiş mi */
} FriendlyUnit;

typedef struct {
    Vector2  position;
    Vector2  direction;
    float    speed;
    float    maxHp;
    float    currentHp;
    float    slowTimer;
    float    slowFactor;
    int      currentWaypoint;
    EnemyType type;
    Color    color;
    float    radius;
    bool     active;
    bool     isFlying;           /* true → dost birimleri atlar */
    int      engagedFriendlyIdx; /* Savastigi dost birimin indeksi, -1 = yok */
    bool     isBoss;             /* T55 */
    int      bossPhase;          /* 1, 2, 3 */
    float    abilityTimer;       /* T55 */
    bool     isCasting;          /* T55 Boss cast state */
    float    castTimer;
    Vector2  targetAbilityPos;   /* T55 Meteor warning center */
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
    float     flashTimer;     /* B04: ateş flash süresi */
    bool      synergyApplied; /* T59: bu kuleye sinerji bonusu uygulandı mı */
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
    bool      isEnemyProjectile;  /* T55 — Boss projectiles hit towers/allies */
    /* T58 — mermi kuyruk efekti */
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

/* T64 — Ses sistemi: ses dosyası yoksa sessiz devam eder */
typedef struct {
    Sound    sounds[SFX_COUNT]; /* Raylib Sound handle'ları */
    bool     loaded[SFX_COUNT]; /* Dosya yüklenip yüklenmediği */
    bool     deviceReady;       /* InitAudioDevice() basariliysa true */
    float    masterVolume;      /* 0.0 – 1.0 */
} AudioManager;

/* T59 — Kule sinerji çifti (çizim ve takip için) */
typedef struct {
    int idxA, idxB;
} SynergyPair;

/* T56 — Ekran sarsıntısı */
typedef struct {
    float intensity;   /* Mevcut şiddet (piksel) */
    float decay;       /* Sönümleme hızı (şiddet/saniye) */
    float offsetX;     /* Her frame hesaplanan rastgele X ofseti */
    float offsetY;     /* Her frame hesaplanan rastgele Y ofseti */
} ScreenShake;

/* T63 — Kamera sistemi */
typedef struct {
    Camera2D cam;        /* Raylib Camera2D: target, offset, zoom */
    float    zoom;       /* Aktif zoom seviyesi (0.5×–2.0×) */
    bool     heroFollow; /* H tuşu: hero'yu takip et */
} GameCamera;

/* T57 — Uçan hasar/iyileşme sayıları */
typedef struct {
    Vector2 position;
    float   value;
    float   lifetime;
    float   maxLifetime;
    Color   color;
    bool    isCrit;
    bool    active;
} FloatingText;

/* T62 — 20 dalga sistemi: karışık tip + boss wave desteği */
typedef struct {
    EnemyType enemyTypes[3];       /* Dalgadaki düşman tipleri (maks 3) */
    int       enemyCounts[3];      /* Her tip için sayı */
    int       typeCount;           /* Aktif tip sayısı: 1/2/3 */
    int       enemyCount;          /* Toplam düşman sayısı */
    float     spawnInterval;
    float     spawnTimer;
    int       spawnedCount;
    float     preDelay;
    bool      started;
    float     enemyHpMultiplier;   /* HP çarpanı */
    bool      isBossWave;          /* Boss dalgası mı */
    int       bonusGold;           /* Dalga tamamlama altın ödülü */
    char      waveName[32];        /* Dalga adı: "İlk Akın" vb. */
} GameWave;

/* ================================================================
 * ASSET SİSTEMİ — PLACEHOLDER
 * Gerçek implementasyonda aşağıdaki yorumlu alanlar aktif edilir.
 * Tüm dosya yolları "assets/" altındaki yapıyı gösterir.
 * ================================================================ */
typedef struct {
    /* ---- Sprite'lar (Texture2D) ----
     * Texture2D enemyNormal;      // "assets/sprites/enemies/goblin_walk.png"
     * Texture2D enemyFast;        // "assets/sprites/enemies/wolf_run.png"
     * Texture2D enemyTank;        // "assets/sprites/enemies/troll_walk.png"
     * Texture2D towerBasic;       // "assets/sprites/towers/archer_tower.png"
     * Texture2D towerSniper;      // "assets/sprites/towers/mage_tower.png"
     * Texture2D towerSplash;      // "assets/sprites/towers/cannon_tower.png"
     * Texture2D tileGrass;        // "assets/sprites/tiles/grass_tile.png"
     * Texture2D tilePath;         // "assets/sprites/tiles/dirt_road.png"
     * Texture2D uiHudFrame;       // "assets/sprites/ui/hud_frame.png"
     * Texture2D worldMapBg;       // "assets/sprites/ui/world_map.png"
     * Texture2D portraitHero;     // "assets/sprites/portraits/aldric.png"
     * Texture2D portraitVillain;  // "assets/sprites/portraits/vex.png"
     * Texture2D portraitNarrator; // "assets/sprites/portraits/narrator.png"
     */

    /* ---- Ses Efektleri (Sound) ----
     * Sound sfxShootBasic;        // "assets/sfx/arrow_shoot.wav"
     * Sound sfxShootSniper;       // "assets/sfx/magic_bolt.wav"
     * Sound sfxShootSplash;       // "assets/sfx/cannon_fire.wav"
     * Sound sfxEnemyDeath;        // "assets/sfx/enemy_death.wav"
     * Sound sfxTankDeath;         // "assets/sfx/troll_death.wav"
     * Sound sfxTowerPlace;        // "assets/sfx/tower_build.wav"
     * Sound sfxTowerUpgrade;      // "assets/sfx/upgrade.wav"
     * Sound sfxTowerSell;         // "assets/sfx/coins.wav"
     * Sound sfxWaveStart;         // "assets/sfx/drum_roll.wav"
     * Sound sfxLiveLost;          // "assets/sfx/life_lost.wav"
     * Sound sfxDialogueBlip;      // "assets/sfx/dialogue_blip.wav"
     * Sound sfxButtonClick;       // "assets/sfx/button_click.wav"
     * Sound sfxVictory;           // "assets/sfx/fanfare.wav"
     * Sound sfxDefeat;            // "assets/sfx/defeat.wav"
     */

    /* ---- Müzik (Music) ----
     * Music bgmMenu;              // "assets/music/main_theme.ogg"
     * Music bgmBattle[MAX_LEVELS];// "assets/music/level_N_battle.ogg"
     * Music bgmVictory;           // "assets/music/victory.ogg"
     * Music bgmWorldMap;          // "assets/music/world_map.ogg"
     */

    /* ---- Font ----
     * Font fontMain;              // "assets/fonts/medievalsharp.ttf"
     * Font fontHUD;               // "assets/fonts/pixel_hud.ttf"
     */

    bool loaded;  /* true → draw fonksiyonları gerçek asset kullanır */
} Assets;

/* ================================================================
 * NARATİF / SENARYO SİSTEMİ — PLACEHOLDER
 * speaker + portrait + metin + renk tupleından oluşur.
 * Typewriter efekti DialogueSystem üzerinden çalışır.
 * ================================================================ */
typedef struct {
    const char *speaker;      /* "Komutan Aldric", "Karanlik Lord Vex", "[Anlatici]" */
    const char *portrait;     /* TODO: Assets içindeki Texture2D*'ya pointer olacak  */
    const char *text;
    Color       speakerColor;
} DialogueLine;

typedef struct {
    DialogueLine lines[MAX_DIALOGUE_LINES];
    int          lineCount;
    int          currentLine;
    bool         active;
    float        charTimer;    /* harf-harf yazma hız sayacı */
    int          visibleChars; /* şu an ekranda görünen karakter sayısı */
} DialogueSystem;

/* ================================================================
 * LEVEL / SEVİYE VERİSİ — PLACEHOLDER
 * Her level'ın adı, backstory'si, harita asset referansı,
 * dünya haritasındaki konumu ve diyalogları buradadır.
 * ================================================================ */
typedef struct {
    const char  *name;           /* "Yesil Tepeler", "Karanlik Orman" ... */
    const char  *description;    /* Harita seçim ekranındaki kısa açıklama */
    const char  *enemyLore;      /* Bu haritadaki düşmanların backstory'si */
    const char  *mapBgAsset;     /* TODO: "assets/sprites/maps/map_0N.png" */
    int          nodeX, nodeY;   /* Dünya haritasındaki düğüm merkezi */

    DialogueLine briefing[MAX_DIALOGUE_LINES];   /* Level öncesi diyalog */
    int          briefingCount;

    DialogueLine completion[MAX_DIALOGUE_LINES]; /* Level bitti diyaloğu */
    int          completionCount;

    int   star3Lives;  /* 3 yıldız için kalan minimum can */
    int   star2Lives;  /* 2 yıldız için kalan minimum can */
    int   stars;       /* kazanılan yıldız 0–3 */
    bool  unlocked;
} LevelData;

/* ================================================================
 * T43 — PREP PHASE BİNALARI
 * ================================================================ */
#define MAX_BUILDINGS 10
#define PREP_PHASE_DURATION 25.0f

typedef enum {
    BUILDING_BARRACKS,    /* Her dalga başında +1 dost birim */
    BUILDING_MARKET,      /* Düşman öldürme altın ödülü +20% */
    BUILDING_BARRICADE,   /* Yol üstüne engel — düşmanı yavaşlatır */
    BUILDING_TOWN_CENTER, /* T67 — Pasif gelir: 10 sn’de +5 altın. Maks 1 */
    BUILDING_TYPE_COUNT
} BuildingType;

typedef struct {
    int          gridX, gridY;
    BuildingType type;
    bool         active;
} Building;

/* ================================================================
 * LOADING SCREEN
 * ================================================================ */
typedef struct {
    float      progress;    /* 0.0 – 1.0 */
    float      timer;
    float      duration;    /* sahte loading süresi (saniye) */
    GameState  nextState;
    const char *tipText;    /* Loading sırasında gösterilen ipucu */
} LoadingScreen;

typedef struct {
    int     grid[GRID_ROWS][GRID_COLS];
    Vector2 waypoints[MAX_WAYPOINTS];
    int     waypointCount;

    Enemy       enemies[MAX_ENEMIES];
    Tower       towers[MAX_TOWERS];
    Projectile  projectiles[MAX_PROJECTILES];
    Particle    particles[MAX_PARTICLES];
    GameWave waves[MAX_WAVES];

    int   currentWave;
    int   totalWaves;
    bool  waveActive;

    int gold;
    int lives;
    int score;
    int enemiesKilled;

    GameState state;
    TowerType selectedTowerType;
    float     gameSpeed;
    bool      showGrid;

    /* B01: sağ tık bağlam menüsü */
    bool    contextMenuOpen;
    int     contextMenuTowerIdx;
    Vector2 contextMenuPos;

    /* Kingdom Rush Kampanya Sistemi */
    Assets       assets;
    LevelData    levels[MAX_LEVELS];
    int          currentLevel;   /* aktif level indeksi (0 tabanlı) */
    int          levelStars;     /* mevcut level için hesaplanan yıldız */
    DialogueSystem dialogue;
    LoadingScreen  loading;
    HomeCity       homeCity;

    /* T42 — Siege Mechanics */
    SiegeMechanics siege;

    /* T43 — Prep Phase */
    Building       buildings[MAX_BUILDINGS];
    int            buildingCount;
    float          prepTimer;
    BuildingType   selectedBuilding;

    /* T44 — Dungeon Mode */
    DungeonMode    dungeon;
    Hero           hero;
    GameState      preDungeonState;

    /* Dost birim sistemi */
    FriendlyUnit   friendlyUnits[MAX_FRIENDLY_UNITS];
    int            pendingPlacementCount; /* Kac birim daha yerlestirilecek */
    FUnitType      pendingPlacementType;

    /* T56 — Screen Shake */
    ScreenShake    screenShake;

    /* T57 — Floating Damage Numbers */
    FloatingText   floatingTexts[MAX_FLOATING_TEXTS];

    /* T59 — Tower Synergy Pairs */
    SynergyPair    synergyPairs[MAX_SYNERGY_PAIRS];
    int            synergyPairCount;

    /* T62 — Wave Name Banner */
    float          waveNameTimer;    /* Saniye cinsinden görünme süresi */
    char           waveNameText[32]; /* Gösterilecek dalga adı */

    /* T63 — Kamera Sistemi */
    GameCamera     camera;

    /* T64 — Ses Sistemi */
    AudioManager   audio;

    /* T65 — RTS Seçim Kutusu */
    bool       isSelecting;
    Vector2    selectionStart;
    Vector2    selectionEnd;

    /* T66/T67 — Kule/Bina seçim modu: -1 = hiçbiri seçili değil */
    int        selectedBuildingType;   /* -1 veya BuildingType */
    float      townCenterTimer;        /* T67 — Pasif gelir zamanlayıcı */

    /* T70 — Fog of War */
    bool       fogVisible[GRID_ROWS][GRID_COLS]; /* true = şu an görünür */
    bool       fogExplored[GRID_ROWS][GRID_COLS]; /* true = en az bir kez keşfedildi */
} Game;

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================ */

/* Init */
void InitGame(Game *g);
void InitMap(Game *g);
void InitWaypoints(Game *g);
void InitWaves(Game *g);
void InitLevels(Game *g);
void RestartGame(Game *g);
void RestartCurrentLevel(Game *g);

/* Update */
void UpdateMenu(Game *g);
void UpdatePause(Game *g);
void HandleInput(Game *g);
void UpdateEnemies(Game *g, float dt);
void UpdateTowers(Game *g, float dt);
void UpdateProjectiles(Game *g, float dt);
void UpdateParticles(Game *g, float dt);
void UpdateWaves(Game *g, float dt);
void CheckGameConditions(Game *g);
void UpdateWorldMap(Game *g);
void UpdateDialogue(Game *g, float dt);
void UpdateLevelBriefing(Game *g, float dt);
void UpdateLoading(Game *g, float dt);
void UpdateLevelComplete(Game *g, float dt);
void UpdatePrepPhase(Game *g, float dt);
void UpdateGameCamera(Game *g, float dt);   /* T63 */
void InitAudioManager(AudioManager *am);    /* T64 */
void PlaySFX(AudioManager *am, SFXType t);  /* T64 */
void CloseAudioManager(AudioManager *am);   /* T64 */

/* Draw */
void DrawMap(Game *g);
void DrawEnemies(Game *g);
void DrawTowers(Game *g);
void DrawProjectiles(Game *g);
void DrawParticles(Game *g);
void DrawHUD(Game *g);
void DrawPlacementPreview(Game *g);
void DrawMenu(Game *g);
void DrawPauseOverlay(Game *g);
void DrawGameOver(Game *g);
void DrawVictory(Game *g);
void DrawGame(Game *g);
void DrawContextMenu(Game *g);
void DrawPathArrows(Game *g);
void DrawWorldMap(Game *g);
void DrawDialogueBox(Game *g);
void DrawLevelBriefing(Game *g);
void DrawLoading(Game *g);
void DrawLevelComplete(Game *g);
void DrawPrepPhase(Game *g);
void DrawFloatingTexts(Game *g);   /* T57 */
void UpdateClassSelect(Game *g);   /* T52 */
void DrawClassSelect(Game *g);     /* T52 */
void DrawTowerSynergies(Game *g);  /* T59 */

/* Yardımcı */
int  CalcLevelStars(Game *g);

/* T59 — Tower Synergy */
void ApplySynergyBonuses(Game *g, int newIdx);

/* Dost birim sistemi */
void InitFriendlyUnit(FriendlyUnit *fu, FUnitType type, Vector2 pos);
void SpawnHeroUnit(Game *g);
void UpdateFriendlyUnits(Game *g, float dt);
void DrawFriendlyUnits(Game *g);

/* T56 — Screen Shake */
void TriggerScreenShake(Game *g, float intensity);
void UpdateScreenShake(Game *g, float rawDt);

/* T57 — Floating Damage Numbers */
void SpawnFloatingText(Game *g, Vector2 pos, float value, bool isCrit, bool isHeal);
void UpdateFloatingTexts(Game *g, float dt);

/* Yardımcılar */
float   Vec2Distance(Vector2 a, Vector2 b);
Vector2 Vec2Normalize(Vector2 v);
Vector2 Vec2Subtract(Vector2 a, Vector2 b);
Vector2 Vec2Scale(Vector2 v, float s);
Vector2 Vec2Add(Vector2 a, Vector2 b);
float   Vec2Length(Vector2 v);

Vector2 GridToWorld(int gx, int gy);
bool    WorldToGrid(Vector2 wp, int *gx, int *gy);

void SpawnEnemy(Game *g, EnemyType type, float hpMult);
void CreateProjectile(Game *g, Vector2 origin, int targetIdx, float damage, float splash, TowerType src);
void SpawnParticles(Game *g, Vector2 pos, Color color, int count, float speed, float lifetime);
int  FindNearestEnemy(Game *g, Vector2 pos, float range);
bool CanPlaceTower(Game *g, int gx, int gy);
int  GetTowerCost(TowerType type);
bool IsButtonClicked(Rectangle btn);
void DrawButton(Rectangle btn, const char *label, Color bg, Color fg);

/* ============================================================
 * YARDIMCI FONKSİYONLAR — Vektör İşlemleri
 * ============================================================ */

/* İki nokta arasındaki Öklid mesafesini döndürür. */
float Vec2Distance(Vector2 a, Vector2 b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return sqrtf(dx*dx + dy*dy);
}

/* Vektörü birim uzunluğa indirger; sıfır vektörde (0,0) döner. */
Vector2 Vec2Normalize(Vector2 v) {
    float len = Vec2Length(v);
    if (len < 0.0001f) return (Vector2){0, 0};
    return (Vector2){v.x / len, v.y / len};
}

Vector2 Vec2Subtract(Vector2 a, Vector2 b) { return (Vector2){a.x - b.x, a.y - b.y}; }
Vector2 Vec2Add(Vector2 a, Vector2 b)      { return (Vector2){a.x + b.x, a.y + b.y}; }
Vector2 Vec2Scale(Vector2 v, float s)      { return (Vector2){v.x * s, v.y * s}; }
float   Vec2Length(Vector2 v)              { return sqrtf(v.x*v.x + v.y*v.y); }

/* ============================================================
 * KOORDİNAT DÖNÜŞÜM FONKSİYONLARI
 * ============================================================ */

/* Grid indeksinden hücrenin merkez piksel koordinatını hesaplar. */
Vector2 GridToWorld(int gx, int gy) {
    return (Vector2){
        GRID_OFFSET_X + gx * CELL_SIZE + CELL_SIZE / 2.0f,
        GRID_OFFSET_Y + gy * CELL_SIZE + CELL_SIZE / 2.0f
    };
}

/* Piksel koordinatını grid indeksine çevirir; grid dışındaysa false döner. */
bool WorldToGrid(Vector2 wp, int *gx, int *gy) {
    int ix = (int)((wp.x - GRID_OFFSET_X) / CELL_SIZE);
    int iy = (int)((wp.y - GRID_OFFSET_Y) / CELL_SIZE);
    if (ix < 0 || ix >= GRID_COLS || iy < 0 || iy >= GRID_ROWS) return false;
    *gx = ix;
    *gy = iy;
    return true;
}

/* ============================================================
 * T09 — GetTowerCost / CanPlaceTower
 * ============================================================ */

int GetTowerCost(TowerType type) {
    switch (type) {
        case TOWER_BASIC:  return TOWER_COST_BASIC;
        case TOWER_SNIPER: return TOWER_COST_SNIPER;
        case TOWER_SPLASH: return TOWER_COST_SPLASH;
        default:           return 999;
    }
}

/* Sınır, buildable hücre ve altın kontrolü yapar. */
bool CanPlaceTower(Game *g, int gx, int gy) {
    if (gx < 0 || gx >= GRID_COLS || gy < 0 || gy >= GRID_ROWS) return false;
    if (g->grid[gy][gx] != CELL_BUILDABLE) return false;
    if (g->gold < GetTowerCost(g->selectedTowerType)) return false;
    return true;
}

/* ============================================================
 * T06/T68 — InitMap: 32×18 Büyük Harita + Köy
 * ============================================================ */

/* Haritayı başlatır: yol, buildable, rural, village. 120×80 büyük harita. */
void InitMap(Game *g) {
    memset(g->grid, CELL_EMPTY, sizeof(g->grid));

    /* === UZUN S-KIVRIMI YOL (sol kenardan → sağ alt köye) ===
     * Seg  1: satır 10, sütun 0..25     (yatay sağa)
     * Seg  2: sütun 25, satır 10..30    (dikey aşağı)
     * Seg  3: satır 30, sütun 10..25    (yatay sola)
     * Seg  4: sütun 10, satır 30..45    (dikey aşağı)
     * Seg  5: satır 45, sütun 10..50    (yatay sağa)
     * Seg  6: sütun 50, satır 45..55    (dikey aşağı)
     * Seg  7: satır 55, sütun 50..80    (yatay sağa)
     * Seg  8: sütun 80, satır 55..65    (dikey aşağı)
     * Seg  9: satır 65, sütun 80..105   (yatay sağa)
     * Seg 10: sütun 105, satır 65..72   (dikey aşağı → köy)
     */
    for (int c = 0;   c <= 25;  c++) g->grid[10][c]  = CELL_PATH;
    for (int r = 10;  r <= 30;  r++) g->grid[r][25]  = CELL_PATH;
    for (int c = 10;  c <= 25;  c++) g->grid[30][c]  = CELL_PATH;
    for (int r = 30;  r <= 45;  r++) g->grid[r][10]  = CELL_PATH;
    for (int c = 10;  c <= 50;  c++) g->grid[45][c]  = CELL_PATH;
    for (int r = 45;  r <= 55;  r++) g->grid[r][50]  = CELL_PATH;
    for (int c = 50;  c <= 80;  c++) g->grid[55][c]  = CELL_PATH;
    for (int r = 55;  r <= 65;  r++) g->grid[r][80]  = CELL_PATH;
    for (int c = 80;  c <= 105; c++) g->grid[65][c]  = CELL_PATH;
    for (int r = 65;  r <= 72;  r++) g->grid[r][105] = CELL_PATH;

    /* === KÖY (VILLAGE) — yolun sonunda 6×6 blok === */
    for (int r = 70; r <= 77; r++)
        for (int c = 107; c <= 115; c++)
            g->grid[r][c] = CELL_VILLAGE;

    /* === BUILDABLE hücreler: yol etrafı (2 hücre mesafe, 8 yön) === */
    int dr8[] = {-1, 1, 0, 0, -1, -1, 1, 1};
    int dc8[] = {0, 0, -1, 1, -1, 1, -1, 1};
    for (int pass = 0; pass < 2; pass++) { /* 2 geçiş: 2 hücre derinlik */
        for (int r = 0; r < GRID_ROWS; r++) {
            for (int c = 0; c < GRID_COLS; c++) {
                if (g->grid[r][c] != CELL_PATH && g->grid[r][c] != CELL_BUILDABLE) continue;
                for (int d = 0; d < 8; d++) {
                    int nr = r + dr8[d], nc = c + dc8[d];
                    if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS) continue;
                    if (g->grid[nr][nc] == CELL_EMPTY)
                        g->grid[nr][nc] = CELL_BUILDABLE;
                }
            }
        }
    }

    /* === RURAL alanlar: kalan boş hücreler === */
    for (int r = 0; r < GRID_ROWS; r++)
        for (int c = 0; c < GRID_COLS; c++)
            if (g->grid[r][c] == CELL_EMPTY)
                g->grid[r][c] = CELL_RURAL;
}

/* ============================================================
 * T07 — InitWaypoints
 * ============================================================ */

/* Düşmanların izleyeceği dönüş noktalarını piksel koordinatıyla doldurur. */
void InitWaypoints(Game *g) {
    g->waypointCount = 0;
    int wp[][2] = {
        {0,    10},   /* giriş — sol kenar */
        {25,   10},   /* sağa dön */
        {25,   30},   /* aşağı in */
        {10,   30},   /* sola dön */
        {10,   45},   /* aşağı in */
        {50,   45},   /* sağa git */
        {50,   55},   /* aşağı in */
        {80,   55},   /* sağa git */
        {80,   65},   /* aşağı in */
        {105,  65},   /* sağa git */
        {105,  72}    /* köy kapısı — çıkış */
    };
    int n = sizeof(wp) / sizeof(wp[0]);
    for (int i = 0; i < n && i < MAX_WAYPOINTS; i++) {
        g->waypoints[i] = GridToWorld(wp[i][0], wp[i][1]);
        g->waypointCount++;
    }
    /* İlk waypoint spawn noktasını harita dışına kaydır */
    g->waypoints[0].x = -CELL_SIZE * 2.0f;
}

/* ============================================================
 * T62 — InitWaves: 20 Dalga + Boss Dalgaları
 * ============================================================ */

/* Tek tip dalga başlatıcı yardımcı fonksiyon */
static void SetWave1(GameWave *w, EnemyType t, int cnt,
                     float intv, float pre, float hpMult, int bonus, const char *name) {
    memset(w, 0, sizeof(GameWave));
    w->enemyTypes[0] = t; w->enemyCounts[0] = cnt; w->typeCount = 1;
    w->enemyCount = cnt; w->spawnInterval = intv; w->preDelay = pre;
    w->enemyHpMultiplier = hpMult; w->bonusGold = bonus;
    strncpy(w->waveName, name, 31);
}
/* İki tipli dalga başlatıcı */
static void SetWave2(GameWave *w, EnemyType t1, int c1, EnemyType t2, int c2,
                     float intv, float pre, float hpMult, int bonus, const char *name) {
    memset(w, 0, sizeof(GameWave));
    w->enemyTypes[0] = t1; w->enemyCounts[0] = c1;
    w->enemyTypes[1] = t2; w->enemyCounts[1] = c2; w->typeCount = 2;
    w->enemyCount = c1 + c2; w->spawnInterval = intv; w->preDelay = pre;
    w->enemyHpMultiplier = hpMult; w->bonusGold = bonus;
    strncpy(w->waveName, name, 31);
}
/* Üç tipli dalga başlatıcı */
static void SetWave3(GameWave *w, EnemyType t1, int c1, EnemyType t2, int c2,
                     EnemyType t3, int c3, float intv, float pre,
                     float hpMult, int bonus, const char *name) {
    memset(w, 0, sizeof(GameWave));
    w->enemyTypes[0] = t1; w->enemyCounts[0] = c1;
    w->enemyTypes[1] = t2; w->enemyCounts[1] = c2;
    w->enemyTypes[2] = t3; w->enemyCounts[2] = c3; w->typeCount = 3;
    w->enemyCount = c1 + c2 + c3; w->spawnInterval = intv; w->preDelay = pre;
    w->enemyHpMultiplier = hpMult; w->bonusGold = bonus;
    strncpy(w->waveName, name, 31);
}

/* Progresif zorlukla 20 dalganın parametrelerini atar — guidance §13.2 */
void InitWaves(Game *g) {
    GameWave *w = g->waves;
    /* ─── BÖLÜM 1 (Dalga 1-9) ─────────────────────────────── */
    SetWave1(&w[ 0], ENEMY_NORMAL,  8, 1.5f, 2.0f, 1.0f,  20, "Ilk Akin");
    SetWave1(&w[ 1], ENEMY_NORMAL, 10, 1.2f, 2.0f, 1.1f,  25, "Yol Kesfi");
    SetWave1(&w[ 2], ENEMY_FAST,    6, 1.0f, 2.0f, 1.0f,  30, "Hizli Izciler");
    SetWave2(&w[ 3], ENEMY_NORMAL,  8, ENEMY_FAST,   4, 0.8f, 2.0f, 1.2f, 35, "Karisik Gucler");
    SetWave1(&w[ 4], ENEMY_TANK,    4, 2.5f, 2.0f, 1.0f,  40, "Zirh Onculer");
    SetWave1(&w[ 5], ENEMY_NORMAL, 15, 0.8f, 2.0f, 1.3f,  45, "Dalga Dalgasi");
    SetWave1(&w[ 6], ENEMY_FAST,   12, 0.6f, 2.0f, 1.2f,  50, "Hiz Firtinasi");
    SetWave2(&w[ 7], ENEMY_TANK,    6, ENEMY_NORMAL, 6, 1.2f, 2.0f, 1.4f, 60, "Demir Duvar");
    SetWave3(&w[ 8], ENEMY_NORMAL, 10, ENEMY_FAST, 6, ENEMY_TANK, 3, 0.7f, 2.0f, 1.5f, 70, "Son Hazirlik");
    /* ─── BOSS DALGASI 10 ─────────────────────────────────── */
    SetWave2(&w[ 9], ENEMY_BOSS,    1, ENEMY_NORMAL, 4, 1.0f, 3.0f, 5.0f, 150, "* GOLEM KRAL *");
    w[9].isBossWave = true;
    /* ─── BÖLÜM 2 (Dalga 11-19) ──────────────────────────── */
    SetWave1(&w[10], ENEMY_NORMAL, 20, 0.5f, 2.0f, 1.7f,  80, "Sessiz Tehdit");
    SetWave1(&w[11], ENEMY_FAST,   15, 0.4f, 2.0f, 1.5f,  85, "Golge Kosucular");
    SetWave1(&w[12], ENEMY_TANK,    8, 1.5f, 2.0f, 1.6f,  90, "Agir Tugay");
    SetWave3(&w[13], ENEMY_NORMAL,  4, ENEMY_FAST, 4, ENEMY_TANK, 4, 0.6f, 2.0f, 1.8f, 100, "Kaos Dalgasi");
    SetWave2(&w[14], ENEMY_NORMAL, 25, ENEMY_FAST, 10, 0.4f, 2.0f, 2.0f, 110, "Cehennem Gecidi");
    SetWave1(&w[15], ENEMY_TANK,   12, 1.0f, 2.0f, 2.0f, 120, "Tank Filosu");
    SetWave1(&w[16], ENEMY_FAST,   30, 0.3f, 2.0f, 1.8f, 130, "Sel Baskini");
    SetWave3(&w[17], ENEMY_TANK,    8, ENEMY_FAST, 8, ENEMY_NORMAL, 8, 0.5f, 2.0f, 2.2f, 150, "Elit Birlikleri");
    SetWave3(&w[18], ENEMY_NORMAL, 10, ENEMY_FAST, 10, ENEMY_TANK, 10, 0.4f, 2.0f, 2.5f, 180, "Son Savunma");
    /* ─── FINAL BOSS DALGASI 20 ───────────────────────────── */
    SetWave3(&w[19], ENEMY_BOSS,    1, ENEMY_NORMAL, 10, ENEMY_FAST, 5, 0.5f, 3.0f, 8.0f, 300, "* KARANLIK LORD *");
    w[19].isBossWave = true;

    g->totalWaves   = MAX_WAVES;
    g->currentWave  = 0;
    g->waveActive   = false;
}

/* ============================================================
 * InitLevels — 5 Seviyenin Placeholder Verisi
 * ============================================================ */

/* Her seviyenin adı, backstory'si, harita asset yolu,
 * diyalogları ve yıldız eşikleri burada tanımlanır.
 * "[TODO]" ile işaretli alanlar senaryo yazarı tarafından doldurulacak. */
/* ── RULER — Dünya Kurgusu ──────────────────────────────────────
 *
 *  KRALLIK    : Arendalm — yüzyıllık barışın ardından işgale uğruyor.
 *  HÜKÜMDAr  : Sen — adın yok, kararlar senin.
 *  DANIŞMAN   : Başkomutan Aldric — sadık, deneyimli, sert gerçekçi.
 *  DÜŞMAN     : General Krom — Kuzey Bozkırları'nın acımasız savaş lordu.
 *               Krom para için savaşmaz; krallığı çökertmek ister.
 *  [ULAK]     : Anlatıcı — sahneler arası bağlantı, tarihin sesi.
 *
 *  Düşman tipleri:
 *    Normal → Kuzey Akincilari  (hızlı, hafif zırh, kalabalık)
 *    Fast   → Kurt Suvarileri   (atı yok ama pençeli hafif piyade, cok hizli)
 *    Tank   → Demir Muhafizlar  (plaka zırh, ok geçirmez, yavaş ama ölmez)
 * ──────────────────────────────────────────────────────────────── */
void InitLevels(Game *g) {

    /* ── Level 0 : Kral Yolu ─────────────────────────────────── */
    g->levels[0].name        = "Kral Yolu";
    g->levels[0].description = "Krom'un akincilari siniri gecti. Kral yolu tehlike altinda.";
    g->levels[0].enemyLore   = "Kuzey Akincilari: Krom'un ucucu kuvvetleri. "
                               "Tek tek zayif, ama durdurulamazlarsa hic bitmezler.";
    g->levels[0].mapBgAsset  = "assets/sprites/maps/map_01_kings_road.png"; /* TODO */
    g->levels[0].nodeX       = 160;  g->levels[0].nodeY = 420;
    g->levels[0].star3Lives  = 17;   g->levels[0].star2Lives = 10;
    g->levels[0].unlocked    = true;
    g->levels[0].briefingCount = 3;
    g->levels[0].briefing[0] = (DialogueLine){
        "[Ulak]", "assets/sprites/portraits/narrator.png",
        "Kuzey Bozkirlari'ndan General Krom'un ordusu siniri gecti.\n"
        "Ilk dalgasi Kral Yolu'nda goruldu.",
        LIGHTGRAY
    };
    g->levels[0].briefing[1] = (DialogueLine){
        "General Krom", "assets/sprites/portraits/krom.png",
        "Hukumdar! Tahtini bana birak. Kani dokmeden teslim ol.\n"
        "Son sans bu.",
        (Color){200, 70, 50, 255}
    };
    g->levels[0].briefing[2] = (DialogueLine){
        "Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
        "Kral Yolu'nun iki yanina kule hatlarimizi kuralim.\n"
        "Akincilari yolda durdurmalisiniz — sahile ulasirlarsa koy yanar.",
        (Color){220, 190, 80, 255}
    };
    g->levels[0].completionCount = 2;
    g->levels[0].completion[0] = (DialogueLine){
        "Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
        "Kral Yolu tutuldu. Iyi savastiniz.",
        (Color){220, 190, 80, 255}
    };
    g->levels[0].completion[1] = (DialogueLine){
        "General Krom", "assets/sprites/portraits/krom.png",
        "Ilginc... Sanildigi kadar zayif degilsiniz.\n"
        "Geri cekiliyorum — simdilik.",
        (Color){200, 70, 50, 255}
    };

    /* ── Level 1 : Orman Siniri ──────────────────────────────── */
    g->levels[1].name        = "Orman Siniri";
    g->levels[1].description = "Kurt Suvarileri orman yollarindan kacak geciyor.";
    g->levels[1].enemyLore   = "Kurt Suvarileri: Krom'un ormanda yetismis hafif birlikleri. "
                               "Inanilmaz hizlilar — durup savunma kurmayi bilmiyorlar.";
    g->levels[1].mapBgAsset  = "assets/sprites/maps/map_02_forest_border.png"; /* TODO */
    g->levels[1].nodeX       = 380;  g->levels[1].nodeY = 330;
    g->levels[1].star3Lives  = 16;   g->levels[1].star2Lives = 9;
    g->levels[1].unlocked    = false;
    g->levels[1].briefingCount = 3;
    g->levels[1].briefing[0] = (DialogueLine){
        "[Ulak]", "assets/sprites/portraits/narrator.png",
        "Krom, kuzey ormanlari boyunca ikinci bir hat actirdi.\n"
        "Kurt Suvarileri hiz kesmeden ilerlemeye devam ediyor.",
        LIGHTGRAY
    };
    g->levels[1].briefing[1] = (DialogueLine){
        "Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
        "Bu birlikler cok hizli — normal kulelerin ates hizi yetmeyebilir.\n"
        "Sniper kulelerinizi mesafeli konumlara kurun.",
        (Color){220, 190, 80, 255}
    };
    g->levels[1].briefing[2] = (DialogueLine){
        "General Krom", "assets/sprites/portraits/krom.png",
        "Ormanim benim evirim. Sizi agaclarin arasinda buldugumda\n"
        "kacacak yer bulamazsiniz.",
        (Color){200, 70, 50, 255}
    };
    g->levels[1].completionCount = 2;
    g->levels[1].completion[0] = (DialogueLine){
        "Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
        "Orman siniri tutuldu. Krom'un hiz avantaji islemiyor.",
        (Color){220, 190, 80, 255}
    };
    g->levels[1].completion[1] = (DialogueLine){
        "General Krom", "assets/sprites/portraits/krom.png",
        "Savunmanizi hafife aldim. Bu hatami telafi edecegim.\n"
        "Demir Muhafizlarim sizi ezecek.",
        (Color){200, 70, 50, 255}
    };

    /* ── Level 2 : Tas Kopru ─────────────────────────────────── */
    g->levels[2].name        = "Tas Kopru";
    g->levels[2].description = "Demir Muhafizlar tek gecis noktasi olan kopruye yuruyuyor.";
    g->levels[2].enemyLore   = "Demir Muhafizlar: Krom'un zirh ustalarinin doktugu plaka zirh. "
                               "Oklar sekmez. Yalnizca patlama hasari onu durdurabilir.";
    g->levels[2].mapBgAsset  = "assets/sprites/maps/map_03_stone_bridge.png"; /* TODO */
    g->levels[2].nodeX       = 600;  g->levels[2].nodeY = 260;
    g->levels[2].star3Lives  = 15;   g->levels[2].star2Lives = 8;
    g->levels[2].unlocked    = false;
    g->levels[2].briefingCount = 3;
    g->levels[2].briefing[0] = (DialogueLine){
        "[Ulak]", "assets/sprites/portraits/narrator.png",
        "Nehrin tek gecis noktasi Tas Kopru. Krom buradan gecerse\n"
        "baskente giden yol acik kalir.",
        LIGHTGRAY
    };
    g->levels[2].briefing[1] = (DialogueLine){
        "General Krom", "assets/sprites/portraits/krom.png",
        "Demir Muhafizlarim bu kopruyu gecmeyi haketti.\n"
        "Oklarin, kizginligim karsisinda is yapmayacak.",
        (Color){200, 70, 50, 255}
    };
    g->levels[2].briefing[2] = (DialogueLine){
        "Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
        "Normal oklar zirha karsi etkisiz. Splash kuleleri on planda olsun —\n"
        "patlama hasari onlarin tek zafiyeti.",
        (Color){220, 190, 80, 255}
    };
    g->levels[2].completionCount = 2;
    g->levels[2].completion[0] = (DialogueLine){
        "[Ulak]", "assets/sprites/portraits/narrator.png",
        "Tas Kopru tutuldu. Demir Muhafizlar nehri gecemedi.",
        LIGHTGRAY
    };
    g->levels[2].completion[1] = (DialogueLine){
        "General Krom", "assets/sprites/portraits/krom.png",
        "... Tamam. Siz iyi savasiyorsunuz.\n"
        "Ama savasin sonu gelmedi. Ordusu'nun tamami simdi yolda.",
        (Color){200, 70, 50, 255}
    };

    /* ── Level 3 : Baskent Kapisi ────────────────────────────── */
    g->levels[3].name        = "Baskent Kapisi";
    g->levels[3].description = "Krom'un tam ordusu baskente yuruyuyor. Her tur dusman var.";
    g->levels[3].enemyLore   = "Krom artik ayrilmis kuvvetlerini birlestiirdi. "
                               "Akinci, Kurt Suvarisi ve Demir Muhafiz omuz omuza geliyor.";
    g->levels[3].mapBgAsset  = "assets/sprites/maps/map_04_capital_gate.png"; /* TODO */
    g->levels[3].nodeX       = 820;  g->levels[3].nodeY = 370;
    g->levels[3].star3Lives  = 14;   g->levels[3].star2Lives = 7;
    g->levels[3].unlocked    = false;
    g->levels[3].briefingCount = 3;
    g->levels[3].briefing[0] = (DialogueLine){
        "[Ulak]", "assets/sprites/portraits/narrator.png",
        "Krom, tum kuvvetlerini tek bir hatta topladi.\n"
        "Baskentin dis kapisi ilk hedef.",
        LIGHTGRAY
    };
    g->levels[3].briefing[1] = (DialogueLine){
        "General Krom", "assets/sprites/portraits/krom.png",
        "Artik oyun bitti. Tum birlikleri kisimlara ayirmak hataymis.\n"
        "Bu sefer ordum bir dalgada, tam gucte geliyor.",
        (Color){200, 70, 50, 255}
    };
    g->levels[3].briefing[2] = (DialogueLine){
        "Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
        "Karmisik dalga — her tur dusmana hazir olun.\n"
        "Kule cesidinizi dengede tutun. Bu savasi kaybedersek baskent duser.",
        (Color){220, 190, 80, 255}
    };
    g->levels[3].completionCount = 2;
    g->levels[3].completion[0] = (DialogueLine){
        "Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
        "Kapi tutuldu! Krom'un buyuk saldirisi kırıldı.\n"
        "Ama ordunun son kalintisi hala sarayda.",
        (Color){220, 190, 80, 255}
    };
    g->levels[3].completion[1] = (DialogueLine){
        "General Krom", "assets/sprites/portraits/krom.png",
        "Ben... bu kadar yaklasmistim.\n"
        "Son kozum var. Sizi taht odasinda karsilayacagim.",
        (Color){200, 70, 50, 255}
    };

    /* ── Level 4 : Taht Odasi (Son Seviye) ───────────────────── */
    g->levels[4].name        = "Taht Odasi";
    g->levels[4].description = "Krom bizzat liderlik ediyor. Son savunma — tahti koru.";
    g->levels[4].enemyLore   = "Krom'un seckin muhafizlari ve kuzey ordusunun tam gucu. "
                               "Her tip, tam kapasite, durmadan geliyor.";
    g->levels[4].mapBgAsset  = "assets/sprites/maps/map_05_throne_room.png"; /* TODO */
    g->levels[4].nodeX       = 1040; g->levels[4].nodeY = 200;
    g->levels[4].star3Lives  = 13;   g->levels[4].star2Lives = 6;
    g->levels[4].unlocked    = false;
    g->levels[4].briefingCount = 4;
    g->levels[4].briefing[0] = (DialogueLine){
        "[Ulak]", "assets/sprites/portraits/narrator.png",
        "Krom artik saray koridorlarindaki son barikata kadar ilerledi.\n"
        "Bu tahti, kralligi ve her seyi belirleyecek son savas.",
        LIGHTGRAY
    };
    g->levels[4].briefing[1] = (DialogueLine){
        "General Krom", "assets/sprites/portraits/krom.png",
        "Hukumdar. Ordumun tamami burada. Ben buradayim.\n"
        "Artik kacinecak yer yok. Tahtini bana ver ya da onu harabelerde bul.",
        (Color){200, 70, 50, 255}
    };
    g->levels[4].briefing[2] = (DialogueLine){
        "Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
        "Bu son savas. Tum savunma hatlarimizi tam kapasite kuralim.\n"
        "Burada yenilirsek geri donus olmaz.",
        (Color){220, 190, 80, 255}
    };
    g->levels[4].briefing[3] = (DialogueLine){
        "[Ulak]", "assets/sprites/portraits/narrator.png",
        "Ruler — kralligi kurtaran tek kisi sensin.\n"
        "Sonu gelsin.",
        LIGHTGRAY
    };
    g->levels[4].completionCount = 3;
    g->levels[4].completion[0] = (DialogueLine){
        "General Krom", "assets/sprites/portraits/krom.png",
        "Bu... imkansiz. Ordumun tamamini yendiniz.\n"
        "Asla... asla boyle bir hukumdar gormedim.",
        (Color){200, 70, 50, 255}
    };
    g->levels[4].completion[1] = (DialogueLine){
        "Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
        "TEBRIKLER! General Krom'un ordusu tamamen dagitildi!\n"
        "Arendalm Kralligi kurtuldu!",
        (Color){220, 190, 80, 255}
    };
    g->levels[4].completion[2] = (DialogueLine){
        "[Ulak]", "assets/sprites/portraits/narrator.png",
        "Ve halk Ruler'in adindan baska bir sey konusmaz oldu.\n"
        "Krom bir daha gelmedi.",
        LIGHTGRAY
    };
}

/* ============================================================
 * CalcLevelStars — Kalan cana göre yıldız hesapla
 * ============================================================ */
int CalcLevelStars(Game *g) {
    LevelData *lv = &g->levels[g->currentLevel];
    if (g->lives >= lv->star3Lives) return 3;
    if (g->lives >= lv->star2Lives) return 2;
    if (g->lives > 0)               return 1;
    return 0;
}

/* ============================================================
 * InitGame / RestartGame / RestartCurrentLevel
 * ============================================================ */

/* ============================================================
 * T64 — AudioManager
 * ============================================================ */

/* Ses dosyası yol tablosu — assets/ dizini dolunca kullanıma hazır */
static const char *SFX_PATHS[SFX_COUNT] = {
    "assets/sfx/arrow_shoot.wav",    /* SFX_SHOOT_BASIC   */
    "assets/sfx/magic_bolt.wav",     /* SFX_SHOOT_SNIPER  */
    "assets/sfx/cannon_fire.wav",    /* SFX_SHOOT_SPLASH  */
    "assets/sfx/enemy_death.wav",    /* SFX_ENEMY_DEATH   */
    "assets/sfx/troll_death.wav",    /* SFX_TANK_DEATH    */
    "assets/sfx/tower_build.wav",    /* SFX_TOWER_PLACE   */
    "assets/sfx/upgrade.wav",        /* SFX_TOWER_UPGRADE */
    "assets/sfx/coins.wav",          /* SFX_TOWER_SELL    */
    "assets/sfx/drum_roll.wav",      /* SFX_WAVE_START    */
    "assets/sfx/life_lost.wav",      /* SFX_LIVE_LOST     */
    "assets/sfx/button_click.wav",   /* SFX_BUTTON_CLICK  */
    "assets/sfx/fanfare.wav",        /* SFX_VICTORY       */
    "assets/sfx/defeat.wav",         /* SFX_DEFEAT        */
    "assets/sfx/level_up.wav",       /* SFX_LEVEL_UP      */
};

/* Ses cihazını başlatır, mevcut dosyaları yükler; yoksa sessiz çalışır. */
void InitAudioManager(AudioManager *am) {
    memset(am, 0, sizeof(AudioManager));
    am->masterVolume = 1.0f;
    InitAudioDevice();
    am->deviceReady = IsAudioDeviceReady();
    if (!am->deviceReady) return;
    for (int i = 0; i < SFX_COUNT; i++) {
        if (FileExists(SFX_PATHS[i])) {
            am->sounds[i] = LoadSound(SFX_PATHS[i]);
            am->loaded[i] = true;
        }
    }
}

/* Ses efekti çalar; dosya yüklü değilse veya cihaz hazır değilse sessiz geçer. */
void PlaySFX(AudioManager *am, SFXType t) {
    if (!am->deviceReady || !am->loaded[t]) return;
    SetSoundVolume(am->sounds[t], am->masterVolume);
    PlaySound(am->sounds[t]);
}

/* Sesleri serbest bırakır ve ses cihazını kapatır. */
void CloseAudioManager(AudioManager *am) {
    if (!am->deviceReady) return;
    for (int i = 0; i < SFX_COUNT; i++)
        if (am->loaded[i]) UnloadSound(am->sounds[i]);
    CloseAudioDevice();
    am->deviceReady = false;
}

/* Oyunu tamamen sıfırlar ve başlangıç durumuna getirir. */
void InitGame(Game *g) {
    memset(g, 0, sizeof(Game));
    g->gold                  = STARTING_GOLD;
    g->lives                 = STARTING_LIVES;
    g->gameSpeed             = 1.0f;
    g->state                 = STATE_MENU;
    g->selectedTowerType     = TOWER_BASIC;
    g->showGrid              = true;
    g->contextMenuTowerIdx   = -1;
    g->loading.duration      = 1.5f;
    InitMap(g);
    InitWaypoints(g);
    InitWaves(g);
    InitLevels(g);
    InitHomeCity(&g->homeCity);
    InitSiegeMechanics(&g->siege);
    g->selectedBuilding = BUILDING_BARRACKS;
    g->selectedBuildingType = -1;
    /* T69 — Kamera: yakın görünüm, yolun başlangıcına merkezle */
    g->camera.zoom = 1.8f;
    g->camera.cam.zoom = 1.8f;
    g->camera.cam.target = (Vector2){0, 10 * CELL_SIZE};
    g->camera.cam.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    g->camera.heroFollow = false;
    InitUI(SCREEN_WIDTH);
    /* Dost birim dizisini sifirla */
    memset(g->friendlyUnits, 0, sizeof(g->friendlyUnits));
    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++)
        g->friendlyUnits[i].engagedEnemyIdx = -1;
    g->pendingPlacementCount = 0;
    g->pendingPlacementType  = FUNIT_ARCHER;
}

/* Sadece gameplay alanlarını sıfırlar — kampanya verisi ve level'lar korunur. */
static void ResetGameplay(Game *g) {
    int          savedLevel = g->currentLevel;
    LevelData    savedLevels[MAX_LEVELS];
    memcpy(savedLevels, g->levels, sizeof(savedLevels));

    memset(g->enemies,     0, sizeof(g->enemies));
    memset(g->towers,      0, sizeof(g->towers));
    memset(g->projectiles, 0, sizeof(g->projectiles));
    memset(g->particles,   0, sizeof(g->particles));
    g->gold              = STARTING_GOLD;
    g->lives             = STARTING_LIVES;
    g->score             = 0;
    g->enemiesKilled     = 0;
    g->waveActive        = false;
    g->currentWave       = 0;
    g->gameSpeed         = 1.0f;
    g->selectedTowerType = TOWER_BASIC;
    g->showGrid          = true;
    g->contextMenuOpen   = false;
    g->contextMenuTowerIdx = -1;
    g->buildingCount     = 0;
    g->prepTimer         = 0.0f;
    memset(g->buildings, 0, sizeof(g->buildings));
    InitMap(g);
    InitWaypoints(g);
    InitWaves(g);
    InitSiegeMechanics(&g->siege);

    g->currentLevel = savedLevel;
    memcpy(g->levels, savedLevels, sizeof(savedLevels));

    /* Dost birimleri sifirla, hero birimini yeniden olustur */
    memset(g->friendlyUnits, 0, sizeof(g->friendlyUnits));
    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++)
        g->friendlyUnits[i].engagedEnemyIdx = -1;
    g->pendingPlacementCount = 0;
    g->pendingPlacementType  = FUNIT_ARCHER;
    if (g->hero.heroClass != HERO_CLASS_NONE) SpawnHeroUnit(g);
}

/* Loading screen üzerinden mevcut level'ı yeniden başlatır. */
void RestartCurrentLevel(Game *g) {
    g->loading.progress = 0.0f;
    g->loading.timer    = 0.0f;
    g->loading.duration = 1.0f;
    g->loading.nextState = STATE_PLAYING;
    g->loading.tipText   = "Tekrar deniyorsunuz — bu sefer basaracaksiniz!";
    g->state = STATE_LOADING;
}

void RestartGame(Game *g) {
    InitGame(g);
    g->state = STATE_WORLD_MAP;
}

/* ============================================================
 * DOST BIRIM SISTEMI
 * ============================================================ */

/* Tipe gore baslangic istatistiklerini atar */
void InitFriendlyUnit(FriendlyUnit *fu, FUnitType type, Vector2 pos) {
    memset(fu, 0, sizeof(FriendlyUnit));
    fu->type             = type;
    fu->order            = FUNIT_HOLD;
    fu->position         = pos;
    fu->active           = true;
    fu->engagedEnemyIdx  = -1;
    switch (type) {
        case FUNIT_ARCHER:
            fu->maxHp=90;  fu->hp=90;  fu->atk=18; fu->attackRange=120; fu->attackSpeed=1.5f; break;
        case FUNIT_WARRIOR:
            fu->maxHp=220; fu->hp=220; fu->atk=28; fu->attackRange=36;  fu->attackSpeed=0.9f; break;
        case FUNIT_MAGE:
            fu->maxHp=70;  fu->hp=70;  fu->atk=40; fu->attackRange=110; fu->attackSpeed=0.5f; break;
        case FUNIT_KNIGHT:
            fu->maxHp=320; fu->hp=320; fu->atk=22; fu->attackRange=44;  fu->attackSpeed=1.0f; break;
        case FUNIT_HERO:
        default:
            fu->maxHp=250; fu->hp=250; fu->atk=30; fu->attackRange=55;  fu->attackSpeed=1.2f; break;
    }
}

/* Hero sinifina gore path sonunda ozel birim olusturur */
void SpawnHeroUnit(Game *g) {
    /* Son waypointten 1 tile oncesi */
    int wc = g->waypointCount;
    if (wc < 2) return;
    Vector2 pos = g->waypoints[wc - 2]; /* son waypoint oncesi */

    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
        if (g->friendlyUnits[i].active) continue;
        InitFriendlyUnit(&g->friendlyUnits[i], FUNIT_HERO, pos);
        /* Hero sinifi bonuslari */
        if (g->hero.heroClass == HERO_WARRIOR) {
            g->friendlyUnits[i].maxHp = 400; g->friendlyUnits[i].hp = 400;
            g->friendlyUnits[i].atk   = 42;  g->friendlyUnits[i].attackRange = 44;
        } else if (g->hero.heroClass == HERO_MAGE) {
            g->friendlyUnits[i].maxHp = 180; g->friendlyUnits[i].hp = 180;
            g->friendlyUnits[i].atk   = 60;  g->friendlyUnits[i].attackRange = 120;
        } else { /* ARCHER */
            g->friendlyUnits[i].maxHp = 220; g->friendlyUnits[i].hp = 220;
            g->friendlyUnits[i].atk   = 30;  g->friendlyUnits[i].attackRange = 100;
        }
        break;
    }
}

/* Dost birimleri guncelle: hold/attack AI + engaged takibi */
void UpdateFriendlyUnits(Game *g, float dt) {
    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
        FriendlyUnit *fu = &g->friendlyUnits[i];
        if (!fu->active) continue;

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
        int   bestIdx  = -1;
        float bestDist = fu->attackRange;
        for (int j = 0; j < MAX_ENEMIES; j++) {
            Enemy *e = &g->enemies[j];
            if (!e->active || e->isFlying) continue;
            float d = Vec2Distance(fu->position, e->position);
            if (d < bestDist) { bestDist = d; bestIdx = j; }
        }

        /* ATTACK modu: hedefe dog_ru yuru */
        if (fu->order == FUNIT_ATTACK && bestIdx < 0) {
            /* Menzil disindaki en yakin dusmana yuru */
            float walkDist = 9999;
            int   walkIdx  = -1;
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (!g->enemies[j].active || g->enemies[j].isFlying) continue;
                float d = Vec2Distance(fu->position, g->enemies[j].position);
                if (d < walkDist) { walkDist = d; walkIdx = j; }
            }
            if (walkIdx >= 0) {
                Vector2 dir = Vec2Normalize(Vec2Subtract(g->enemies[walkIdx].position, fu->position));
                fu->position.x += dir.x * 90.0f * dt;
                fu->position.y += dir.y * 90.0f * dt;
            }
        }

        /* Saldiri */
        if (fu->attackCooldown > 0.0f) fu->attackCooldown -= dt;
        if (bestIdx >= 0 && fu->attackCooldown <= 0.0f) {
            fu->attackCooldown = 1.0f / fu->attackSpeed;
            g->enemies[bestIdx].currentHp -= fu->atk;
            if (g->enemies[bestIdx].currentHp <= 0.0f) {
                g->enemies[bestIdx].active = false;
                /* Engaged iliskisini temizle */
                if (fu->engagedEnemyIdx == bestIdx) fu->engagedEnemyIdx = -1;
                g->gold  += KILL_REWARD;
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
        (Color){50,200,255,230},  /* ARCHER  — mavi */
        (Color){60,180,60,230},   /* WARRIOR — yesil */
        (Color){200,80,255,230},  /* MAGE    — mor */
        (Color){240,160,30,230},  /* KNIGHT  — turuncu */
        (Color){255,255,80,230},  /* HERO    — sari */
    };
    const char *labels[FUNIT_TYPE_COUNT] = {"Ok","Sv","By","Sv","Hr"};

    /* Bekleyen yerlestirme gostergesi */
    if (g->pendingPlacementCount > 0) {
        Vector2 mp = GetMousePosition();
        DrawCircleLines((int)mp.x, (int)mp.y, 14, typeColors[g->pendingPlacementType]);
        DrawText(TextFormat("Yerlestirilecek: %d", g->pendingPlacementCount),
                 (int)mp.x + 16, (int)mp.y - 8, 13, WHITE);
    }

    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
        FriendlyUnit *fu = &g->friendlyUnits[i];
        if (!fu->active) continue;
        Color col = typeColors[fu->type];

        /* Gövde */
        DrawCircle((int)fu->position.x, (int)fu->position.y, 12, col);
        DrawCircleLines((int)fu->position.x, (int)fu->position.y, 12,
                        fu->order == FUNIT_HOLD ? WHITE : ORANGE);

        /* Tip etiketi */
        DrawText(labels[fu->type],
                 (int)fu->position.x - 6, (int)fu->position.y - 6, 10, BLACK);

        /* HP barı */
        float hr = fu->hp / fu->maxHp;
        DrawRectangle((int)fu->position.x - 14, (int)fu->position.y - 20, 28, 4,
                      (Color){60,0,0,180});
        DrawRectangle((int)fu->position.x - 14, (int)fu->position.y - 20,
                      (int)(28 * hr), 4, (Color){30,200,30,220});

        /* Hold/Attack durum ikonu */
        if (fu->order == FUNIT_HOLD)
            DrawText("H", (int)fu->position.x + 8, (int)fu->position.y - 22, 10,
                     (Color){200,200,255,200});
        else
            DrawText("A", (int)fu->position.x + 8, (int)fu->position.y - 22, 10,
                     (Color){255,160,30,220});

        /* T65 — Seçili birim beyaz halka */
        if (fu->selected)
            DrawCircleLines((int)fu->position.x, (int)fu->position.y, 16, WHITE);
    }
}

/* ============================================================
 * T11 — SpawnEnemy
 * ============================================================ */

/* Boş bir düşman yuvası bulur, belirtilen tipte + HP çarpanıyla düşman oluşturur. */
void SpawnEnemy(Game *g, EnemyType type, float hpMult) {
    if (hpMult <= 0.0f) hpMult = 1.0f;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g->enemies[i].active) continue;
        Enemy *e = &g->enemies[i];
        memset(e, 0, sizeof(Enemy));
        e->type             = type;
        e->currentWaypoint  = 1; /* Waypoints[0] spawn noktası, hedef [1]'den başlar */
        e->position         = g->waypoints[0];
        e->slowFactor           = 1.0f;
        e->active               = true;
        e->isFlying             = false;
        e->engagedFriendlyIdx   = -1;
        switch (type) {
            case ENEMY_NORMAL:
                e->maxHp   = ENEMY_BASE_HP * 1.0f * hpMult;
                e->speed   = ENEMY_BASE_SPEED * 1.0f;
                e->radius  = 10.0f;
                e->color   = RED;
                break;
            case ENEMY_FAST:
                e->maxHp   = ENEMY_BASE_HP * 0.6f * hpMult;
                e->speed   = ENEMY_BASE_SPEED * 1.8f;
                e->radius  = 8.0f;
                e->color   = ORANGE;
                break;
            case ENEMY_TANK:
                e->maxHp   = ENEMY_BASE_HP * 3.0f * hpMult;
                e->speed   = ENEMY_BASE_SPEED * 0.6f;
                e->radius  = 14.0f;
                e->color   = DARKPURPLE;
                break;
            case ENEMY_BOSS:
                e->maxHp   = ENEMY_BASE_HP * 20.0f * hpMult;
                e->speed   = ENEMY_BASE_SPEED * 0.4f;
                e->radius  = 22.0f;
                e->color   = MAROON;
                e->isBoss  = true;
                e->bossPhase = 1;
                e->abilityTimer = 3.0f;
                e->isCasting = false;
                e->castTimer = 0.0f;
                e->targetAbilityPos = (Vector2){0,0};
                break;
            default: break;
        }
        e->currentHp = e->maxHp;
        return;
    }
}

/* ============================================================
 * T20 — CreateProjectile
 * ============================================================ */

/* Boş mermi yuvası bulur ve verilen parameterlerle başlatır. */
void CreateProjectile(Game *g, Vector2 origin, int targetIdx, float damage, float splash, TowerType src) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (g->projectiles[i].active) continue;
        Projectile *p = &g->projectiles[i];
        p->position    = origin;
        p->damage      = damage;
        p->splashRadius= splash;
        p->targetIndex = targetIdx;
        p->sourceType  = src;
        p->active      = true;
        switch (src) {
            case TOWER_BASIC:  p->speed = 300.0f; p->color = SKYBLUE;   break;
            case TOWER_SNIPER: p->speed = 500.0f; p->color = LIME;      break;
            case TOWER_SPLASH: p->speed = 200.0f; p->color = ORANGE;    break;
            default:           p->speed = 300.0f; p->color = WHITE;     break;
        }
        return;
    }
}

/* ============================================================
 * T23 — SpawnParticles
 * ============================================================ */

/* Belirtilen pozisyonda rastgele yönlerde parçacıklar oluşturur. */
void SpawnParticles(Game *g, Vector2 pos, Color color, int count, float speed, float lifetime) {
    int spawned = 0;
    for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
        if (g->particles[i].active) continue;
        Particle *p = &g->particles[i];
        float angle = (float)(GetRandomValue(0, 359)) * DEG2RAD;
        float spd   = speed * (0.5f + (float)GetRandomValue(0, 100) / 100.0f);
        p->position    = pos;
        p->velocity    = (Vector2){cosf(angle) * spd, sinf(angle) * spd};
        p->lifetime    = lifetime;
        p->maxLifetime = lifetime;
        p->color       = color;
        p->size        = 4.0f + (float)GetRandomValue(0, 4);
        p->active      = true;
        spawned++;
    }
}

/* ============================================================
 * T16 — FindNearestEnemy
 * ============================================================ */

/* Verilen pozisyon ve menzil içindeki en yakın aktif düşmanın indeksini döner.
 * Hiçbiri yoksa -1 döner. */
int FindNearestEnemy(Game *g, Vector2 pos, float range) {
    float minDist  = range + 1.0f;
    int   bestIdx  = -1;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g->enemies[i].active) continue;
        /* T70 — Fogdaki düşmanlara vurulamaz */
        int er = (int)(g->enemies[i].position.y) / CELL_SIZE;
        int ec = (int)(g->enemies[i].position.x) / CELL_SIZE;
        if (er >= 0 && er < GRID_ROWS && ec >= 0 && ec < GRID_COLS) {
            if (!g->fogVisible[er][ec]) continue;
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
void UpdateEnemies(Game *g, float dt) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g->enemies[i];
        if (!e->active) continue;

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
                                if (g->projectiles[pidx].active) continue;
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
                                for (int j=0; j<TRAIL_LENGTH; j++) p->trailPositions[j] = p->position;
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
                            if (!g->towers[tidx].active) continue;
                            if (Vec2Distance(g->towers[tidx].position, e->targetAbilityPos) < 100.0f) {
                                g->towers[tidx].fireCooldown += 3.0f; /* 3 sn stun */
                            }
                        }
                        /* Dost birimlere hasar */
                        for (int fidx = 0; fidx < MAX_FRIENDLY_UNITS; fidx++) {
                            if (!g->friendlyUnits[fidx].active) continue;
                            if (Vec2Distance(g->friendlyUnits[fidx].position, e->targetAbilityPos) < 100.0f) {
                                g->friendlyUnits[fidx].hp -= 50.0f;
                            }
                        }
                        /* Hero'ya hasar (eğer sahadaysa) */
                        if (g->hero.alive && Vec2Distance(g->hero.position, e->targetAbilityPos) < 100.0f) {
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
                        for(int tidx = 0; tidx < MAX_TOWERS; tidx++) {
                            if (g->towers[tidx].active) activeTowers[tCount++] = tidx;
                        }
                        if (tCount > 0) {
                            e->targetAbilityPos = g->towers[activeTowers[GetRandomValue(0, tCount-1)]].position;
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
            if (g->lives < 0) g->lives = 0;
            e->active = false;
            PlaySFX(&g->audio, SFX_LIVE_LOST);
            continue;
        }

        /* Ucmayan dusman: dost birim bloklama kontrolu */
        if (!e->isFlying) {
            /* Mevcut engaged birim hala aktif mi? */
            if (e->engagedFriendlyIdx >= 0 &&
                !g->friendlyUnits[e->engagedFriendlyIdx].active) {
                e->engagedFriendlyIdx = -1;
            }

            if (e->engagedFriendlyIdx >= 0) {
                /* Savasiyor: dur ve dost birime saldır */
                FriendlyUnit *fu = &g->friendlyUnits[e->engagedFriendlyIdx];
                fu->hp -= e->speed * 0.04f * dt; /* Hasar: hiza orantili */
                continue; /* Hareket etme */
            }

            /* Blok aramayi sadece enemy kendi waypoint'ine gidiyorken yap */
            /* Yakin dost birim bul — 1:1 kurali */
            int blockSlot = -1;
            float blockDist = FUNIT_BLOCK_DIST;
            for (int j = 0; j < MAX_FRIENDLY_UNITS; j++) {
                FriendlyUnit *fu = &g->friendlyUnits[j];
                if (!fu->active || fu->hp <= 0.0f) continue;
                if (fu->engagedEnemyIdx != -1) continue; /* Zaten mesgul */
                float d = Vec2Distance(e->position, fu->position);
                if (d < blockDist) { blockDist = d; blockSlot = j; }
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
            if (e->slowTimer < 0.0f) e->slowTimer = 0.0f;
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
void UpdateTowers(Game *g, float dt) {
    for (int i = 0; i < MAX_TOWERS; i++) {
        Tower *t = &g->towers[i];
        if (!t->active) continue;

        /* B04: flash zamanlayıcısını azalt */
        if (t->flashTimer > 0.0f) {
            t->flashTimer -= dt;
            if (t->flashTimer < 0.0f) t->flashTimer = 0.0f;
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
            CreateProjectile(g, t->position, t->targetEnemyIndex, t->damage, t->splashRadius, t->type);
            t->fireCooldown = 1.0f / t->fireRate;
            t->flashTimer   = 0.1f;  /* B04 */
        }
    }
}

/* ============================================================
 * T21 — UpdateProjectiles
 * ============================================================ */

/* Mermileri hedeflerine doğru hareket ettirir, isabet ve splash hasarı uygular. */
void UpdateProjectiles(Game *g, float dt) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &g->projectiles[i];
        if (!p->active) continue;

        Enemy *target = NULL;
        if (!p->isEnemyProjectile && p->targetIndex >= 0 && p->targetIndex < MAX_ENEMIES)
            target = &g->enemies[p->targetIndex];

        /* T58 — kuyruk: mevcut pozisyonu kaydet */
        p->trailPositions[p->trailIndex] = p->position;
        p->trailIndex = (p->trailIndex + 1) % TRAIL_LENGTH;

        if (target && target->active) {
            /* Hedefe yönlen */
            p->velocity = Vec2Scale(Vec2Normalize(Vec2Subtract(target->position, p->position)), p->speed);
        }
        p->position = Vec2Add(p->position, Vec2Scale(p->velocity, dt));

        /* Ekran dışına çıktıysa deaktive et */
        if (p->position.x < 0 || p->position.x > SCREEN_WIDTH ||
            p->position.y < 0 || p->position.y > SCREEN_HEIGHT) {
            p->active = false;
            continue;
        }

        /* Düşman Mermisi Kontrolü (Boss Phase 1 vb.) */
        if (p->isEnemyProjectile) {
            bool hit = false;
            /* Kulelere isabet kontrolü */
            for (int t = 0; t < MAX_TOWERS; t++) {
                if (!g->towers[t].active) continue;
                if (Vec2Distance(p->position, g->towers[t].position) < 20.0f) {
                    g->towers[t].fireCooldown += 2.0f; /* 2 sn stun */
                    hit = true;
                }
            }
            /* Dost birimlere isabet kontrolü */
            for (int f = 0; f < MAX_FRIENDLY_UNITS; f++) {
                if (!g->friendlyUnits[f].active) continue;
                if (Vec2Distance(p->position, g->friendlyUnits[f].position) < 15.0f) {
                    g->friendlyUnits[f].hp -= p->damage;
                    hit = true;
                }
            }
            if (g->hero.alive && Vec2Distance(p->position, g->hero.position) < 15.0f) {
                g->hero.stats.hp -= p->damage;
                hit = true;
            }
            if (hit) {
                SpawnParticles(g, p->position, p->color, 5, 80.0f, 0.2f);
                p->active = false;
            }
            continue;
        }

        /* İsabet kontrolü */
        if (target && target->active &&
            Vec2Distance(p->position, target->position) < target->radius + 4.0f) {

            if (p->splashRadius > 0.0f) {
                /* Splash hasar: mesafeye göre azalan hasar */
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    if (!g->enemies[j].active) continue;
                    float dist = Vec2Distance(p->position, g->enemies[j].position);
                    if (dist < p->splashRadius) {
                        float dmg = p->damage * (1.0f - dist / p->splashRadius);
                        g->enemies[j].currentHp -= dmg;
                        SpawnFloatingText(g, g->enemies[j].position, dmg, false, false); /* T57 */
                    }
                }
                SpawnParticles(g, p->position, ORANGE, 12, 120.0f, 0.5f);
                TriggerScreenShake(g, 3.0f); /* T56 — splash patlama sarsıntısı */
                /* Splash'tan ölen tüm düşmanları işle */
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    Enemy *e = &g->enemies[j];
                    if (!e->active || e->currentHp > 0.0f) continue;
                    SpawnParticles(g, e->position, e->color, 10, 100.0f, 0.4f);
                    int reward = KILL_REWARD;
                    if (e->type == ENEMY_FAST) reward = (int)(KILL_REWARD * 1.2f);
                    if (e->type == ENEMY_TANK) reward = (int)(KILL_REWARD * 2.0f);
                    g->gold  += reward;
                    g->score += reward * 2;
                    g->enemiesKilled++;
                    EarnProsperity(&g->homeCity, 5);
                    e->active = false;
                    PlaySFX(&g->audio, e->type == ENEMY_TANK ? SFX_TANK_DEATH : SFX_ENEMY_DEATH); /* T64 */
                }
            } else {
                target->currentHp -= p->damage;
                SpawnFloatingText(g, target->position, p->damage, false, false); /* T57 */
                SpawnParticles(g, p->position, p->color, 5, 80.0f, 0.2f);
                PlaySFX(&g->audio, p->sourceType == TOWER_SNIPER ? SFX_SHOOT_SNIPER : SFX_SHOOT_BASIC); /* T64 */
                /* Öldü mü? */
                if (target->currentHp <= 0.0f) {
                    SpawnParticles(g, target->position, target->color, 10, 100.0f, 0.4f);
                    int reward = KILL_REWARD;
                    if (target->type == ENEMY_FAST) reward = (int)(KILL_REWARD * 1.2f);
                    if (target->type == ENEMY_TANK) reward = (int)(KILL_REWARD * 2.0f);
                    g->gold  += reward;
                    g->score += reward * 2;
                    g->enemiesKilled++;
                    EarnProsperity(&g->homeCity, 5);
                    target->active = false;
                    PlaySFX(&g->audio, target->type == ENEMY_TANK ? SFX_TANK_DEATH : SFX_ENEMY_DEATH); /* T64 */
                }
            }
            p->active = false;
        }
    }
}

/* ============================================================
 * T24 — UpdateParticles
 * ============================================================ */

/* Parçacıkları hareket ettirir, ömürlerini ve alfa değerlerini günceller. */
void UpdateParticles(Game *g, float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &g->particles[i];
        if (!p->active) continue;
        p->position  = Vec2Add(p->position, Vec2Scale(p->velocity, dt));
        p->lifetime -= dt;
        if (p->lifetime <= 0.0f) { p->active = false; continue; }
        float ratio  = p->lifetime / p->maxLifetime;
        p->size      = (4.0f + 4.0f) * ratio; /* Zamanla küçül */
        p->color.a   = (unsigned char)(255.0f * ratio);
    }
}

/* ============================================================
 * T56 — ScreenShake
 * ============================================================ */

/* Sarsıntı başlatır; büyük patlama, level-up vb. olaylardan çağrılır. */
void TriggerScreenShake(Game *g, float intensity) {
    if (intensity > g->screenShake.intensity)
        g->screenShake.intensity = intensity;
    g->screenShake.decay = intensity * 6.0f;
}

/* Her frame sarsıntı ofseti güncellenir (rawDt ile çalışır — gameSpeed etkilemez). */
void UpdateScreenShake(Game *g, float rawDt) {
    ScreenShake *s = &g->screenShake;
    if (s->intensity <= 0.0f) {
        s->offsetX = 0.0f; s->offsetY = 0.0f;
        return;
    }
    s->intensity -= s->decay * rawDt;
    if (s->intensity < 0.0f) s->intensity = 0.0f;
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
        if (g->floatingTexts[i].active) continue;
        FloatingText *ft = &g->floatingTexts[i];
        ft->position    = pos;
        ft->value       = value;
        ft->lifetime    = 1.0f;
        ft->maxLifetime = 1.0f;
        ft->isCrit      = isCrit;
        ft->active      = true;
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
        if (!ft->active) continue;
        ft->position.y -= 40.0f * dt;
        ft->lifetime   -= dt;
        if (ft->lifetime <= 0.0f) ft->active = false;
    }
}

/* FloatingText'leri opaklık ve boyut artışıyla çizer. */
void DrawFloatingTexts(Game *g) {
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        FloatingText *ft = &g->floatingTexts[i];
        if (!ft->active) continue;
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
            DrawText(cbuf, (int)ft->position.x - MeasureText(cbuf, 22)/2,
                     (int)ft->position.y, 22, col);
        } else {
            DrawText(buf, (int)ft->position.x - MeasureText(buf, 16)/2,
                     (int)ft->position.y, 16, col);
        }
    }
}

/* ============================================================
 * T59 — Tower Synergy System
 * ============================================================ */

/* İki kule arasındaki sinerji bonusunu tip çiftine göre uygular ve çifti kaydeder. */
static void ApplySynergyPair(Game *g, int a, int b) {
    Tower *ta = &g->towers[a];
    Tower *tb = &g->towers[b];

    /* Küçük tipten büyüğe sıralanmış çift anahtarı */
    TowerType lo = (ta->type <= tb->type) ? ta->type : tb->type;
    TowerType hi = (ta->type <= tb->type) ? tb->type : ta->type;

    if (lo == TOWER_BASIC  && hi == TOWER_BASIC) {
        ta->fireRate *= 1.15f; tb->fireRate *= 1.15f;
    } else if (lo == TOWER_BASIC  && hi == TOWER_SNIPER) {
        /* Sniper'a +10% hasar */
        Tower *sn = (ta->type == TOWER_SNIPER) ? ta : tb;
        sn->damage *= 1.10f;
    } else if (lo == TOWER_BASIC  && hi == TOWER_SPLASH) {
        /* Splash yarıçapı +20% */
        Tower *sp = (ta->type == TOWER_SPLASH) ? ta : tb;
        sp->splashRadius *= 1.20f;
    } else if (lo == TOWER_SNIPER && hi == TOWER_SNIPER) {
        ta->range *= 1.15f; tb->range *= 1.15f;
    } else if (lo == TOWER_SNIPER && hi == TOWER_SPLASH) {
        Tower *sp = (ta->type == TOWER_SPLASH) ? ta : tb;
        Tower *sn = (ta->type == TOWER_SNIPER) ? ta : tb;
        sp->damage   *= 1.20f;
        sn->fireRate *= 1.10f;
    } else if (lo == TOWER_SPLASH && hi == TOWER_SPLASH) {
        ta->splashRadius *= 1.25f; tb->splashRadius *= 1.25f;
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
        if (i == newIdx || !g->towers[i].active) continue;
        Tower *other = &g->towers[i];
        if (Vec2Distance(newT->position, other->position) <= SYNERGY_RADIUS) {
            /* Her çift yalnızca bir kez işlenmeli */
            bool alreadyPaired = false;
            for (int p = 0; p < g->synergyPairCount; p++) {
                SynergyPair *sp = &g->synergyPairs[p];
                if ((sp->idxA == i && sp->idxB == newIdx) ||
                    (sp->idxA == newIdx && sp->idxB == i)) {
                    alreadyPaired = true; break;
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
        if (!ta->active || !tb->active) continue;
        /* Sinüs dalgası ile pulsing opaklık */
        float alpha = 0.18f + sinf(t * 2.0f + i * 0.8f) * 0.10f;
        Color lineCol = {72, 145, 220, (unsigned char)(alpha * 255)};
        DrawLineEx(ta->position, tb->position, 1.5f, lineCol);
        /* Uç noktalarda küçük daireler */
        DrawCircleLines((int)ta->position.x, (int)ta->position.y,
                        (float)CELL_SIZE * 0.55f, lineCol);
        DrawCircleLines((int)tb->position.x, (int)tb->position.y,
                        (float)CELL_SIZE * 0.55f, lineCol);
    }
}

/* ============================================================
 * T63 — UpdateGameCamera
 * ============================================================ */

/* T69 — Kamera: WASD pan, kenar kaydırma, zoom, sınır kontrolü */
void UpdateGameCamera(Game *g, float dt) {
    GameCamera *gc = &g->camera;
    float panSpeed = 400.0f / gc->zoom; /* Yakın zoomda yavaş, uzakta hızlı */

    /* WASD ile kamera kaydırma (STATE_DUNGEON dışında) */
    if (g->state == STATE_PLAYING || g->state == STATE_PREP_PHASE) {
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    gc->cam.target.y -= panSpeed * dt;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))   gc->cam.target.y += panSpeed * dt;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))   gc->cam.target.x -= panSpeed * dt;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))  gc->cam.target.x += panSpeed * dt;
    }

    /* Ekran kenarına fare gelince kaydırma (20px bant) */
    Vector2 mp = GetMousePosition();
    float edgeBand = 20.0f;
    if (mp.x < edgeBand)                     gc->cam.target.x -= panSpeed * dt;
    if (mp.x > SCREEN_WIDTH - edgeBand)      gc->cam.target.x += panSpeed * dt;
    if (mp.y < edgeBand)                     gc->cam.target.y -= panSpeed * dt;
    if (mp.y > SCREEN_HEIGHT - edgeBand)     gc->cam.target.y += panSpeed * dt;

    /* Scroll ile zoom: 1.2× – 3.0× */
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        gc->zoom += wheel * 0.15f;
        if (gc->zoom < 1.2f) gc->zoom = 1.2f;
        if (gc->zoom > 3.0f) gc->zoom = 3.0f;
        gc->cam.zoom = gc->zoom;
    }

    /* Hero takibi */
    if (gc->heroFollow && g->hero.alive) {
        gc->cam.target = g->hero.position;
    }

    /* Kamera offset = ekran merkezi */
    gc->cam.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};

    /* Harita sınırlarına klample */
    float worldW = (float)(GRID_COLS * CELL_SIZE);
    float worldH = (float)(GRID_ROWS * CELL_SIZE);
    float halfViewW = (SCREEN_WIDTH / 2.0f) / gc->zoom;
    float halfViewH = (SCREEN_HEIGHT / 2.0f) / gc->zoom;
    if (gc->cam.target.x < halfViewW) gc->cam.target.x = halfViewW;
    if (gc->cam.target.y < halfViewH) gc->cam.target.y = halfViewH;
    if (gc->cam.target.x > worldW - halfViewW) gc->cam.target.x = worldW - halfViewW;
    if (gc->cam.target.y > worldH - halfViewH) gc->cam.target.y = worldH - halfViewH;
}

/* ============================================================
 * T27 — UpdateWaves
 * ============================================================ */

/* Dalganın spawn zamanlayıcısını yönetir, düşman üretir,
 * dalga bitişini ve geçiş durumunu kontrol eder. */
void UpdateWaves(Game *g, float dt) {
    if (!g->waveActive || g->currentWave >= g->totalWaves) return;

    GameWave *w = &g->waves[g->currentWave];

    /* Ön gecikme */
    if (!w->started) {
        w->preDelay -= dt;
        if (w->preDelay > 0.0f) return;
        w->started = true;
        /* T62 — Dalga adı bannerını göster */
        strncpy(g->waveNameText, w->waveName, 31);
        g->waveNameTimer = 3.0f;
        if (w->isBossWave) TriggerScreenShake(g, 8.0f);
        PlaySFX(&g->audio, SFX_WAVE_START); /* T64 */
    }

    /* T62 — Dalga adı banner sayacı */
    if (g->waveNameTimer > 0.0f) g->waveNameTimer -= dt;

    /* T62 — Spawn: Çoklu tip desteği — type[0] tükendikten sonra type[1], sonra type[2] */
    if (w->spawnedCount < w->enemyCount) {
        w->spawnTimer -= dt;
        if (w->spawnTimer <= 0.0f) {
            /* Hangi tipin sırası olduğunu spawnedCount'a göre belirle */
            EnemyType typeToSpawn = w->enemyTypes[0];
            int cumulative = 0;
            for (int t = 0; t < w->typeCount; t++) {
                cumulative += w->enemyCounts[t];
                if (w->spawnedCount < cumulative) {
                    typeToSpawn = w->enemyTypes[t];
                    break;
                }
            }
            SpawnEnemy(g, typeToSpawn, w->enemyHpMultiplier);
            w->spawnedCount++;
            w->spawnTimer = w->spawnInterval;
        }
    }

    /* Dalga bitti mi? Tüm spawn edildi ve aktif düşman kalmadı */
    if (w->spawnedCount >= w->enemyCount) {
        bool anyActive = false;
        for (int i = 0; i < MAX_ENEMIES; i++)
            if (g->enemies[i].active) { anyActive = true; break; }
        if (!anyActive) {
            /* T62 — Dalga tamamlama altın ödülü */
            g->gold += w->bonusGold;
            g->waveActive = false;
            g->currentWave++;
            if (g->currentWave >= g->totalWaves) {
                /* Tüm dalgalar bitti — yıldız hesapla, completion diyaloğunu yükle */
                g->levelStars = CalcLevelStars(g);
                g->levels[g->currentLevel].stars = g->levelStars;
                LevelData *lv = &g->levels[g->currentLevel];
                g->dialogue.lineCount    = lv->completionCount;
                g->dialogue.currentLine  = 0;
                g->dialogue.visibleChars = 0;
                g->dialogue.charTimer    = 0.0f;
                g->dialogue.active       = (lv->completionCount > 0);
                for (int i = 0; i < lv->completionCount; i++)
                    g->dialogue.lines[i] = lv->completion[i];
                PlaySFX(&g->audio, SFX_VICTORY); /* T64 */
                g->state = STATE_LEVEL_COMPLETE;
            } else {
                g->state = STATE_WAVE_CLEAR;
            }
        }
    }
}

/* ============================================================
 * CheckGameConditions
 * ============================================================ */

void CheckGameConditions(Game *g) {
    if (g->lives <= 0) {
        PlaySFX(&g->audio, SFX_DEFEAT); /* T64 */
        g->state = STATE_GAMEOVER;
    }
}

/* ============================================================
 * UpdateDialogue — Typewriter efekti, satır ilerleme
 * ============================================================ */

/* SPACE veya sol tık ile önce mevcut metni tamamlar,
 * sonraki tıkta bir sonraki satıra geçer. */
void UpdateDialogue(Game *g, float dt) {
    DialogueSystem *d = &g->dialogue;
    if (!d->active) return;

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
            d->charTimer    = 0.0f;
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
        if (!lv->unlocked) continue;
        float dx = mp.x - lv->nodeX, dy = mp.y - lv->nodeY;
        if ((dx*dx + dy*dy) <= 30.0f * 30.0f &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            g->currentLevel = i;
            /* Brifing diyaloğunu yükle */
            g->dialogue.lineCount    = lv->briefingCount;
            g->dialogue.currentLine  = 0;
            g->dialogue.visibleChars = 0;
            g->dialogue.charTimer    = 0.0f;
            g->dialogue.active       = (lv->briefingCount > 0);
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
    Rectangle btn = {SCREEN_WIDTH/2.0f - 100, (float)SCREEN_HEIGHT - 100, 200, 50};
    if (IsButtonClicked(btn)) {
        const char *tips[MAX_TIP_TEXTS] = {
            "Ipucu: Sniper kuleler Tank dusmanlara karsi cok etkilidir.",
            "Ipucu: Splash kuleler grup dusmanlarini hizla temizler.",
            "Ipucu: Kule yükseltmek fiyatina deger — hasar %30 artar.",
            "Ipucu: [F] tusu ile oyun hizini 2x yapabilirsiniz.",
            "Ipucu: Satilan kule altinin %50'sini geri verir.",
            "Ipucu: Hizli dusmanlar gelmeden once yolu kapatmayi unutmayin."
        };
        g->loading.progress  = 0.0f;
        g->loading.timer     = 0.0f;
        g->loading.duration  = 1.8f;
        g->loading.nextState = STATE_PLAYING;
        g->loading.tipText   = tips[g->currentLevel % MAX_TIP_TEXTS];
        /* TODO: StopMusic(g->assets.bgmMenu); PlayMusic(g->assets.bgmBattle[g->currentLevel]) */
        g->state = STATE_LOADING;
    }
}

/* ============================================================
 * UpdateLoading — Sahte progress bar, sonra level başlat
 * ============================================================ */

void UpdateLoading(Game *g, float dt) {
    g->loading.timer    += dt;
    g->loading.progress  = g->loading.timer / g->loading.duration;
    if (g->loading.progress < 1.0f) return;

    g->loading.progress = 1.0f;
    /* Gameplay sıfırla, kampanya verisi koru */
    ResetGameplay(g);
    g->state      = STATE_PLAYING;
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
    Rectangle btn = {SCREEN_WIDTH/2.0f - 100, (float)SCREEN_HEIGHT - 120, 200, 55};
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
void HandleInput(Game *g) {
    /* Kule tipi seç — bina seçimini sıfırla */
    if (IsKeyPressed(KEY_ONE))   { g->selectedTowerType = TOWER_BASIC;  g->selectedBuildingType = -1; }
    if (IsKeyPressed(KEY_TWO))   { g->selectedTowerType = TOWER_SNIPER; g->selectedBuildingType = -1; }
    if (IsKeyPressed(KEY_THREE)) { g->selectedTowerType = TOWER_SPLASH; g->selectedBuildingType = -1; }

    /* Oyun hızı */
    if (IsKeyPressed(KEY_F))
        g->gameSpeed = (g->gameSpeed < 1.5f) ? 2.0f : 1.0f;

    /* Grid göster/gizle */
    if (IsKeyPressed(KEY_G))
        g->showGrid = !g->showGrid;

    /* T63 — Hero kamera takibi toggle */
    if (IsKeyPressed(KEY_H))
        g->camera.heroFollow = !g->camera.heroFollow;

    /* T66 — ESC: önce kule/bina seçimi iptal, yoksa duraklat */
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (g->selectedTowerType != (TowerType)-1 || g->selectedBuildingType >= 0) {
            /* Kule veya bina seçimini iptal et */
            g->selectedTowerType = (TowerType)-1;
            g->selectedBuildingType = -1;
        } else {
            g->contextMenuOpen = false;
            g->state = STATE_PAUSED;
        }
    }
    if (IsKeyPressed(KEY_P)) {
        g->contextMenuOpen = false;
        g->state = STATE_PAUSED;
    }

    /* T67 — Bina seçimi: 4/5/6 */
    if (IsKeyPressed(KEY_FOUR))  { g->selectedBuildingType = BUILDING_BARRACKS;    g->selectedTowerType = (TowerType)-1; }
    if (IsKeyPressed(KEY_FIVE))  { g->selectedBuildingType = BUILDING_MARKET;      g->selectedTowerType = (TowerType)-1; }
    if (IsKeyPressed(KEY_SIX))   { g->selectedBuildingType = BUILDING_TOWN_CENTER; g->selectedTowerType = (TowerType)-1; }

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
        float dragDist = sqrtf(dx*dx + dy*dy);

        if (dragDist > 8.0f) {
            /* Büyük sürükleme → birim seçimi */
            /* Seçim dikdörtgenini dünya koordinatına çevir */
            Vector2 worldStart = GetScreenToWorld2D(g->selectionStart, g->camera.cam);
            Vector2 worldEnd   = GetScreenToWorld2D(g->selectionEnd, g->camera.cam);
            float minX = fminf(worldStart.x, worldEnd.x);
            float maxX = fmaxf(worldStart.x, worldEnd.x);
            float minY = fminf(worldStart.y, worldEnd.y);
            float maxY = fmaxf(worldStart.y, worldEnd.y);
            for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
                FriendlyUnit *fu = &g->friendlyUnits[i];
                if (!fu->active) { fu->selected = false; continue; }
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
                        if (g->friendlyUnits[i].active) continue;
                        InitFriendlyUnit(&g->friendlyUnits[i],
                                         g->pendingPlacementType,
                                         GridToWorld(gx, gy));
                        g->pendingPlacementCount--;
                        break;
                    }
                }
            }
            /* Bağlam menüsü */
            else if (g->contextMenuOpen) {
                Rectangle menuRect = {g->contextMenuPos.x - 2, g->contextMenuPos.y - 2, 144, 80};
                if (CheckCollisionPointRec(screenMp, menuRect) && g->contextMenuTowerIdx >= 0) {
                    Tower *t = &g->towers[g->contextMenuTowerIdx];
                    Rectangle upgradeBtn = {g->contextMenuPos.x, g->contextMenuPos.y,      140, 34};
                    Rectangle sellBtn    = {g->contextMenuPos.x, g->contextMenuPos.y + 38, 140, 34};
                    if (t->active && CheckCollisionPointRec(screenMp, upgradeBtn) && t->level < 3) {
                        int cost = GetTowerCost(t->type) * t->level;
                        if (g->gold >= cost) {
                            g->gold   -= cost;
                            t->level++;
                            t->damage *= 1.3f;
                            t->range  *= 1.1f;
                            PlaySFX(&g->audio, SFX_TOWER_UPGRADE);
                        }
                    } else if (t->active && CheckCollisionPointRec(screenMp, sellBtn)) {
                        g->gold += GetTowerCost(t->type) / 2;
                        PlaySFX(&g->audio, SFX_TOWER_SELL);
                        g->grid[t->gridY][t->gridX] = CELL_BUILDABLE;
                        t->active = false;
                    }
                }
                g->contextMenuOpen = false;
            }
            /* T67 — Bina yerleştirme (rural alana) */
            else if (g->selectedBuildingType >= 0) {
                int gx, gy;
                if (WorldToGrid(worldMp, &gx, &gy) && g->grid[gy][gx] == CELL_RURAL) {
                    int cost = 0;
                    switch (g->selectedBuildingType) {
                        case BUILDING_BARRACKS:    cost = 100; break;
                        case BUILDING_MARKET:      cost = 120; break;
                        case BUILDING_TOWN_CENTER: cost = 200; break;
                        default: break;
                    }
                    /* Town Center maks 1 adet kontrolü */
                    bool canBuild = (g->gold >= cost);
                    if (g->selectedBuildingType == BUILDING_TOWN_CENTER) {
                        for (int i = 0; i < g->buildingCount; i++)
                            if (g->buildings[i].active && g->buildings[i].type == BUILDING_TOWN_CENTER)
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
            else if ((int)g->selectedTowerType >= 0 && (int)g->selectedTowerType < TOWER_TYPE_COUNT) {
                int gx, gy;
                if (WorldToGrid(worldMp, &gx, &gy) && CanPlaceTower(g, gx, gy)) {
                    for (int i = 0; i < MAX_TOWERS; i++) {
                        if (g->towers[i].active) continue;
                        Tower *t = &g->towers[i];
                        memset(t, 0, sizeof(Tower));
                        t->gridX   = gx;
                        t->gridY   = gy;
                        t->position= GridToWorld(gx, gy);
                        t->type    = g->selectedTowerType;
                        t->level   = 1;
                        t->targetEnemyIndex = -1;
                        t->active  = true;
                        switch (t->type) {
                            case TOWER_BASIC:
                                t->range=150; t->damage=20; t->fireRate=2.0f;
                                t->splashRadius=0; t->color=BLUE; break;
                            case TOWER_SNIPER:
                                t->range=300; t->damage=80; t->fireRate=0.5f;
                                t->splashRadius=0; t->color=DARKGREEN; break;
                            case TOWER_SPLASH:
                                t->range=120; t->damage=30; t->fireRate=1.0f;
                                t->splashRadius=60; t->color=MAROON; break;
                            default: break;
                        }
                        if (g->hero.heroClass == HERO_WARRIOR)     t->damage   *= 1.15f;
                        else if (g->hero.heroClass == HERO_MAGE)   t->range    *= 1.20f;
                        else if (g->hero.heroClass == HERO_ARCHER) t->fireRate *= 1.25f;
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

    /* Sağ tık — bağlam menüsünü aç */
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        bool hit = false;
        for (int i = 0; i < MAX_TOWERS; i++) {
            Tower *t = &g->towers[i];
            if (!t->active) continue;
            if (Vec2Distance(worldMp, t->position) < CELL_SIZE / 2.0f) {
                g->contextMenuOpen     = true;
                g->contextMenuTowerIdx = i;
                g->contextMenuPos      = screenMp;
                hit = true;
                break;
            }
        }
        if (!hit) g->contextMenuOpen = false;
    }
}

/* ============================================================
 * UpdateMenu / UpdatePause / UpdateWaveClear
 * ============================================================ */

void UpdateMenu(Game *g) {
    Rectangle btn = {SCREEN_WIDTH/2.0f - 80, SCREEN_HEIGHT/2.0f + 20, 160, 50};
    if (IsButtonClicked(btn)) {
        /* TODO: PlaySound(g->assets.sfxButtonClick) */
        g->state = STATE_CLASS_SELECT; /* T52 — önce sınıf seç */
    }
}

/* T52 — Kahraman sınıf seçim ekranı güncelleme */
void UpdateClassSelect(Game *g) {
    /* Her sınıf için buton dikdörtgeni */
    Rectangle btns[3] = {
        {SCREEN_WIDTH/2.0f - 330, SCREEN_HEIGHT/2.0f - 60, 200, 180},
        {SCREEN_WIDTH/2.0f - 100, SCREEN_HEIGHT/2.0f - 60, 200, 180},
        {SCREEN_WIDTH/2.0f + 130, SCREEN_HEIGHT/2.0f - 60, 200, 180}
    };
    HeroClass classes[3] = {HERO_WARRIOR, HERO_MAGE, HERO_ARCHER};

    for (int i = 0; i < 3; i++) {
        if (IsButtonClicked(btns[i])) {
            InitHero(&g->hero, classes[i]);
            SpawnHeroUnit(g);
            g->state = STATE_WORLD_MAP;
            break;
        }
    }

    /* ESC → geri */
    if (IsKeyPressed(KEY_ESCAPE)) g->state = STATE_MENU;
}

/* T52 — Kahraman sınıf seçim ekranı çizimi */
void DrawClassSelect(Game *g) {
    /* Arkaplan */
    DrawGradientPanel((Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT},
                      (Color){10, 8, 20, 255}, (Color){20, 15, 40, 255});

    /* Başlık */
    DrawEpicTitle("KAHRAMAN SINIFI SEC",
                  (Vector2){SCREEN_WIDTH/2.0f - UIS(210), 60}, 28.0f);

    /* Sınıf verileri */
    const char *names[3]   = {"SAVASCI", "BUYUCU", "OKCU"};
    const char *descs[3][4] = {
        {"HP: 700  Def: 20", "Hiz: 100 px/s", "Menzil: 35 (yakin)", "Kule: +15% Hasar"},
        {"HP: 350  Mana: 300", "Hiz:  90 px/s", "Menzil: 120 (buyu)", "Kule: +20% Menzil"},
        {"HP: 450  Mana: 150", "Hiz: 130 px/s", "Menzil: 160 (ok)",  "Kule: +25% Atis Hz"}
    };
    Color bodyColors[3] = {SKYBLUE, VIOLET, LIME};
    Rectangle btns[3] = {
        {SCREEN_WIDTH/2.0f - 330, SCREEN_HEIGHT/2.0f - 60, 200, 180},
        {SCREEN_WIDTH/2.0f - 100, SCREEN_HEIGHT/2.0f - 60, 200, 180},
        {SCREEN_WIDTH/2.0f + 130, SCREEN_HEIGHT/2.0f - 60, 200, 180}
    };

    for (int i = 0; i < 3; i++) {
        Rectangle b = btns[i];
        bool hover = CheckCollisionPointRec(GetMousePosition(), b);

        /* Kart arka planı */
        DrawGradientPanel(b, UI_PANEL_TOP, UI_PANEL_BOT);
        DrawOrnateBorder(b, hover ? UI_GOLD_BRIGHT : UI_GOLD, hover ? 2.0f : 1.0f);

        /* Sınıf ikonu */
        DrawCircle((int)(b.x + b.width/2), (int)(b.y + 35), 22, bodyColors[i]);
        DrawCircleLines((int)(b.x + b.width/2), (int)(b.y + 35), 22, WHITE);

        /* İsim */
        int tw = MeasureText(names[i], 18);
        DrawText(names[i], (int)(b.x + b.width/2 - tw/2), (int)(b.y + 65), 18,
                 hover ? UI_GOLD_BRIGHT : UI_GOLD);

        /* Açıklama satırları */
        for (int d = 0; d < 4; d++) {
            DrawText(descs[i][d], (int)(b.x + 8), (int)(b.y + 95 + d * 18), 12,
                     hover ? UI_IVORY : (Color){180, 180, 200, 200});
        }

        if (hover)
            DrawText("[ Sec ]", (int)(b.x + b.width/2 - 28), (int)(b.y + b.height - 24),
                     14, UI_GOLD);
    }

    DrawText("ESC: Geri", 20, SCREEN_HEIGHT - 30, 14, (Color){150, 150, 170, 200});
}

void UpdatePause(Game *g) {
    Rectangle resume = {SCREEN_WIDTH/2.0f - 90, SCREEN_HEIGHT/2.0f,      180, 50};
    Rectangle quit   = {SCREEN_WIDTH/2.0f - 90, SCREEN_HEIGHT/2.0f + 70, 180, 50};
    if (IsButtonClicked(resume) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) g->state = STATE_PLAYING;
    if (IsButtonClicked(quit))   g->state = STATE_MENU; /* Menüye dön */
}

/* ============================================================
 * T08 — DrawMap
 * ============================================================ */

/* Haritayı hücre tipine göre renklendirerek çizer, opsiyonel grid çizgileriyle. */
void DrawMap(Game *g) {
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            int x = GRID_OFFSET_X + c * CELL_SIZE;
            int y = GRID_OFFSET_Y + r * CELL_SIZE;
            Color fill;
            switch (g->grid[r][c]) {
                case CELL_PATH:      fill = (Color){160, 130, 90, 255}; break;
                case CELL_BUILDABLE: fill = (Color){50,  80,  50, 255}; break;
                case CELL_TOWER:     fill = (Color){40,  60,  40, 255}; break;
                case CELL_RURAL:     fill = (Color){38,  55,  28, 255}; break;  /* T67 Koyu çayır */
                case CELL_VILLAGE:   fill = (Color){90,  70,  45, 255}; break;  /* T68 Köy zemini */
                default:             fill = (Color){30,  50,  30, 255}; break;
            }
            DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, fill);

            /* T68 — Köy hücrelerine mini ev çiz */
            if (g->grid[r][c] == CELL_VILLAGE) {
                int hx = x + CELL_SIZE/4, hy = y + CELL_SIZE/4;
                int hw = CELL_SIZE/2, hh = CELL_SIZE/3;
                DrawRectangle(hx, hy + hh/2, hw, hh/2, (Color){120, 90, 55, 255});
                /* Çatı (üçgen) */
                DrawTriangle(
                    (Vector2){(float)(hx + hw/2), (float)hy},
                    (Vector2){(float)hx, (float)(hy + hh/2)},
                    (Vector2){(float)(hx + hw), (float)(hy + hh/2)},
                    (Color){180, 60, 40, 255});
            }

            if (g->showGrid)
                DrawRectangleLines(x, y, CELL_SIZE, CELL_SIZE, (Color){255, 255, 255, 25});
        }
    }
}

/* ============================================================
 * T13 — DrawEnemies
 * ============================================================ */

/* Düşmanları tipine göre şekil ve HP bar ile çizer. */
void DrawEnemies(Game *g) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g->enemies[i];
        if (!e->active) continue;

        /* T70 — Fogdaki düşmanlar çizilmez */
        int er = (int)(e->position.y) / CELL_SIZE;
        int ec = (int)(e->position.x) / CELL_SIZE;
        if (er >= 0 && er < GRID_ROWS && ec >= 0 && ec < GRID_COLS) {
            if (!g->fogVisible[er][ec]) continue;
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
                        DrawCircleGradient((int)e->position.x, (int)e->position.y, e->radius * 2.0f * pulse, WHITE, BLANK);
                    } else {
                        DrawCircleGradient((int)e->position.x, (int)e->position.y, e->radius * 1.5f * pulse, e->color, BLANK);
                    }
                }
                DrawCircleV(e->position, e->radius, e->color);
                DrawCircleLines((int)e->position.x, (int)e->position.y, e->radius, WHITE);
                break;
            case ENEMY_FAST: {
                /* Küçük yukarı bakan üçgen */
                float r = e->radius;
                Vector2 v1 = {e->position.x,       e->position.y - r};
                Vector2 v2 = {e->position.x - r,   e->position.y + r};
                Vector2 v3 = {e->position.x + r,   e->position.y + r};
                DrawTriangle(v1, v2, v3, e->color);
                break;
            }
            case ENEMY_TANK: {
                float r = e->radius;
                DrawRectangle((int)(e->position.x - r), (int)(e->position.y - r),
                              (int)(r * 2), (int)(r * 2), e->color);
                DrawRectangleLines((int)(e->position.x - r), (int)(e->position.y - r),
                                   (int)(r * 2), (int)(r * 2), WHITE);
                break;
            }
            default: break;
        }

        /* T55 — Boss yetenek görsel efektleri (Meteor uyarı dairesi vb.) */
        if (e->isBoss && e->isCasting && e->bossPhase == 2) {
            float blink = (int)(GetTime() * 10) % 2 == 0 ? 0.8f : 0.4f;
            DrawCircleLines((int)e->targetAbilityPos.x, (int)e->targetAbilityPos.y, 100.0f, Fade(RED, blink));
            DrawCircle((int)e->targetAbilityPos.x, (int)e->targetAbilityPos.y, 100.0f, Fade(RED, blink * 0.3f));
        }

        /* HP bar — sadece hasar almışsa göster (Boss ise HUD'a havâle et) */
        if (!e->isBoss && e->currentHp < e->maxHp) {
            float barW = e->radius * 2.5f;
            float barH = 4.0f;
            float barX = e->position.x - barW / 2.0f;
            float barY = e->position.y - e->radius - 8.0f;
            float ratio= e->currentHp / e->maxHp;
            DrawRectangle((int)barX, (int)barY, (int)barW, (int)barH, DARKGRAY);
            Color hpColor = (ratio > 0.5f) ?
                (Color){(unsigned char)(255 * (1.0f - ratio) * 2), 200, 0, 255} :
                (Color){220, (unsigned char)(200 * ratio * 2), 0, 255};
            DrawRectangle((int)barX, (int)barY, (int)(barW * ratio), (int)barH, hpColor);
        }
    }
}

/* ============================================================
 * T18 — DrawTowers
 * ============================================================ */

/* Kuleleri kare gövde + yönlendirilmiş namlu çizgisi ile çizer.
 * Seviyeye göre renk koyulaşır ve boyut büyür. */
void DrawTowers(Game *g) {
    for (int i = 0; i < MAX_TOWERS; i++) {
        Tower *t = &g->towers[i];
        if (!t->active) continue;

        float sizeScale = 0.65f + 0.1f * t->level; /* Seviye 1:0.75, 2:0.85, 3:0.95 */
        int   half      = (int)(CELL_SIZE * sizeScale / 2.0f);
        int   px        = (int)t->position.x;
        int   py        = (int)t->position.y;

        /* Seviyeye göre rengi koyulaştır */
        Color c = t->color;
        c.r = (unsigned char)(c.r * (0.7f + 0.1f * t->level));
        c.g = (unsigned char)(c.g * (0.7f + 0.1f * t->level));
        c.b = (unsigned char)(c.b * (0.7f + 0.1f * t->level));

        DrawRectangle(px - half, py - half, half * 2, half * 2, c);
        DrawRectangleLines(px - half, py - half, half * 2, half * 2, WHITE);

        /* Namlu çizgisi — kule merkezinden menzile doğru ince çizgi */
        float angle = t->rotation * DEG2RAD;
        float barLen = (float)half * 1.4f;
        Vector2 tip  = {px + cosf(angle) * barLen, py + sinf(angle) * barLen};
        DrawLineV(t->position, tip, WHITE);

        /* Seviye göstergesi (küçük nokta) */
        for (int lv = 0; lv < t->level; lv++)
            DrawCircle(px - half + 4 + lv * 8, py + half - 5, 3, YELLOW);

        /* B04: ateş flash — kısa beyaz parıltı */
        if (t->flashTimer > 0.0f) {
            float alpha = t->flashTimer / 0.1f;
            DrawCircle(px, py, (float)half * 1.3f,
                       (Color){255, 255, 255, (unsigned char)(90 * alpha)});
        }
    }
}

/* ============================================================
 * T22 — DrawProjectiles
 * ============================================================ */

void DrawProjectiles(Game *g) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &g->projectiles[i];
        if (!p->active) continue;
        float radius = (p->sourceType == TOWER_SNIPER) ? 3.0f : 5.0f;

        /* T58 — Kuyruk efekti: eski pozisyonları azalan opaklık/boyutla çiz */
        for (int t = 0; t < TRAIL_LENGTH; t++) {
            int idx   = (p->trailIndex - t - 1 + TRAIL_LENGTH) % TRAIL_LENGTH;
            float age = (float)(t + 1) / (float)TRAIL_LENGTH;  /* 0 = taze, 1 = eski */
            float tr  = radius * (1.0f - age * 0.75f);
            if (tr < 0.5f) continue;
            Color tc = p->color;
            tc.a = (unsigned char)(160.0f * (1.0f - age));
            DrawCircleV(p->trailPositions[idx], tr, tc);
        }

        DrawCircleV(p->position, radius, p->color);
    }
}

/* ============================================================
 * T25 — DrawParticles
 * ============================================================ */

void DrawParticles(Game *g) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &g->particles[i];
        if (!p->active) continue;
        DrawRectangle(
            (int)(p->position.x - p->size / 2),
            (int)(p->position.y - p->size / 2),
            (int)p->size, (int)p->size, p->color
        );
    }
}

/* ============================================================
 * DrawPlacementPreview
 * ============================================================ */

/* Fare grid üzerindeyken yerleştirilebilirliği gösteren yarı saydam önizleme çizer. */
void DrawPlacementPreview(Game *g) {
    Vector2 mp = GetMousePosition();
    int gx, gy;
    if (!WorldToGrid(mp, &gx, &gy)) return;

    bool canPlace = CanPlaceTower(g, gx, gy);
    Color previewColor = canPlace ? (Color){0, 255, 0, 80} : (Color){255, 0, 0, 80};
    int x = GRID_OFFSET_X + gx * CELL_SIZE;
    int y = GRID_OFFSET_Y + gy * CELL_SIZE;
    DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, previewColor);
    DrawRectangleLines(x, y, CELL_SIZE, CELL_SIZE, canPlace ? GREEN : RED);

    /* Menzil dairesi */
    float range = 150;
    switch (g->selectedTowerType) {
        case TOWER_BASIC:  range = 150; break;
        case TOWER_SNIPER: range = 300; break;
        case TOWER_SPLASH: range = 120; break;
        default: break;
    }
    Vector2 center = GridToWorld(gx, gy);
    DrawCircleLines((int)center.x, (int)center.y, range, (Color){255, 255, 255, 60});
}

/* ============================================================
 * T30 — DrawHUD
 * ============================================================ */

/* Üst bilgi panelini ve kule seçim butonlarını çizer. */
void DrawHUD(Game *g) {
    /* Üst panel — obsidyen gradient */
    DrawGradientPanel((Rectangle){0, 0, SCREEN_WIDTH, GRID_OFFSET_Y},
                      UI_PANEL_TOP, (Color){8, 10, 18, 255});
    DrawOrnateBorder((Rectangle){0, 0, SCREEN_WIDTH, GRID_OFFSET_Y},
                     UI_GOLD, UIS(1.2f));

    /* Sol: Can / Altın / Skor — ResourceText göstergeler */
    DrawResourceText((Vector2){10, 8},  g->lives, "CAN",   (Color){200, 50,  50, 220});
    DrawResourceText((Vector2){130, 8}, g->gold,  "ALTIN", (Color){200, 160,  0, 220});
    DrawResourceText((Vector2){290, 8}, g->score, "SKOR",  (Color){ 80, 180,220, 220});

    /* Sağ: Dalga / Hız / Kill */
    char buf[128];
    snprintf(buf, sizeof(buf), "Dalga: %d/%d   Hiz: %.0fx   Kill: %d",
             g->currentWave + 1, g->totalWaves, g->gameSpeed, g->enemiesKilled);
    DrawBodyText(buf,
                 (Vector2){(float)(SCREEN_WIDTH - UISI(280)), 8.0f},
                 13.0f, (Color){200, 200, 220, 200});

    /* Alt satır: Kule seçim butonları */
    const char *labels[3] = {"[1] Basic 50g", "[2] Sniper 100g", "[3] Splash 150g"};
    Color       colors[3] = {BLUE, DARKGREEN, MAROON};
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
        DrawText("[ SPACE ] - Sonraki Dalgayi Baslat", SCREEN_WIDTH/2 - 180, 50, 18, YELLOW);

    /* B03 — Dalga ilerleme çubuğu */
    if (g->waveActive && g->currentWave < g->totalWaves) {
        GameWave *w = &g->waves[g->currentWave];
        int activeCount = 0;
        for (int i = 0; i < MAX_ENEMIES; i++)
            if (g->enemies[i].active) activeCount++;
        float progress = (w->enemyCount > 0) ? (float)w->spawnedCount / w->enemyCount : 0.0f;
        int bx = SCREEN_WIDTH / 2 - 200, by = 35, bw = 400, bh = 8;
        DrawRectangle(bx, by, bw, bh, (Color){40, 40, 40, 200});
        DrawRectangle(bx, by, (int)(bw * progress), bh, (Color){255, 180, 0, 230});
        DrawRectangleLines(bx, by, bw, bh, (Color){160, 160, 160, 120});
        char pbuf[48];
        snprintf(pbuf, sizeof(pbuf), "%d/%d  [%d aktif]",
                 w->spawnedCount, w->enemyCount, activeCount);
        DrawText(pbuf, bx + bw/2 - MeasureText(pbuf, 13)/2, by - 15, 13,
                 (Color){200, 200, 200, 200});
    }

    /* T62 — Dalga adı banner (dalga başlangıcında 3 saniye görünür) */
    if (g->waveNameTimer > 0.0f && g->waveNameText[0] != '\0') {
        float alpha = (g->waveNameTimer > 0.5f) ? 1.0f : g->waveNameTimer / 0.5f;
        int   ba = (int)(200 * alpha);
        int   fa = (int)(255 * alpha);
        /* Dalga numarası + adı */
        char bannerBuf[64];
        bool isBoss = (g->currentWave < MAX_WAVES) &&
                       g->waves[g->currentWave].isBossWave;
        snprintf(bannerBuf, sizeof(bannerBuf), "Dalga %d: %s",
                 g->currentWave + 1, g->waveNameText);
        int bw = MeasureText(bannerBuf, isBoss ? 28 : 22) + 30;
        int bx = SCREEN_WIDTH / 2 - bw / 2;
        int by = SCREEN_HEIGHT / 2 - 30;
        DrawRectangle(bx, by, bw, isBoss ? 44 : 34, (Color){0, 0, 0, ba});
        DrawRectangleLines(bx, by, bw, isBoss ? 44 : 34,
                           isBoss ? (Color){255, 200, 0, fa} : (Color){160, 160, 160, fa});
        DrawText(bannerBuf, bx + 15, by + (isBoss ? 8 : 6),
                 isBoss ? 28 : 22, isBoss ? (Color){255, 220, 0, fa} : (Color){255, 255, 255, fa});
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
            DrawRectangle(bbx - 4, bby - 4, bbw + 8, bbh + 8, (Color){30, 0, 0, (unsigned char)(200 * pulse)});
            
            DrawRectangle(bbx, bby, bbw, bbh, (Color){40, 40, 40, 240});
            float bossRatio = e->currentHp / e->maxHp;
            DrawRectangle(bbx, bby, (int)(bbw * bossRatio), bbh, MAROON);
            DrawRectangleLines(bbx, bby, bbw, bbh, GOLD);
            
            char tbuf[64];
            snprintf(tbuf, sizeof(tbuf), "BOSS: %s (Faz %d) - %.0f/%.0f", 
                     g->waves[g->currentWave].waveName, e->bossPhase, e->currentHp, e->maxHp);
            DrawText(tbuf, bbx + bbw/2 - MeasureText(tbuf, 16)/2, bby + 2, 16, WHITE);
            break; /* Tek boss varsayımı */
        }
    }

    /* T41 — Home City UI */
    DrawHomeCityUI(&g->homeCity, SCREEN_WIDTH, SCREEN_HEIGHT);
}

/* ============================================================
 * T33 — DrawMenu
 * ============================================================ */

void DrawMenu(Game *g) {
    (void)g;
    /* Arka plan */
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                           (Color){6, 8, 18, 255}, (Color){18, 22, 40, 255});

    /* Merkez panel */
    Rectangle panel = {(float)SCREEN_WIDTH/2 - 220, (float)SCREEN_HEIGHT/2 - 160, 440, 320};
    DrawEpicPanel(panel, NULL);

    /* Başlık */
    DrawEpicTitle("RULER",
                  (Vector2){(float)SCREEN_WIDTH/2 - UIS(72), (float)SCREEN_HEIGHT/2 - 130},
                  72.0f);

    /* Alt başlık */
    DrawBodyText("Kralligini Koru",
                 (Vector2){(float)SCREEN_WIDTH/2 - UIS(90), (float)SCREEN_HEIGHT/2 - 42},
                 22.0f, UI_IVORY);

    /* Buton */
    Rectangle btn = {(float)SCREEN_WIDTH/2 - 100, (float)SCREEN_HEIGHT/2 + 10, 200, 56};
    DrawButton(btn, "OYUNA BASLA", GREEN, BLACK);
}

/* ============================================================
 * T34 — DrawPauseOverlay
 * ============================================================ */

void DrawPauseOverlay(Game *g) {
    (void)g;
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 170});

    Rectangle panel = {(float)SCREEN_WIDTH/2 - 180, (float)SCREEN_HEIGHT/2 - 120, 360, 260};
    DrawEpicPanel(panel, "DURAKLATILDI");

    Rectangle resume = {(float)SCREEN_WIDTH/2 - 90, (float)SCREEN_HEIGHT/2 - 10, 180, 50};
    Rectangle quit   = {(float)SCREEN_WIDTH/2 - 90, (float)SCREEN_HEIGHT/2 + 72, 180, 50};
    DrawButton(resume, "DEVAM ET", DARKGREEN, WHITE);
    DrawButton(quit,   "CIK",      DARKGRAY,  WHITE);
}

/* ============================================================
 * T35 — DrawGameOver
 * ============================================================ */

void DrawGameOver(Game *g) {
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                           (Color){18, 4, 4, 255}, (Color){6, 2, 2, 255});

    Rectangle panel = {(float)SCREEN_WIDTH/2 - 240, (float)SCREEN_HEIGHT/2 - 150, 480, 310};
    DrawEpicPanel(panel, NULL);

    DrawShadowText(g_ui.title, "YENILGI",
                   (Vector2){(float)SCREEN_WIDTH/2 - UIS(100), (float)SCREEN_HEIGHT/2 - 130},
                   UIS(64.0f), 2.0f, (Color){220, 50, 50, 255});

    char buf[64];
    snprintf(buf, sizeof(buf), "Skor: %d   Dusmanlar: %d", g->score, g->enemiesKilled);
    DrawBodyText(buf,
                 (Vector2){(float)SCREEN_WIDTH/2 - UIS(115), (float)SCREEN_HEIGHT/2 - 20},
                 20.0f, UI_IVORY);

    Rectangle btn = {(float)SCREEN_WIDTH/2 - 110, (float)SCREEN_HEIGHT/2 + 50, 220, 56};
    DrawButton(btn, "Yeniden Baslat [R]", DARKGRAY, WHITE);
}

/* ============================================================
 * T36 — DrawVictory
 * ============================================================ */

void DrawVictory(Game *g) {
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                           (Color){4, 18, 8, 255}, (Color){2, 8, 4, 255});

    Rectangle panel = {(float)SCREEN_WIDTH/2 - 260, (float)SCREEN_HEIGHT/2 - 150, 520, 310};
    DrawEpicPanel(panel, NULL);

    DrawEpicTitle("ZAFER!",
                  (Vector2){(float)SCREEN_WIDTH/2 - UIS(72), (float)SCREEN_HEIGHT/2 - 130},
                  72.0f);

    char buf[64];
    snprintf(buf, sizeof(buf), "Final Skor: %d", g->score);
    DrawBodyText(buf,
                 (Vector2){(float)SCREEN_WIDTH/2 - UIS(100), (float)SCREEN_HEIGHT/2 - 20},
                 24.0f, UI_IVORY);

    Rectangle btn = {(float)SCREEN_WIDTH/2 - 110, (float)SCREEN_HEIGHT/2 + 50, 220, 56};
    DrawButton(btn, "Yeniden Baslat [R]", DARKGREEN, BLACK);
}

/* ============================================================
 * T70 — Fog of War Hesaplama
 * ============================================================ */

/* Fog grid'ini her frame kuleler + dost birimler + hero'dan yeniden hesaplar. */
void UpdateFogOfWar(Game *g) {
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
        if (!t->active) continue;
        int cr = t->gridY, cc = t->gridX;
        int vr = (int)(t->range / CELL_SIZE) + 1;
        for (int dr = -vr; dr <= vr; dr++) {
            for (int dc = -vr; dc <= vr; dc++) {
                int nr = cr + dr, nc = cc + dc;
                if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS) continue;
                if (dr*dr + dc*dc <= vr*vr) {
                    g->fogVisible[nr][nc] = true;
                    g->fogExplored[nr][nc] = true;
                }
            }
        }
    }

    /* Dost birimlerden görüş (80px = ~7 hücre) */
    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
        FriendlyUnit *fu = &g->friendlyUnits[i];
        if (!fu->active) continue;
        int cr = (int)(fu->position.y) / CELL_SIZE;
        int cc = (int)(fu->position.x) / CELL_SIZE;
        int vr = 7;
        for (int dr = -vr; dr <= vr; dr++) {
            for (int dc = -vr; dc <= vr; dc++) {
                int nr = cr + dr, nc = cc + dc;
                if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS) continue;
                if (dr*dr + dc*dc <= vr*vr) {
                    g->fogVisible[nr][nc] = true;
                    g->fogExplored[nr][nc] = true;
                }
            }
        }
    }

    /* Hero'dan görüş (120px = ~10 hücre) */
    if (g->hero.alive) {
        int cr = (int)(g->hero.position.y) / CELL_SIZE;
        int cc = (int)(g->hero.position.x) / CELL_SIZE;
        int vr = 10;
        for (int dr = -vr; dr <= vr; dr++) {
            for (int dc = -vr; dc <= vr; dc++) {
                int nr = cr + dr, nc = cc + dc;
                if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS) continue;
                if (dr*dr + dc*dc <= vr*vr) {
                    g->fogVisible[nr][nc] = true;
                    g->fogExplored[nr][nc] = true;
                }
            }
        }
    }
}

/* Fog overlay: karanlık hücrelere yarı saydam siyah dikdörtgen çizer. */
void DrawFogOverlay(Game *g) {
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (g->fogVisible[r][c]) continue; /* Görünür: fog yok */
            int x = GRID_OFFSET_X + c * CELL_SIZE;
            int y = GRID_OFFSET_Y + r * CELL_SIZE;
            if (g->fogExplored[r][c]) {
                /* Keşfedilmiş ama şu an görünmüyor: hafif sis (%55) */
                DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, (Color){7, 7, 10, 140});
            } else {
                /* Hiç keşfedilmemiş: koyu karanlık (%78) */
                DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, (Color){7, 7, 10, 200});
            }
        }
    }
}

/* ============================================================
 * T70 — Minimap (ekranın sol altında, 200×120 px)
 * ============================================================ */

void DrawMinimap(Game *g) {
    int mmW = 200, mmH = 130;
    int mmX = 10, mmY = SCREEN_HEIGHT - mmH - 10;
    float scaleX = (float)mmW / (GRID_COLS * CELL_SIZE);
    float scaleY = (float)mmH / (GRID_ROWS * CELL_SIZE);

    /* Arka plan */
    DrawRectangle(mmX - 2, mmY - 2, mmW + 4, mmH + 4, (Color){200, 175, 80, 200});
    DrawRectangle(mmX, mmY, mmW, mmH, (Color){7, 7, 10, 220});

    /* Grid hücreleri */
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            int px = mmX + (int)(c * CELL_SIZE * scaleX);
            int py = mmY + (int)(r * CELL_SIZE * scaleY);
            int pw = (int)(CELL_SIZE * scaleX) + 1;
            int ph = (int)(CELL_SIZE * scaleY) + 1;
            Color mc;
            if (!g->fogExplored[r][c]) continue; /* Keşfedilmemiş: çizme */
            switch (g->grid[r][c]) {
                case CELL_PATH:      mc = (Color){160, 130, 90, 200}; break;
                case CELL_BUILDABLE: mc = (Color){50, 80, 50, 150}; break;
                case CELL_TOWER:     mc = (Color){60, 120, 200, 255}; break;
                case CELL_VILLAGE:   mc = (Color){200, 160, 80, 255}; break;
                default:             mc = (Color){25, 40, 25, 100}; break;
            }
            DrawRectangle(px, py, pw, ph, mc);
        }
    }

    /* Düşmanlar (kırmızı noktalar — sadece görünürse) */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g->enemies[i];
        if (!e->active) continue;
        int er = (int)(e->position.y) / CELL_SIZE;
        int ec = (int)(e->position.x) / CELL_SIZE;
        if (er >= 0 && er < GRID_ROWS && ec >= 0 && ec < GRID_COLS && g->fogVisible[er][ec]) {
            int ex = mmX + (int)(e->position.x * scaleX);
            int ey = mmY + (int)(e->position.y * scaleY);
            DrawCircle(ex, ey, 2, RED);
        }
    }

    /* Kamera viewport çerçevesi */
    float camL = g->camera.cam.target.x - (SCREEN_WIDTH / 2.0f) / g->camera.zoom;
    float camT = g->camera.cam.target.y - (SCREEN_HEIGHT / 2.0f) / g->camera.zoom;
    float camW = SCREEN_WIDTH / g->camera.zoom;
    float camH = SCREEN_HEIGHT / g->camera.zoom;
    DrawRectangleLines(
        mmX + (int)(camL * scaleX),
        mmY + (int)(camT * scaleY),
        (int)(camW * scaleX),
        (int)(camH * scaleY),
        WHITE);
}

/* ============================================================
 * DrawGame — Ana Çizim Sırası (T08 + Katmanlar)
 * ============================================================ */

void DrawGame(Game *g) {
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
    DrawFogOverlay(g);  /* T70 — Fog en üstte */

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
void DrawDialogueBox(Game *g) {
    DialogueSystem *d = &g->dialogue;
    if (!d->active || d->currentLine >= d->lineCount) return;

    DialogueLine *line = &d->lines[d->currentLine];

    int bx = 40, by = SCREEN_HEIGHT - 210, bw = SCREEN_WIDTH - 80, bh = 160;
    DrawRectangle(bx, by, bw, bh, (Color){8, 8, 18, 215});
    DrawRectangleLinesEx((Rectangle){(float)bx,(float)by,(float)bw,(float)bh},
                         2, (Color){180, 160, 80, 210});

    /* Portreye yer — TODO: DrawTexture(portraitTexture, bx+8, by+8, WHITE) */
    DrawRectangle(bx+8, by+8, 80, 80, (Color){35, 35, 55, 210});
    DrawRectangleLines(bx+8, by+8, 80, 80, (Color){100, 100, 140, 180});
    DrawText("?", bx + 34, by + 28, 32, GRAY); /* Portrenin yerinde placeholder */
    DrawText("[portré]", bx + 14, by + 66, 11, (Color){90,90,90,180});

    /* Konuşmacı adı */
    DrawText(line->speaker, bx + 102, by + 10, 20, line->speakerColor);

    /* Typewriter metni — visibleChars'a kadar kes */
    char buf[512];
    int len = (int)strlen(line->text);
    int vis = (d->visibleChars < len) ? d->visibleChars : len;
    if (vis > 511) vis = 511; // prevent buffer overflow
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
void DrawWorldMap(Game *g) {
    /* Arka plan — TODO: DrawTexture(g->assets.worldMapBg, 0, 0, WHITE) */
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){20, 35, 60, 255});
    /* Basit dekoratif ızgara — atmosfer */
    for (int x = 0; x < SCREEN_WIDTH; x += 64)
        DrawLine(x, 0, x, SCREEN_HEIGHT, (Color){255,255,255,8});
    for (int y = 0; y < SCREEN_HEIGHT; y += 64)
        DrawLine(0, y, SCREEN_WIDTH, y, (Color){255,255,255,8});

    const char *title = "DUNYA HARITASI";
    DrawText(title, SCREEN_WIDTH/2 - MeasureText(title, 40)/2, 24, 40, GOLD);
    DrawText("Bir seviye secin",
             SCREEN_WIDTH/2 - MeasureText("Bir seviye secin", 20)/2, 72, 20, LIGHTGRAY);

    /* Level node'ları arasındaki yol çizgisi */
    for (int i = 0; i + 1 < MAX_LEVELS; i++) {
        LevelData *a = &g->levels[i], *b = &g->levels[i + 1];
        Color lc = b->unlocked ? (Color){160,130,60,210} : (Color){60,60,60,120};
        /* Kesik çizgi: kilitli segmentler */
        if (!b->unlocked) {
            for (int s = 0; s < 8; s++) {
                float t0 = s / 8.0f, t1 = (s + 0.5f) / 8.0f;
                Vector2 p0 = {a->nodeX + (b->nodeX-a->nodeX)*t0,
                               a->nodeY + (b->nodeY-a->nodeY)*t0};
                Vector2 p1 = {a->nodeX + (b->nodeX-a->nodeX)*t1,
                               a->nodeY + (b->nodeY-a->nodeY)*t1};
                DrawLineV(p0, p1, lc);
            }
        } else {
            DrawLine(a->nodeX, a->nodeY, b->nodeX, b->nodeY, lc);
        }
    }

    Vector2 mp = GetMousePosition();
    for (int i = 0; i < MAX_LEVELS; i++) {
        LevelData *lv = &g->levels[i];
        bool hover = Vec2Distance(mp, (Vector2){(float)lv->nodeX,(float)lv->nodeY}) < 32.0f;
        Color nc    = lv->unlocked ? (hover ? YELLOW : GOLD) : (Color){70,70,70,200};

        /* Dış halka (seçim efekti) */
        if (hover && lv->unlocked)
            DrawCircleLines(lv->nodeX, lv->nodeY, 36, (Color){255,220,0,180});

        DrawCircle(lv->nodeX, lv->nodeY, 28, nc);
        DrawCircleLines(lv->nodeX, lv->nodeY, 28,
                        lv->unlocked ? WHITE : (Color){90,90,90,200});

        /* Yıldızlar (üstte) */
        for (int s = 0; s < 3; s++) {
            Color sc = (s < lv->stars) ? GOLD : (Color){50,50,50,180};
            DrawCircle(lv->nodeX - 18 + s * 18, lv->nodeY - 38, 6, sc);
        }

        /* Level numarası */
        char num[4]; snprintf(num, sizeof(num), "%d", i + 1);
        DrawText(num, lv->nodeX - MeasureText(num, 22)/2, lv->nodeY - 11, 22,
                 lv->unlocked ? BLACK : DARKGRAY);

        /* Level adı (altında) */
        DrawText(lv->name, lv->nodeX - MeasureText(lv->name, 13)/2,
                 lv->nodeY + 34, 13, lv->unlocked ? WHITE : DARKGRAY);

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
void DrawLevelBriefing(Game *g) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){8, 12, 25, 255});

    LevelData *lv = &g->levels[g->currentLevel];

    /* Başlık */
    DrawText(lv->name, SCREEN_WIDTH/2 - MeasureText(lv->name, 38)/2, 28, 38, GOLD);

    /* Harita açıklaması */
    DrawText(lv->description, 50, 88, 18, (Color){200,200,220,230});

    /* Düşman lore */
    DrawText(lv->enemyLore, 50, 116, 15, (Color){180,140,140,200});

    /* Asset placeholder bilgi bandı */
    DrawRectangle(0, 145, SCREEN_WIDTH, 22, (Color){60,30,30,180});
    DrawText("TODO: harita arkaplan sprite + müzik",
             SCREEN_WIDTH/2 - 180, 148, 14, (Color){200,100,100,220});

    /* Diyalog kutusu */
    DrawDialogueBox(g);

    /* BAŞLA butonu (diyalog bittiyse) */
    if (!g->dialogue.active) {
        Rectangle btn = {SCREEN_WIDTH/2.0f - 100, (float)SCREEN_HEIGHT - 100, 200, 50};
        DrawButton(btn, "BASLA!", GREEN, BLACK);
    }

    DrawText("[SPACE / Tik] ileri",
             SCREEN_WIDTH - 230, SCREEN_HEIGHT - 26, 15, (Color){120,120,120,200});
}

/* ============================================================
 * DrawLoading — Sahte progress bar + ipucu + spinner
 * ============================================================ */

/* TODO: DrawTexture(levelArtwork, ...) ile level görseli arkaplan */
void DrawLoading(Game *g) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){8, 8, 18, 255});

    const char *title = "RULER";
    DrawText(title, SCREEN_WIDTH/2 - MeasureText(title, 60)/2, 150, 60, GOLD);

    if (g->currentLevel >= 0 && g->currentLevel < MAX_LEVELS) {
        const char *lvName = g->levels[g->currentLevel].name;
        DrawText(lvName, SCREEN_WIDTH/2 - MeasureText(lvName, 30)/2, 228, 30, WHITE);
    }

    /* Progress bar */
    int bx = SCREEN_WIDTH/2 - 260, by = SCREEN_HEIGHT/2, bw = 520, bh = 22;
    DrawRectangle(bx, by, bw, bh, (Color){35,35,35,200});
    DrawRectangle(bx, by, (int)(bw * g->loading.progress), bh, (Color){70,200,90,230});
    DrawRectangleLines(bx, by, bw, bh, (Color){140,140,140,180});
    char pct[16]; snprintf(pct, sizeof(pct), "%d%%", (int)(g->loading.progress * 100));
    DrawText(pct, SCREEN_WIDTH/2 - MeasureText(pct, 18)/2, by + 30, 18, LIGHTGRAY);

    /* İpucu metni */
    if (g->loading.tipText)
        DrawText(g->loading.tipText,
                 SCREEN_WIDTH/2 - MeasureText(g->loading.tipText, 16)/2,
                 by + 70, 16, (Color){180,180,180,200});

    /* Dönen spinner (animasyon placeholder) */
    float angle = g->loading.timer * 180.0f;
    int sx = SCREEN_WIDTH/2, sy = by + 120, sr = 18;
    DrawCircleLines(sx, sy, (float)sr, (Color){60,100,180,160});
    float ar = angle * DEG2RAD;
    DrawCircle((int)(sx + cosf(ar)*sr), (int)(sy + sinf(ar)*sr), 4,
               (Color){100,180,255,220});
    /* TODO: Spinner yerine dönen emblem sprite animasyonu */
}

/* ============================================================
 * DrawLevelComplete — Yıldız / ödül ekranı
 * ============================================================ */

void DrawLevelComplete(Game *g) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){4, 18, 4, 245});

    const char *t1 = "SEVIYE TAMAMLANDI!";
    DrawText(t1, SCREEN_WIDTH/2 - MeasureText(t1, 52)/2, 100, 52, GOLD);

    /* Level adı */
    if (g->currentLevel >= 0 && g->currentLevel < MAX_LEVELS)
        DrawText(g->levels[g->currentLevel].name,
                 SCREEN_WIDTH/2 - MeasureText(g->levels[g->currentLevel].name, 26)/2,
                 162, 26, WHITE);

    /* Yıldızlar */
    int stars = g->levelStars;
    for (int s = 0; s < 3; s++) {
        int cx = SCREEN_WIDTH/2 - 80 + s * 80;
        Color fc = (s < stars) ? GOLD : (Color){50,50,50,180};
        DrawCircle(cx, 240, 28, fc);
        DrawCircleLines(cx, 240, 28, (s < stars) ? YELLOW : GRAY);
        DrawText("*", cx - 8, 228, 28, (s < stars) ? BLACK : DARKGRAY);
    }

    /* Skor satırı */
    char buf[80];
    snprintf(buf, sizeof(buf), "Skor: %d   Kill: %d   Can: %d",
             g->score, g->enemiesKilled, g->lives);
    DrawText(buf, SCREEN_WIDTH/2 - MeasureText(buf, 22)/2, 298, 22, WHITE);

    /* Completion diyaloğu */
    DrawDialogueBox(g);

    /* Devam butonu (diyalog bittiyse) */
    if (!g->dialogue.active) {
        bool isLast = (g->currentLevel + 1 >= MAX_LEVELS);
        Rectangle btn = {SCREEN_WIDTH/2.0f - 100, (float)SCREEN_HEIGHT - 120, 200, 55};
        DrawButton(btn, isLast ? "KAMPANYA BITTI!" : "Haritaya Don",
                   isLast ? GOLD : DARKGREEN, isLast ? BLACK : WHITE);
    }
}

/* ============================================================
 * B02 — DrawPathArrows: Yol üzerinde yön okları
 * ============================================================ */

/* Waypoint segmentleri ortasına küçük üçgen oklar çizer. */
void DrawPathArrows(Game *g) {
    for (int i = 0; i + 1 < g->waypointCount; i++) {
        Vector2 from = g->waypoints[i];
        Vector2 to   = g->waypoints[i + 1];
        Vector2 mid  = {(from.x + to.x) / 2.0f, (from.y + to.y) / 2.0f};
        Vector2 dir  = Vec2Normalize(Vec2Subtract(to, from));
        Vector2 perp = {-dir.y, dir.x};
        float   s    = 9.0f;
        Vector2 tip  = {mid.x + dir.x * s,    mid.y + dir.y * s};
        Vector2 left = {mid.x - dir.x * s + perp.x * s * 0.55f,
                        mid.y - dir.y * s + perp.y * s * 0.55f};
        Vector2 right= {mid.x - dir.x * s - perp.x * s * 0.55f,
                        mid.y - dir.y * s - perp.y * s * 0.55f};
        DrawTriangle(tip, left, right, (Color){255, 255, 255, 90});
    }
}

/* ============================================================
 * B01 — DrawContextMenu: Kule sağ tık menüsü
 * ============================================================ */

/* Seçili kulenin yanında yükselt/sat seçeneklerini çizer. */
void DrawContextMenu(Game *g) {
    if (!g->contextMenuOpen || g->contextMenuTowerIdx < 0) return;
    Tower *t = &g->towers[g->contextMenuTowerIdx];
    if (!t->active) { g->contextMenuOpen = false; return; }

    float mx = g->contextMenuPos.x;
    float my = g->contextMenuPos.y;

    DrawRectangle((int)mx - 2, (int)my - 2, 144, 80, (Color){15, 15, 25, 230});
    DrawRectangleLines((int)mx - 2, (int)my - 2, 144, 80, (Color){200, 200, 200, 160});

    Rectangle upgradeBtn = {mx, my,      140, 34};
    Rectangle sellBtn    = {mx, my + 38, 140, 34};

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
    (void)bg; (void)fg;
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
            slot = i; break;
        }
    }
    if (slot < 0 && btnSlotCount < MAX_BTN_SLOTS) {
        slot = btnSlotCount++;
        btnKeys[slot]  = btn;
        btnAnims[slot] = (BtnAnim){1.0f, 0.0f};
    }
    if (slot < 0) {
        /* Overflow fallback: basit çizim */
        DrawRectangleRec(btn, (Color){60,45,0,200});
        DrawRectangleLinesEx(btn, 1.5f, UI_GOLD);
        DrawText(label, (int)(btn.x + btn.width/2 - MeasureText(label,18)/2),
                 (int)(btn.y + btn.height/2 - 9), 18, UI_IVORY);
        return;
    }

    float dt = GetFrameTime();
    DrawEpicButton(btn, label, &btnAnims[slot], dt);
}

/* ============================================================
 * T43 — UpdatePrepPhase / DrawPrepPhase
 * ============================================================ */

/* Dalga arası inşa fazı: bina yerleştirme + geri sayım */
void UpdatePrepPhase(Game *g, float dt) {
    g->prepTimer -= dt;

    /* 4/5/6: bina tipi seç */
    if (IsKeyPressed(KEY_FOUR))  g->selectedBuilding = BUILDING_BARRACKS;
    if (IsKeyPressed(KEY_FIVE))  g->selectedBuilding = BUILDING_MARKET;
    if (IsKeyPressed(KEY_SIX))   g->selectedBuilding = BUILDING_BARRICADE;

    /* A/S: birim sipari modu */
    if (IsKeyPressed(KEY_A))
        for (int i = 0; i < MAX_FRIENDLY_UNITS; i++)
            if (g->friendlyUnits[i].active) g->friendlyUnits[i].order = FUNIT_ATTACK;
    if (IsKeyPressed(KEY_S))
        for (int i = 0; i < MAX_FRIENDLY_UNITS; i++)
            if (g->friendlyUnits[i].active) g->friendlyUnits[i].order = FUNIT_HOLD;

    /* Sol tık: once birim yerlestirme, yoksa bina */
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mp = GetMousePosition();
        int gx, gy;
        if (!WorldToGrid(mp, &gx, &gy)) goto done_prep_click;

        if (g->pendingPlacementCount > 0) {
            if (g->grid[gy][gx] == CELL_BUILDABLE || g->grid[gy][gx] == CELL_PATH) {
                for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
                    if (g->friendlyUnits[i].active) continue;
                    InitFriendlyUnit(&g->friendlyUnits[i], g->pendingPlacementType,
                                     GridToWorld(gx, gy));
                    g->pendingPlacementCount--;
                    break;
                }
            }
            goto done_prep_click;
        }

        /* Bina yerleştir */
        if (g->buildingCount < MAX_BUILDINGS) {
            bool canPlace = (g->selectedBuilding == BUILDING_BARRICADE)
                            ? (g->grid[gy][gx] == CELL_PATH)
                            : (g->grid[gy][gx] == CELL_BUILDABLE);
            if (canPlace) {
                Building *b = &g->buildings[g->buildingCount++];
                b->gridX  = gx; b->gridY  = gy;
                b->type   = g->selectedBuilding; b->active = true;
                if (g->selectedBuilding == BUILDING_MARKET && g->gold >= 30)
                    g->gold -= 30;
            }
        }
        done_prep_click:;
    }

    /* Sağ tık: siege duvarı yerleştir */
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        Vector2 mp = GetMousePosition();
        int gx, gy;
        if (WorldToGrid(mp, &gx, &gy) && g->grid[gy][gx] == CELL_BUILDABLE)
            TryPlaceWall(&g->siege, gx, gy);
    }

    /* D tuşu: Dungeon'a gir */
    if (IsKeyPressed(KEY_D)) {
        g->preDungeonState = STATE_PREP_PHASE;
        InitDungeon(&g->dungeon, &g->hero);
        g->state = STATE_DUNGEON;
        return;
    }

    /* SPACE veya süre dolunca sonraki dalgaya geç */
    if (g->prepTimer <= 0.0f || IsKeyPressed(KEY_SPACE)) {
        g->state      = STATE_PLAYING;
        g->waveActive = true;
        OnWaveStart(&g->homeCity, &g->gold);
        /* Barracks: +40 altın bonus */
        for (int i = 0; i < g->buildingCount; i++)
            if (g->buildings[i].active && g->buildings[i].type == BUILDING_BARRACKS)
                g->gold += 40;
    }
}

void DrawPrepPhase(Game *g) {
    DrawGame(g);
    DrawHUD(g);

    /* Yarı saydam overlay */
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 20, 0, 60});

    /* Başlık */
    const char *title = "HAZIRLIK FAZI";
    DrawText(title, SCREEN_WIDTH/2 - MeasureText(title, 36)/2, 90, 36, (Color){80,255,80,230});

    /* Geri sayım */
    char timer[32];
    int secs = (int)g->prepTimer + 1;
    snprintf(timer, sizeof(timer), "%d", secs > 0 ? secs : 0);
    DrawText(timer, SCREEN_WIDTH/2 - MeasureText(timer, 56)/2, 134, 56, secs <= 5 ? RED : GOLD);

    /* Bina seçim paneli */
    const char *labels[BUILDING_TYPE_COUNT] = {
        "[4] Kışla  (+40 Altin/dalga, ücretsiz)",
        "[5] Pazar  (-maliyet, 30 Altin)",
        "[6] Barikat  (Düsman yavaşlatır)"
    };
    Color sel_colors[BUILDING_TYPE_COUNT] = { GREEN, SKYBLUE, ORANGE };
    for (int i = 0; i < BUILDING_TYPE_COUNT; i++) {
        Color bg = (g->selectedBuilding == (BuildingType)i)
                   ? (Color){60, 80, 60, 200} : (Color){30, 35, 30, 160};
        DrawRectangle(20, 110 + i * 44, 360, 40, bg);
        DrawRectangleLines(20, 110 + i * 44, 360, 40, sel_colors[i]);
        DrawText(labels[i], 30, 121 + i * 44, 14, sel_colors[i]);
    }

    /* Yerleştirilen binalar */
    for (int i = 0; i < g->buildingCount; i++) {
        Building *b = &g->buildings[i];
        if (!b->active) continue;
        int px = GRID_OFFSET_X + b->gridX * CELL_SIZE;
        int py = GRID_OFFSET_Y + b->gridY * CELL_SIZE;
        Color bc = (b->type == BUILDING_BARRACKS) ? GREEN :
                   (b->type == BUILDING_MARKET)   ? SKYBLUE : ORANGE;
        DrawRectangle(px + 4, py + 4, CELL_SIZE - 8, CELL_SIZE - 8, Fade(bc, 0.55f));
        DrawRectangleLines(px + 4, py + 4, CELL_SIZE - 8, CELL_SIZE - 8, bc);
        const char *sym = (b->type == BUILDING_BARRACKS) ? "K" :
                          (b->type == BUILDING_MARKET)   ? "P" : "B";
        DrawText(sym, px + CELL_SIZE/2 - 5, py + CELL_SIZE/2 - 8, 18, WHITE);
    }

    /* Siege duvarları */
    DrawSiegeMechanics(&g->siege);

    /* Alt talimat */
    DrawRectangle(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50, (Color){0,0,0,160});
    DrawText("[4/5/6]: Bina Sec  |  Sol Tık: Bina Koy  |  Sag Tık: Duvar Koy  |  [D]: Dungeon  |  [SPACE]: Dalga Başlat",
             30, SCREEN_HEIGHT - 34, 13, (Color){200,200,180,220});
}

/* ============================================================
 * T05 — MAIN — Game Loop
 * ============================================================ */

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Ruler");
    SetExitKey(0); // Prevent ESC from closing the game
    SetTargetFPS(60);

    Game game;
    InitGame(&game);
    InitAudioManager(&game.audio); /* T64 — pencere açıldıktan sonra, bir kez */

    while (!WindowShouldClose()) {
        float rawDt = GetFrameTime();
        float dt = rawDt * game.gameSpeed;

        /* Update */
        switch (game.state) {
            case STATE_MENU:
                UpdateMenu(&game);
                break;
            case STATE_CLASS_SELECT:
                UpdateClassSelect(&game);
                break;
            case STATE_WORLD_MAP:
                UpdateWorldMap(&game);
                break;
            case STATE_LEVEL_BRIEFING:
                UpdateLevelBriefing(&game, dt);
                break;
            case STATE_LOADING:
                UpdateLoading(&game, dt);
                break;
            case STATE_PLAYING:
                HandleInput(&game);
                UpdateEnemies(&game, dt);
                UpdateFriendlyUnits(&game, dt);
                UpdateTowers(&game, dt);
                UpdateProjectiles(&game, dt);
                UpdateParticles(&game, dt);
                UpdateFloatingTexts(&game, dt);
                UpdateScreenShake(&game, rawDt);
                UpdateGameCamera(&game, rawDt);
                UpdateFogOfWar(&game); /* T70 */
                UpdateWaves(&game, dt);
                UpdateHomeCity(&game.homeCity, dt);
                CheckGameConditions(&game);
                /* HomeCity pendingRequest: birim yerlestirme modunu ac */
                if (game.homeCity.pendingRequest >= 0) {
                    game.pendingPlacementType  = (FUnitType)(game.homeCity.pendingRequest - 1);
                    game.pendingPlacementCount = game.homeCity.pendingCount;
                    game.homeCity.pendingRequest = -1;
                    game.homeCity.pendingCount   = 0;
                }
                /* SPACE ile sonraki dalgayı başlat */
                if (!game.waveActive && game.currentWave < game.totalWaves &&
                    IsKeyPressed(KEY_SPACE)) {
                    game.waveActive = true;
                    OnWaveStart(&game.homeCity, &game.gold);
                }
                break;
            case STATE_WAVE_CLEAR:
                UpdateParticles(&game, dt);
                UpdateHomeCity(&game.homeCity, dt);
                /* HomeCity pending birim */
                if (game.homeCity.pendingRequest >= 0) {
                    game.pendingPlacementType  = (FUnitType)(game.homeCity.pendingRequest - 1);
                    game.pendingPlacementCount = game.homeCity.pendingCount;
                    game.homeCity.pendingRequest = -1;
                    game.homeCity.pendingCount   = 0;
                }
                /* D: Dungeon'a gir */
                if (IsKeyPressed(KEY_D)) {
                    game.preDungeonState = STATE_WAVE_CLEAR;
                    InitDungeon(&game.dungeon, &game.hero);
                    game.state = STATE_DUNGEON;
                    break;
                }
                /* SPACE: Prep fazına geç */
                if (IsKeyPressed(KEY_SPACE) && game.currentWave < game.totalWaves) {
                    game.state     = STATE_PREP_PHASE;
                    game.prepTimer = PREP_PHASE_DURATION;
                    OnWaveStart(&game.homeCity, &game.gold);
                } else if (IsKeyPressed(KEY_SPACE)) {
                    game.state      = STATE_PLAYING;
                    game.waveActive = true;
                    OnWaveStart(&game.homeCity, &game.gold);
                }
                break;
            case STATE_PREP_PHASE:
                UpdateParticles(&game, dt);
                UpdatePrepPhase(&game, dt);
                UpdateSiegeMechanics(&game.siege, dt);
                UpdateHomeCity(&game.homeCity, dt);
                /* HomeCity pending birim */
                if (game.homeCity.pendingRequest >= 0) {
                    game.pendingPlacementType  = (FUnitType)(game.homeCity.pendingRequest - 1);
                    game.pendingPlacementCount = game.homeCity.pendingCount;
                    game.homeCity.pendingRequest = -1;
                    game.homeCity.pendingCount   = 0;
                }
                break;
            case STATE_DUNGEON:
                UpdateDungeon(&game.dungeon, &game.hero, dt);
                /* ESC: dungeon'dan çık, altın bonus uygula */
                if (IsKeyPressed(KEY_ESCAPE)) {
                    game.gold += game.dungeon.inventory.bonusGold;
                    game.dungeon.isDungeonActive = false;
                    game.state = game.preDungeonState;
                }
                break;
            case STATE_LEVEL_COMPLETE:
                UpdateLevelComplete(&game, dt);
                break;
            case STATE_PAUSED:
                UpdatePause(&game);
                break;
            case STATE_GAMEOVER:
                /* R → seviyeyi tekrar dene, ESC → dünya haritası */
                if (IsKeyPressed(KEY_R))      RestartCurrentLevel(&game);
                if (IsKeyPressed(KEY_ESCAPE)) { game.state = STATE_WORLD_MAP; }
                break;
            case STATE_VICTORY:
                if (IsKeyPressed(KEY_R)) RestartGame(&game);
                break;
        }

        /* Draw */
        BeginDrawing();
            ClearBackground((Color){34, 40, 49, 255});
            switch (game.state) {
                case STATE_MENU:
                    DrawMenu(&game);
                    break;
                case STATE_CLASS_SELECT:
                    DrawClassSelect(&game);
                    break;
                case STATE_WORLD_MAP:
                    DrawWorldMap(&game);
                    break;
                case STATE_LEVEL_BRIEFING:
                    DrawLevelBriefing(&game);
                    break;
                case STATE_LOADING:
                    DrawLoading(&game);
                    break;
                case STATE_PLAYING: {
                    /* T63 — Kamera: zoom + hero takip + T56 ScreenShake offset */
                    Camera2D renderCam = game.camera.cam;
                    renderCam.offset.x += game.screenShake.offsetX;
                    renderCam.offset.y += game.screenShake.offsetY;
                    BeginMode2D(renderCam);
                        DrawGame(&game);
                    EndMode2D();
                    DrawHUD(&game);
                    DrawMinimap(&game); /* T70 — Minimap (ekran koordinatı) */
                    break;
                }
                case STATE_WAVE_CLEAR: {
                    DrawGame(&game);
                    DrawHUD(&game);
                    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0,0,0,100});
                    const char *wc = "DALGA TEMIZLENDI!";
                    DrawText(wc, SCREEN_WIDTH/2 - MeasureText(wc, 40)/2,
                             SCREEN_HEIGHT/2 - 60, 40, GOLD);
                    DrawText("[ SPACE ] - Hazirlik Fazi / Sonraki Dalga",
                             SCREEN_WIDTH/2 - 230, SCREEN_HEIGHT/2, 24, WHITE);
                    DrawText("[ D ] - Dungeon Moduna Gir",
                             SCREEN_WIDTH/2 - 160, SCREEN_HEIGHT/2 + 36, 20,
                             (Color){120,220,255,220});
                    break;
                }
                case STATE_PREP_PHASE:
                    DrawPrepPhase(&game);
                    break;
                case STATE_DUNGEON:
                    ClearBackground((Color){15, 10, 8, 255});
                    DrawDungeon(&game.dungeon, &game.hero);
                    break;
                case STATE_LEVEL_COMPLETE:
                    DrawLevelComplete(&game);
                    break;
                case STATE_PAUSED:
                    DrawGame(&game);
                    DrawHUD(&game);
                    DrawPauseOverlay(&game);
                    break;
                case STATE_GAMEOVER:
                    DrawGameOver(&game);
                    break;
                case STATE_VICTORY:
                    DrawVictory(&game);
                    break;
            }
        EndDrawing();
    }

    UnloadUI();
    CloseAudioManager(&game.audio); /* T64 */
    CloseWindow();
    return 0;
}
