/* Tower Defense — C99 + Raylib
 * Derleme: gcc src/main.c -lraylib -lm -lpthread -ldl -lrt -o game
 * Windows: gcc src/main.c -lraylib -lopengl32 -lgdi32 -lwinmm -o game.exe
 */

#include <raylib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================
 * T03 — SABİTLER VE KONFİGÜRASYON
 * ============================================================ */

#define SCREEN_WIDTH            1280
#define SCREEN_HEIGHT           720
#define GRID_COLS               20
#define GRID_ROWS               12
#define CELL_SIZE               48
#define GRID_OFFSET_X           ((SCREEN_WIDTH - GRID_COLS * CELL_SIZE) / 2)
#define GRID_OFFSET_Y           80

#define MAX_ENEMIES             50
#define MAX_TOWERS              30
#define MAX_PROJECTILES         100
#define MAX_PARTICLES           200
#define MAX_WAYPOINTS           20
#define MAX_WAVES               10

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

/* ============================================================
 * T03 — ENUM TANIMLARI
 * ============================================================ */

typedef enum {
    CELL_EMPTY    = 0,
    CELL_PATH     = 1,
    CELL_BUILDABLE= 2,
    CELL_TOWER    = 3
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
    ENEMY_TYPE_COUNT
} EnemyType;

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_WAVE_CLEAR,
    STATE_GAMEOVER,
    STATE_VICTORY
} GameState;

/* ============================================================
 * T04 — STRUCT TANIMLARI
 * ============================================================ */

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
    EnemyType enemyType;
    int       enemyCount;
    float     spawnInterval;
    float     spawnTimer;
    int       spawnedCount;
    float     preDelay;
    bool      started;
} GameWave;

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
} Game;

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================ */

/* Init */
void InitGame(Game *g);
void InitMap(Game *g);
void InitWaypoints(Game *g);
void InitWaves(Game *g);
void RestartGame(Game *g);

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

/* Yardımcılar */
float   Vec2Distance(Vector2 a, Vector2 b);
Vector2 Vec2Normalize(Vector2 v);
Vector2 Vec2Subtract(Vector2 a, Vector2 b);
Vector2 Vec2Scale(Vector2 v, float s);
Vector2 Vec2Add(Vector2 a, Vector2 b);
float   Vec2Length(Vector2 v);

Vector2 GridToWorld(int gx, int gy);
bool    WorldToGrid(Vector2 wp, int *gx, int *gy);

void SpawnEnemy(Game *g, EnemyType type);
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
 * T06 — InitMap: 20×12 Hardcoded Harita
 * ============================================================ */

/* Haritayı başlatır: yol (1), buildable (2) ve boş (0) hücreleri atar.
 * Yol: sol kenardan sağa, 3 virajlı S benzeri rota. */
void InitMap(Game *g) {
    /* Tümünü boş başlat */
    memset(g->grid, CELL_EMPTY, sizeof(g->grid));

    /* Yol tanımı — satır bazlı (row, colStart, colEnd) */
    /* Segment 1: üst yatay  — satır 2, sütun 0..7   */
    for (int c = 0; c <= 7; c++)  g->grid[2][c] = CELL_PATH;
    /* Segment 2: sol dikey  — sütun 7, satır 2..6   */
    for (int r = 2; r <= 6; r++)  g->grid[r][7] = CELL_PATH;
    /* Segment 3: orta yatay — satır 6, sütun 7..13  */
    for (int c = 7; c <= 13; c++) g->grid[6][c] = CELL_PATH;
    /* Segment 4: sağ dikey  — sütun 13, satır 6..9  */
    for (int r = 6; r <= 9; r++)  g->grid[r][13] = CELL_PATH;
    /* Segment 5: alt yatay  — satır 9, sütun 13..19 */
    for (int c = 13; c <= 19; c++) g->grid[9][c] = CELL_PATH;

    /* Buildable hücreler: yol kenarları (CELL_EMPTY olan komşular) */
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    /* İki geçişte işaretle; önce path'ı tara, sonra buildable'a at */
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (g->grid[r][c] != CELL_PATH) continue;
            for (int d = 0; d < 4; d++) {
                int nr = r + dr[d], nc = c + dc[d];
                if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS) continue;
                if (g->grid[nr][nc] == CELL_EMPTY)
                    g->grid[nr][nc] = CELL_BUILDABLE;
            }
        }
    }
}

