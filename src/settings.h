/* settings.h — T57/T58: Ayarlar sistemi (settings.ini) + Key Rebinding */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

#define SETTINGS_FILE "settings.ini"

/* T58 — Yeniden atanabilir eylemler */
typedef enum {
    KA_PAUSE = 0,      /* P          — Duraklat */
    KA_SPEED,          /* F          — Hiz toggle */
    KA_GRID,           /* G          — Grid goster/gizle */
    KA_HERO_FOLLOW,    /* H          — Hero kamera takibi */
    KA_NEXT_WAVE,      /* SPACE      — Sonraki dalga */
    KA_TOWER_BASIC,    /* 1          — Basic kule */
    KA_TOWER_SNIPER,   /* 2          — Sniper kule */
    KA_TOWER_SPLASH,   /* 3          — Splash kule */
    KA_BLDG_BARRACKS,  /* 4          — Kislak */
    KA_BLDG_MARKET,    /* 5          — Pazar */
    KA_BLDG_TOWNCENTER,/* 6          — Sehir Merkezi */
    KA_COUNT
} KeyAction;

typedef struct {
    bool  fullscreen;
    float masterVolume; /* 0.0 – 1.0 */
    float sfxVolume;
    float musicVolume;
    int   keymap[KA_COUNT]; /* T58 — tuş atamaları (Raylib KeyboardKey) */
} Settings;

void        DefaultSettings(Settings *s);
bool        LoadSettings(Settings *s);
bool        SaveSettings(const Settings *s);
const char *KeyActionLabel(KeyAction a); /* T58 — eylem etiketi */
const char *KeyName(int key);            /* T58 — tuş ismi */

#endif
