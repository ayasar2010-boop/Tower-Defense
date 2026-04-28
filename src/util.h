/* util.h — T52: Vec2 matematik + izometrik koordinat dönüşümleri */

#ifndef UTIL_H
#define UTIL_H

#include "types.h"

/* Vec2 yardımcıları */
float   Vec2Distance(Vector2 a, Vector2 b);
float   Vec2Length(Vector2 v);
Vector2 Vec2Normalize(Vector2 v);
Vector2 Vec2Subtract(Vector2 a, Vector2 b);
Vector2 Vec2Add(Vector2 a, Vector2 b);
Vector2 Vec2Scale(Vector2 v, float s);

/* İzometrik koordinat dönüşümleri */
Vector2 GridToWorld(int gx, int gy);
bool    WorldToGrid(Vector2 wp, int *gx, int *gy);

/* Kule yardımcıları */
int  GetTowerCost(TowerType type);
bool CanPlaceTower(Game *g, int gx, int gy);

#endif
