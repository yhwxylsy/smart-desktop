#pragma once
#include <Arduino.h>
#include "../../config.h"

// 心跳与语音状态（原 main.ino L843-867 原样搬运）。
// voiceState 被遥测桥接、麦克风流水线共享，故以 extern 暴露；
// 去耦合阶段将改为访问接口。
extern String voiceState;

void sendHeartbeat(bool force = false);
void setVoiceState(const String &state, bool sendNow = false);
