#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "../core/types.h"

// 麦克风采集（原 main.ino L146-152、L869-883、L892-935、L1360-1383 原样搬运）。
// micReady / micBusy 由采集与流水线共享，以 extern 暴露；去耦合阶段改为访问接口。
extern bool micReady;
extern bool micBusy;

bool initMicrophone();
void generateWavHeader(uint8_t *wavHeader, uint32_t wavSize, uint32_t sampleRate);
void conditionPcm16(uint8_t *pcmBuffer, size_t pcmSize);
MicCapture captureMicWav();
uint8_t *allocLargeBuffer(size_t size);
