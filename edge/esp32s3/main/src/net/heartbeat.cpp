#include "heartbeat.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "../../config.h"
#include "../config/config_store.h"
#include "http_client.h"
#include "../bridge/stm32_link.h"

String voiceState = "text_bridge";
static unsigned long lastHeartbeatMs = 0;

void sendHeartbeat(bool force) {
  if (!force && millis() - lastHeartbeatMs < 5000) {
    return;
  }
  lastHeartbeatMs = millis();

  bool uartOk = lastAckMs > 0 && millis() - lastAckMs < UART_OK_WINDOW_MS;
  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  doc["edge_id"] = EDGE_ID;
  doc["online"] = WiFi.status() == WL_CONNECTED;
  doc["uart_ok"] = uartOk;
  doc["voice_state"] = voiceState;
  doc["uptime_ms"] = millis();
  String body;
  serializeJson(doc, body);
  postJson("/api/hardware/heartbeat", body);
}

void setVoiceState(const String &state, bool sendNow) {
  voiceState = state;
  if (sendNow) {
    sendHeartbeat(true);
  }
}
