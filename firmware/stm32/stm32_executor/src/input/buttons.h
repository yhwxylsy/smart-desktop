#pragma once
#include <Arduino.h>
#include "../../config.h"

// KEY1(信息)/KEY2(DEMO) 双按键（原 sketch L1767-1879 原样搬运）。
extern String lastButtonEvent;
extern uint32_t lastButtonEventMs;

void writeButtonEvent(const String &event);
void updateKey2Button();
void updateInfoButton();
