#ifndef DEFSIM_H
#define DEFSIM_H

/* T102 — Background Defense Simulation
 * Dungeon'dayken kule DPS + asker sayısı üzerinden savunma simülasyonu yapar. */

#include <stdbool.h>

/* Forward declaration — types.h döngüsel include önleme */
typedef struct Game Game;

void DefSimUpdate(Game *g, float dt);
void DefSimTakeSnapshot(Game *g);
void DefSimDrawWarning(Game *g);
void DefSimSpawnCourier(Game *g, const char *message);

#endif /* DEFSIM_H */
