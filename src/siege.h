#ifndef SIEGE_H
#define SIEGE_H

#include "raylib.h"
#include <stdbool.h>

/* T42 — Isometric View & Siege Mechanics */

#define MAX_WALL_SEGMENTS   20
#define MAX_UNIT_STATIONS   10
#define MAX_BATTALIONS       5
#define MAX_BATTALION_UNITS 10

/* Harita grid sabitleri (main.c ile senkron) */
#define SIEGE_CELL      48
#define SIEGE_OFF_X     160
#define SIEGE_OFF_Y     80

typedef struct {
    int   gridX, gridY;
    int   health, maxHealth;
    bool  isGate;
    bool  active;
} WallSegment;

typedef enum { STATION_ARCHER, STATION_CANNON, STATION_CAVALRY } UnitStationType;

typedef struct {
    Vector2         position;
    UnitStationType type;
    bool            occupied;
    float           cooldown;
    bool            active;
} UnitStation;

/* Kıta (horde) tabanlı düşman — birden fazla birimi temsil eder */
typedef struct {
    Vector2 worldPos;
    Vector2 targetPos;
    int     activeUnits;
    float   healthPerUnit;
    float   speed;
    Vector2 unitOffsets[MAX_BATTALION_UNITS];
    bool    active;
} ArmyBattalion;

typedef struct {
    WallSegment   walls[MAX_WALL_SEGMENTS];
    int           wallCount;
    UnitStation   stations[MAX_UNIT_STATIONS];
    int           stationCount;
    ArmyBattalion battalions[MAX_BATTALIONS];
    int           battalionCount;
    bool          placingWall;
} SiegeMechanics;

Vector2 WorldToIsometric(Vector2 worldPos);
Vector2 IsometricToWorld(Vector2 isoPos);
Vector2 GetIsometricMousePos(void);

void InitSiegeMechanics(SiegeMechanics *s);
void UpdateSiegeMechanics(SiegeMechanics *s, float dt);
void DrawSiegeMechanics(SiegeMechanics *s);
bool TryPlaceWall(SiegeMechanics *s, int gx, int gy);
void SpawnBattalion(SiegeMechanics *s, Vector2 start, Vector2 target,
                    int units, float hpPerUnit, float speed);

#endif
