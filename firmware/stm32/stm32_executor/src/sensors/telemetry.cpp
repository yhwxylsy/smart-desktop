#include "telemetry.h"
#include <string.h>
#include "../../config.h"
#include "../core/board.h"
#include "aht20.h"
#include "encoder.h"
#include "ultrasonic.h"

uint32_t lastTelemetryMs = 0;

bool latestTelemetryValid = false;
int latestPotRaw = 0;
uint8_t latestPotPct = 0;
int latestNtcRaw = 0;
uint8_t latestNtcPct = 0;
bool latestTrackingSignal = false;
bool latestAhtOk = false;
float latestTemperatureC = 0.0f;
float latestHumidityPct = 0.0f;
bool latestDistanceOk = false;
bool latestDistanceEnabled = false;
float latestDistanceCm = 0.0f;
long latestEncoderPosition = 0;
long latestEncoderDelta = 0;
bool latestEncoderButtonPressed = false;
const char *latestDistanceZone = "unknown";
const char *latestEnvState = "unknown";
const char *latestInteractionHint = "idle";
const char *latestRgbStatus = "waiting";
const char *latestRgbReason = "waiting_telemetry";
bool rgbSensorMode = true;

uint8_t analogPercent(int raw) {
  if (raw <= 0) {
    return 0;
  }
  uint32_t pct = ((uint32_t)raw * 100UL) / ANALOG_RAW_MAX;
  return pct > 100 ? 100 : (uint8_t)pct;
}

const char *distanceZone(bool enabled, bool ok, float cm) {
  if (!enabled) {
    return "disabled";
  }
  if (!ok) {
    return "missing";
  }
  if (cm < 10.0f) {
    return "too_close";
  }
  if (cm < 35.0f) {
    return "near";
  }
  if (cm < 90.0f) {
    return "ready";
  }
  return "far";
}

const char *environmentState(bool ok, float temperatureC, float humidityPct) {
  if (!ok) {
    return "missing";
  }
  if (temperatureC >= 32.0f) {
    return "hot";
  }
  if (temperatureC >= 29.0f) {
    return "warm";
  }
  if (temperatureC <= 16.0f) {
    return "cold";
  }
  if (humidityPct >= 75.0f) {
    return "humid";
  }
  if (humidityPct <= 30.0f) {
    return "dry";
  }
  return "comfortable";
}

const char *interactionHint(bool trackingSignal, bool encoderButtonPressed, long encoderDelta,
                            bool distanceOk, float distanceCm) {
  if (encoderButtonPressed) {
    return "encoder_press";
  }
  if (encoderDelta != 0) {
    return "encoder_turn";
  }
  if (distanceOk && distanceCm < 35.0f) {
    return "object_near";
  }
  if (trackingSignal) {
    return "tracking_high";
  }
  return "idle";
}

void updateLatestRgbStatus() {
  if (!latestTelemetryValid) {
    latestRgbStatus = "waiting";
    latestRgbReason = "waiting_telemetry";
    return;
  }

  if (!latestAhtOk || (latestDistanceEnabled && !latestDistanceOk)) {
    latestRgbStatus = "sensor_check";
    latestRgbReason = !latestAhtOk ? "aht20_missing" : "distance_missing";
    return;
  }

  if (latestDistanceOk && latestDistanceCm < 10.0f) {
    latestRgbStatus = "too_close";
    latestRgbReason = "object_too_close";
    return;
  }

  if (strcmp(latestEnvState, "comfortable") != 0) {
    latestRgbStatus = "env_watch";
    latestRgbReason = latestEnvState;
    return;
  }

  if (latestDistanceOk && latestDistanceCm < 35.0f) {
    latestRgbStatus = "near_object";
    latestRgbReason = "interaction_zone";
    return;
  }

  if (latestTrackingSignal) {
    latestRgbStatus = "tracking_high";
    latestRgbReason = "tracking_signal_high";
    return;
  }

  latestRgbStatus = "ready";
  latestRgbReason = "all_ok";
}

String buildRgbStatusLine() {
  String line = "BT:RGB:";
  line += "mode=";
  line += rgbSensorMode ? "sensor" : "event";
  line += ",status=";
  line += latestRgbStatus;
  line += ",reason=";
  line += latestRgbReason;
  line += ",env=";
  line += latestEnvState;
  line += ",distance=";
  line += latestDistanceZone;
  line += ",pot_pct=";
  line += String(latestPotPct);
  line += ",ntc_pct=";
  line += String(latestNtcPct);
  line += ",tracking=";
  line += latestTrackingSignal ? "1" : "0";
  return line;
}

