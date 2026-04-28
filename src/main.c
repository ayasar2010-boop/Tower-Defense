/* Ruler — C99 + Raylib
 * Derleme: gcc src/main.c -lraylib -lm -lpthread -ldl -lrt -o game
 * Windows: gcc src/main.c -lraylib -lopengl32 -lgdi32 -lwinmm -o game.exe
 */

#include "audio.h"
#include "enemy.h"
#include "hud.h"
#include "logger.h"
#include "map.h"
#include "particle.h"
#include "projectile.h"
#include "tower.h"
#include "types.h"
#include "ui.h"
#include "util.h"
#include "input.h"
#include "wave.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * FORWARD DECLARATIONS (main.c'de kalan fonksiyonlar)
 * ============================================================ */
void InitGame(Game *g);
bool GameSave(Game *g, int slot);
bool GameLoad(Game *g, int slot);
void InitLevels(Game *g);
void RestartGame(Game *g);
void RestartCurrentLevel(Game *g);
void UpdateMenu(Game *g);
void UpdatePause(Game *g);
void UpdateWorldMap(Game *g);
void UpdateDialogue(Game *g, float dt);
void UpdateLevelBriefing(Game *g, float dt);
void UpdateLoading(Game *g, float dt);
void UpdateLevelComplete(Game *g, float dt);
void UpdatePrepPhase(Game *g, float dt);
void UpdateSettingsMenu(Game *g);
void UpdateClassSelect(Game *g);
void DrawMenu(Game *g);
void DrawPauseOverlay(Game *g);
void DrawSettingsMenu(Game *g);
void DrawGameOver(Game *g);
void DrawVictory(Game *g);
void DrawWorldMap(Game *g);
void DrawDialogueBox(Game *g);
void DrawLevelBriefing(Game *g);
void DrawLoading(Game *g);
void DrawLevelComplete(Game *g);
void DrawPrepPhase(Game *g);
void DrawClassSelect(Game *g);

