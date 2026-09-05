#include "ui_demo.h"
#include "../../config.h"
#include "../actuators/fan.h"
#include "../audio/buzzer.h"
#include "../audio/tts.h"
#include "../core/board.h"
#include "../ui/rgb.h"
#include "../ui/ui_state.h"

static const UiDemoStep UI_DEMO_STEPS[] = {
  {UI_EVENT_OLED, "OLED READY", UI_DEMO_DEFAULT_STEP_MS, -1, false, nullptr},
  {UI_EVENT_AI_BUSY, "AI BUSY", UI_DEMO_DEFAULT_STEP_MS, -1, false, nullptr},
  {UI_EVENT_TTS, "TTS HELLO", 1200, -1, false, "E4BDA0E5A5BD"},
  {UI_EVENT_FAN_ON, "FAN ON L2", UI_DEMO_DEFAULT_STEP_MS, 1, false, nullptr},
  {UI_EVENT_BEEP, "BEEP ALERT", UI_DEMO_DEFAULT_STEP_MS, -1, true, nullptr},
  {UI_EVENT_LOCK_ON, "LOCKED", UI_DEMO_DEFAULT_STEP_MS, -1, false, nullptr},
  {UI_EVENT_LOCK_OFF, "UNLOCK ACK", UI_DEMO_DEFAULT_STEP_MS, -1, false, nullptr},
  {UI_EVENT_RFID, "RFID ACK OK", UI_DEMO_DEFAULT_STEP_MS, -1, false, nullptr},
  {UI_EVENT_FAN_OFF, "FAN OFF", UI_DEMO_DEFAULT_STEP_MS, 0, false, nullptr},
  {UI_EVENT_AI_IDLE, "AI IDLE", UI_DEMO_DEFAULT_STEP_MS, -1, false, nullptr}
};
static const uint8_t UI_DEMO_STEP_COUNT = sizeof(UI_DEMO_STEPS) / sizeof(UI_DEMO_STEPS[0]);

bool uiDemoActive = false;
static uint8_t uiDemoIndex = 0;
static uint32_t uiDemoStepStartedMs = 0;

void logDemoStep(const UiDemoStep &step, uint32_t actionMs) {
  usbConsole.print("[DEMO] step=");
  usbConsole.print(eventLabel(step.type));
  usbConsole.print(" detail=");
  usbConsole.print(step.detail);
  usbConsole.print(" action_ms=");
  usbConsole.println(actionMs);
}

void runUiDemoStep(uint8_t index) {
  const UiDemoStep &step = UI_DEMO_STEPS[index];
  uint32_t startedMs = millis();
  beginUiEvent(step.type, step.detail, "ui_demo", false, startedMs);

  commandHadLight = false;
  commandLightResponseMs = 0;
  commandLightMeasureActive = true;
  RgbState cueRgb = uiMachineBaseRgb();
  setRgb(cueRgb.red, cueRgb.green, cueRgb.blue);

  uint32_t actionStartedMs = millis();
  if (step.fanState >= 0) {
    if (step.fanState > 0) {
      driveFanOn(2);
    } else {
      stopDrv8833();
    }
  }
  if (step.beep) {
    playBeep(2400, 120);
  }
  if (step.ttsHex != nullptr) {
    speakHexText(String(step.ttsHex));
  }
  uint32_t actionMs = millis() - actionStartedMs;
  commandLightMeasureActive = false;

  finishUiEvent(0, actionMs, millis() - startedMs, true);
  logDemoStep(step, actionMs);
  uiDemoStepStartedMs = startedMs;
}

void finishUiDemo(const char *detail) {
  uiDemoActive = false;
  stopDrv8833();

  uint32_t startedMs = millis();
  beginUiEvent(UI_EVENT_AI_IDLE, detail, "ui_demo", false, startedMs);
  commandHadLight = false;
  commandLightResponseMs = 0;
  commandLightMeasureActive = true;
  setRgb(false, true, false);
  commandLightMeasureActive = false;
  finishUiEvent(0, 0, millis() - startedMs, true);
  usbConsole.print("[DEMO] ");
  usbConsole.println(detail);
}

void startUiDemo() {
  uiDemoActive = true;
  uiDemoIndex = 0;
  uiDemoStepStartedMs = 0;
  usbConsole.println("[DEMO] start");
}

void stopUiDemo() {
  finishUiDemo("DEMO STOP");
}

void updateUiDemo() {
  if (!uiDemoActive) {
    return;
  }

  uint32_t now = millis();
  if (uiDemoStepStartedMs == 0) {
    runUiDemoStep(uiDemoIndex);
    uiDemoIndex++;
    return;
  }

  uint8_t currentIndex = uiDemoIndex == 0 ? 0 : uiDemoIndex - 1;
  if (now - uiDemoStepStartedMs < UI_DEMO_STEPS[currentIndex].durationMs) {
    return;
  }

  if (uiDemoIndex >= UI_DEMO_STEP_COUNT) {
    finishUiDemo("DEMO DONE");
    return;
  }

  runUiDemoStep(uiDemoIndex);
  uiDemoIndex++;
}
