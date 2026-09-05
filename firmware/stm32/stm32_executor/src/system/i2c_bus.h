#pragma once
#include <Arduino.h>
#include "../../config.h"

// I2C 总线诊断（原 sketch L1256-1283、L1333-1342 原样搬运）。
uint8_t scanI2cBus();
bool handleI2cScanCommand();