void InitLevels(Game *g) {

    /* ── Level 0 : Kral Yolu ─────────────────────────────────── */
    g->levels[0].name = "Kral Yolu";
    g->levels[0].description = "Krom'un akincilari siniri gecti. Kral yolu tehlike altinda.";
    g->levels[0].enemyLore = "Kuzey Akincilari: Krom'un ucucu kuvvetleri. "
                             "Tek tek zayif, ama durdurulamazlarsa hic bitmezler.";
    g->levels[0].mapBgAsset = "assets/sprites/maps/map_01_kings_road.png"; /* TODO */
    g->levels[0].nodeX = 160;
    g->levels[0].nodeY = 420;
    g->levels[0].star3Lives = 17;
    g->levels[0].star2Lives = 10;
    g->levels[0].unlocked = true;
    g->levels[0].briefingCount = 3;
    g->levels[0].briefing[0] =
        (DialogueLine){"[Ulak]", "assets/sprites/portraits/narrator.png",
                       "Kuzey Bozkirlari'ndan General Krom'un ordusu siniri gecti.\n"
                       "Ilk dalgasi Kral Yolu'nda goruldu.",
                       LIGHTGRAY};
    g->levels[0].briefing[1] =
        (DialogueLine){"General Krom", "assets/sprites/portraits/krom.png",
                       "Hukumdar! Tahtini bana birak. Kani dokmeden teslim ol.\n"
                       "Son sans bu.",
                       (Color){200, 70, 50, 255}};
    g->levels[0].briefing[2] =
        (DialogueLine){"Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
                       "Kral Yolu'nun iki yanina kule hatlarimizi kuralim.\n"
                       "Akincilari yolda durdurmalisiniz — sahile ulasirlarsa koy yanar.",
                       (Color){220, 190, 80, 255}};
    g->levels[0].completionCount = 2;
    g->levels[0].completion[0] =
        (DialogueLine){"Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
                       "Kral Yolu tutuldu. Iyi savastiniz.", (Color){220, 190, 80, 255}};
    g->levels[0].completion[1] = (DialogueLine){"General Krom", "assets/sprites/portraits/krom.png",
                                                "Ilginc... Sanildigi kadar zayif degilsiniz.\n"
                                                "Geri cekiliyorum — simdilik.",
                                                (Color){200, 70, 50, 255}};

    /* ── Level 1 : Orman Siniri ──────────────────────────────── */
    g->levels[1].name = "Orman Siniri";
    g->levels[1].description = "Kurt Suvarileri orman yollarindan kacak geciyor.";
    g->levels[1].enemyLore = "Kurt Suvarileri: Krom'un ormanda yetismis hafif birlikleri. "
                             "Inanilmaz hizlilar — durup savunma kurmayi bilmiyorlar.";
    g->levels[1].mapBgAsset = "assets/sprites/maps/map_02_forest_border.png"; /* TODO */
    g->levels[1].nodeX = 380;
    g->levels[1].nodeY = 330;
    g->levels[1].star3Lives = 16;
    g->levels[1].star2Lives = 9;
    g->levels[1].unlocked = false;
    g->levels[1].briefingCount = 3;
    g->levels[1].briefing[0] =
        (DialogueLine){"[Ulak]", "assets/sprites/portraits/narrator.png",
                       "Krom, kuzey ormanlari boyunca ikinci bir hat actirdi.\n"
                       "Kurt Suvarileri hiz kesmeden ilerlemeye devam ediyor.",
                       LIGHTGRAY};
    g->levels[1].briefing[1] =
        (DialogueLine){"Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
                       "Bu birlikler cok hizli — normal kulelerin ates hizi yetmeyebilir.\n"
                       "Sniper kulelerinizi mesafeli konumlara kurun.",
                       (Color){220, 190, 80, 255}};
    g->levels[1].briefing[2] =
        (DialogueLine){"General Krom", "assets/sprites/portraits/krom.png",
                       "Ormanim benim evirim. Sizi agaclarin arasinda buldugumda\n"
                       "kacacak yer bulamazsiniz.",
                       (Color){200, 70, 50, 255}};
    g->levels[1].completionCount = 2;
    g->levels[1].completion[0] = (DialogueLine){
        "Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
        "Orman siniri tutuldu. Krom'un hiz avantaji islemiyor.", (Color){220, 190, 80, 255}};
    g->levels[1].completion[1] =
        (DialogueLine){"General Krom", "assets/sprites/portraits/krom.png",
                       "Savunmanizi hafife aldim. Bu hatami telafi edecegim.\n"
                       "Demir Muhafizlarim sizi ezecek.",
                       (Color){200, 70, 50, 255}};

    /* ── Level 2 : Tas Kopru ─────────────────────────────────── */
    g->levels[2].name = "Tas Kopru";
    g->levels[2].description = "Demir Muhafizlar tek gecis noktasi olan kopruye yuruyuyor.";
    g->levels[2].enemyLore = "Demir Muhafizlar: Krom'un zirh ustalarinin doktugu plaka zirh. "
                             "Oklar sekmez. Yalnizca patlama hasari onu durdurabilir.";
    g->levels[2].mapBgAsset = "assets/sprites/maps/map_03_stone_bridge.png"; /* TODO */
    g->levels[2].nodeX = 600;
    g->levels[2].nodeY = 260;
    g->levels[2].star3Lives = 15;
    g->levels[2].star2Lives = 8;
    g->levels[2].unlocked = false;
    g->levels[2].briefingCount = 3;
    g->levels[2].briefing[0] =
        (DialogueLine){"[Ulak]", "assets/sprites/portraits/narrator.png",
                       "Nehrin tek gecis noktasi Tas Kopru. Krom buradan gecerse\n"
                       "baskente giden yol acik kalir.",
                       LIGHTGRAY};
    g->levels[2].briefing[1] = (DialogueLine){"General Krom", "assets/sprites/portraits/krom.png",
                                              "Demir Muhafizlarim bu kopruyu gecmeyi haketti.\n"
                                              "Oklarin, kizginligim karsisinda is yapmayacak.",
                                              (Color){200, 70, 50, 255}};
    g->levels[2].briefing[2] =
        (DialogueLine){"Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
                       "Normal oklar zirha karsi etkisiz. Splash kuleleri on planda olsun —\n"
                       "patlama hasari onlarin tek zafiyeti.",
                       (Color){220, 190, 80, 255}};
    g->levels[2].completionCount = 2;
    g->levels[2].completion[0] =
        (DialogueLine){"[Ulak]", "assets/sprites/portraits/narrator.png",
                       "Tas Kopru tutuldu. Demir Muhafizlar nehri gecemedi.", LIGHTGRAY};
    g->levels[2].completion[1] =
        (DialogueLine){"General Krom", "assets/sprites/portraits/krom.png",
                       "... Tamam. Siz iyi savasiyorsunuz.\n"
                       "Ama savasin sonu gelmedi. Ordusu'nun tamami simdi yolda.",
                       (Color){200, 70, 50, 255}};

    /* ── Level 3 : Baskent Kapisi ────────────────────────────── */
    g->levels[3].name = "Baskent Kapisi";
    g->levels[3].description = "Krom'un tam ordusu baskente yuruyuyor. Her tur dusman var.";
    g->levels[3].enemyLore = "Krom artik ayrilmis kuvvetlerini birlestiirdi. "
                             "Akinci, Kurt Suvarisi ve Demir Muhafiz omuz omuza geliyor.";
    g->levels[3].mapBgAsset = "assets/sprites/maps/map_04_capital_gate.png"; /* TODO */
    g->levels[3].nodeX = 820;
    g->levels[3].nodeY = 370;
    g->levels[3].star3Lives = 14;
    g->levels[3].star2Lives = 7;
    g->levels[3].unlocked = false;
    g->levels[3].briefingCount = 3;
    g->levels[3].briefing[0] = (DialogueLine){"[Ulak]", "assets/sprites/portraits/narrator.png",
                                              "Krom, tum kuvvetlerini tek bir hatta topladi.\n"
                                              "Baskentin dis kapisi ilk hedef.",
                                              LIGHTGRAY};
    g->levels[3].briefing[1] =
        (DialogueLine){"General Krom", "assets/sprites/portraits/krom.png",
                       "Artik oyun bitti. Tum birlikleri kisimlara ayirmak hataymis.\n"
                       "Bu sefer ordum bir dalgada, tam gucte geliyor.",
                       (Color){200, 70, 50, 255}};
    g->levels[3].briefing[2] =
        (DialogueLine){"Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
                       "Karmisik dalga — her tur dusmana hazir olun.\n"
                       "Kule cesidinizi dengede tutun. Bu savasi kaybedersek baskent duser.",
                       (Color){220, 190, 80, 255}};
    g->levels[3].completionCount = 2;
    g->levels[3].completion[0] =
        (DialogueLine){"Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
                       "Kapi tutuldu! Krom'un buyuk saldirisi kırıldı.\n"
                       "Ama ordunun son kalintisi hala sarayda.",
                       (Color){220, 190, 80, 255}};
    g->levels[3].completion[1] = (DialogueLine){"General Krom", "assets/sprites/portraits/krom.png",
                                                "Ben... bu kadar yaklasmistim.\n"
                                                "Son kozum var. Sizi taht odasinda karsilayacagim.",
                                                (Color){200, 70, 50, 255}};

    /* ── Level 4 : Taht Odasi (Son Seviye) ───────────────────── */
    g->levels[4].name = "Taht Odasi";
    g->levels[4].description = "Krom bizzat liderlik ediyor. Son savunma — tahti koru.";
    g->levels[4].enemyLore = "Krom'un seckin muhafizlari ve kuzey ordusunun tam gucu. "
                             "Her tip, tam kapasite, durmadan geliyor.";
    g->levels[4].mapBgAsset = "assets/sprites/maps/map_05_throne_room.png"; /* TODO */
    g->levels[4].nodeX = 1040;
    g->levels[4].nodeY = 200;
    g->levels[4].star3Lives = 13;
    g->levels[4].star2Lives = 6;
    g->levels[4].unlocked = false;
    g->levels[4].briefingCount = 4;
    g->levels[4].briefing[0] =
        (DialogueLine){"[Ulak]", "assets/sprites/portraits/narrator.png",
                       "Krom artik saray koridorlarindaki son barikata kadar ilerledi.\n"
                       "Bu tahti, kralligi ve her seyi belirleyecek son savas.",
                       LIGHTGRAY};
    g->levels[4].briefing[1] =
        (DialogueLine){"General Krom", "assets/sprites/portraits/krom.png",
                       "Hukumdar. Ordumun tamami burada. Ben buradayim.\n"
                       "Artik kacinecak yer yok. Tahtini bana ver ya da onu harabelerde bul.",
                       (Color){200, 70, 50, 255}};
    g->levels[4].briefing[2] =
        (DialogueLine){"Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
                       "Bu son savas. Tum savunma hatlarimizi tam kapasite kuralim.\n"
                       "Burada yenilirsek geri donus olmaz.",
                       (Color){220, 190, 80, 255}};
    g->levels[4].briefing[3] = (DialogueLine){"[Ulak]", "assets/sprites/portraits/narrator.png",
                                              "Ruler — kralligi kurtaran tek kisi sensin.\n"
                                              "Sonu gelsin.",
                                              LIGHTGRAY};
    g->levels[4].completionCount = 3;
    g->levels[4].completion[0] = (DialogueLine){"General Krom", "assets/sprites/portraits/krom.png",
                                                "Bu... imkansiz. Ordumun tamamini yendiniz.\n"
                                                "Asla... asla boyle bir hukumdar gormedim.",
                                                (Color){200, 70, 50, 255}};
    g->levels[4].completion[1] =
        (DialogueLine){"Baskumandan Aldric", "assets/sprites/portraits/aldric.png",
                       "TEBRIKLER! General Krom'un ordusu tamamen dagitildi!\n"
                       "Arendalm Kralligi kurtuldu!",
                       (Color){220, 190, 80, 255}};
    g->levels[4].completion[2] =
        (DialogueLine){"[Ulak]", "assets/sprites/portraits/narrator.png",
                       "Ve halk Ruler'in adindan baska bir sey konusmaz oldu.\n"
                       "Krom bir daha gelmedi.",
                       LIGHTGRAY};
}