/* ============================================================
 * T07 — InitWaypoints
 * ============================================================ */

/* Düşmanların izleyeceği dönüş noktalarını piksel koordinatıyla doldurur. */
void InitWaypoints(Game *g) {
    g->waypointCount = 0;
    int wp[][2] = {
        {0,  2},   /* giriş — sol kenar */
        {7,  2},   /* sağa dön */
        {7,  6},   /* aşağı in */
        {13, 6},   /* sağa git */
        {13, 9},   /* aşağı in */
        {19, 9}    /* çıkış — sağ kenar */
    };
    int n = sizeof(wp) / sizeof(wp[0]);
    for (int i = 0; i < n && i < MAX_WAYPOINTS; i++) {
        g->waypoints[i] = GridToWorld(wp[i][0], wp[i][1]);
        g->waypointCount++;
    }
    /* İlk waypoint spawn noktasının soluna kaydır (görünür girişten gelsin) */
    g->waypoints[0].x = (float)GRID_OFFSET_X - CELL_SIZE;
}

/* ============================================================
 * T26 — InitWaves: 10 Dalga
 * ============================================================ */

/* Progresif zorlukla 10 dalganın tip/sayı/aralık parametrelerini atar. */
void InitWaves(Game *g) {
    GameWave w[MAX_WAVES] = {
        /* type,         count, interval, timer, spawned, preDelay, started */
        {ENEMY_NORMAL,   8,  1.2f, 0, 0, 2.0f, false},
        {ENEMY_NORMAL,  10,  1.0f, 0, 0, 2.0f, false},
        {ENEMY_FAST,     8,  0.8f, 0, 0, 2.0f, false},
        {ENEMY_NORMAL,  12,  0.9f, 0, 0, 2.0f, false},
        {ENEMY_TANK,     5,  2.0f, 0, 0, 2.0f, false},
        {ENEMY_FAST,    12,  0.6f, 0, 0, 2.0f, false},
        {ENEMY_TANK,     7,  1.8f, 0, 0, 2.0f, false},
        {ENEMY_FAST,    15,  0.5f, 0, 0, 2.0f, false},
        {ENEMY_TANK,    10,  1.5f, 0, 0, 2.0f, false},
        {ENEMY_NORMAL,  20,  0.4f, 0, 0, 2.0f, false},
    };
    memcpy(g->waves, w, sizeof(w));
    g->totalWaves   = MAX_WAVES;
    g->currentWave  = 0;
    g->waveActive   = false;
}

/* ============================================================
 * InitGame / RestartGame
 * ============================================================ */

/* Oyunu tamamen sıfırlar ve başlangıç durumuna getirir. */
void InitGame(Game *g) {
    memset(g, 0, sizeof(Game));
    g->gold              = STARTING_GOLD;
    g->lives             = STARTING_LIVES;
    g->gameSpeed         = 1.0f;
    g->state             = STATE_MENU;
    g->selectedTowerType = TOWER_BASIC;
    g->showGrid          = true;
    InitMap(g);
    InitWaypoints(g);
    InitWaves(g);
}

void RestartGame(Game *g) {
    InitGame(g);
    g->state = STATE_PLAYING;
}

/* ============================================================
 * T11 — SpawnEnemy
 * ============================================================ */

