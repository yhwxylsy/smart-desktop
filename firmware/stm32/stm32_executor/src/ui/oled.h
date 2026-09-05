#pragma once
#include <Arduino.h>
#include "../../config.h"

// OLED 显存/图元/驱动（原 sketch L943-1104、L1251-1402 原样搬运）。
// 屏幕编排（render* / showOledText / updateSystemUi）位于 oled_screens 模块。
extern uint8_t oledAddress;
extern uint8_t oledBuffer[OLED_WIDTH * OLED_PAGES];
extern uint8_t oledFlushPage;
extern bool oledAvailable;
extern bool oledDirty;
extern bool oledRenderPending;
extern uint32_t lastUiFrameMs;
extern uint32_t lastOledFlushMs;
extern uint32_t oledFpsWindowMs;
extern uint16_t oledFrameCounter;
extern uint8_t oledFps;

void clearOledBuffer();
void oledSetPixel(uint8_t x, uint8_t y, bool on);
void oledFillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on);
void oledDrawChar(uint8_t x, uint8_t y, char ch, bool inverted);
void oledDrawText(uint8_t x, uint8_t y, const String &text, uint8_t maxChars, bool inverted);
void oledDrawTextCentered(const String &text, uint8_t y, uint8_t maxChars);
void oledDrawCjkTextCentered(const char *text, uint8_t y);
void oledDrawLine(int x0, int y0, int x1, int y1, bool on);
bool oledProbe(uint8_t address);
bool oledWriteCommand(uint8_t command);
bool oledWriteDataChunk(uint16_t offset, uint8_t length);
bool initializeOled();
void flushOledPage();
