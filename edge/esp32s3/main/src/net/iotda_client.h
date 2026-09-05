#pragma once
#include <Arduino.h>
#include "../../config.h"

// 华为云 IoTDA MQTT 客户端（原 main.ino L69-76、L253-482 整块搬运）。
// 整文件在 SMARTDESK_IOTDA_ENABLED 关闭时不参与编译，运行行为完全不变。
#if SMARTDESK_IOTDA_ENABLED
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#if SMARTDESK_IOTDA_PORT == 1883
extern WiFiClient iotdaClient;
#else
extern WiFiClientSecure iotdaClient;
#endif
extern PubSubClient iotdaMqtt;

void iotdaLoop();
void publishTelemetryToIotda(JsonObject sensors);
#endif