/* Boş bir düşman yuvası bulur ve belirtilen tipte düşman oluşturur. */
void SpawnEnemy(Game *g, EnemyType type) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g->enemies[i].active) continue;
        Enemy *e = &g->enemies[i];
        memset(e, 0, sizeof(Enemy));
        e->type             = type;
        e->currentWaypoint  = 1; /* Waypoints[0] spawn noktası, hedef [1]'den başlar */
        e->position         = g->waypoints[0];
        e->slowFactor       = 1.0f;
        e->active           = true;
        switch (type) {
            case ENEMY_NORMAL:
                e->maxHp   = ENEMY_BASE_HP * 1.0f;
                e->speed   = ENEMY_BASE_SPEED * 1.0f;
                e->radius  = 10.0f;
                e->color   = RED;
                break;
            case ENEMY_FAST:
                e->maxHp   = ENEMY_BASE_HP * 0.6f;
                e->speed   = ENEMY_BASE_SPEED * 1.8f;
                e->radius  = 8.0f;
                e->color   = ORANGE;
                break;
            case ENEMY_TANK:
                e->maxHp   = ENEMY_BASE_HP * 3.0f;
                e->speed   = ENEMY_BASE_SPEED * 0.6f;
                e->radius  = 14.0f;
                e->color   = DARKPURPLE;
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
    float minDist  = range + 1.0f; /* range dışından başla, böylece hiç düşman yoksa -1 kalır */
    int   bestIdx  = -1;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g->enemies[i].active) continue;
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

        /* Adım 1: Hedef waypoint'e yön vektörü */
        if (e->currentWaypoint >= g->waypointCount) {
            /* Son waypoint'i geçti — can düş, deaktive et */
            g->lives--;
            if (g->lives < 0) g->lives = 0;
            e->active = false;
            continue;
        }
        Vector2 target = g->waypoints[e->currentWaypoint];
        e->direction = Vec2Normalize(Vec2Subtract(target, e->position));

        /* Adım 2: Yavaşlama efekti */
        float effectiveSpeed = e->speed;
        if (e->slowTimer > 0.0f) {
            effectiveSpeed *= e->slowFactor;
            e->slowTimer -= dt;
            if (e->slowTimer < 0.0f) e->slowTimer = 0.0f;
        }

        /* Adım 3: Pozisyon güncelle */
        e->position = Vec2Add(e->position, Vec2Scale(e->direction, effectiveSpeed * dt));

        /* Adım 4: Hedefe ulaşıldı mı? */
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
        if (p->targetIndex >= 0 && p->targetIndex < MAX_ENEMIES)
            target = &g->enemies[p->targetIndex];

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
                    }
                }
                SpawnParticles(g, p->position, ORANGE, 12, 120.0f, 0.5f);
            } else {
                target->currentHp -= p->damage;
                SpawnParticles(g, p->position, p->color, 5, 80.0f, 0.2f);
            }

            /* Öldü mü? */
            if (target->currentHp <= 0.0f) {
                SpawnParticles(g, target->position, target->color, 10, 100.0f, 0.4f);
                /* Ödül tipi çarpanı */
                int reward = KILL_REWARD;
                if (target->type == ENEMY_FAST) reward = (int)(KILL_REWARD * 1.2f);
                if (target->type == ENEMY_TANK) reward = (int)(KILL_REWARD * 2.0f);
                g->gold += reward;
                g->score += reward * 2;
                g->enemiesKilled++;
                target->active = false;
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
    }

    /* Spawn */
    if (w->spawnedCount < w->enemyCount) {
        w->spawnTimer -= dt;
        if (w->spawnTimer <= 0.0f) {
            SpawnEnemy(g, w->enemyType);
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
            g->waveActive = false;
            g->currentWave++;
            if (g->currentWave >= g->totalWaves)
                g->state = STATE_VICTORY;
            else
                g->state = STATE_WAVE_CLEAR;
        }
    }
}

/* ============================================================
 * CheckGameConditions
 * ============================================================ */

