#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "../../config.h"

// 板级串口与回写（原 sketch L84-89、L323-326、L2619-2640 原样搬运）。
// 约束：espCommandSerial 同时承担 ESP 命令接收(PB11) 与 SYN6288 发送(PB10)，
// 全项目仅本模块持有该对象，其他模块经本头声明的引用访问。
extern SoftwareSerial espAckSerial;   // RX=PB4(未用), TX=PB3 -> SYN6288
extern HardwareSerial usbConsole;     // PA3(RX), PA2(TX) -> USB/调试
extern HardwareSerial espCommandSerial;  // PB11(RX), PB10(TX) -> ESP32 + SYN6288

// ESP 上行活动时间戳：pollSerial 写入，遥测/屏幕读取。
extern unsigned long lastEspRxActivityMs;
// ESP 接收行缓冲：loop 持有并传给 pollSerial。
extern String espLine;

void writeBack(const String &line);
void pollSerial(Stream &stream, String &buffer, const char *source);
