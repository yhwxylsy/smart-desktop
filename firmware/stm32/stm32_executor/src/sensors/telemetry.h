#pragma once
#include <Arduino.h>
#include "../../config.h"

// 遥测采样与上报（原 sketch L446-572、L2220-2322 原样搬运）。
// latest* 快照与 rgbSensorMode 由本模块独占；RGB 遥测状态归类
// （updateLatestRgbStatus / buildRgbStatusLine）一并收归本模块。
extern uint32_t lastTelemetryMs;  // 周期遥测节流（主 loop 读写）

extern bool latestTelemetryValid;
extern int latestPotRaw;
extern uint8_t latestPotPct;
extern int latestNtcRaw;
extern uint8_t latestNtcPct;
extern bool latestTrackingSignal;
extern bool latestAhtOk;
extern float latestTemperatureC;
extern float latestHumidityPct;
extern bool latestDistanceOk;
extern bool latestDistanceEnabled;
extern float latestDistanceCm;
extern long latestEncoderPosition;
extern long latestEncoderDelta;
extern bool latestEncoderButtonPressed;
extern const char *latestDistanceZone;
extern const char *latestEnvState;
extern const char *latestInteractionHint;
extern const char *latestRgbStatus;
extern const char *latestRgbReason;
extern bool rgbSensorMode;

uint8_t analogPercent(int raw);
const char *distanceZone(bool enabled, bool ok, float cm);
const char *environmentState(bool ok, float temperatureC, float humidityPct);
const char *interactionHint(bool trackingSignal, bool encoderButtonPressed, long encoderDelta,
                            bool distanceOk, float distanceCm);
void updateLatestRgbStatus();
String buildRgbStatusLine();
String buildTelemetryJson();
void sendTelemetrySnapshot();
bool canSendPeriodicTelemetry();
