#ifndef PARTICLE_H
#define PARTICLE_H
#include "types.h"
void SpawnParticles(Game *g, Vector2 pos, Color color, int count, float speed, float lifetime);
void UpdateParticles(Game *g, float dt);
void DrawParticles(Game *g);
#endif
