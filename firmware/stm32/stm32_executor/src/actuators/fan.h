#pragma once
#include <Arduino.h>
#include "../../config.h"

// DRV8833 风扇驱动（原 sketch L2078-2125 原样搬运）。
extern uint8_t currentFanLevel;
extern bool drv8833Connected;  // setup 依据此开关决定是否初始化 IN1/IN2 引脚

void stopDrv8833();
uint8_t drv8833FanDutyForLevel(uint8_t level);
bool driveFanOn(uint8_t level);
uint8_t fanLevelFromCommand(const String &command);