void CheckGameConditions(Game *g) {
    if (g->lives <= 0) g->state = STATE_GAMEOVER;
}

/* ============================================================
 * HandleInput
 * ============================================================ */

/* Klavye ve fare girdisini işler: kule seçimi, yerleştirme, yükseltme ve durum değişiklikleri. */
void HandleInput(Game *g) {
    /* Kule tipi seç */
    if (IsKeyPressed(KEY_ONE))   g->selectedTowerType = TOWER_BASIC;
    if (IsKeyPressed(KEY_TWO))   g->selectedTowerType = TOWER_SNIPER;
    if (IsKeyPressed(KEY_THREE)) g->selectedTowerType = TOWER_SPLASH;

    /* Oyun hızı */
    if (IsKeyPressed(KEY_F))
        g->gameSpeed = (g->gameSpeed < 1.5f) ? 2.0f : 1.0f;

    /* Grid göster/gizle */
    if (IsKeyPressed(KEY_G))
        g->showGrid = !g->showGrid;

    /* Duraklat */
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE))
        g->state = STATE_PAUSED;

    /* Sonraki dalgayı başlat (dalga bekleme durumunda) */
    /* HandleInput sadece STATE_PLAYING'de çağrılır; WAVE_CLEAR'da bu satır işlemez */

    /* Sol tık — kule yerleştir */
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mp = GetMousePosition();
        int gx, gy;
        if (WorldToGrid(mp, &gx, &gy) && CanPlaceTower(g, gx, gy)) {
            /* Boş kule yuvası bul */
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
                /* Tip özelliklerini ata */
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
                g->gold -= GetTowerCost(t->type);
                g->grid[gy][gx] = CELL_TOWER;
                break;
            }
        }
    }

    /* Sağ tık — kule yükselt */
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        Vector2 mp = GetMousePosition();
        for (int i = 0; i < MAX_TOWERS; i++) {
            Tower *t = &g->towers[i];
            if (!t->active) continue;
            if (Vec2Distance(mp, t->position) < CELL_SIZE / 2.0f) {
                if (t->level >= 3) break; /* Maks seviye */
                int cost = GetTowerCost(t->type) * t->level;
                if (g->gold < cost) break;
                g->gold     -= cost;
                t->level++;
                t->damage   *= 1.3f;
                t->range    *= 1.1f;
                break;
            }
        }
    }
}

/* ============================================================
 * UpdateMenu / UpdatePause / UpdateWaveClear
 * ============================================================ */

void UpdateMenu(Game *g) {
    Rectangle btn = {SCREEN_WIDTH/2.0f - 80, SCREEN_HEIGHT/2.0f + 20, 160, 50};
    if (IsButtonClicked(btn)) {
        g->state      = STATE_PLAYING;
        g->waveActive = true;
    }
}

