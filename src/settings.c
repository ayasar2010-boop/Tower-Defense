/* settings.c — T57/T58: settings.ini okuma/yazma + key rebinding */

#include "settings.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

void DefaultSettings(Settings *s) {
    s->fullscreen   = false;
    s->masterVolume = 1.0f;
    s->sfxVolume    = 1.0f;
    s->musicVolume  = 1.0f;
    /* T58 — varsayilan tus atamalari */
    s->keymap[KA_PAUSE]           = KEY_P;
    s->keymap[KA_SPEED]           = KEY_F;
    s->keymap[KA_GRID]            = KEY_G;
    s->keymap[KA_HERO_FOLLOW]     = KEY_H;
    s->keymap[KA_NEXT_WAVE]       = KEY_SPACE;
    s->keymap[KA_TOWER_BASIC]     = KEY_ONE;
    s->keymap[KA_TOWER_SNIPER]    = KEY_TWO;
    s->keymap[KA_TOWER_SPLASH]    = KEY_THREE;
    s->keymap[KA_BLDG_BARRACKS]   = KEY_FOUR;
    s->keymap[KA_BLDG_MARKET]     = KEY_FIVE;
    s->keymap[KA_BLDG_TOWNCENTER] = KEY_SIX;
}

bool LoadSettings(Settings *s) {
    DefaultSettings(s);
    FILE *f = fopen(SETTINGS_FILE, "r");
    if (!f) return false;

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        int ival; float fval;
        if (sscanf(line, "fullscreen=%d",    &ival) == 1) s->fullscreen   = (ival != 0);
        if (sscanf(line, "masterVolume=%f",  &fval) == 1) s->masterVolume = fval;
        if (sscanf(line, "sfxVolume=%f",     &fval) == 1) s->sfxVolume    = fval;
        if (sscanf(line, "musicVolume=%f",   &fval) == 1) s->musicVolume  = fval;
        /* T58 — tuş atamaları */
        for (int i = 0; i < KA_COUNT; i++) {
            char key_pat[32];
            snprintf(key_pat, sizeof(key_pat), "keymap_%d=%%d", i);
            if (sscanf(line, key_pat, &ival) == 1)
                s->keymap[i] = ival;
        }
    }
    fclose(f);
    return true;
}

bool SaveSettings(const Settings *s) {
    FILE *f = fopen(SETTINGS_FILE, "w");
    if (!f) return false;
    fprintf(f, "fullscreen=%d\n",      (int)s->fullscreen);
    fprintf(f, "masterVolume=%.2f\n",  s->masterVolume);
    fprintf(f, "sfxVolume=%.2f\n",     s->sfxVolume);
    fprintf(f, "musicVolume=%.2f\n",   s->musicVolume);
    for (int i = 0; i < KA_COUNT; i++)
        fprintf(f, "keymap_%d=%d\n", i, s->keymap[i]);
    fclose(f);
    return true;
}

const char *KeyActionLabel(KeyAction a) {
    static const char *labels[KA_COUNT] = {
        "Duraklat", "Hiz Toggle", "Grid", "Hero Takibi", "Sonraki Dalga",
        "Kule: Basic", "Kule: Sniper", "Kule: Splash",
        "Bina: Kislak", "Bina: Pazar", "Bina: Sehir Merkezi"
    };
    return (a >= 0 && a < KA_COUNT) ? labels[a] : "?";
}

const char *KeyName(int key) {
    static char buf[8];
    /* A-Z harfleri */
    if (key >= KEY_A && key <= KEY_Z) {
        buf[0] = (char)('A' + (key - KEY_A));
        buf[1] = '\0';
        return buf;
    }
    /* 0-9 rakamları */
    if (key >= KEY_ZERO && key <= KEY_NINE) {
        buf[0] = (char)('0' + (key - KEY_ZERO));
        buf[1] = '\0';
        return buf;
    }
    switch (key) {
    case KEY_SPACE:     return "SPC";
    case KEY_ESCAPE:    return "ESC";
    case KEY_ENTER:     return "ENT";
    case KEY_TAB:       return "TAB";
    case KEY_LEFT:      return "SOL";
    case KEY_RIGHT:     return "SAG";
    case KEY_UP:        return "YUK";
    case KEY_DOWN:      return "ASA";
    case KEY_F1:        return "F1";
    case KEY_F2:        return "F2";
    case KEY_F3:        return "F3";
    case KEY_F4:        return "F4";
    case KEY_F5:        return "F5";
    case KEY_F6:        return "F6";
    case KEY_F7:        return "F7";
    case KEY_F8:        return "F8";
    case KEY_F9:        return "F9";
    case KEY_F10:       return "F10";
    case KEY_F11:       return "F11";
    case KEY_F12:       return "F12";
    default:            return "?";
    }
}
