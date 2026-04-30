/* defsim.c — T102: Background Defense Simulation
 * Dungeon'dayken kule DPS + asker sayısı ile savunma gücünü hesaplar.
 * Her 10 saniyede snapshot alır, kritik eşik altında uyarı verir. */

#include "defsim.h"
#include "types.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

/* Tüm aktif kulelerin toplam DPS'ini hesapla */
static float CalcTotalTowerDPS(const Game *g) {
    float total = 0.0f;
    for (int i = 0; i < MAX_TOWERS; i++) {
        if (!g->towers[i].active) continue;
        float dps = g->towers[i].damage * g->towers[i].fireRate;
        /* Splash kuleleri etkin alana göre bonus */
        if (g->towers[i].type == TOWER_SPLASH) dps *= 1.4f;
        total += dps;
    }
    return total;
}

/* Aktif asker sayısını say */
static int CalcSoldierCount(const Game *g) {
    int count = 0;
    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++)
        if (g->friendlyUnits[i].active) count++;
    return count;
}

/* Mevcut dalganın tehdit seviyesini 0–1 aralığında hesapla */
static float CalcThreatLevel(const Game *g) {
    int aliveEnemies = 0;
    float totalHp = 0.0f;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g->enemies[i].active) continue;
        aliveEnemies++;
        totalHp += g->enemies[i].currentHp;
    }
    if (aliveEnemies == 0) return 0.0f;
    /* Tehdit: toplam düşman HP'sini savunma gücüne böl, 0–1 sınırla */
    float defense = g->defSim.lastSnapshotDPS * 5.0f
                  + (float)g->defSim.lastSnapshotSoldiers * 30.0f
                  + 1.0f;
    float threat = totalHp / defense;
    if (threat > 1.0f) threat = 1.0f;
    return threat;
}

/* Her 10 saniyede snapshot güncelle */
void DefSimTakeSnapshot(Game *g) {
    g->defSim.totalTowerDPS        = CalcTotalTowerDPS(g);
    g->defSim.totalSoldierCount    = CalcSoldierCount(g);
    g->defSim.lastSnapshotDPS      = g->defSim.totalTowerDPS;
    g->defSim.lastSnapshotSoldiers = g->defSim.totalSoldierCount;
}

/* Dungeon'dayken savunma simülasyonu ilerlet */
void DefSimUpdate(Game *g, float dt) {
    BackgroundDefSim *sim = &g->defSim;

    sim->snapshotTimer += dt;
    if (sim->snapshotTimer >= DEFSIM_SNAPSHOT_INTERVAL) {
        sim->snapshotTimer = 0.0f;
        DefSimTakeSnapshot(g);
    }

    /* Tehdit seviyesini güncelle */
    sim->threatLevel = CalcThreatLevel(g);

    /* Dungeon modundayken savunma yetersizse can kay. sim. */
    if (g->state == STATE_DUNGEON && g->waveActive) {
        float defenseScore = sim->lastSnapshotDPS / 10.0f
                           + (float)sim->lastSnapshotSoldiers * 2.0f;
        float waveStrength = (float)(g->currentWave + 1) * 3.0f;
        if (defenseScore < waveStrength) {
            /* Her 10 saniyede savunma zayıfsa 1 can kay */
            if (sim->snapshotTimer < dt * 2.0f) {
                int livesToLose = (int)((waveStrength - defenseScore) / waveStrength + 0.5f);
                if (livesToLose < 1) livesToLose = 1;
                g->lives -= livesToLose;
                sim->livesLostWhileAway += livesToLose;
                if (g->lives < 0) g->lives = 0;
            }
        }
    }

    /* Kritik uyarı: kalan can azsa veya tehdit yüksekse */
    bool nowCritical = (g->lives <= DEFSIM_CRITICAL_LIVES)
                     || (sim->threatLevel > 0.75f && g->waveActive);
    sim->criticalWarning = nowCritical;
    if (nowCritical)
        sim->critWarnTimer += dt;
    else
        sim->critWarnTimer = 0.0f;
}

/* Yeni bir ulak birimi oluştur */
void DefSimSpawnCourier(Game *g, const char *message) {
    /* Boş slot bul */
    for (int i = 0; i < MAX_COURIER_UNITS; i++) {
        if (g->courierUnits[i].active) continue;
        CourierUnit *cu = &g->courierUnits[i];
        cu->active      = true;
        cu->speed       = 120.0f;
        cu->lifetime    = 0.0f;
        cu->color       = (Color){255, 220, 80, 255};
        /* Waypoint'in başından sonuna doğru koş */
        if (g->waypointCount > 0) {
            cu->position    = g->waypoints[0];
            cu->destination = g->waypoints[g->waypointCount > 1 ? 1 : 0];
        } else {
            cu->position    = (Vector2){100, 360};
            cu->destination = (Vector2){1180, 360};
        }
        cu->angle = 0.0f;
        if (message) {
            strncpy(cu->message, message, 47);
            cu->message[47] = '\0';
        } else {
            cu->message[0] = '\0';
        }
        g->courierCount++;
        break;
    }
}

/* Kritik uyarı HUD elementi çiz */
void DefSimDrawWarning(Game *g) {
    BackgroundDefSim *sim = &g->defSim;
    if (!sim->criticalWarning) return;

    /* Kırmızı yanıp sönen uyarı bandı */
    float pulse = 0.4f + sinf(sim->critWarnTimer * 4.0f) * 0.3f;
    DrawRectangle(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50,
                  Fade((Color){200, 30, 30, 255}, pulse));

    /* Uyarı metni */
    const char *warnText = (sim->livesLostWhileAway > 0)
        ? "!! SAVUNMA KRITIK — DUNGEON'DA CAN KAYBI !!"
        : "!! SAVUNMA KRITIK — GERI DON !!";
    int tw = MeasureText(warnText, 22);
    DrawText(warnText, SCREEN_WIDTH / 2 - tw / 2, SCREEN_HEIGHT - 38, 22, WHITE);

    /* DPS bilgisi */
    char buf[64];
    snprintf(buf, sizeof(buf), "Kule DPS: %.0f  |  Asker: %d  |  Tehdit: %d%%",
             sim->lastSnapshotDPS,
             sim->lastSnapshotSoldiers,
             (int)(sim->threatLevel * 100.0f));
    int bw = MeasureText(buf, 14);
    DrawText(buf, SCREEN_WIDTH / 2 - bw / 2, SCREEN_HEIGHT - 16, 14,
             (Color){255, 200, 200, 220});
}
