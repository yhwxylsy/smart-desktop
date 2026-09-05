#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "../core/types.h"

// 麦克风采集 -> 上传流水线与自检（原 main.ino L885-890、L1385-1576 原样搬运）。
bool rejectMicPath(const char *trigger);
bool captureAndUploadMic(bool inject = true, const String &source = "esp32_mic");
bool captureAndUploadMicAfterCue(String cueText, bool inject = true, const String &source = "esp32_mic");
bool runMicSelfTest(String phrase);
String normalizeSelfTestText(String value);
bool selfTestTextMatches(const String &recognized, const String &expected);
void sendMicSelfTestTelemetry(const String &expected, const AsrUploadResult &result, bool passed);