String buildTelemetryJson() {
  int potRaw = analogRead(PIN_POTENTIOMETER);
  int ntcRaw = analogRead(PIN_NTC);
  float temperatureC = 0.0f;
  float humidityPct = 0.0f;
  float distanceCm = 0.0f;
  long encoderDelta = encoderDeltaSinceTelemetry;
  encoderDeltaSinceTelemetry = 0;
  long encoderPositionSnapshot = encoderPosition;
  bool encoderButtonPressed = digitalRead(PIN_ENCODER_BUTTON) == LOW;
  bool trackingSignal = digitalRead(PIN_TRACKING_SENSOR) == HIGH;
  bool ahtOk = readAht20(temperatureC, humidityPct);
  bool distanceOk = readDistanceCm(distanceCm);

  latestTelemetryValid = true;
  latestPotRaw = potRaw;
  latestPotPct = analogPercent(potRaw);
  latestNtcRaw = ntcRaw;
  latestNtcPct = analogPercent(ntcRaw);
  latestTrackingSignal = trackingSignal;
  latestAhtOk = ahtOk;
  latestTemperatureC = temperatureC;
  latestHumidityPct = humidityPct;
  latestDistanceOk = distanceOk;
  latestDistanceEnabled = ultrasonicEnabled;
  latestDistanceCm = distanceCm;
  latestEncoderDelta = encoderDelta;
  latestEncoderPosition = encoderPositionSnapshot;
  latestEncoderButtonPressed = encoderButtonPressed;
  latestDistanceZone = distanceZone(ultrasonicEnabled, distanceOk, distanceCm);
  latestEnvState = environmentState(ahtOk, temperatureC, humidityPct);
  latestInteractionHint = interactionHint(trackingSignal, encoderButtonPressed, encoderDelta,
                                          distanceOk, distanceCm);
  updateLatestRgbStatus();

  String json = "BT:{";
  json += "\"pot_raw\":";
  json += String(potRaw);
  json += ",\"pot_pct\":";
  json += String(latestPotPct);
  json += ",\"ntc_raw\":";
  json += String(ntcRaw);
  json += ",\"ntc_pct\":";
  json += String(latestNtcPct);
  json += ",\"tracking_signal\":";
  json += trackingSignal ? "true" : "false";
  json += ",\"aht20_ok\":";
  json += ahtOk ? "true" : "false";
  if (ahtOk) {
    json += ",\"temperature_c\":";
    json += String(temperatureC, 1);
    json += ",\"humidity_pct\":";
    json += String(humidityPct, 1);
  }
  json += ",\"distance_ok\":";
  json += distanceOk ? "true" : "false";
  json += ",\"distance_enabled\":";
  json += ultrasonicEnabled ? "true" : "false";
  if (distanceOk) {
    json += ",\"distance_cm\":";
    json += String(distanceCm, 1);
  }
  json += ",\"distance_zone\":\"";
  json += latestDistanceZone;
  json += "\"";
  json += ",\"env_state\":\"";
  json += latestEnvState;
  json += "\"";
  json += ",\"interaction_hint\":\"";
  json += latestInteractionHint;
  json += "\"";
  json += ",\"rgb_mode\":\"";
  json += rgbSensorMode ? "sensor" : "event";
  json += "\"";
  json += ",\"rgb_status\":\"";
  json += latestRgbStatus;
  json += "\"";
  json += ",\"rgb_reason\":\"";
  json += latestRgbReason;
  json += "\"";
  json += ",\"encoder_delta\":";
  json += String(encoderDelta);
  json += ",\"encoder_position\":";
  json += String(encoderPositionSnapshot);
  json += ",\"encoder_button\":";
  json += encoderButtonPressed ? "true" : "false";
  json += "}";
  return json;
}

void sendTelemetrySnapshot() {
  writeBack(buildTelemetryJson());
}

bool canSendPeriodicTelemetry() {
  if (espLine.length() > 0) {
    return false;
  }
  if (lastEspRxActivityMs == 0) {
    return true;
  }
  return millis() - lastEspRxActivityMs >= TELEMETRY_ESP_QUIET_MS;
}
