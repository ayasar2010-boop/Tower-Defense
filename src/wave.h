#ifndef WAVE_H
#define WAVE_H
#include "types.h"
void InitWaves(Game *g);
int  CalcLevelStars(Game *g);
void UpdateWaves(Game *g, float dt);
void CheckGameConditions(Game *g);
#endif
