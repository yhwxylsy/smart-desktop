#pragma once
#include <Arduino.h>
#include "../../config.h"

// STM32 串口链路（原 main.ino L64-65、L231-251、L1578-1590、L1830-1871 原样搬运）。
// 双 HardwareSerial 实例与轮询由本模块独占。
// lastAckMs 被心跳模块读取，以 extern 暴露；去耦合阶段将改为访问接口。
extern HardwareSerial stm32Tx;
extern HardwareSerial stm32Rx;
extern unsigned long lastAckMs;

String stm32LogLine(const String &line);
void sendToStm32(const String &line, uint16_t postDelayMs = 80);
void sendUartKeepalive();
void pollStm32();
