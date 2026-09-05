#include "dispatcher.h"
#include <string.h>
#include "../../config.h"
#include "../actuators/fan.h"
#include "../actuators/servo.h"
#include "../audio/buzzer.h"
#include "../audio/tts.h"
#include "../core/board.h"
#include "../core/text_util.h"
#include "../sensors/telemetry.h"
#include "../sensors/ultrasonic.h"
#include "../system/i2c_bus.h"
#include "../system/ui_demo.h"
#include "../system/user_context.h"
#include "../ui/oled.h"
#include "../ui/oled_screens.h"
#include "../ui/rgb.h"
#include "../ui/ui_state.h"

// ============================================================================
// 命令表（去耦合阶段一：执行层表驱动化）。
// 每个表项 = 前缀 + 精确匹配标志 + 处理器。行顺序 = 原 executeNetCommand 的
// if 分支顺序，行为逐字保留（返回语义、副作用、日志均不变）。
// 阶段二：将 classifyNetCommand / commandPreview / 前缀扫描并入本表。
// 阶段二前，command_line.cpp 中的三个纯函数保持独立并受 command_knowledge_reference.py
// 语料冻结保护（见 backend/tests/test_firmware_command_knowledge.py）。
// ============================================================================

struct NetCommandDef {
  const char *prefix;
  bool exact;
  bool (*handler)(const String &command);
};

// ---- UI 状态提示类命令：原实现仅返回 true，不改任何状态 ----
static bool onUiStateHint(const String &command) {
  return true;
}

static bool onUiUser(const String &command) {
  return handleUserContextCommand(command.substring(strlen("NET:UI:USER:")));
}

static bool onUiDemo(const String &command) {
  startUiDemo();
  return true;
}

static bool onUiDemoStop(const String &command) {
  stopUiDemo();
  return true;
}

static bool onUiStatus(const String &command) {
  String line = "BT:UI:";
  line += "demo=";
  line += uiDemoActive ? "ON" : "OFF";
  line += ",oled=";
  line += oledAvailable ? "OK" : "MISS";
  line += ",addr=0x";
  line += String(oledAddress, HEX);
  line += ",screen=";
  line += String((uint8_t)infoScreen);
  line += ",fps=";
  line += String(oledFps);
  line += ",volume_pct=";
  line += String(speechVolumePercent);
  writeBack(line);
  return true;
}

static bool onUartPing(const String &command) {
  writeBack("BT:PONG:" + String(millis()));
  return true;
}

static bool onI2cScan(const String &command) {
  return handleI2cScanCommand();
}

static bool onTelemetryRequest(const String &command) {
  sendTelemetrySnapshot();
  return true;
}

static bool onRgbStatus(const String &command) {
  writeBack(buildRgbStatusLine());
  return true;
}

static bool onRgbLegend(const String &command) {
  writeBack("BT:RGB:LEGEND:GREEN=ready,CYAN=object_or_tracking,YELLOW=env_watch,RED=sensor_or_too_close,BLUE=waiting");
  return true;
}

static bool onRgbModeSensor(const String &command) {
  rgbSensorMode = true;
  updateLatestRgbStatus();
  writeBack("BT:RGB:MODE:SENSOR");
  return true;
}

static bool onRgbModeEvent(const String &command) {
  rgbSensorMode = false;
  writeBack("BT:RGB:MODE:EVENT");
  return true;
}

static bool onUltrasonicOn(const String &command) {
  ultrasonicEnabled = true;
  writeBack("BT:ULTRASONIC:ON");
  return true;
}

static bool onUltrasonicOff(const String &command) {
  ultrasonicEnabled = false;
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  writeBack("BT:ULTRASONIC:OFF");
  return true;
}

static bool onSpeechStop(const String &command) {
  return stopSpeechOutput();
}

static bool onSpeakText(const String &command) {
  return speakText(command.substring(strlen("NET:TTS:")));
}

static bool onSpeakHex(const String &command) {
  return speakHexText(command.substring(strlen("NET:TTSHEX:")));
}

static bool onSetVolume(const String &command) {
  return setSpeechVolume(command.substring(strlen("NET:VOLUME:")), false);
}

static bool onOledText(const String &command) {
  if (!oledAvailable) {
    initializeOled();
  }
  showOledText(command.substring(strlen("NET:OLED:")));
  return true;
}

static bool onRfidEvent(const String &command) {
  uiEvent.detail = String("RFID ") + compactForDisplay(command.substring(strlen("NET:RFID:")), 15);
  oledRenderPending = true;
  return true;
}

