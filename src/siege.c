#include "siege.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

/* 2D dünya → izometrik ekran (45° dönüş, Y ekseninde %50 baskı) */
Vector2 WorldToIsometric(Vector2 worldPos) {
    return (Vector2){
        worldPos.x - worldPos.y,
        (worldPos.x + worldPos.y) / 2.0f
    };
}

/* İzometrik ekran → 2D dünya */
Vector2 IsometricToWorld(Vector2 isoPos) {
    return (Vector2){
        (2.0f * isoPos.y + isoPos.x) / 2.0f,
        (2.0f * isoPos.y - isoPos.x) / 2.0f
    };
}

/* Farenin izometrik grid karşılığını döndürür */
Vector2 GetIsometricMousePos(void) {
    return IsometricToWorld(GetMousePosition());
}

void InitSiegeMechanics(SiegeMechanics *s) {
    memset(s, 0, sizeof(SiegeMechanics));
}

/* Belirtilen grid hücresine duvar koyar; aynı hücre ya da limit aşımında false */
bool TryPlaceWall(SiegeMechanics *s, int gx, int gy) {
    if (s->wallCount >= MAX_WALL_SEGMENTS) return false;
    for (int i = 0; i < s->wallCount; i++) {
        if (s->walls[i].active && s->walls[i].gridX == gx && s->walls[i].gridY == gy)
            return false;
    }
    WallSegment *w = &s->walls[s->wallCount++];
    w->gridX     = gx;
    w->gridY     = gy;
    w->health    = 100;
    w->maxHealth = 100;
    w->isGate    = false;
    w->active    = true;
    return true;
}

/* Yeni bir kıta (battalion) oluşturur; birimleri 3'lü sıra düzenine dizer */
void SpawnBattalion(SiegeMechanics *s, Vector2 start, Vector2 target,
                    int units, float hpPerUnit, float speed) {
    if (s->battalionCount >= MAX_BATTALIONS) return;
    ArmyBattalion *b = &s->battalions[s->battalionCount++];
    b->worldPos      = start;
    b->targetPos     = target;
    b->activeUnits   = units;
    b->healthPerUnit = hpPerUnit;
    b->speed         = speed;
    b->active        = true;
    for (int i = 0; i < units && i < MAX_BATTALION_UNITS; i++) {
        b->unitOffsets[i] = (Vector2){
            (float)((i % 3) - 1) * 16.0f,
            (float)(i / 3)       * 16.0f
        };
    }
}

void UpdateSiegeMechanics(SiegeMechanics *s, float dt) {
    /* Kıtaları hedeflerine doğru hareket ettir */
    for (int i = 0; i < s->battalionCount; i++) {
        ArmyBattalion *b = &s->battalions[i];
        if (!b->active || b->activeUnits <= 0) continue;
        float dx  = b->targetPos.x - b->worldPos.x;
        float dy  = b->targetPos.y - b->worldPos.y;
        float len = sqrtf(dx*dx + dy*dy);
        if (len > 2.0f) {
            b->worldPos.x += (dx / len) * b->speed * dt;
            b->worldPos.y += (dy / len) * b->speed * dt;
        }
    }
    /* İstasyon bekleme sürelerini düşür */
    for (int i = 0; i < s->stationCount; i++) {
        if (s->stations[i].cooldown > 0.0f)
            s->stations[i].cooldown -= dt;
    }
}

void DrawSiegeMechanics(SiegeMechanics *s) {
    /* Duvar segmentleri */
    for (int i = 0; i < s->wallCount; i++) {
        WallSegment *w = &s->walls[i];
        if (!w->active) continue;
        int px = SIEGE_OFF_X + w->gridX * SIEGE_CELL;
        int py = SIEGE_OFF_Y + w->gridY * SIEGE_CELL;

        DrawRectangle(px, py, SIEGE_CELL, SIEGE_CELL, (Color){75, 65, 55, 210});
        DrawRectangleLines(px, py, SIEGE_CELL, SIEGE_CELL, (Color){150, 130, 100, 255});

        /* Mazgallar (merlons) */
        for (int m = 0; m < 3; m++)
            DrawRectangle(px + 4 + m * 15, py + 2, 10, 10, (Color){110, 95, 78, 255});

        /* Can barı */
        float ratio = (float)w->health / (float)w->maxHealth;
        DrawRectangle(px + 1, py + SIEGE_CELL - 6, SIEGE_CELL - 2, 5, (Color){80,0,0,180});
        DrawRectangle(px + 1, py + SIEGE_CELL - 6,
                      (int)((SIEGE_CELL - 2) * ratio), 5, (Color){20,200,20,220});
    }

    /* Kıtalar */
    for (int i = 0; i < s->battalionCount; i++) {
        ArmyBattalion *b = &s->battalions[i];
        if (!b->active || b->activeUnits <= 0) continue;
        for (int u = 0; u < b->activeUnits && u < MAX_BATTALION_UNITS; u++) {
            Vector2 p = {
                b->worldPos.x + b->unitOffsets[u].x,
                b->worldPos.y + b->unitOffsets[u].y
            };
            DrawCircle((int)p.x, (int)p.y, 7, (Color){180, 50, 50, 220});
            DrawCircleLines((int)p.x, (int)p.y, 7, (Color){255, 100, 80, 255});
        }
        char label[24];
        snprintf(label, sizeof(label), "Kita %d", b->activeUnits);
        DrawText(label, (int)b->worldPos.x - 22, (int)b->worldPos.y - 24, 11, RED);
    }

    /* Birlik istasyonları */
    for (int i = 0; i < s->stationCount; i++) {
        UnitStation *st = &s->stations[i];
        if (!st->active) continue;
        Color c = (st->type == STATION_ARCHER)  ? GREEN  :
                  (st->type == STATION_CANNON)   ? ORANGE : SKYBLUE;
        DrawCircle((int)st->position.x, (int)st->position.y, 9, c);
        DrawCircleLines((int)st->position.x, (int)st->position.y, 9, WHITE);
    }
}
