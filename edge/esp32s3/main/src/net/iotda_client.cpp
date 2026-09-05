// 华为云 IoTDA MQTT（原 main.ino L69-76、L253-482 整块搬运）。
// 仅当 SMARTDESK_IOTDA_ENABLED=1 时参与编译；关闭时本文件内容被整体剔除。
// 注意：#if 宏判断前必须已包含 config.h（独立 TU 中未定义宏按 0 处理，
// 否则整个文件会被误判为空翻译单元导致链接期 undefined reference）。
#include "iotda_client.h"
#include "../../config.h"

#if SMARTDESK_IOTDA_ENABLED

#include <mbedtls/md.h>
#include "../core/hex_util.h"
#include "../bridge/stm32_link.h"
#include "../config/config_store.h"

#if SMARTDESK_IOTDA_PORT == 1883
WiFiClient iotdaClient;
#else
WiFiClientSecure iotdaClient;
#endif
PubSubClient iotdaMqtt(iotdaClient);

static unsigned long lastIotdaReconnectMs = 0;

String hmacSha256Hex(const String &key, const String &message) {
  unsigned char digest[32];
  const mbedtls_md_info_t *mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_hmac(
      mdInfo,
      reinterpret_cast<const unsigned char *>(key.c_str()),
      key.length(),
      reinterpret_cast<const unsigned char *>(message.c_str()),
      message.length(),
      digest);

  static const char *HEX_DIGITS = "0123456789abcdef";
  String encoded;
  encoded.reserve(64);
  for (uint8_t value : digest) {
    encoded += HEX_DIGITS[(value >> 4) & 0x0F];
    encoded += HEX_DIGITS[value & 0x0F];
  }
  return encoded;
}

String iotdaDeviceId() {
  return String(SMARTDESK_IOTDA_DEVICE_ID);
}

String iotdaPropertyReportTopic() {
  return "$oc/devices/" + iotdaDeviceId() + "/sys/properties/report";
}

String iotdaCommandSubscribeTopic() {
  return "$oc/devices/" + iotdaDeviceId() + "/sys/commands/#";
}

String iotdaCommandResponseTopic(const String &requestId) {
  return "$oc/devices/" + iotdaDeviceId() + "/sys/commands/response/request_id=" + requestId;
}

bool isIotdaPropertyKey(const char *key) {
  static const char *IOTDA_KEYS[] = {
      "temperature_c",
      "humidity_pct",
      "distance_cm",
      "pot_raw",
      "ntc_raw",
      "tracking_signal",
      "wifi_rssi_dbm",
      "aht20_ok",
      "distance_ok",
      "encoder_position",
  };
  for (const char *allowed : IOTDA_KEYS) {
    if (strcmp(key, allowed) == 0) {
      return true;
    }
  }
  return false;
}

String commandFromIotda(const String &commandName, JsonObject paras, bool *accepted) {
  *accepted = true;
  if (commandName == "fan_control") {
    String state = paras["state"] | "on";
    state.toLowerCase();
    if (state == "off" || state == "0" || state == "false") {
      return "NET:FAN:OFF";
    }
    int level = paras["level"] | 2;
    if (level < 1) level = 1;
    if (level > 3) level = 3;
    return "NET:FAN:ON:" + String(level);
  }
  if (commandName == "lock_control") {
    String state = paras["state"] | "on";
    state.toLowerCase();
    if (state == "off" || state == "unlock" || state == "0" || state == "false") {
      return "NET:LOCK:OFF";
    }
    return "NET:LOCK:ON";
  }
  if (commandName == "buzzer_alert") {
    return "NET:BEEP";
  }
  if (commandName == "tts_speak") {
    String text = paras["text"] | "";
    if (text.length() == 0) {
      *accepted = false;
      return "";
    }
    return "NET:TTSHEX:" + utf8Hex(text);
  }
  if (commandName == "volume_control") {
    int level = paras["level"] | 10;
    if (level < 0) level = 0;
    if (level > 16) level = 16;
    return "NET:VOLUME:" + String(level);
  }
  if (commandName == "raw_stm32") {
    String command = paras["command"] | "";
    if (!command.startsWith("NET:")) {
      *accepted = false;
      return "";
    }
    return command;
  }
  *accepted = false;
  return "";
}