/* ============================================================
 * CalcLevelStars — Kalan cana göre yıldız hesapla
 * ============================================================ */
void InitGame(Game *g) {
    memset(g, 0, sizeof(Game));
    g->gold = STARTING_GOLD;
    g->lives = STARTING_LIVES;
    g->gameSpeed = 1.0f;
    g->state = STATE_MENU;
    g->selectedTowerType = TOWER_BASIC;
    g->showGrid = true;
    g->contextMenuTowerIdx = -1;
    g->loading.duration = 1.5f;
    InitMap(g);
    InitTerrain(g);
    InitWaypoints(g);
    InitWaves(g);
    InitLevels(g);
    InitHomeCity(&g->homeCity);
    InitSiegeMechanics(&g->siege);
    LoadSettings(&g->settings); /* T57 — settings.ini'den oku, yoksa varsayılan */
    g->selectedBuilding = BUILDING_BARRACKS;
    g->selectedBuildingType = -1;
    /* T69 — Kamera: yakin gorunum, yolun ilk kösesine merkezle */
    g->camera.zoom = 1.8f;
    g->camera.cam.zoom = 1.8f;
    g->camera.cam.target = GridToWorld(25, 10);
    g->camera.cam.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    g->camera.heroFollow = false;
    InitUI(SCREEN_WIDTH);
    /* Dost birim dizisini sifirla */
    memset(g->friendlyUnits, 0, sizeof(g->friendlyUnits));
    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++)
        g->friendlyUnits[i].engagedEnemyIdx = -1;
    g->pendingPlacementCount = 0;
    g->pendingPlacementType = FUNIT_ARCHER;
}

/* ============================================================
 * T56 — SAVE / LOAD
 * ============================================================ */

static void GameToSaveData(const Game *g, SaveData *sd) {
    sd->gold                  = g->gold;
    sd->lives                 = g->lives;
    sd->score                 = g->score;
    sd->enemiesKilled         = g->enemiesKilled;
    sd->currentWave           = g->currentWave;
    sd->totalWaves            = g->totalWaves;
    sd->waveActive            = g->waveActive;
    sd->state                 = g->state;
    sd->preDungeonState       = g->preDungeonState;
    sd->currentLevel          = g->currentLevel;
    sd->buildingCount         = g->buildingCount;
    sd->prepTimer             = g->prepTimer;
    sd->townCenterTimer       = g->townCenterTimer;
    sd->pendingPlacementCount = g->pendingPlacementCount;
    sd->pendingPlacementType  = g->pendingPlacementType;
    memcpy(sd->towers,        g->towers,        sizeof(g->towers));
    memcpy(sd->waves,         g->waves,         sizeof(g->waves));
    memcpy(sd->buildings,     g->buildings,     sizeof(g->buildings));
    memcpy(sd->friendlyUnits, g->friendlyUnits, sizeof(g->friendlyUnits));
    memcpy(&sd->homeCity,     &g->homeCity,     sizeof(g->homeCity));
    memcpy(&sd->siege,        &g->siege,        sizeof(g->siege));
    memcpy(&sd->hero,         &g->hero,         sizeof(g->hero));
    memcpy(&sd->dungeon,      &g->dungeon,      sizeof(g->dungeon));
    memcpy(sd->fogExplored,   g->fogExplored,   sizeof(g->fogExplored));
    memcpy(sd->terrainLayer,  g->terrainLayer,  sizeof(g->terrainLayer));
}

static void SaveDataToGame(const SaveData *sd, Game *g) {
    g->gold                  = sd->gold;
    g->lives                 = sd->lives;
    g->score                 = sd->score;
    g->enemiesKilled         = sd->enemiesKilled;
    g->currentWave           = sd->currentWave;
    g->totalWaves            = sd->totalWaves;
    g->waveActive            = sd->waveActive;
    g->state                 = sd->state;
    g->preDungeonState       = sd->preDungeonState;
    g->currentLevel          = sd->currentLevel;
    g->buildingCount         = sd->buildingCount;
    g->prepTimer             = sd->prepTimer;
    g->townCenterTimer       = sd->townCenterTimer;
    g->pendingPlacementCount = sd->pendingPlacementCount;
    g->pendingPlacementType  = sd->pendingPlacementType;
    memcpy(g->towers,        sd->towers,        sizeof(sd->towers));
    memcpy(g->waves,         sd->waves,         sizeof(sd->waves));
    memcpy(g->buildings,     sd->buildings,     sizeof(sd->buildings));
    memcpy(g->friendlyUnits, sd->friendlyUnits, sizeof(sd->friendlyUnits));
    memcpy(&g->homeCity,     &sd->homeCity,     sizeof(sd->homeCity));
    memcpy(&g->siege,        &sd->siege,        sizeof(sd->siege));
    memcpy(&g->hero,         &sd->hero,         sizeof(sd->hero));
    memcpy(&g->dungeon,      &sd->dungeon,      sizeof(sd->dungeon));
    memcpy(g->fogExplored,   sd->fogExplored,   sizeof(sd->fogExplored));
    memcpy(g->terrainLayer,  sd->terrainLayer,  sizeof(sd->terrainLayer));
}

/* Slot'a kaydet (0-2). true → başarı. */
bool GameSave(Game *g, int slot) {
    SaveData sd;
    GameToSaveData(g, &sd);
    bool ok = SaveRaw(&sd, sizeof(sd), slot);
    if (ok) LOG_INFO("Kaydedildi — slot %d (dalga %d, altin %d)", slot, g->currentWave, g->gold);
    else    LOG_ERR("Kayit basarisiz — slot %d", slot);
    return ok;
}

/* Slot'tan yükle. InitGame ile taze başlatır, sonra save data'yı üstüne yazar. */
bool GameLoad(Game *g, int slot) {
    SaveData sd;
    if (!LoadRaw(&sd, sizeof(sd), slot, SAVE_MAGIC, SAVE_VERSION)) {
        LOG_WARN("Yukleme basarisiz — slot %d (bos veya bozuk)", slot);
        return false;
    }
    InitGame(g);
    SaveDataToGame(&sd, g);
    LOG_INFO("Yuklendi — slot %d (dalga %d, altin %d)", slot, g->currentWave, g->gold);
    return true;
}

