#pragma once
#include <Arduino.h>
#include "../../config.h"

// Wi-Fi 连接与网络诊断（原 main.ino L728-813、L1592-1635 原样搬运）
bool scanTargetWifi(int32_t *channelOut, uint8_t bssidOut[6], int32_t *rssiOut, bool logResult = true);
void connectWifi();
void scanWifi();
void probeTcp(String target);