void publishIotdaCommandResponse(const String &topic, bool accepted, const String &commandName) {
  int requestPos = topic.indexOf("request_id=");
  if (requestPos < 0 || !iotdaMqtt.connected()) {
    return;
  }
  String requestId = topic.substring(requestPos + strlen("request_id="));
  JsonDocument response;
  response["result_code"] = accepted ? 0 : 1;
  response["response_name"] = "smartdesk_response";
  JsonObject paras = response["paras"].to<JsonObject>();
  paras["accepted"] = accepted;
  paras["command_name"] = commandName;
  String payload;
  serializeJson(response, payload);
  iotdaMqtt.publish(iotdaCommandResponseTopic(requestId).c_str(), payload.c_str());
}

void iotdaCallback(char *topic, byte *payload, unsigned int length) {
  String text;
  text.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) {
    text += static_cast<char>(payload[i]);
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, text);
  if (error) {
    Serial.print("[IOTDA] bad command json: ");
    Serial.println(error.c_str());
    publishIotdaCommandResponse(String(topic), false, "");
    return;
  }

  String commandName = doc["command_name"] | "";
  JsonObject paras = doc["paras"].as<JsonObject>();
  bool accepted = false;
  String stm32Command = commandFromIotda(commandName, paras, &accepted);
  if (accepted && stm32Command.length() > 0) {
    sendToStm32("NET:UI:ACTION", 0);
    sendToStm32(stm32Command);
  }
  publishIotdaCommandResponse(String(topic), accepted, commandName);
}

bool ensureIotdaMqttConnected() {
  if (strlen(SMARTDESK_IOTDA_HOST) == 0 || strlen(SMARTDESK_IOTDA_DEVICE_ID) == 0 ||
      strlen(SMARTDESK_IOTDA_SECRET) == 0) {
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  if (iotdaMqtt.connected()) {
    return true;
  }
  if (millis() - lastIotdaReconnectMs < 5000) {
    return false;
  }
  lastIotdaReconnectMs = millis();

#if SMARTDESK_IOTDA_PORT != 1883
  iotdaClient.setInsecure();
#endif
  iotdaMqtt.setServer(SMARTDESK_IOTDA_HOST, SMARTDESK_IOTDA_PORT);
  iotdaMqtt.setCallback(iotdaCallback);
  iotdaMqtt.setBufferSize(1024);
  iotdaMqtt.setKeepAlive(120);

  String deviceId = iotdaDeviceId();
  String timestamp = SMARTDESK_IOTDA_TIMESTAMP;
  String clientId = deviceId + "_0_0_" + timestamp;
  String password = hmacSha256Hex(timestamp, SMARTDESK_IOTDA_SECRET);

  Serial.print("[IOTDA] connecting ");
  Serial.println(SMARTDESK_IOTDA_HOST);
  bool ok = iotdaMqtt.connect(clientId.c_str(), deviceId.c_str(), password.c_str());
  if (!ok) {
    Serial.print("[IOTDA] connect failed state=");
    Serial.println(iotdaMqtt.state());
    return false;
  }
  Serial.println("[IOTDA] connected");
  iotdaMqtt.subscribe(iotdaCommandSubscribeTopic().c_str());
  return true;
}

void iotdaLoop() {
  if (ensureIotdaMqttConnected()) {
    iotdaMqtt.loop();
  }
}

void publishTelemetryToIotda(JsonObject sensors) {
  if (!ensureIotdaMqttConnected()) {
    return;
  }

  JsonDocument report;
  JsonArray services = report["services"].to<JsonArray>();
  JsonObject service = services.add<JsonObject>();
  service["service_id"] = "smartdesk";
  JsonObject properties = service["properties"].to<JsonObject>();
  bool hasProperties = false;
  for (JsonPair kv : sensors) {
    const char *key = kv.key().c_str();
    if (isIotdaPropertyKey(key)) {
      properties[key] = kv.value();
      hasProperties = true;
    }
  }
  if (!hasProperties) {
    return;
  }

  String payload;
  serializeJson(report, payload);
  bool ok = iotdaMqtt.publish(iotdaPropertyReportTopic().c_str(), payload.c_str());
  Serial.print("[IOTDA] property report ");
  Serial.println(ok ? "ok" : "failed");
}

#endif
