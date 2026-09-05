#pragma once
#include <Arduino.h>
#include "../../config.h"

// 串口 CLI（原 main.ino L1665-1828 原样搬运）。
void handleSerialCommand(String line);
void pollUsbSerial();
