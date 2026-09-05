#pragma once
#include <Arduino.h>
#include "../../config.h"
#include "../protocol/protocol.h"

// UI 状态服务（原 sketch L138-218、L386-436、L824-941、L1092-1132 原样搬运）。
enum UiEventType : uint8_t {
  UI_EVENT_BOOT,
  UI_EVENT_DEMO,
  UI_EVENT_LISTEN,
  UI_EVENT_THINK,
  UI_EVENT_ACTION,
  UI_EVENT_ACK,
  UI_EVENT_UART,
  UI_EVENT_I2C,
  UI_EVENT_TELEMETRY,
  UI_EVENT_OLED,
  UI_EVENT_TTS,
  UI_EVENT_FAN_ON,
  UI_EVENT_FAN_OFF,
  UI_EVENT_BEEP,
  UI_EVENT_MUSIC,
  UI_EVENT_LOCK_ON,
  UI_EVENT_LOCK_OFF,
  UI_EVENT_AI_BUSY,
  UI_EVENT_AI_IDLE,
  UI_EVENT_AI_OFF,
  UI_EVENT_SERVO,
  UI_EVENT_RFID,
  UI_EVENT_ERROR
};

enum UiMachineState : uint8_t {
  UI_STATE_BOOT,
  UI_STATE_LOCKED,
  UI_STATE_READY,
  UI_STATE_LISTENING,
  UI_STATE_PROCESSING,
  UI_STATE_SPEAKING,
  UI_STATE_EXECUTING,
  UI_STATE_ERROR
};

enum InfoScreen : uint8_t {
  INFO_SCREEN_MAIN,
  INFO_SCREEN_USER,
  INFO_SCREEN_LINK,
  INFO_SCREEN_SENSORS,
  INFO_SCREEN_ACTUATORS,
  INFO_SCREEN_COUNT
};

struct UiEventState {
  UiEventType type;
  String detail;
  String actionId;
  uint32_t startedMs;
  uint32_t parseMs;
  uint32_t actionMs;
  uint32_t lightMs;
  uint32_t ackMs;
  bool hasLight;
  bool ackSeen;
  bool ackOk;
  bool wrapped;
};

extern UiEventState uiEvent;
extern UiMachineState uiMachineState;
extern uint32_t uiMachineStateStartedMs;
extern InfoScreen infoScreen;
extern uint32_t infoOverlayUntilMs;
extern uint32_t ackFlashUntilMs;
extern bool ackFlashOk;
extern bool commandLightMeasureActive;
extern bool commandHadLight;
extern uint32_t commandLightResponseMs;
extern bool currentLockOn;

void enterUiMachineState(UiMachineState state, uint32_t startedMs);
void transitionUiMachineForEvent(UiEventType type, uint32_t startedMs);
void updateUiMachineState(uint32_t now);
void beginUiEvent(UiEventType type, const String &detail, const String &actionId, bool wrapped, uint32_t startedMs);
void finishUiEvent(uint32_t parseMs, uint32_t actionMs, uint32_t ackMs, bool ok);
void logEventTiming(const char *source, const ParsedCommand &parsed, bool ok);
const char *eventLabel(UiEventType type);
const char *uiMachineCode(UiMachineState state);
const char *uiMachineTitle(UiMachineState state);
const char *uiMachineCompactTitle(UiMachineState state);