static bool onBeep(const String &command) {
  playBeep(2200, 120);
  return true;
}

static bool onMusic(const String &command) {
  return startMusicByPreset(command.substring(strlen("NET:MUSIC:")));
}

static bool onMotorOff(const String &command) {
  stopDrv8833();
  return true;
}

static bool onFanOn(const String &command) {
  return driveFanOn(fanLevelFromCommand(command));
}

static bool onFanOff(const String &command) {
  stopDrv8833();
  return true;
}

static bool onLockOn(const String &command) {
  currentLockOn = true;
  setRgb(true, false, false);
  return true;
}

static bool onLockOff(const String &command) {
  currentLockOn = false;
  setRgb(false, true, false);
  return true;
}

static bool onAiBusy(const String &command) {
  setRgb(true, true, false);
  return true;
}

static bool onAiIdle(const String &command) {
  setRgb(false, true, false);
  return true;
}

static bool onAiOff(const String &command) {
  setRgb(false, false, false);
  return true;
}

static bool onServo(const String &command) {
  uint8_t angle = 0;
  if (!parseServoAngle(command.substring(strlen("NET:SERVO:")), angle)) {
    usbConsole.print("[SERVO] invalid angle ");
    usbConsole.println(command.substring(strlen("NET:SERVO:")));
    return false;
  }
  setServoAngle(angle);
  usbConsole.print("[SERVO] ");
  usbConsole.println(angle);
  return true;
}

// 行顺序 == 原 executeNetCommand 分支顺序（不得调整，避免行为漂移）
static const NetCommandDef NET_COMMANDS[] = {
    // UI 状态提示（原首分支：整组 return true）
    {"NET:UI:LISTEN", true, onUiStateHint},
    {"NET:UI:THINK", true, onUiStateHint},
    {"NET:UI:ACTION", true, onUiStateHint},
    {"NET:UI:ACK", true, onUiStateHint},
    {"NET:UI:OUTPUT", true, onUiStateHint},
    {"NET:UI:IDLE", true, onUiStateHint},
    {"NET:UI:ERROR", true, onUiStateHint},
    {"NET:UI:USER:", false, onUiUser},
    {"NET:UI:DEMO", true, onUiDemo},
    {"NET:UI:DEMO:STOP", true, onUiDemoStop},
    {"NET:UI:STATUS?", true, onUiStatus},
    {"NET:UART?", true, onUartPing},
    {"NET:I2C?", true, onI2cScan},
    {"NET:TELEMETRY?", true, onTelemetryRequest},
    {"NET:RGB:STATUS?", true, onRgbStatus},
    {"NET:RGB:LEGEND?", true, onRgbLegend},
    {"NET:RGB:MODE:SENSOR", true, onRgbModeSensor},
    {"NET:RGB:MODE:EVENT", true, onRgbModeEvent},
    {"NET:ULTRASONIC:ON", true, onUltrasonicOn},
    {"NET:ULTRASONIC:OFF", true, onUltrasonicOff},
    {"NET:TTS:STOP", true, onSpeechStop},
    {"NET:AUDIO:STOP", true, onSpeechStop},
    {"NET:TTS:", false, onSpeakText},
    {"NET:TTSHEX:", false, onSpeakHex},
    {"NET:VOLUME:", false, onSetVolume},
    {"NET:OLED:", false, onOledText},
    {"NET:RFID:", false, onRfidEvent},
    {"NET:BEEP", true, onBeep},
    {"NET:MUSIC:", false, onMusic},
    {"NET:MOTOR:OFF", true, onMotorOff},
    {"NET:FAN:ON", false, onFanOn},
    {"NET:FAN:OFF", true, onFanOff},
    {"NET:LOCK:ON", true, onLockOn},
    {"NET:LOCK:OFF", true, onLockOff},
    {"NET:AI:BUSY", true, onAiBusy},
    {"NET:AI:IDLE", true, onAiIdle},
    {"NET:AI:OFF", true, onAiOff},
    {"NET:SERVO:", false, onServo},
};

bool executeNetCommand(const String &command) {
  for (const NetCommandDef &entry : NET_COMMANDS) {
    bool matched = entry.exact ? (command == entry.prefix) : command.startsWith(entry.prefix);
    if (matched) {
      return entry.handler(command);
    }
  }

  usbConsole.print("[ERR] unsupported command: ");
  usbConsole.println(command);
  return false;
}
