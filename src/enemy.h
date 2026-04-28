#ifndef ENEMY_H
#define ENEMY_H
#include "types.h"
void SpawnEnemy(Game *g, EnemyType type, float hpMult);
void UpdateEnemies(Game *g, float dt);
void DrawEnemies(Game *g);
#endif
