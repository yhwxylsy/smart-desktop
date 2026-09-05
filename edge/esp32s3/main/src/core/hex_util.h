#pragma once
#include <Arduino.h>

// 将 UTF-8 文本编码为十六进制串（原 main.ino L166-176 原样搬运）
String utf8Hex(const String &text);
