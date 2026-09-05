#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "oled.h"
#include "ui_state.h"

// OLED 屏幕编排（原 sketch L1134-1249、L1344-1368、L1448-1457、L1907-1912 原样搬运）。
void renderStateHeader(UiMachineState state);
void renderStateBody(UiMachineState state);
void renderInfoScreen(uint32_t now);
void renderStatusScreen(uint32_t now);
void renderSystemOled();
void showOledText(const String &text);
void updateSystemUi();
