#include "homecity.h"
#include "raylib.h"
#include <stdio.h>

/* Destek türü maliyetleri (shipmentPoints cinsinden) */
static int ReinforceCost(ReinforcementType t) {
    switch (t) {
        case REINFORCE_GOLD:    return 8;
        case REINFORCE_ARCHER:  return 10;
        case REINFORCE_WARRIOR: return 14;
        case REINFORCE_MAGE:    return 14;
        case REINFORCE_KNIGHT:  return 18;
        default: return 999;
    }
}

void InitHomeCity(HomeCity *hc) {
    hc->civLevel        = 1;
    hc->shipmentPoints  = 20;
    hc->prosperity      = 0;
    hc->menuOpen        = false;
    hc->usedThisRound   = false;
    hc->pendingRequest  = -1;
    hc->pendingCount    = 0;
    hc->pendingGoldBonus= 0;
}

/* Dalga başında: biriken altın bonusunu ekle, round hakkını sıfırla */
void OnWaveStart(HomeCity *hc, int *gold) {
    if (hc->pendingGoldBonus > 0) {
        *gold += hc->pendingGoldBonus;
        hc->pendingGoldBonus = 0;
    }
    hc->usedThisRound = false;
}

void UpdateHomeCity(HomeCity *hc, float dt) {
    (void)dt;

    if (IsKeyPressed(KEY_C)) hc->menuOpen = !hc->menuOpen;

    /* Menü kapalıysa veya bu tur hakkı kullanıldıysa işlem yok */
    if (!hc->menuOpen || hc->usedThisRound) return;

    /* 1-5 tuşlarıyla seçim */
    int sel = -1;
    if (IsKeyPressed(KEY_ONE))  sel = REINFORCE_GOLD;
    if (IsKeyPressed(KEY_TWO))  sel = REINFORCE_ARCHER;
    if (IsKeyPressed(KEY_THREE)) sel = REINFORCE_WARRIOR;
    if (IsKeyPressed(KEY_FOUR)) sel = REINFORCE_MAGE;
    if (IsKeyPressed(KEY_FIVE)) sel = REINFORCE_KNIGHT;

    if (sel < 0) return;
    if (hc->shipmentPoints < ReinforceCost((ReinforcementType)sel)) return;

    hc->shipmentPoints -= ReinforceCost((ReinforcementType)sel);
    hc->usedThisRound   = true;
    hc->menuOpen        = false;

    if (sel == REINFORCE_GOLD) {
        hc->pendingGoldBonus += 200;
    } else {
        hc->pendingRequest = sel;
        hc->pendingCount   = 3;
    }
}

void DrawHomeCityUI(HomeCity *hc, int screenWidth, int screenHeight) {
    /* Üst HUD bilgisi */
    const char *hint = hc->usedThisRound ? "[C] Destek (kullanildi)" : "[C] Destek Cagir";
    DrawText(TextFormat("Medeniyet Lv:%d  |  Sevk.Puani:%d  |  Refah:%d  |  %s",
                        hc->civLevel, hc->shipmentPoints, hc->prosperity, hint),
             screenWidth / 2 - 280, 10, 14, GOLD);

    if (!hc->menuOpen) return;

    /* Panel */
    int pw = 460, ph = 340;
    int px = (screenWidth  - pw) / 2;
    int py = (screenHeight - ph) / 2;
    DrawRectangle(px, py, pw, ph, Fade(DARKBLUE, 0.90f));
    DrawRectangleLines(px, py, pw, ph, GOLD);
    DrawText("ANA KENT DESTEGI  [C]:Kapat", px + 16, py + 14, 18, GOLD);

    if (hc->usedThisRound) {
        DrawText("Bu tur destek hakki kullanildi.", px + 16, py + 50, 15, ORANGE);
        DrawText("Bir sonraki dalgada tekrar kullanabilirsiniz.", px + 16, py + 72, 13, LIGHTGRAY);
        return;
    }

    /* Seçenekler */
    const char *labels[REINFORCE_COUNT] = {
        "[1] Altin       (8 puan)  — Sonraki dalgada +200 altin",
        "[2] Okcu x3    (10 puan)  — 3 okcu birimi yerlestirilir",
        "[3] Savasci x3 (14 puan)  — 3 savasci birimi yerlestirilir",
        "[4] Buyucu x3  (14 puan)  — 3 buyucu birimi yerlestirilir",
        "[5] Sovalye x3 (18 puan)  — 3 sovalye birimi yerlestirilir",
    };
    Color rowColors[REINFORCE_COUNT] = {GOLD, GREEN, SKYBLUE, VIOLET, ORANGE};

    for (int i = 0; i < REINFORCE_COUNT; i++) {
        int ry     = py + 100 + i * 46;
        bool afford = hc->shipmentPoints >= ReinforceCost((ReinforcementType)i);
        Color bg   = afford ? Fade(rowColors[i], 0.22f) : Fade(DARKGRAY, 0.30f);
        DrawRectangle(px + 12, ry, pw - 24, 38, bg);
        DrawRectangleLines(px + 12, ry, pw - 24, 38,
                           afford ? rowColors[i] : DARKGRAY);
        DrawText(labels[i], px + 20, ry + 10, 14,
                 afford ? WHITE : DARKGRAY);
    }
}

void EarnProsperity(HomeCity *hc, int amount) {
    hc->prosperity += amount;
    if (hc->prosperity >= 50) {
        hc->shipmentPoints += hc->prosperity / 50;
        hc->prosperity      = hc->prosperity % 50;
    }
}
