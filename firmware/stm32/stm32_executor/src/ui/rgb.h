#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "ui_state.h"

// 三色指示灯（原 sketch L438-444、L574-591、L809-822、L1404-1446 原样搬运）。
// 仅负责物理灯与灯效；遥测状态归类（updateLatestRgbStatus / buildRgbStatusLine）
// 已迁入 sensors/telemetry，本模块不持有传感器快照。
struct RgbState {
  bool red;
  bool green;
  bool blue;
};

RgbState rgbState(bool red, bool green, bool blue);
void writeRgbRaw(bool red, bool green, bool blue);
void setRgb(bool red, bool green, bool blue);
RgbState uiMachineBaseRgb();
void updateRgbAnimation();
