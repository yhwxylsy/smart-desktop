#include "oled_screens.h"
#include "../../config.h"
#include "../actuators/fan.h"
#include "../audio/tts.h"
#include "../core/board.h"
#include "../core/text_util.h"
#include "../input/buttons.h"
#include "../sensors/telemetry.h"
#include "../system/user_context.h"
#include "rgb.h"

void renderStateHeader(UiMachineState state) {
  oledFillRect(0, 0, OLED_WIDTH, 10, true);
  oledDrawText(4, 1, uiMachineCode(state), 16, true);
  oledDrawText(91, 1, String("P") + String((uint8_t)infoScreen), 3, true);
  oledDrawText(108, 1, String("F") + String(oledFps), 4, true);
}

void renderStateBody(UiMachineState state) {
  uint32_t now = millis();
  oledDrawTextCentered(uiMachineCompactTitle(state), 13, 20);
  oledDrawLine(4, 24, 123, 24, true);

  switch (state) {
    case UI_STATE_BOOT:
      oledDrawText(6, 29, "OLED/AHT/UART CHECK", 20, false);
      oledDrawText(6, 39, String("I2C 0x") + String(oledAddress, HEX), 20, false);
      oledDrawText(6, 49, "FEATURES ONLINE...", 20, false);
      break;
    case UI_STATE_LOCKED:
      oledDrawText(6, 29, "USER -", 20, false);
      oledDrawText(6, 39, "RFID REQUIRED", 20, false);
      oledDrawText(6, 49, "K1 PAGE  K2 STOP", 20, false);
      break;
    case UI_STATE_READY: {
      String temperature = latestAhtOk ? String(latestTemperatureC, 1) + "C" : "--.-C";
      String humidity = latestAhtOk ? String(latestHumidityPct, 0) + "%" : "--%";
      String distance = latestDistanceOk ? String(latestDistanceCm, 0) + "cm" : "--cm";
      oledDrawText(6, 29, String("USER ") + compactForDisplay(currentUserId, 14), 20, false);
      oledDrawText(6, 39, temperature + " " + humidity + " " + distance, 20, false);
      if (now < volumeOverlayUntilMs) {
        oledDrawText(6, 49, String("VOL ") + String(speechVolumePercent) + "% K1 PAGE", 20, false);
      } else {
        oledDrawText(6, 49, "K1 PAGE K2 HOLD REC", 20, false);
      }
      break;
    }
    case UI_STATE_LISTENING:
      oledDrawText(6, 29, "LAPTOP MIC PTT", 20, false);
      oledDrawText(6, 39, "RELEASE TO UPLOAD", 20, false);
      oledDrawText(6, 49, String("USER ") + compactForDisplay(currentUserId, 14), 20, false);
      break;
    case UI_STATE_PROCESSING:
      oledDrawText(6, 29, "ASR/QWEN PIPELINE", 20, false);
      oledDrawText(6, 39, "WAIT FOR RESULT", 20, false);
      oledDrawText(6, 49, String("USER ") + compactForDisplay(currentUserId, 14), 20, false);
      break;
    case UI_STATE_SPEAKING:
      oledDrawText(6, 29, "SYN6288 OUTPUT", 20, false);
      oledDrawText(6, 39, "K2 SHORT: STOP", 20, false);
      oledDrawText(6, 49, "K2 HOLD: BARGE-IN", 20, false);
      break;
    case UI_STATE_EXECUTING:
      oledDrawText(6, 29, "COMMAND", 20, false);
      oledDrawText(6, 39, compactForDisplay(uiEvent.detail, 20), 20, false);
      oledDrawText(6, 49, String("ACK ") + String(uiEvent.ackSeen ? (uiEvent.ackOk ? "OK" : "ERR") : "..."), 20, false);
      break;
    case UI_STATE_ERROR:
      oledDrawText(6, 29, "CHECK LINK / RETRY", 20, false);
      oledDrawText(6, 39, compactForDisplay(uiEvent.detail, 20), 20, false);
      oledDrawText(6, 49, String("BTN ") + compactForDisplay(lastButtonEvent, 15), 20, false);
      break;
  }
}

