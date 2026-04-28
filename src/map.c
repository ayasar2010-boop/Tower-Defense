#include "map.h"
#include "util.h"
#include <string.h>

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
    for (int c = 0; c <= 25; c++)
        g->grid[10][c] = CELL_PATH;
    for (int r = 10; r <= 30; r++)
        g->grid[r][25] = CELL_PATH;
    for (int c = 10; c <= 25; c++)
        g->grid[30][c] = CELL_PATH;
    for (int r = 30; r <= 45; r++)
        g->grid[r][10] = CELL_PATH;
    for (int c = 10; c <= 50; c++)
        g->grid[45][c] = CELL_PATH;
    for (int r = 45; r <= 55; r++)
        g->grid[r][50] = CELL_PATH;
    for (int c = 50; c <= 80; c++)
        g->grid[55][c] = CELL_PATH;
    for (int r = 55; r <= 65; r++)
        g->grid[r][80] = CELL_PATH;
    for (int c = 80; c <= 105; c++)
        g->grid[65][c] = CELL_PATH;
    for (int r = 65; r <= 72; r++)
        g->grid[r][105] = CELL_PATH;

    /* === KÖY (VILLAGE) — büyük köy bloğu (14×16 hücre) === */
    for (int r = 64; r <= 79; r++)
        for (int c = 106; c <= 119; c++)
            g->grid[r][c] = CELL_VILLAGE;

    /* Köy ana caddesi — yoldan köy merkezine uzanan yatay cadde */
    for (int c = 106; c <= 119; c++)
        g->grid[72][c] = CELL_VILLAGE;
    /* Köy çapraz caddesi — dikey */
    for (int r = 64; r <= 79; r++)
        g->grid[r][113] = CELL_VILLAGE;
    /* Giriş geçidi — yol ile köyü birleştir */
    for (int r = 70; r <= 74; r++)
        g->grid[r][106] = CELL_PATH;

    /* === BUILDABLE hücreler: yol etrafı (2 hücre mesafe, 8 yön) === */
    int dr8[] = {-1, 1, 0, 0, -1, -1, 1, 1};
    int dc8[] = {0, 0, -1, 1, -1, 1, -1, 1};
    for (int pass = 0; pass < 2; pass++) { /* 2 geçiş: 2 hücre derinlik */
        for (int r = 0; r < GRID_ROWS; r++) {
            for (int c = 0; c < GRID_COLS; c++) {
                if (g->grid[r][c] != CELL_PATH && g->grid[r][c] != CELL_BUILDABLE)
                    continue;
                for (int d = 0; d < 8; d++) {
                    int nr = r + dr8[d], nc = c + dc8[d];
                    if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS)
                        continue;
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
 * TERRAIN — InitTerrain
 * ============================================================ */

/* Deterministik hash: pozisyona göre sabit sözde-rastgele değer üretir */
static int TerrainHash(int r, int c) {
    unsigned int h = (unsigned int)(r * 2654435761u ^ (unsigned int)(c * 2246822519u));
    return (int)(h & 0x7fffffff);
}

/* Terrain katmanını harita üzerine yerleştirir.
 * Yalnızca CELL_RURAL hücrelerine terrain atar. */
void InitTerrain(Game *g) {
    memset(g->terrainLayer, TERRAIN_NONE, sizeof(g->terrainLayer));

    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (g->grid[r][c] != CELL_RURAL)
                continue;
            int h = TerrainHash(r, c);

            /* Dağ bölgeleri: KK köşe + SA bölgesi */
            if ((r < 14 && c < 20) || (r < 12 && c > 76 && c < 96)) {
                g->terrainLayer[r][c] = ((h & 3) == 0) ? TERRAIN_ROCK : TERRAIN_MOUNTAIN;
            }
            /* Batı ormanı */
            else if (r >= 6 && r <= 24 && c >= 26 && c <= 50) {
                int v = h % 5;
                g->terrainLayer[r][c] = (v < 3)    ? TERRAIN_TREE
                                        : (v == 3) ? TERRAIN_BUSH
                                                   : TERRAIN_NONE;
            }
            /* Güney-batı ormanı */
            else if (r >= 30 && r <= 48 && c >= 10 && c <= 26) {
                int v = h % 5;
                g->terrainLayer[r][c] = (v < 3)    ? TERRAIN_TREE
                                        : (v == 3) ? TERRAIN_BUSH
                                                   : TERRAIN_NONE;
            }
            /* Doğu ormanı (köy yakını) */
            else if (r >= 50 && r <= 68 && c >= 82 && c <= 100) {
                int v = h % 6;
                g->terrainLayer[r][c] = (v < 2)    ? TERRAIN_TREE
                                        : (v == 2) ? TERRAIN_BUSH
                                                   : TERRAIN_NONE;
            }
            /* Tarla alanları */
            else if ((r >= 16 && r <= 28 && c >= 2 && c <= 22) ||
                     (r >= 52 && r <= 64 && c >= 54 && c <= 82)) {
                g->terrainLayer[r][c] = ((h % 4) < 3) ? TERRAIN_FIELD : TERRAIN_NONE;
            }
            /* Dağınık taş ve çalı */
            else {
                int v = h % 18;
                if (v == 0)
                    g->terrainLayer[r][c] = TERRAIN_ROCK;
                else if (v <= 2)
                    g->terrainLayer[r][c] = TERRAIN_BUSH;
            }
        }
    }

    /* Nehir: köşegen bant — (r=4,c=56) → (r=28,c=72) */
    for (int i = 0; i <= 24; i++) {
        int rr = 4 + i;
        int rc = 56 + i;
        for (int dr = -1; dr <= 1; dr++) {
            int nr = rr + dr;
            int nc = rc + (dr == 0 ? -1 : 0);
            for (int dc = -1; dc <= 1; dc++) {
                int fnr = nr, fnc = nc + dc;
                if (fnr < 0 || fnr >= GRID_ROWS || fnc < 0 || fnc >= GRID_COLS)
                    continue;
                if (g->grid[fnr][fnc] == CELL_RURAL)
                    g->terrainLayer[fnr][fnc] = TERRAIN_RIVER;
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
        {0, 10},   /* giriş — sol kenar */
        {25, 10},  /* sağa dön */
        {25, 30},  /* aşağı in */
        {10, 30},  /* sola dön */
        {10, 45},  /* aşağı in */
        {50, 45},  /* sağa git */
        {50, 55},  /* aşağı in */
        {80, 55},  /* sağa git */
        {80, 65},  /* aşağı in */
        {105, 65}, /* saga git */
        {105, 72}, /* koy kapisi */
        {113, 72}  /* koy merkezi — nihai hedef */
    };
    int n = sizeof(wp) / sizeof(wp[0]);
    for (int i = 0; i < n && i < MAX_WAYPOINTS; i++) {
        g->waypoints[i] = GridToWorld(wp[i][0], wp[i][1]);
        g->waypointCount++;
    }
    /* Ilk waypoint spawn noktasini izometrik haritanin sol disina kaydir */
    g->waypoints[0].x -= (float)(ISO_HALF_W * 8);
}