#ifndef AUDIO_H
#define AUDIO_H
#include "types.h"
void InitAudioManager(AudioManager *am);
void PlaySFX(AudioManager *am, SFXType t);
void CloseAudioManager(AudioManager *am);
#endif