void UpdatePause(Game *g) {
    Rectangle resume = {SCREEN_WIDTH/2.0f - 90, SCREEN_HEIGHT/2.0f,      180, 50};
    Rectangle quit   = {SCREEN_WIDTH/2.0f - 90, SCREEN_HEIGHT/2.0f + 70, 180, 50};
    if (IsButtonClicked(resume)) g->state = STATE_PLAYING;
    if (IsButtonClicked(quit))   CloseWindow(); /* Basit çıkış */
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
                case CELL_PATH:      fill = (Color){160, 130, 90, 255}; break;  /* Açık kahverengi */
                case CELL_BUILDABLE: fill = (Color){50,  80,  50, 255}; break;  /* Hafif yeşil */
                case CELL_TOWER:     fill = (Color){40,  60,  40, 255}; break;  /* Kule altı */
                default:             fill = (Color){30,  50,  30, 255}; break;  /* Koyu yeşil */
            }
            DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, fill);
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

        /* Şekil: Normal=daire, Fast=üçgen, Tank=kare */
        switch (e->type) {
            case ENEMY_NORMAL:
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

        /* HP bar — sadece hasar almışsa göster */
        if (e->currentHp < e->maxHp) {
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
    /* Üst panel arkaplanı */
    DrawRectangle(0, 0, SCREEN_WIDTH, GRID_OFFSET_Y, (Color){20, 25, 35, 230});

    /* Sol: Can / Altın / Skor */
    char buf[128];
    snprintf(buf, sizeof(buf), "Can: %d   Altin: %d   Skor: %d", g->lives, g->gold, g->score);
    DrawText(buf, 10, 12, 20, WHITE);

    /* Sağ: Dalga / Hız / Öldürülen */
    snprintf(buf, sizeof(buf), "Dalga: %d/%d   Hiz: %.0fx   Kill: %d",
             g->currentWave + 1, g->totalWaves, g->gameSpeed, g->enemiesKilled);
    DrawText(buf, SCREEN_WIDTH - MeasureText(buf, 18) - 10, 12, 18, LIGHTGRAY);

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
}

/* ============================================================
 * T33 — DrawMenu
 * ============================================================ */

void DrawMenu(Game *g) {
    (void)g;
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){10, 15, 25, 255});
    const char *title = "TOWER DEFENSE";
    DrawText(title, SCREEN_WIDTH/2 - MeasureText(title, 60)/2, SCREEN_HEIGHT/2 - 100, 60, GOLD);
    DrawText("C + Raylib", SCREEN_WIDTH/2 - MeasureText("C + Raylib", 24)/2,
             SCREEN_HEIGHT/2 - 30, 24, LIGHTGRAY);
    Rectangle btn = {(float)SCREEN_WIDTH/2 - 80, (float)SCREEN_HEIGHT/2 + 20, 160, 50};
    DrawButton(btn, "BASLAT", GREEN, BLACK);
}

/* ============================================================
 * T34 — DrawPauseOverlay
 * ============================================================ */

void DrawPauseOverlay(Game *g) {
    (void)g;
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 160});
    const char *title = "DURAKLATILDI";
    DrawText(title, SCREEN_WIDTH/2 - MeasureText(title, 48)/2, SCREEN_HEIGHT/2 - 80, 48, WHITE);
    Rectangle resume = {(float)SCREEN_WIDTH/2 - 90, (float)SCREEN_HEIGHT/2,      180, 50};
    Rectangle quit   = {(float)SCREEN_WIDTH/2 - 90, (float)SCREEN_HEIGHT/2 + 70, 180, 50};
    DrawButton(resume, "DEVAM ET", DARKGREEN, WHITE);
    DrawButton(quit,   "CIK",      DARKGRAY,  WHITE);
}

/* ============================================================
 * T35 — DrawGameOver
 * ============================================================ */

void DrawGameOver(Game *g) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){10, 5, 5, 240});
    const char *t1 = "GAME OVER";
    DrawText(t1, SCREEN_WIDTH/2 - MeasureText(t1, 72)/2, SCREEN_HEIGHT/2 - 120, 72, RED);
    char buf[64];
    snprintf(buf, sizeof(buf), "Skor: %d   Kill: %d", g->score, g->enemiesKilled);
    DrawText(buf, SCREEN_WIDTH/2 - MeasureText(buf, 28)/2, SCREEN_HEIGHT/2, 28, LIGHTGRAY);
    Rectangle btn = {(float)SCREEN_WIDTH/2 - 100, (float)SCREEN_HEIGHT/2 + 60, 200, 55};
    DrawButton(btn, "Yeniden Baslat [R]", DARKGRAY, WHITE);
}

/* ============================================================
 * T36 — DrawVictory
 * ============================================================ */

