#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "../net/heartbeat.h"

// 遥测桥接与白名单（原 main.ino L613-668 原样搬运）。
// 复用 heartbeat 暴露的 voiceState。
bool isAllowedTelemetryKey(const char *key);
void sendTelemetryToBackend(String line);
