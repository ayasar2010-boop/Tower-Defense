#ifndef MAP_H
#define MAP_H
#include "types.h"
void InitMap(Game *g);
void InitTerrain(Game *g);
void InitWaypoints(Game *g);
void DrawMap(Game *g);
void DrawPathArrows(Game *g);
#endif
