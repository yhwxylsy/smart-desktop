#pragma once
#include <Arduino.h>
#include "../../config.h"

// SYN6288 语音合成（原 sketch L120-136、L1459-1696 原样搬运）。
// speechVolumePercent / volumeOverlayUntilMs 供屏幕层读取，以 extern 暴露。
extern uint8_t speechVolumePercent;
extern uint32_t volumeOverlayUntilMs;

bool appendCodePointAsUtf16Be(uint32_t codePoint, uint8_t *out, size_t capacity, size_t &outLen);
bool decodeNextUtf8CodePoint(const uint8_t *bytes, size_t length, size_t &index, uint32_t &codePoint);
bool sendSyn6288Frame(const uint8_t *textBytes, size_t textLen, uint8_t textType);
bool sendSyn6288Command(uint8_t command);
bool speakUtf8Bytes(const uint8_t *bytes, size_t length);
bool speakText(const String &text);
bool speakHexText(const String &hexText);
bool stopSpeechOutput();
uint8_t volumeLevelFromPercent(uint8_t percent);
uint8_t volumePercentFromLevel(uint8_t level);
bool setSpeechVolume(const String &value, bool announce);
// TODO(decouple): announceVolumeWhenSettled 为空函数且 volumeAnnouncementPending 只写不读，
// 属确证死代码，去耦合阶段删除并移除 loop 调用点。
void announceVolumeWhenSettled();
