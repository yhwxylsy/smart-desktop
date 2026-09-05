#pragma once
#include <Arduino.h>
#include "../../config.h"

// 舵机脉冲驱动（原 sketch L2167-2218 原样搬运）。
bool parseServoAngle(const String &value, uint8_t &angle);
void setServoAngle(uint8_t angle);
void updateServoPulse();
