#pragma once
#include <Arduino.h>
#include "../../config.h"

// 协议解析（原 sketch L214-218、L328-358 原样搬运）。
struct ParsedCommand {
  String actionId;
  String command;
  bool wrapped;
};

ParsedCommand parseLine(String line);
void sendAck(const ParsedCommand &parsed, bool ok);
