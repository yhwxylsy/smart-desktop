#pragma once
#include <Arduino.h>
#include "../../config.h"

// AHT20 温湿度（原 sketch L1914-1963 原样搬运）。
extern bool aht20Initialized;

bool initializeAht20();
bool readAht20(float &temperatureC, float &humidityPct);
