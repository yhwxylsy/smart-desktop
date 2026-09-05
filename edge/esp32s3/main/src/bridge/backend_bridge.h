#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "../net/websocket_link.h"

// 后端命令桥接（原 main.ino L542-611、L1492-1494 原样搬运）。
// 复用 websocket_link 暴露的 webSocket / wsConnected。
void forwardCommandsFromJson(const String &jsonText);
void pollBackendCommands();
void sendAckToBackend(const String &line);
void sendButtonEventToBackend(const String &line);
void handleStm32ButtonEvent(const String &line);
