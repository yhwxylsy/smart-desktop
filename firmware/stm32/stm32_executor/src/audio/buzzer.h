#pragma once
#include <Arduino.h>
#include "../../config.h"

// 蜂鸣器旋律播放（原 sketch L91-94、L300-321、L1985-2076 原样搬运）。
struct MelodyNote {
  uint16_t frequency;
  uint16_t durationMs;
};

void stopMusic();
bool startMusic(const MelodyNote *melody, uint8_t length);
void updateMusicPlayer();
bool startMusicByPreset(String preset);
bool playBeep(uint16_t frequency, uint16_t durationMs);
