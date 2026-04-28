#ifndef TOWER_H
#define TOWER_H
#include "types.h"
int  FindNearestEnemy(Game *g, Vector2 pos, float range);
void UpdateTowers(Game *g, float dt);
void ApplySynergyBonuses(Game *g, int newIdx);
void DrawTowers(Game *g);
void DrawTowerSynergies(Game *g);
#endif