/* Sadece gameplay alanlarını sıfırlar — kampanya verisi ve level'lar korunur. */
void ResetGameplay(Game *g) {
    int savedLevel = g->currentLevel;
    LevelData savedLevels[MAX_LEVELS];
    memcpy(savedLevels, g->levels, sizeof(savedLevels));

    memset(g->enemies, 0, sizeof(g->enemies));
    memset(g->towers, 0, sizeof(g->towers));
    memset(g->projectiles, 0, sizeof(g->projectiles));
    memset(g->particles, 0, sizeof(g->particles));
    g->gold = STARTING_GOLD;
    g->lives = STARTING_LIVES;
    g->score = 0;
    g->enemiesKilled = 0;
    g->waveActive = false;
    g->currentWave = 0;
    g->gameSpeed = 1.0f;
    g->selectedTowerType = TOWER_BASIC;
    g->showGrid = true;
    g->contextMenuOpen = false;
    g->contextMenuTowerIdx = -1;
    g->buildingCount = 0;
    g->prepTimer = 0.0f;
    memset(g->buildings, 0, sizeof(g->buildings));
    InitMap(g);
    InitWaypoints(g);
    InitWaves(g);
    InitSiegeMechanics(&g->siege);

    g->currentLevel = savedLevel;
    memcpy(g->levels, savedLevels, sizeof(savedLevels));

    /* Dost birimleri sifirla, hero birimini yeniden olustur */
    memset(g->friendlyUnits, 0, sizeof(g->friendlyUnits));
    for (int i = 0; i < MAX_FRIENDLY_UNITS; i++)
        g->friendlyUnits[i].engagedEnemyIdx = -1;
    g->pendingPlacementCount = 0;
    g->pendingPlacementType = FUNIT_ARCHER;
    if (g->hero.heroClass != HERO_CLASS_NONE)
        SpawnHeroUnit(g);
}

/* Loading screen üzerinden mevcut level'ı yeniden başlatır. */
void RestartCurrentLevel(Game *g) {
    g->loading.progress = 0.0f;
    g->loading.timer = 0.0f;
    g->loading.duration = 1.0f;
    g->loading.nextState = STATE_PLAYING;
    g->loading.tipText = "Tekrar deniyorsunuz — bu sefer basaracaksiniz!";
    g->state = STATE_LOADING;
}

void RestartGame(Game *g) {
    InitGame(g);
    g->state = STATE_WORLD_MAP;
}

/* ============================================================
 * DOST BIRIM SISTEMI
 * ============================================================ */

