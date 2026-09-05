#include "websocket_link.h"
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include "../../config.h"
#include "../core/hex_util.h"
#include "../bridge/stm32_link.h"
#include "../config/config_store.h"

WebSocketsClient webSocket;
bool wsConnected = false;
bool wsStarted = false;

void handleWsText(const uint8_t *payload, size_t length) {
  String text;
  text.reserve(length + 1);
  for (size_t i = 0; i < length; i++) {
    text += (char)payload[i];
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, text);
  if (error) {
    Serial.print("[WS] bad json: ");
    Serial.println(error.c_str());
    return;
  }

  String type = doc["type"] | "";
  Serial.print("[WS] ");
  Serial.println(type);

  if (type == "stm32/commands") {
    JsonArray lines = doc["lines"].as<JsonArray>();
    if (!lines.isNull() && lines.size() > 0) {
      sendToStm32("NET:UI:ACTION", 0);
      sendToStm32("NET:UART?", 0);
    }
    for (JsonVariant line : lines) {
      sendToStm32(line.as<String>());
    }
  } else if (type == "speak") {
    String speak = doc["text"] | "";
    if (speak.length() > 0) {
      sendToStm32("NET:UI:ACTION", 0);
      sendToStm32("NET:TTSHEX:" + utf8Hex(speak));
    }
  } else if (type == "ping") {
    webSocket.sendTXT("{\"type\":\"pong\"}");
  }
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      wsConnected = true;
      Serial.println("[WS] connected");
      webSocket.sendTXT("{\"type\":\"ping\"}");
      break;
    case WStype_DISCONNECTED:
      wsConnected = false;
      Serial.println("[WS] disconnected");
      break;
    case WStype_TEXT:
      handleWsText(payload, length);
      break;
    default:
      break;
  }
}

void startWebSocket() {
  if (WiFi.status() != WL_CONNECTED || configStore::host().isEmpty() || wsStarted) {
    return;
  }
  if (configStore::secure()) {
    webSocket.beginSSL(configStore::host().c_str(), configStore::port(), configStore::wsPath().c_str());
  } else {
    webSocket.begin(configStore::host().c_str(), configStore::port(), configStore::wsPath().c_str());
  }
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  webSocket.enableHeartbeat(15000, 3000, 2);
  wsStarted = true;
  Serial.print("[WS] connecting to ");
  Serial.println(configStore::wsBase() + configStore::wsPath());
}

bool pauseWebSocketForMicUpload() {
  if (!wsStarted) {
    return false;
  }
  webSocket.disconnect();
  wsConnected = false;
  wsStarted = false;
  delay(150);
  return true;
}
