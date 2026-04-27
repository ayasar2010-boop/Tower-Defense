#ifndef HOMECITY_H
#define HOMECITY_H

#include "raylib.h"
#include <stdbool.h>

/* Destek türleri — C menüsünde gösterilir */
typedef enum {
    REINFORCE_GOLD    = 0,  /* Sonraki dalgada +200 altın */
    REINFORCE_ARCHER  = 1,  /* 3 okçu birimi yerleştirme */
    REINFORCE_WARRIOR = 2,  /* 3 savaşçı birimi */
    REINFORCE_MAGE    = 3,  /* 3 büyücü birimi */
    REINFORCE_KNIGHT  = 4,  /* 3 şövalye birimi */
    REINFORCE_COUNT
} ReinforcementType;

typedef struct {
    int  civLevel;
    int  shipmentPoints;
    int  prosperity;

    bool menuOpen;
    bool usedThisRound;     /* Bölüm başına 1 seçim hakkı */
    int  pendingRequest;    /* ReinforcementType veya -1 */
    int  pendingCount;      /* Yerleştirilecek birim sayısı (3) */
    int  pendingGoldBonus;  /* Sonraki dalgada eklenecek altın */
} HomeCity;

void InitHomeCity(HomeCity *hc);
void UpdateHomeCity(HomeCity *hc, float dt);
void DrawHomeCityUI(HomeCity *hc, int screenWidth, int screenHeight);
void EarnProsperity(HomeCity *hc, int amount);
void OnWaveStart(HomeCity *hc, int *gold); /* Dalga başında gold ve reset uygular */

#endif
