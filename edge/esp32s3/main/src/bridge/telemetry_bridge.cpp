#include "telemetry_bridge.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "../../config.h"
#include "../config/config_store.h"
#include "../net/http_client.h"
#include "../net/iotda_client.h"

bool isAllowedTelemetryKey(const char *key) {
  static const char *ALLOWED_KEYS[] = {
      "pot_raw",          "pot_pct",          "ntc_raw",          "ntc_pct",
      "tracking_signal",  "aht20_ok",         "temperature_c",    "humidity_pct",
      "distance_ok",      "distance_enabled", "distance_cm",      "distance_zone",
      "env_state",        "interaction_hint", "rgb_mode",         "rgb_status",
      "rgb_reason",       "encoder_delta",    "encoder_position", "encoder_button",
  };
  for (const char *allowed : ALLOWED_KEYS) {
    if (strcmp(key, allowed) == 0) {
      return true;
    }
  }
  return false;
}

void sendTelemetryToBackend(String line) {
  line.trim();
  if (line.startsWith("BT:")) {
    line = line.substring(3);
  }
  if (!line.startsWith("{")) {
    return;
  }

  JsonDocument source;
  DeserializationError error = deserializeJson(source, line);
  if (error) {
    Serial.print("[TELEMETRY] bad json: ");
    Serial.println(error.c_str());
    return;
  }

  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  doc["edge_id"] = EDGE_ID;
  doc["voice_state"] = voiceState;
  JsonObject sensors = doc["sensors"].to<JsonObject>();
  if (WiFi.status() == WL_CONNECTED) {
    sensors["wifi_rssi_dbm"] = WiFi.RSSI();
  }
  JsonObject sourceObject = source.as<JsonObject>();
  for (JsonPair kv : sourceObject) {
    const char *key = kv.key().c_str();
    if (isAllowedTelemetryKey(key)) {
      sensors[key] = kv.value();
    }
  }

  String body;
  serializeJson(doc, body);
#if SMARTDESK_IOTDA_ENABLED
  publishTelemetryToIotda(sensors);
#endif
  postJson("/api/hardware/telemetry", body);
}