/* Tipe gore baslangic istatistiklerini atar */
void UpdateMenu(Game * g) {
        Rectangle btn  = {SCREEN_WIDTH / 2.0f - 80, SCREEN_HEIGHT / 2.0f + 20, 160, 50};
        Rectangle sBtn = {SCREEN_WIDTH / 2.0f - 80, SCREEN_HEIGHT / 2.0f + 84, 160, 40};
        if (IsButtonClicked(btn)) {
            g->state = STATE_CLASS_SELECT;
        }
        /* T57 — Ayarlar butonu veya S tuşu */
        if (IsButtonClicked(sBtn) || IsKeyPressed(KEY_S)) {
            g->preSettingsState = STATE_MENU;
            g->state = STATE_SETTINGS;
        }
    }

    /* T52 — Kahraman sınıf seçim ekranı güncelleme */
    void UpdateClassSelect(Game * g) {
        /* Her sınıf için buton dikdörtgeni */
        Rectangle btns[3] = {{SCREEN_WIDTH / 2.0f - 330, SCREEN_HEIGHT / 2.0f - 60, 200, 180},
                             {SCREEN_WIDTH / 2.0f - 100, SCREEN_HEIGHT / 2.0f - 60, 200, 180},
                             {SCREEN_WIDTH / 2.0f + 130, SCREEN_HEIGHT / 2.0f - 60, 200, 180}};
        HeroClass classes[3] = {HERO_WARRIOR, HERO_MAGE, HERO_ARCHER};

        for (int i = 0; i < 3; i++) {
            if (IsButtonClicked(btns[i])) {
                InitHero(&g->hero, classes[i]);
                SpawnHeroUnit(g);
                g->state = STATE_WORLD_MAP;
                break;
            }
        }

        /* ESC → geri */
        if (IsKeyPressed(KEY_ESCAPE))
            g->state = STATE_MENU;
    }

    /* T52 — Kahraman sınıf seçim ekranı çizimi */
    void DrawClassSelect(Game * g) {
        /* Arkaplan */
        DrawGradientPanel((Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, (Color){10, 8, 20, 255},
                          (Color){20, 15, 40, 255});

        /* Başlık */
        DrawEpicTitle("KAHRAMAN SINIFI SEC", (Vector2){SCREEN_WIDTH / 2.0f - UIS(210), 60}, 28.0f);

        /* Sınıf verileri */
        const char *names[3] = {"SAVASCI", "BUYUCU", "OKCU"};
        const char *descs[3][4] = {
            {"HP: 700  Def: 20", "Hiz: 100 px/s", "Menzil: 35 (yakin)", "Kule: +15% Hasar"},
            {"HP: 350  Mana: 300", "Hiz:  90 px/s", "Menzil: 120 (buyu)", "Kule: +20% Menzil"},
            {"HP: 450  Mana: 150", "Hiz: 130 px/s", "Menzil: 160 (ok)", "Kule: +25% Atis Hz"}};
        Color bodyColors[3] = {SKYBLUE, VIOLET, LIME};
        Rectangle btns[3] = {{SCREEN_WIDTH / 2.0f - 330, SCREEN_HEIGHT / 2.0f - 60, 200, 180},
                             {SCREEN_WIDTH / 2.0f - 100, SCREEN_HEIGHT / 2.0f - 60, 200, 180},
                             {SCREEN_WIDTH / 2.0f + 130, SCREEN_HEIGHT / 2.0f - 60, 200, 180}};

        for (int i = 0; i < 3; i++) {
            Rectangle b = btns[i];
            bool hover = CheckCollisionPointRec(GetMousePosition(), b);

            /* Kart arka planı */
            DrawGradientPanel(b, UI_PANEL_TOP, UI_PANEL_BOT);
            DrawOrnateBorder(b, hover ? UI_GOLD_BRIGHT : UI_GOLD, hover ? 2.0f : 1.0f);

            /* Sınıf ikonu */
            DrawCircle((int)(b.x + b.width / 2), (int)(b.y + 35), 22, bodyColors[i]);
            DrawCircleLines((int)(b.x + b.width / 2), (int)(b.y + 35), 22, WHITE);

            /* İsim */
            int tw = MeasureText(names[i], 18);
            DrawText(names[i], (int)(b.x + b.width / 2 - tw / 2), (int)(b.y + 65), 18,
                     hover ? UI_GOLD_BRIGHT : UI_GOLD);

            /* Açıklama satırları */
            for (int d = 0; d < 4; d++) {
                DrawText(descs[i][d], (int)(b.x + 8), (int)(b.y + 95 + d * 18), 12,
                         hover ? UI_IVORY : (Color){180, 180, 200, 200});
            }

            if (hover)
                DrawText("[ Sec ]", (int)(b.x + b.width / 2 - 28), (int)(b.y + b.height - 24), 14,
                         UI_GOLD);
        }

        DrawText("ESC: Geri", 20, SCREEN_HEIGHT - 30, 14, (Color){150, 150, 170, 200});
    }

    void UpdatePause(Game * g) {
        Rectangle resume   = {SCREEN_WIDTH / 2.0f - 90, SCREEN_HEIGHT / 2.0f,      180, 50};
        Rectangle quit     = {SCREEN_WIDTH / 2.0f - 90, SCREEN_HEIGHT / 2.0f + 70, 180, 50};
        if (IsButtonClicked(resume) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))
            g->state = STATE_PLAYING;
        if (IsButtonClicked(quit))
            g->state = STATE_MENU;
        /* T57 — Ayarlar menüsüne geç */
        if (IsKeyPressed(KEY_S)) {
            g->preSettingsState = STATE_PAUSED;
            g->state = STATE_SETTINGS;
        }
    }

    /* ============================================================
     * T08 — DrawMap
     * ============================================================ */

    /* Haritayı hücre tipine göre renklendirerek çizer, opsiyonel grid çizgileriyle. */
    void DrawMap(Game * g) {
        for (int r = 0; r < GRID_ROWS; r++) {
            for (int c = 0; c < GRID_COLS; c++) {
                Color fill;
                switch (g->grid[r][c]) {
                case CELL_PATH:
                    fill = (Color){160, 130, 90, 255};
                    break;
                case CELL_BUILDABLE:
                    fill = (Color){50, 80, 50, 255};
                    break;
                case CELL_TOWER:
                    fill = (Color){40, 60, 40, 255};
                    break;
                case CELL_RURAL:
                    fill = (Color){38, 55, 28, 255};
                    break;
                case CELL_VILLAGE: {
                    bool isStreet = (r == 72) || (c == 113);
                    fill = isStreet ? (Color){140, 110, 75, 255} : (Color){100, 78, 48, 255};
                    break;
                }
                default:
                    fill = (Color){30, 50, 30, 255};
                    break;
                }
                /* Nehir: tile zemin rengi suya dönüşür (terrain override) */
                if ((TerrainType)g->terrainLayer[r][c] == TERRAIN_RIVER)
                    fill = (Color){30, 75, 130, 255};

                /* Izometrik diamond tile — 2 ucgen */
                Vector2 ctr = GridToWorld(c, r);
                Vector2 top = {ctr.x, ctr.y - (float)ISO_HALF_H};
                Vector2 right = {ctr.x + (float)ISO_HALF_W, ctr.y};
                Vector2 bot = {ctr.x, ctr.y + (float)ISO_HALF_H};
                Vector2 left = {ctr.x - (float)ISO_HALF_W, ctr.y};
                DrawTriangle(top, left, right, fill);
                DrawTriangle(right, left, bot, fill);

                /* Köy hücreleri: çit, bina, town center */
                if (g->grid[r][c] == CELL_VILLAGE) {
                    bool isStreet = (r == 72) || (c == 113);
                    bool isTownCenter = (c >= 110 && c <= 116 && r >= 69 && r <= 75 && !isStreet);
                    bool isPerimeter = false;
                    if (!isStreet) {
                        int pr[] = {r - 1, r + 1, r, r};
                        int pc[] = {c, c, c - 1, c + 1};
                        for (int d = 0; d < 4; d++) {
                            int nr2 = pr[d], nc2 = pc[d];
                            if (nr2 < 0 || nr2 >= GRID_ROWS || nc2 < 0 || nc2 >= GRID_COLS) {
                                isPerimeter = true;
                                break;
                            }
                            int ct = g->grid[nr2][nc2];
                            if (ct != CELL_VILLAGE && ct != CELL_PATH) {
                                isPerimeter = true;
                                break;
                            }
                        }
                    }
                    if (isTownCenter) {
                        /* Town center — devasa ana kent binasi */
                        DrawTriangle((Vector2){ctr.x, ctr.y - (float)ISO_HALF_H * 6.0f},
                                     (Vector2){ctr.x - (float)ISO_HALF_W * 1.5f,
                                               ctr.y - (float)ISO_HALF_H * 2.5f},
                                     (Vector2){ctr.x + (float)ISO_HALF_W * 1.5f,
                                               ctr.y - (float)ISO_HALF_H * 2.5f},
                                     (Color){200, 160, 80, 240});
                        DrawRectangle((int)(ctr.x-(float)ISO_HALF_W*1.2f),
                                      (int)(ctr.y-(float)ISO_HALF_H*2.5f),
                                      (int)(ISO_HALF_W*2.4f), (int)(ISO_HALF_H*2.5f),
                                      (Color){160, 120, 60, 200});
                    } else if (!isStreet && !isPerimeter) {
                        /* Ic koy — kucuk ev */
                        DrawTriangle((Vector2){ctr.x, ctr.y - (float)ISO_HALF_H * 2.0f},
                                     (Vector2){ctr.x - (float)ISO_HALF_W * 0.6f,
                                               ctr.y - (float)ISO_HALF_H * 0.8f},
                                     (Vector2){ctr.x + (float)ISO_HALF_W * 0.6f,
                                               ctr.y - (float)ISO_HALF_H * 0.8f},
                                     (Color){180, 60, 40, 200});
                        DrawRectangle((int)(ctr.x - (float)ISO_HALF_W * 0.5f),
                                      (int)(ctr.y - (float)ISO_HALF_H * 0.8f),
                                      (int)(ISO_HALF_W * 1.0f), (int)(ISO_HALF_H * 0.8f),
                                      (Color){140, 100, 70, 200});
                    }
                    if (isPerimeter) {
                        /* Çit direği */
                        float px2 = ctr.x, py2 = ctr.y - (float)ISO_HALF_H;
                        DrawRectangle((int)px2 - 1, (int)(py2 - (float)ISO_HALF_H * 1.2f), 2,
                                      (int)((float)ISO_HALF_H * 1.2f), (Color){100, 70, 35, 255});
                        DrawCircle((int)px2, (int)(py2 - (float)ISO_HALF_H * 1.2f), 1.5f,
                                   (Color){130, 90, 45, 255});
                    }
                }

                /* === TERRAIN ÇİZİMİ === */
                TerrainType terrain = (TerrainType)g->terrainLayer[r][c];
                if (terrain != TERRAIN_NONE) {
                    float hw = (float)ISO_HALF_W, hh = (float)ISO_HALF_H;
                    switch (terrain) {

                    case TERRAIN_MOUNTAIN: {
                        /* Dağ: 3 katmanlı konik kaya */
                        float w1 = hw * 1.4f, w2 = hw * 0.8f, w3 = hw * 0.35f;
                        float h1 = hh * 2.5f, h2 = hh * 4.5f, h3 = hh * 6.8f;
                        DrawTriangle((Vector2){ctr.x, ctr.y - h1}, (Vector2){ctr.x - w1, ctr.y},
                                     (Vector2){ctr.x + w1, ctr.y}, (Color){90, 85, 80, 255});
                        DrawTriangle(
                            (Vector2){ctr.x, ctr.y - h2}, (Vector2){ctr.x - w2, ctr.y - h1 * 0.4f},
                            (Vector2){ctr.x + w2, ctr.y - h1 * 0.4f}, (Color){115, 110, 105, 255});
                        DrawTriangle(
                            (Vector2){ctr.x, ctr.y - h3}, (Vector2){ctr.x - w3, ctr.y - h2 * 0.5f},
                            (Vector2){ctr.x + w3, ctr.y - h2 * 0.5f}, (Color){145, 140, 135, 255});
                        /* Kar başlığı */
                        DrawTriangle((Vector2){ctr.x, ctr.y - h3},
                                     (Vector2){ctr.x - w3 * 0.55f, ctr.y - h3 + hh * 1.2f},
                                     (Vector2){ctr.x + w3 * 0.55f, ctr.y - h3 + hh * 1.2f},
                                     (Color){235, 240, 255, 220});
                        break;
                    }

                    case TERRAIN_ROCK: {
                        /* Küçük taş: yuvarlak gri kaya */
                        float rr2 = hw * 0.55f;
                        DrawCircle((int)(ctr.x + hw * 0.1f), (int)(ctr.y - hh * 0.5f), rr2,
                                   (Color){105, 100, 95, 255});
                        DrawCircle((int)(ctr.x - hw * 0.25f), (int)(ctr.y), rr2 * 0.65f,
                                   (Color){85, 80, 78, 255});
                        DrawCircle((int)(ctr.x + hw * 0.1f), (int)(ctr.y - hh * 0.5f), rr2 * 0.4f,
                                   (Color){135, 130, 125, 200});
                        break;
                    }

                    case TERRAIN_RIVER: {
                        /* Nehir: tile'ı mavi yeniden boya + dalga */
                        DrawTriangle(top, left, right, (Color){45, 100, 165, 230});
                        DrawTriangle(right, left, bot, (Color){45, 100, 165, 230});
                        /* Ripple çizgisi */
                        float t2 = (float)GetTime();
                        float wave =
                            sinf(t2 * 2.0f + (float)r * 0.8f + (float)c * 0.5f) * hh * 0.25f;
                        DrawLineEx((Vector2){ctr.x - hw * 0.6f, ctr.y + wave},
                                   (Vector2){ctr.x + hw * 0.6f, ctr.y - wave}, 1.5f,
                                   (Color){130, 200, 255, 140});
                        break;
                    }

                    case TERRAIN_FIELD: {
                        /* Tarla: ekin sıraları */
                        Color fc = (Color){180, 155, 50, 200};
                        for (int row2 = 0; row2 < 3; row2++) {
                            float fy = ctr.y - hh * 0.6f + row2 * hh * 0.45f;
                            float span = hw * (0.7f - row2 * 0.15f);
                            DrawLineEx((Vector2){ctr.x - span, fy}, (Vector2){ctr.x + span, fy},
                                       1.2f, fc);
                        }
                        break;
                    }

                    case TERRAIN_TREE: {
                        /* Ağaç: gövde + konik taç */
                        float tw = 1.8f, th = hh * 2.2f, ch = hh * 4.5f;
                        float cw = hw * 0.72f;
                        /* Gövde */
                        DrawRectangle((int)(ctr.x - tw), (int)(ctr.y - th), (int)(tw * 2), (int)th,
                                      (Color){90, 55, 25, 255});
                        /* Alt yaprak katmanı */
                        DrawTriangle((Vector2){ctr.x, ctr.y - ch * 0.55f},
                                     (Vector2){ctr.x - cw, ctr.y - th * 0.3f},
                                     (Vector2){ctr.x + cw, ctr.y - th * 0.3f},
                                     (Color){35, 105, 40, 240});
                        /* Üst yaprak katmanı */
                        DrawTriangle((Vector2){ctr.x, ctr.y - ch},
                                     (Vector2){ctr.x - cw * 0.65f, ctr.y - ch * 0.45f},
                                     (Vector2){ctr.x + cw * 0.65f, ctr.y - ch * 0.45f},
                                     (Color){45, 130, 50, 240});
                        break;
                    }

                    case TERRAIN_BUSH: {
                        /* Çalı: üç iç içe daire kümesi */
                        float br = hw * 0.42f;
                        DrawCircle((int)(ctr.x), (int)(ctr.y - hh * 0.9f), br,
                                   (Color){40, 115, 35, 230});
                        DrawCircle((int)(ctr.x - hw * 0.38f), (int)(ctr.y - hh * 0.5f), br * 0.75f,
                                   (Color){50, 130, 40, 220});
                        DrawCircle((int)(ctr.x + hw * 0.38f), (int)(ctr.y - hh * 0.5f), br * 0.75f,
                                   (Color){35, 110, 30, 220});
                        DrawCircle((int)(ctr.x), (int)(ctr.y - hh * 0.9f), br * 0.45f,
                                   (Color){75, 170, 55, 180});
                        break;
                    }

                    default:
                        break;
                    }
                }

                if (g->showGrid) {
                    Color gl = (Color){255, 255, 255, 18};
                    DrawLineV(top, right, gl);
                    DrawLineV(right, bot, gl);
                    DrawLineV(bot, left, gl);
                    DrawLineV(left, top, gl);
                }
            }
        }
    }

    /* ============================================================
     * T13 — DrawEnemies
     * ============================================================ */

    /* Düşmanları tipine göre şekil ve HP bar ile çizer. */
        void UpdatePrepPhase(Game * g, float dt) {
            g->prepTimer -= dt;

            /* 4/5/6: bina tipi seç */
            if (IsKeyPressed(KEY_FOUR))
                g->selectedBuilding = BUILDING_BARRACKS;
            if (IsKeyPressed(KEY_FIVE))
                g->selectedBuilding = BUILDING_MARKET;
            if (IsKeyPressed(KEY_SIX))
                g->selectedBuilding = BUILDING_BARRICADE;

            /* A/S: birim sipari modu */
            if (IsKeyPressed(KEY_A))
                for (int i = 0; i < MAX_FRIENDLY_UNITS; i++)
                    if (g->friendlyUnits[i].active)
                        g->friendlyUnits[i].order = FUNIT_ATTACK;
            if (IsKeyPressed(KEY_S))
                for (int i = 0; i < MAX_FRIENDLY_UNITS; i++)
                    if (g->friendlyUnits[i].active)
                        g->friendlyUnits[i].order = FUNIT_HOLD;

            /* Sol tık: once birim yerlestirme, yoksa bina */
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mp = GetMousePosition();
                int gx, gy;
                if (!WorldToGrid(mp, &gx, &gy))
                    goto done_prep_click;

                if (g->pendingPlacementCount > 0) {
                    if (g->grid[gy][gx] == CELL_BUILDABLE || g->grid[gy][gx] == CELL_PATH) {
                        for (int i = 0; i < MAX_FRIENDLY_UNITS; i++) {
                            if (g->friendlyUnits[i].active)
                                continue;
                            InitFriendlyUnit(&g->friendlyUnits[i], g->pendingPlacementType,
                                             GridToWorld(gx, gy));
                            g->pendingPlacementCount--;
                            break;
                        }
                    }
                    goto done_prep_click;
                }

                /* Bina yerleştir */
                if (g->buildingCount < MAX_BUILDINGS) {
                    bool canPlace = (g->selectedBuilding == BUILDING_BARRICADE)
                                        ? (g->grid[gy][gx] == CELL_PATH)
                                        : (g->grid[gy][gx] == CELL_BUILDABLE);
                    if (canPlace) {
                        Building *b = &g->buildings[g->buildingCount++];
                        b->gridX = gx;
                        b->gridY = gy;
                        b->type = g->selectedBuilding;
                        b->active = true;
                        if (g->selectedBuilding == BUILDING_MARKET && g->gold >= 30)
                            g->gold -= 30;
                    }
                }
            done_prep_click:;
            }

            /* Sağ tık: siege duvarı yerleştir */
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                Vector2 mp = GetMousePosition();
                int gx, gy;
                if (WorldToGrid(mp, &gx, &gy) && g->grid[gy][gx] == CELL_BUILDABLE)
                    TryPlaceWall(&g->siege, gx, gy);
            }

            /* D tuşu: Dungeon'a gir */
            if (IsKeyPressed(KEY_D)) {
                g->preDungeonState = STATE_PREP_PHASE;
                InitDungeon(&g->dungeon, &g->hero);
                g->state = STATE_DUNGEON;
                return;
            }

            /* SPACE veya süre dolunca sonraki dalgaya geç */
            if (g->prepTimer <= 0.0f || IsKeyPressed(KEY_SPACE)) {
                g->state = STATE_PLAYING;
                g->waveActive = true;
                OnWaveStart(&g->homeCity, &g->gold);
                /* Barracks: +40 altın bonus */
                for (int i = 0; i < g->buildingCount; i++)
                    if (g->buildings[i].active && g->buildings[i].type == BUILDING_BARRACKS)
                        g->gold += 40;
            }
        }

        void DrawPrepPhase(Game * g) {
            DrawGame(g);
            DrawHUD(g);

            /* Yarı saydam overlay */
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 20, 0, 60});

            /* Başlık */
            const char *title = "HAZIRLIK FAZI";
            DrawText(title, SCREEN_WIDTH / 2 - MeasureText(title, 36) / 2, 90, 36,
                     (Color){80, 255, 80, 230});

            /* Geri sayım */
            char timer[32];
            int secs = (int)g->prepTimer + 1;
            snprintf(timer, sizeof(timer), "%d", secs > 0 ? secs : 0);
            DrawText(timer, SCREEN_WIDTH / 2 - MeasureText(timer, 56) / 2, 134, 56,
                     secs <= 5 ? RED : GOLD);

            /* Bina seçim paneli */
            const char *labels[BUILDING_TYPE_COUNT] = {"[4] Kışla  (+40 Altin/dalga, ücretsiz)",
                                                       "[5] Pazar  (-maliyet, 30 Altin)",
                                                       "[6] Barikat  (Düsman yavaşlatır)"};
            Color sel_colors[BUILDING_TYPE_COUNT] = {GREEN, SKYBLUE, ORANGE};
            for (int i = 0; i < BUILDING_TYPE_COUNT; i++) {
                Color bg = (g->selectedBuilding == (BuildingType)i) ? (Color){60, 80, 60, 200}
                                                                    : (Color){30, 35, 30, 160};
                DrawRectangle(20, 110 + i * 44, 360, 40, bg);
                DrawRectangleLines(20, 110 + i * 44, 360, 40, sel_colors[i]);
                DrawText(labels[i], 30, 121 + i * 44, 14, sel_colors[i]);
            }

            /* Yerlestirilen binalar — izometrik pozisyon */
            for (int i = 0; i < g->buildingCount; i++) {
                Building *b = &g->buildings[i];
                if (!b->active)
                    continue;
                Vector2 bp = GridToWorld(b->gridX, b->gridY);
                Color bc = (b->type == BUILDING_BARRACKS) ? GREEN
                           : (b->type == BUILDING_MARKET) ? SKYBLUE
                                                          : ORANGE;
                DrawCircle((int)bp.x, (int)bp.y, (float)ISO_HALF_H * 1.4f, Fade(bc, 0.55f));
                DrawCircleLines((int)bp.x, (int)bp.y, (float)ISO_HALF_H * 1.4f, bc);
                const char *sym = (b->type == BUILDING_BARRACKS) ? "K"
                                  : (b->type == BUILDING_MARKET) ? "P"
                                                                 : "B";
                DrawText(sym, (int)bp.x - 4, (int)bp.y - 6, 10, WHITE);
            }

            /* Siege duvarları */
            DrawSiegeMechanics(&g->siege);

            /* Alt talimat */
            DrawRectangle(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50, (Color){0, 0, 0, 160});
            DrawText("[4/5/6]: Bina Sec  |  Sol Tık: Bina Koy  |  Sag Tık: Duvar Koy  |  [D]: "
                     "Dungeon  |  [SPACE]: Dalga Başlat",
                     30, SCREEN_HEIGHT - 34, 13, (Color){200, 200, 180, 220});
        }

        /* ============================================================
         * T05 — MAIN — Game Loop
         * ============================================================ */

        int main(void) {
            InitLogger(); /* T60 — ilk önce logger */
            LOG_INFO("Oyun başlatılıyor");
            InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Ruler");
            SetExitKey(0); // Prevent ESC from closing the game
            SetTargetFPS(60);

            Game game;
            InitGame(&game);
            InitAudioManager(&game.audio); /* T64 — pencere açıldıktan sonra, bir kez */

            while (!WindowShouldClose()) {
                float rawDt = GetFrameTime();
                float dt = rawDt * game.gameSpeed;

                /* Update */
                switch (game.state) {
                case STATE_MENU:
                    UpdateMenu(&game);
                    break;
                case STATE_CLASS_SELECT:
                    UpdateClassSelect(&game);
                    break;
                case STATE_WORLD_MAP:
                    UpdateWorldMap(&game);
                    break;
                case STATE_LEVEL_BRIEFING:
                    UpdateLevelBriefing(&game, dt);
                    break;
                case STATE_LOADING:
                    UpdateLoading(&game, dt);
                    break;
                case STATE_PLAYING:
                    HandleInput(&game);
                    UpdateEnemies(&game, dt);
                    UpdateFriendlyUnits(&game, dt);
                    UpdateTowers(&game, dt);
                    UpdateProjectiles(&game, dt);
                    UpdateParticles(&game, dt);
                    UpdateFloatingTexts(&game, dt);
                    UpdateScreenShake(&game, rawDt);
                    UpdateGameCamera(&game, rawDt);
                    UpdateFogOfWar(&game); /* T70 */
                    UpdateWaves(&game, dt);
                    UpdateHomeCity(&game.homeCity, dt);
                    CheckGameConditions(&game);
                    /* HomeCity pendingRequest: birim yerlestirme modunu ac */
                    if (game.homeCity.pendingRequest >= 0) {
                        game.pendingPlacementType = (FUnitType)(game.homeCity.pendingRequest - 1);
                        game.pendingPlacementCount = game.homeCity.pendingCount;
                        game.homeCity.pendingRequest = -1;
                        game.homeCity.pendingCount = 0;
                    }
                    /* SPACE ile sonraki dalgayı başlat */
                    if (!game.waveActive && game.currentWave < game.totalWaves &&
                        IsKeyPressed(KEY_SPACE)) {
                        game.waveActive = true;
                        OnWaveStart(&game.homeCity, &game.gold);
                    }
                    break;
                case STATE_WAVE_CLEAR:
                    UpdateParticles(&game, dt);
                    UpdateHomeCity(&game.homeCity, dt);
                    /* HomeCity pending birim */
                    if (game.homeCity.pendingRequest >= 0) {
                        game.pendingPlacementType = (FUnitType)(game.homeCity.pendingRequest - 1);
                        game.pendingPlacementCount = game.homeCity.pendingCount;
                        game.homeCity.pendingRequest = -1;
                        game.homeCity.pendingCount = 0;
                    }
                    /* D: Dungeon'a gir */
                    if (IsKeyPressed(KEY_D)) {
                        game.preDungeonState = STATE_WAVE_CLEAR;
                        InitDungeon(&game.dungeon, &game.hero);
                        game.state = STATE_DUNGEON;
                        break;
                    }
                    /* SPACE: Prep fazına geç */
                    if (IsKeyPressed(KEY_SPACE) && game.currentWave < game.totalWaves) {
                        game.state = STATE_PREP_PHASE;
                        game.prepTimer = PREP_PHASE_DURATION;
                        OnWaveStart(&game.homeCity, &game.gold);
                    } else if (IsKeyPressed(KEY_SPACE)) {
                        game.state = STATE_PLAYING;
                        game.waveActive = true;
                        OnWaveStart(&game.homeCity, &game.gold);
                    }
                    break;
                case STATE_PREP_PHASE:
                    UpdateParticles(&game, dt);
                    UpdatePrepPhase(&game, dt);
                    UpdateSiegeMechanics(&game.siege, dt);
                    UpdateHomeCity(&game.homeCity, dt);
                    /* HomeCity pending birim */
                    if (game.homeCity.pendingRequest >= 0) {
                        game.pendingPlacementType = (FUnitType)(game.homeCity.pendingRequest - 1);
                        game.pendingPlacementCount = game.homeCity.pendingCount;
                        game.homeCity.pendingRequest = -1;
                        game.homeCity.pendingCount = 0;
                    }
                    break;
                case STATE_DUNGEON:
                    UpdateDungeon(&game.dungeon, &game.hero, dt);
                    /* ESC: dungeon'dan çık, altın bonus uygula */
                    if (IsKeyPressed(KEY_ESCAPE)) {
                        game.gold += game.dungeon.inventory.bonusGold;
                        game.dungeon.isDungeonActive = false;
                        game.state = game.preDungeonState;
                    }
                    break;
                case STATE_LEVEL_COMPLETE:
                    UpdateLevelComplete(&game, dt);
                    break;
                case STATE_PAUSED:
                    UpdatePause(&game);
                    break;
                case STATE_GAMEOVER:
                    /* R → seviyeyi tekrar dene, ESC → dünya haritası */
                    if (IsKeyPressed(KEY_R))
                        RestartCurrentLevel(&game);
                    if (IsKeyPressed(KEY_ESCAPE)) {
                        game.state = STATE_WORLD_MAP;
                    }
                    break;
                case STATE_VICTORY:
                    if (IsKeyPressed(KEY_R))
                        RestartGame(&game);
                    break;
                case STATE_SETTINGS:
                    UpdateSettingsMenu(&game);
                    break;
                }

                /* Draw */
                BeginDrawing();
                ClearBackground((Color){34, 40, 49, 255});
                switch (game.state) {
                case STATE_MENU:
                    DrawMenu(&game);
                    break;
                case STATE_CLASS_SELECT:
                    DrawClassSelect(&game);
                    break;
                case STATE_WORLD_MAP:
                    DrawWorldMap(&game);
                    break;
                case STATE_LEVEL_BRIEFING:
                    DrawLevelBriefing(&game);
                    break;
                case STATE_LOADING:
                    DrawLoading(&game);
                    break;
                case STATE_PLAYING: {
                    /* T63 — Kamera: zoom + hero takip + T56 ScreenShake offset */
                    Camera2D renderCam = game.camera.cam;
                    renderCam.offset.x += game.screenShake.offsetX;
                    renderCam.offset.y += game.screenShake.offsetY;
                    BeginMode2D(renderCam);
                    DrawGame(&game);
                    EndMode2D();
                    DrawHUD(&game);
                    DrawMinimap(&game); /* T70 — Minimap (ekran koordinatı) */
                    DrawContextMenu(
                        &game); /* Menü Artık Ekran Koordinatlarında (HUD Katmanı) Çiziliyor! */
                    break;
                }
                case STATE_WAVE_CLEAR: {
                    BeginMode2D(game.camera.cam);
                    DrawGame(&game);
                    EndMode2D();
                    DrawHUD(&game);
                    DrawMinimap(&game);
                    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 100});
                    const char *wc = "DALGA TEMIZLENDI!";
                    DrawText(wc, SCREEN_WIDTH / 2 - MeasureText(wc, 40) / 2, SCREEN_HEIGHT / 2 - 60,
                             40, GOLD);
                    DrawText("[ SPACE ] - Hazirlik Fazi / Sonraki Dalga", SCREEN_WIDTH / 2 - 230,
                             SCREEN_HEIGHT / 2, 24, WHITE);
                    DrawText("[ D ] - Dungeon Moduna Gir", SCREEN_WIDTH / 2 - 160,
                             SCREEN_HEIGHT / 2 + 36, 20, (Color){120, 220, 255, 220});
                    break;
                }
                case STATE_PREP_PHASE:
                    DrawPrepPhase(&game);
                    break;
                case STATE_DUNGEON:
                    ClearBackground((Color){15, 10, 8, 255});
                    DrawDungeon(&game.dungeon, &game.hero);
                    break;
                case STATE_LEVEL_COMPLETE:
                    DrawLevelComplete(&game);
                    break;
                case STATE_PAUSED: {
                    Camera2D renderCamP = game.camera.cam;
                    BeginMode2D(renderCamP);
                    DrawGame(&game);
                    EndMode2D();
                    DrawHUD(&game);
                    DrawPauseOverlay(&game);
                    break;
                }
                case STATE_GAMEOVER:
                    DrawGameOver(&game);
                    break;
                case STATE_VICTORY:
                    DrawVictory(&game);
                    break;
                case STATE_SETTINGS:
                    DrawSettingsMenu(&game);
                    break;
                }
                EndDrawing();
            }

            UnloadUI();
            CloseAudioManager(&game.audio); /* T64 */
            CloseWindow();
            LOG_INFO("Oyun kapatıldı");
            CloseLogger();
            return 0;
        }
