#include "wave.h"
#include "audio.h"
#include "enemy.h"
#include "hud.h"
#include <string.h>

static void SetWave1(GameWave *w, EnemyType t, int cnt, float intv, float pre, float hpMult,
                     int bonus, const char *name) {
    memset(w, 0, sizeof(GameWave));
    w->enemyTypes[0] = t;
    w->enemyCounts[0] = cnt;
    w->typeCount = 1;
    w->enemyCount = cnt;
    w->spawnInterval = intv;
    w->preDelay = pre;
    w->enemyHpMultiplier = hpMult;
    w->bonusGold = bonus;
    strncpy(w->waveName, name, 31);
}
/* İki tipli dalga başlatıcı */
static void SetWave2(GameWave *w, EnemyType t1, int c1, EnemyType t2, int c2, float intv, float pre,
                     float hpMult, int bonus, const char *name) {
    memset(w, 0, sizeof(GameWave));
    w->enemyTypes[0] = t1;
    w->enemyCounts[0] = c1;
    w->enemyTypes[1] = t2;
    w->enemyCounts[1] = c2;
    w->typeCount = 2;
    w->enemyCount = c1 + c2;
    w->spawnInterval = intv;
    w->preDelay = pre;
    w->enemyHpMultiplier = hpMult;
    w->bonusGold = bonus;
    strncpy(w->waveName, name, 31);
}
/* Üç tipli dalga başlatıcı */
static void SetWave3(GameWave *w, EnemyType t1, int c1, EnemyType t2, int c2, EnemyType t3, int c3,
                     float intv, float pre, float hpMult, int bonus, const char *name) {
    memset(w, 0, sizeof(GameWave));
    w->enemyTypes[0] = t1;
    w->enemyCounts[0] = c1;
    w->enemyTypes[1] = t2;
    w->enemyCounts[1] = c2;
    w->enemyTypes[2] = t3;
    w->enemyCounts[2] = c3;
    w->typeCount = 3;
    w->enemyCount = c1 + c2 + c3;
    w->spawnInterval = intv;
    w->preDelay = pre;
    w->enemyHpMultiplier = hpMult;
    w->bonusGold = bonus;
    strncpy(w->waveName, name, 31);
}

