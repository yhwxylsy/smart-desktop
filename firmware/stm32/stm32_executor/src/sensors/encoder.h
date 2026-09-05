#pragma once
#include <Arduino.h>
#include "../../config.h"

// 旋钮编码器（原 sketch L2127-2161 原样搬运）。
// encoderPosition / encoderDeltaSinceTelemetry 供遥测采样读取，以 extern 暴露。
extern long encoderPosition;
extern long encoderDeltaSinceTelemetry;

uint8_t readEncoderState();
void initEncoder();
void updateEncoder();
