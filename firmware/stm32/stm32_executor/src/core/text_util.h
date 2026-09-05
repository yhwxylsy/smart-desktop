#pragma once
#include <Arduino.h>
#include "../../config.h"

// 文本工具（原 sketch L360-384、L1610-1621、L2324-2330 原样搬运）。
String upperCopy(const String &text);
bool containsUpperToken(const String &text, const char *token);
String compactForDisplay(const String &text, uint8_t maxLen);
int hexNibble(char ch);
String nextColonField(const String &text, int &offset);