void renderInfoScreen(uint32_t now) {
  switch (infoScreen) {
    case INFO_SCREEN_USER:
      oledDrawTextCentered("USER CONTEXT", 13, 20);
      oledDrawLine(4, 24, 123, 24, true);
      oledDrawText(6, 29, String("ID ") + compactForDisplay(currentUserId, 17), 20, false);
      oledDrawText(6, 39, String("CARD ") + compactForDisplay(currentCardUid, 15), 20, false);
      oledDrawText(6, 49, String("MODE ") + compactForDisplay(currentUserMode, 14), 20, false);
      break;
    case INFO_SCREEN_LINK:
      oledDrawTextCentered("LINK / FPS", 13, 20);
      oledDrawLine(4, 24, 123, 24, true);
      oledDrawText(6, 29, String("FPS ") + String(oledFps) + " FLUSH 4MS", 20, false);
      oledDrawText(6, 39, String("UART ") + (millis() - lastEspRxActivityMs < 5000 ? "ACTIVE" : "QUIET"), 20, false);
      oledDrawText(6, 49, String("ACK ") + String(uiEvent.ackSeen ? (uiEvent.ackOk ? "OK" : "ERR") : "WAIT"), 20, false);
      break;
    case INFO_SCREEN_SENSORS: {
      String temperature = latestAhtOk ? String(latestTemperatureC, 1) + "C" : "--.-C";
      String humidity = latestAhtOk ? String(latestHumidityPct, 0) + "%" : "--%";
      String distance = latestDistanceOk ? String(latestDistanceCm, 0) + "CM" : "--CM";
      oledDrawTextCentered("SENSORS", 13, 20);
      oledDrawLine(4, 24, 123, 24, true);
      oledDrawText(6, 29, temperature + " HUM " + humidity, 20, false);
      oledDrawText(6, 39, String("DIST ") + distance + " " + latestDistanceZone, 20, false);
      oledDrawText(6, 49, String("POT ") + String(latestPotPct) + "% NTC " + String(latestNtcPct) + "%", 20, false);
      break;
    }
    case INFO_SCREEN_ACTUATORS:
      oledDrawTextCentered("ACTUATORS", 13, 20);
      oledDrawLine(4, 24, 123, 24, true);
      oledDrawText(6, 29, String("VOL ") + String(speechVolumePercent) + "% FAN L" + String(currentFanLevel), 20, false);
      oledDrawText(6, 39, String("LOCK ") + (currentLockOn ? "ON" : "OFF") + " RGB " + latestRgbStatus, 20, false);
      oledDrawText(6, 49, String("BTN ") + compactForDisplay(lastButtonEvent, 15), 20, false);
      break;
    case INFO_SCREEN_MAIN:
    case INFO_SCREEN_COUNT:
      renderStateBody(uiMachineState);
      break;
  }
  if (now < infoOverlayUntilMs && infoScreen != INFO_SCREEN_MAIN) {
    oledDrawText(96, 55, "K1>", 4, false);
  }
}

void renderStatusScreen(uint32_t now) {
  renderStateHeader(uiMachineState);
  if (infoScreen == INFO_SCREEN_MAIN) {
    renderStateBody(uiMachineState);
  } else {
    renderInfoScreen(now);
  }
}

void renderSystemOled() {
  if (!oledAvailable) {
    return;
  }

  clearOledBuffer();
  uint32_t now = millis();
  if (oledFpsWindowMs == 0) {
    oledFpsWindowMs = now;
  }
  oledFrameCounter++;
  if (now - oledFpsWindowMs >= 1000) {
    uint32_t elapsedMs = now - oledFpsWindowMs;
    if (elapsedMs == 0) {
      elapsedMs = 1;
    }
    oledFps = (uint8_t)((oledFrameCounter * 1000UL) / elapsedMs);
    oledFrameCounter = 0;
    oledFpsWindowMs = now;
  }
  renderStatusScreen(now);
  oledFlushPage = 0;
  oledRenderPending = false;
  oledDirty = true;
}

void showOledText(const String &text) {
  usbConsole.print("[OLED] ");
  usbConsole.println(text);
  uiEvent.detail = String("OLED ") + text;
  oledRenderPending = true;
}

void updateSystemUi() {
  uint32_t now = millis();
  updateUiMachineState(now);
  updateRgbAnimation();
  if (oledAvailable && (oledRenderPending || now - lastUiFrameMs >= UI_FRAME_INTERVAL_MS)) {
    lastUiFrameMs = now;
    renderSystemOled();
  }
  flushOledPage();
}