void DrawVictory(Game *g) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){5, 15, 5, 240});
    const char *t1 = "TEBRIKLER!";
    DrawText(t1, SCREEN_WIDTH/2 - MeasureText(t1, 72)/2, SCREEN_HEIGHT/2 - 120, 72, GOLD);
    char buf[64];
    snprintf(buf, sizeof(buf), "Final Skor: %d", g->score);
    DrawText(buf, SCREEN_WIDTH/2 - MeasureText(buf, 32)/2, SCREEN_HEIGHT/2, 32, WHITE);
    Rectangle btn = {(float)SCREEN_WIDTH/2 - 100, (float)SCREEN_HEIGHT/2 + 60, 200, 55};
    DrawButton(btn, "Yeniden Baslat [R]", DARKGREEN, BLACK);
}

/* ============================================================
 * DrawGame — Ana Çizim Sırası (T08 + Katmanlar)
 * ============================================================ */

void DrawGame(Game *g) {
    DrawMap(g);
    DrawPlacementPreview(g);
    DrawTowers(g);
    DrawEnemies(g);
    DrawProjectiles(g);
    DrawParticles(g);
}

/* ============================================================
 * BUTTON YARDIMCILARI
 * ============================================================ */

bool IsButtonClicked(Rectangle btn) {
    Vector2 mp = GetMousePosition();
    return IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mp, btn);
}

void DrawButton(Rectangle btn, const char *label, Color bg, Color fg) {
    Vector2 mp = GetMousePosition();
    Color drawBg = bg;
    if (CheckCollisionPointRec(mp, btn)) {
        drawBg.r = (unsigned char)fminf(drawBg.r + 30, 255);
        drawBg.g = (unsigned char)fminf(drawBg.g + 30, 255);
        drawBg.b = (unsigned char)fminf(drawBg.b + 30, 255);
    }
    DrawRectangleRec(btn, drawBg);
    DrawRectangleLinesEx(btn, 1, (Color){255,255,255,80});
    int tw = MeasureText(label, 20);
    DrawText(label, (int)(btn.x + btn.width/2 - tw/2), (int)(btn.y + btn.height/2 - 10), 20, fg);
}

/* ============================================================
 * T05 — MAIN — Game Loop
 * ============================================================ */

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tower Defense - C & Raylib");
    SetTargetFPS(60);

    Game game;
    InitGame(&game);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime() * game.gameSpeed;

        /* Update */
        switch (game.state) {
            case STATE_MENU:
                UpdateMenu(&game);
                break;
            case STATE_PLAYING:
                HandleInput(&game);
                UpdateEnemies(&game, dt);
                UpdateTowers(&game, dt);
                UpdateProjectiles(&game, dt);
                UpdateParticles(&game, dt);
                UpdateWaves(&game, dt);
                CheckGameConditions(&game);
                /* SPACE ile dalga başlat (playing durumunda, dalga aktif değilse) */
                if (!game.waveActive && game.currentWave < game.totalWaves &&
                    IsKeyPressed(KEY_SPACE)) {
                    game.waveActive = true;
                }
                break;
            case STATE_WAVE_CLEAR:
                UpdateParticles(&game, dt);
                if (IsKeyPressed(KEY_SPACE)) {
                    game.state      = STATE_PLAYING;
                    game.waveActive = true;
                }
                break;
            case STATE_PAUSED:
                UpdatePause(&game);
                break;
            case STATE_GAMEOVER:
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
                case STATE_PLAYING:
                    DrawGame(&game);
                    DrawHUD(&game);
                    break;
                case STATE_WAVE_CLEAR: {
                    DrawGame(&game);
                    DrawHUD(&game);
                    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0,0,0,100});
                    const char *wc = "DALGA TEMIZLENDI!";
                    DrawText(wc, SCREEN_WIDTH/2 - MeasureText(wc, 40)/2,
                             SCREEN_HEIGHT/2 - 40, 40, GOLD);
                    DrawText("[ SPACE ] - Sonraki Dalga",
                             SCREEN_WIDTH/2 - 140, SCREEN_HEIGHT/2 + 20, 24, WHITE);
                    break;
                }
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

    CloseWindow();
    return 0;
}