/* Progresif zorlukla 20 dalganın parametrelerini atar — guidance §13.2 */
void InitWaves(Game *g) {
    GameWave *w = g->waves;
    /* ─── BÖLÜM 1 (Dalga 1-9) ─────────────────────────────── */
    SetWave1(&w[0], ENEMY_NORMAL, 8, 1.5f, 2.0f, 1.0f, 20, "Ilk Akin");
    SetWave1(&w[1], ENEMY_NORMAL, 10, 1.2f, 2.0f, 1.1f, 25, "Yol Kesfi");
    SetWave1(&w[2], ENEMY_FAST, 6, 1.0f, 2.0f, 1.0f, 30, "Hizli Izciler");
    SetWave2(&w[3], ENEMY_NORMAL, 8, ENEMY_FAST, 4, 0.8f, 2.0f, 1.2f, 35, "Karisik Gucler");
    SetWave1(&w[4], ENEMY_TANK, 4, 2.5f, 2.0f, 1.0f, 40, "Zirh Onculer");
    SetWave1(&w[5], ENEMY_NORMAL, 15, 0.8f, 2.0f, 1.3f, 45, "Dalga Dalgasi");
    SetWave1(&w[6], ENEMY_FAST, 12, 0.6f, 2.0f, 1.2f, 50, "Hiz Firtinasi");
    SetWave2(&w[7], ENEMY_TANK, 6, ENEMY_NORMAL, 6, 1.2f, 2.0f, 1.4f, 60, "Demir Duvar");
    SetWave3(&w[8], ENEMY_NORMAL, 10, ENEMY_FAST, 6, ENEMY_TANK, 3, 0.7f, 2.0f, 1.5f, 70,
             "Son Hazirlik");
    /* ─── BOSS DALGASI 10 ─────────────────────────────────── */
    SetWave2(&w[9], ENEMY_BOSS, 1, ENEMY_NORMAL, 4, 1.0f, 3.0f, 5.0f, 150, "* GOLEM KRAL *");
    w[9].isBossWave = true;
    /* ─── BÖLÜM 2 (Dalga 11-19) ──────────────────────────── */
    SetWave1(&w[10], ENEMY_NORMAL, 20, 0.5f, 2.0f, 1.7f, 80, "Sessiz Tehdit");
    SetWave1(&w[11], ENEMY_FAST, 15, 0.4f, 2.0f, 1.5f, 85, "Golge Kosucular");
    SetWave1(&w[12], ENEMY_TANK, 8, 1.5f, 2.0f, 1.6f, 90, "Agir Tugay");
    SetWave3(&w[13], ENEMY_NORMAL, 4, ENEMY_FAST, 4, ENEMY_TANK, 4, 0.6f, 2.0f, 1.8f, 100,
             "Kaos Dalgasi");
    SetWave2(&w[14], ENEMY_NORMAL, 25, ENEMY_FAST, 10, 0.4f, 2.0f, 2.0f, 110, "Cehennem Gecidi");
    SetWave1(&w[15], ENEMY_TANK, 12, 1.0f, 2.0f, 2.0f, 120, "Tank Filosu");
    SetWave1(&w[16], ENEMY_FAST, 30, 0.3f, 2.0f, 1.8f, 130, "Sel Baskini");
    SetWave3(&w[17], ENEMY_TANK, 8, ENEMY_FAST, 8, ENEMY_NORMAL, 8, 0.5f, 2.0f, 2.2f, 150,
             "Elit Birlikleri");
    SetWave3(&w[18], ENEMY_NORMAL, 10, ENEMY_FAST, 10, ENEMY_TANK, 10, 0.4f, 2.0f, 2.5f, 180,
             "Son Savunma");
    /* ─── FINAL BOSS DALGASI 20 ───────────────────────────── */
    SetWave3(&w[19], ENEMY_BOSS, 1, ENEMY_NORMAL, 10, ENEMY_FAST, 5, 0.5f, 3.0f, 8.0f, 300,
             "* KARANLIK LORD *");
    w[19].isBossWave = true;

    g->totalWaves = MAX_WAVES;
    g->currentWave = 0;
    g->waveActive = false;
}

/* ============================================================
 * InitLevels — 5 Seviyenin Placeholder Verisi
 * ============================================================ */

/* Her seviyenin adı, backstory'si, harita asset yolu,
 * diyalogları ve yıldız eşikleri burada tanımlanır.
 * "[TODO]" ile işaretli alanlar senaryo yazarı tarafından doldurulacak. */
/* ── RULER — Dünya Kurgusu ──────────────────────────────────────
 *
 *  KRALLIK    : Arendalm — yüzyıllık barışın ardından işgale uğruyor.
 *  HÜKÜMDAr  : Sen — adın yok, kararlar senin.
 *  DANIŞMAN   : Başkomutan Aldric — sadık, deneyimli, sert gerçekçi.
 *  DÜŞMAN     : General Krom — Kuzey Bozkırları'nın acımasız savaş lordu.
 *               Krom para için savaşmaz; krallığı çökertmek ister.
 *  [ULAK]     : Anlatıcı — sahneler arası bağlantı, tarihin sesi.
 *
 *  Düşman tipleri:
 *    Normal → Kuzey Akincilari  (hızlı, hafif zırh, kalabalık)
 *    Fast   → Kurt Suvarileri   (atı yok ama pençeli hafif piyade, cok hizli)
 *    Tank   → Demir Muhafizlar  (plaka zırh, ok geçirmez, yavaş ama ölmez)
 * ──────────────────────────────────────────────────────────────── */

