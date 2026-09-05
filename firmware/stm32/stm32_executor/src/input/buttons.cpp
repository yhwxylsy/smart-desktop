#include "buttons.h"
#include "../../config.h"
#include "../audio/tts.h"
#include "../core/board.h"
#include "../ui/oled.h"
#include "../ui/rgb.h"
#include "../ui/ui_state.h"

String lastButtonEvent = "-";
uint32_t lastButtonEventMs = 0;

static bool key2LastReading = false;
static bool key2StablePressed = false;
static bool key2HoldStartSent = false;
static uint32_t key2LastChangeMs = 0;
static uint32_t key2PressedMs = 0;

static bool infoButtonLastReading = false;
static bool infoButtonStablePressed = false;
static bool infoButtonLongSent = false;
static uint32_t infoButtonLastChangeMs = 0;
static uint32_t infoButtonPressedMs = 0;

void writeButtonEvent(const String &event) {
  lastButtonEvent = event;
  lastButtonEventMs = millis();
  uiEvent.detail = event;
  oledRenderPending = true;
  writeBack(String("BT:BTN:") + event);
}

void handleKey2Pressed(uint32_t now) {
  uint32_t startedMs = now;
  beginUiEvent(UI_EVENT_LISTEN, "K2 INTERRUPT", "key2", false, startedMs);
  commandHadLight = false;
  commandLightResponseMs = 0;
  commandLightMeasureActive = true;
  setRgb(false, true, true);
  commandLightMeasureActive = false;
  stopSpeechOutput();
  finishUiEvent(0, 0, millis() - startedMs, true);
  writeButtonEvent("KEY2:DOWN");
}

void handleKey2HoldStart(uint32_t now) {
  uint32_t duration = now - key2PressedMs;
  beginUiEvent(UI_EVENT_LISTEN, "PTT RECORDING", "key2", false, now);
  setRgb(false, true, true);
  writeButtonEvent(String("KEY2:HOLD_START:") + String(duration));
}

void handleKey2Released(uint32_t now) {
  uint32_t duration = now - key2PressedMs;
  writeButtonEvent(String("KEY2:UP:") + String(duration));
  if (key2HoldStartSent) {
    beginUiEvent(UI_EVENT_THINK, "PTT UPLOAD", "key2", false, now);
  } else {
    beginUiEvent(UI_EVENT_ACTION, "OUTPUT STOP", "key2", false, now);
    stopSpeechOutput();
    writeButtonEvent(String("KEY2:SHORT:") + String(duration));
  }
}

void updateKey2Button() {
  bool pressed = digitalRead(PIN_DEMO_BUTTON) == LOW;
  uint32_t now = millis();

  if (pressed != key2LastReading) {
    key2LastReading = pressed;
    key2LastChangeMs = now;
  }

  if (now - key2LastChangeMs < BUTTON_DEBOUNCE_MS) {
    return;
  }

  if (pressed == key2StablePressed) {
    if (key2StablePressed && !key2HoldStartSent && now - key2PressedMs >= KEY2_HOLD_START_MS) {
      key2HoldStartSent = true;
      handleKey2HoldStart(now);
    }
    return;
  }

  key2StablePressed = pressed;
  if (key2StablePressed) {
    key2PressedMs = now;
    key2HoldStartSent = false;
    handleKey2Pressed(now);
  } else {
    handleKey2Released(now);
  }
}

void handleInfoButtonShort() {
  uint8_t next = ((uint8_t)infoScreen + 1) % (uint8_t)INFO_SCREEN_COUNT;
  infoScreen = (InfoScreen)next;
  infoOverlayUntilMs = millis() + SCREEN_OVERLAY_MS;
  writeButtonEvent(String("KEY1:PAGE:") + String(next));
}

void handleInfoButtonLong() {
  infoScreen = INFO_SCREEN_MAIN;
  infoOverlayUntilMs = millis() + SCREEN_OVERLAY_MS;
  writeButtonEvent("KEY1:HOME");
}

void updateInfoButton() {
  bool pressed = digitalRead(PIN_INFO_BUTTON) == LOW;
  uint32_t now = millis();

  if (pressed != infoButtonLastReading) {
    infoButtonLastReading = pressed;
    infoButtonLastChangeMs = now;
  }

  if (now - infoButtonLastChangeMs < BUTTON_DEBOUNCE_MS) {
    return;
  }

  if (pressed == infoButtonStablePressed) {
    if (infoButtonStablePressed && !infoButtonLongSent && now - infoButtonPressedMs >= KEY1_LONG_PRESS_MS) {
      infoButtonLongSent = true;
      handleInfoButtonLong();
    }
    return;
  }

  infoButtonStablePressed = pressed;
  if (infoButtonStablePressed) {
    infoButtonPressedMs = now;
    infoButtonLongSent = false;
  } else if (!infoButtonLongSent) {
    handleInfoButtonShort();
  }
}
