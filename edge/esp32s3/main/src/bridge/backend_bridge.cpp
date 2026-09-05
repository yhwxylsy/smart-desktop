#include "backend_bridge.h"
#include <ArduinoJson.h>
#include "../../config.h"
#include "../config/config_store.h"
#include "../net/http_client.h"
#include "stm32_link.h"

static unsigned long lastCommandPollMs = 0;

void forwardCommandsFromJson(const String &jsonText) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonText);
  if (error) {
    Serial.print("[JSON] parse failed: ");
    Serial.println(error.c_str());
    return;
  }
  JsonArray commands = doc["commands"].as<JsonArray>();
  if (!commands.isNull() && commands.size() > 0) {
    sendToStm32("NET:UART?", 0);
  }
  for (JsonVariant command : commands) {
    sendToStm32(command.as<String>());
  }
}

void pollBackendCommands() {
  if (wsConnected || millis() - lastCommandPollMs < COMMAND_POLL_INTERVAL_MS) {
    return;
  }
  lastCommandPollMs = millis();
  String response;
  if (getJson("/api/hardware/commands/" + String(DEVICE_ID), &response)) {
    forwardCommandsFromJson(response);
  }
}

void sendAckToBackend(const String &line) {
  JsonDocument httpDoc;
  httpDoc["device_id"] = DEVICE_ID;
  httpDoc["line"] = line;
  String body;
  serializeJson(httpDoc, body);
  if (postJson("/api/hardware/ack", body)) {
    return;
  }

  JsonDocument wsDoc;
  wsDoc["type"] = "ack";
  wsDoc["line"] = line;
  String wsPayload;
  serializeJson(wsDoc, wsPayload);
  if (wsConnected) {
    webSocket.sendTXT(wsPayload);
  }
}

void sendButtonEventToBackend(const String &line) {
  JsonDocument doc;
  doc["type"] = "button";
  doc["device_id"] = DEVICE_ID;
  doc["line"] = line;
  doc["source"] = "stm32";
  String body;
  serializeJson(doc, body);
  if (wsConnected) {
    webSocket.sendTXT(body);
    Serial.print("[BTN] forwarded via websocket ");
    Serial.println(line);
    return;
  }
  if (postJson("/api/hardware/button", body)) {
    Serial.print("[BTN] forwarded via http ");
    Serial.println(line);
    return;
  }
  Serial.print("[BTN] forward failed ");
  Serial.println(line);
}

void handleStm32ButtonEvent(const String &line) {
  sendButtonEventToBackend(line);
}