int CalcLevelStars(Game *g) {
    LevelData *lv = &g->levels[g->currentLevel];
    if (g->lives >= lv->star3Lives)
        return 3;
    if (g->lives >= lv->star2Lives)
        return 2;
    if (g->lives > 0)
        return 1;
    return 0;
}

void UpdateWaves(Game *g, float dt) {
    if (!g->waveActive || g->currentWave >= g->totalWaves)
        return;

    GameWave *w = &g->waves[g->currentWave];

    /* Ön gecikme */
    if (!w->started) {
        w->preDelay -= dt;
        if (w->preDelay > 0.0f)
            return;
        w->started = true;
        /* T62 — Dalga adı bannerını göster */
        strncpy(g->waveNameText, w->waveName, 31);
        g->waveNameTimer = 3.0f;
        if (w->isBossWave)
            TriggerScreenShake(g, 8.0f);
        PlaySFX(&g->audio, SFX_WAVE_START); /* T64 */
    }

    /* T62 — Dalga adı banner sayacı */
    if (g->waveNameTimer > 0.0f)
        g->waveNameTimer -= dt;

    /* T62 — Spawn: Çoklu tip desteği — type[0] tükendikten sonra type[1], sonra type[2] */
    if (w->spawnedCount < w->enemyCount) {
        w->spawnTimer -= dt;
        if (w->spawnTimer <= 0.0f) {
            /* Hangi tipin sırası olduğunu spawnedCount'a göre belirle */
            EnemyType typeToSpawn = w->enemyTypes[0];
            int cumulative = 0;
            for (int t = 0; t < w->typeCount; t++) {
                cumulative += w->enemyCounts[t];
                if (w->spawnedCount < cumulative) {
                    typeToSpawn = w->enemyTypes[t];
                    break;
                }
            }
            SpawnEnemy(g, typeToSpawn, w->enemyHpMultiplier);
            w->spawnedCount++;
            w->spawnTimer = w->spawnInterval;
        }
    }

    /* Dalga bitti mi? Tüm spawn edildi ve aktif düşman kalmadı */
    if (w->spawnedCount >= w->enemyCount) {
        bool anyActive = false;
        for (int i = 0; i < MAX_ENEMIES; i++)
            if (g->enemies[i].active) {
                anyActive = true;
                break;
            }
        if (!anyActive) {
            /* T62 — Dalga tamamlama altın ödülü */
            g->gold += w->bonusGold;
            g->waveActive = false;
            g->currentWave++;
            if (g->currentWave >= g->totalWaves) {
                /* Tüm dalgalar bitti — yıldız hesapla, completion diyaloğunu yükle */
                g->levelStars = CalcLevelStars(g);
                g->levels[g->currentLevel].stars = g->levelStars;
                LevelData *lv = &g->levels[g->currentLevel];
                g->dialogue.lineCount = lv->completionCount;
                g->dialogue.currentLine = 0;
                g->dialogue.visibleChars = 0;
                g->dialogue.charTimer = 0.0f;
                g->dialogue.active = (lv->completionCount > 0);
                for (int i = 0; i < lv->completionCount; i++)
                    g->dialogue.lines[i] = lv->completion[i];
                PlaySFX(&g->audio, SFX_VICTORY); /* T64 */
                g->state = STATE_LEVEL_COMPLETE;
            } else {
                g->state = STATE_WAVE_CLEAR;
            }
        }
    }
}

/* ============================================================
 * CheckGameConditions
 * ============================================================ */

void CheckGameConditions(Game *g) {
    if (g->lives <= 0) {
        PlaySFX(&g->audio, SFX_DEFEAT); /* T64 */
        g->state = STATE_GAMEOVER;
    }
}

/* ============================================================
 * UpdateDialogue — Typewriter efekti, satır ilerleme
 * ============================================================ */

/* SPACE veya sol tık ile önce mevcut metni tamamlar,
 * sonraki tıkta bir sonraki satıra geçer. */