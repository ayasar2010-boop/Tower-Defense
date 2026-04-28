#ifndef PROJECTILE_H
#define PROJECTILE_H
#include "types.h"
void CreateProjectile(Game *g, Vector2 origin, int targetIdx, float damage, float splash, TowerType src);
void UpdateProjectiles(Game *g, float dt);
void DrawProjectiles(Game *g);
#endif
