#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "../ui/ui_state.h"

// DEMO 自检（原 sketch L205-212、L286-298、L1698-1905 原样搬运）。
struct UiDemoStep {
  UiEventType type;
  const char *detail;
  uint16_t durationMs;
  int8_t fanState;
  bool beep;
  const char *ttsHex;
};

// uiDemoActive 被 dispatch 的 NET:UI:STATUS? 读取，以 extern 暴露。
extern bool uiDemoActive;

void startUiDemo();
void stopUiDemo();
void updateUiDemo();
