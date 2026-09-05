#pragma once
#include <Arduino.h>
#include <WebSocketsClient.h>
#include "../../config.h"

// WebSocket 实时链路（原 main.ino L670-726、L815-841 原样搬运）。
// webSocket / wsConnected / wsStarted 由本模块独占，供后端桥接与 CLI 共享。
extern WebSocketsClient webSocket;
extern bool wsConnected;
extern bool wsStarted;

void handleWsText(const uint8_t *payload, size_t length);
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
void startWebSocket();
bool pauseWebSocketForMicUpload();
