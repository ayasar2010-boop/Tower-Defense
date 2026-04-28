#include "audio.h"
#include <string.h>

static const char *SFX_PATHS[SFX_COUNT] = {
    "assets/sfx/arrow_shoot.wav",  /* SFX_SHOOT_BASIC   */
    "assets/sfx/magic_bolt.wav",   /* SFX_SHOOT_SNIPER  */
    "assets/sfx/cannon_fire.wav",  /* SFX_SHOOT_SPLASH  */
    "assets/sfx/enemy_death.wav",  /* SFX_ENEMY_DEATH   */
    "assets/sfx/troll_death.wav",  /* SFX_TANK_DEATH    */
    "assets/sfx/tower_build.wav",  /* SFX_TOWER_PLACE   */
    "assets/sfx/upgrade.wav",      /* SFX_TOWER_UPGRADE */
    "assets/sfx/coins.wav",        /* SFX_TOWER_SELL    */
    "assets/sfx/drum_roll.wav",    /* SFX_WAVE_START    */
    "assets/sfx/life_lost.wav",    /* SFX_LIVE_LOST     */
    "assets/sfx/button_click.wav", /* SFX_BUTTON_CLICK  */
    "assets/sfx/fanfare.wav",      /* SFX_VICTORY       */
    "assets/sfx/defeat.wav",       /* SFX_DEFEAT        */
    "assets/sfx/level_up.wav",     /* SFX_LEVEL_UP      */
};

void InitAudioManager(AudioManager *am) {
    memset(am, 0, sizeof(AudioManager));
    am->masterVolume = 1.0f;
    InitAudioDevice();
    am->deviceReady = IsAudioDeviceReady();
    if (!am->deviceReady)
        return;
    for (int i = 0; i < SFX_COUNT; i++) {
        if (FileExists(SFX_PATHS[i])) {
            am->sounds[i] = LoadSound(SFX_PATHS[i]);
            am->loaded[i] = true;
        }
    }
}

/* Ses efekti çalar; dosya yüklü değilse veya cihaz hazır değilse sessiz geçer. */
void PlaySFX(AudioManager *am, SFXType t) {
    if (!am->deviceReady || !am->loaded[t])
        return;
    SetSoundVolume(am->sounds[t], am->masterVolume);
    PlaySound(am->sounds[t]);
}

/* Sesleri serbest bırakır ve ses cihazını kapatır. */
void CloseAudioManager(AudioManager *am) {
    if (!am->deviceReady)
        return;
    for (int i = 0; i < SFX_COUNT; i++)
        if (am->loaded[i])
            UnloadSound(am->sounds[i]);
    CloseAudioDevice();
    am->deviceReady = false;
}

/* Oyunu tamamen sıfırlar ve başlangıç durumuna getirir. */