/* util.c — T52: Vec2 + koordinat + kule yardımcıları */

#include "util.h"
#include <math.h>

float Vec2Distance(Vector2 a, Vector2 b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

float Vec2Length(Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

Vector2 Vec2Normalize(Vector2 v) {
    float len = Vec2Length(v);
    if (len < 0.0001f) return (Vector2){0, 0};
    return (Vector2){v.x / len, v.y / len};
}

Vector2 Vec2Subtract(Vector2 a, Vector2 b) { return (Vector2){a.x - b.x, a.y - b.y}; }
Vector2 Vec2Add(Vector2 a, Vector2 b)      { return (Vector2){a.x + b.x, a.y + b.y}; }
Vector2 Vec2Scale(Vector2 v, float s)      { return (Vector2){v.x * s,   v.y * s};   }

Vector2 GridToWorld(int gx, int gy) {
    return (Vector2){(float)(ISO_ORIGIN_X + (gx - gy) * ISO_HALF_W),
                     (float)(ISO_ORIGIN_Y + (gx + gy) * ISO_HALF_H)};
}

bool WorldToGrid(Vector2 wp, int *gx, int *gy) {
    float dx  = wp.x - (float)ISO_ORIGIN_X;
    float dy  = wp.y - (float)ISO_ORIGIN_Y;
    int   col = (int)floorf((dx / (float)ISO_HALF_W + dy / (float)ISO_HALF_H) * 0.5f + 0.5f);
    int   row = (int)floorf((dy / (float)ISO_HALF_H - dx / (float)ISO_HALF_W) * 0.5f + 0.5f);
    if (col < 0 || col >= GRID_COLS || row < 0 || row >= GRID_ROWS) return false;
    *gx = col;
    *gy = row;
    return true;
}

int GetTowerCost(TowerType type) {
    switch (type) {
    case TOWER_BASIC:  return TOWER_COST_BASIC;
    case TOWER_SNIPER: return TOWER_COST_SNIPER;
    case TOWER_SPLASH: return TOWER_COST_SPLASH;
    default:           return 999;
    }
}

bool CanPlaceTower(Game *g, int gx, int gy) {
    if (gx < 0 || gx >= GRID_COLS || gy < 0 || gy >= GRID_ROWS) return false;
    if (g->grid[gy][gx] != CELL_BUILDABLE)                       return false;
    if (g->gold < GetTowerCost(g->selectedTowerType))             return false;
    return true;
}
