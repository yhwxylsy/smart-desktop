#include "stm32_link.h"
#include "../../config.h"
#include "../config/config_store.h"
#include "backend_bridge.h"
#include "telemetry_bridge.h"

HardwareSerial stm32Tx(1);
HardwareSerial stm32Rx(2);

unsigned long lastAckMs = 0;
static unsigned long lastStm32TxMs = 0;
static unsigned long lastUartKeepaliveMs = 0;
static String stm32Line;

String stm32LogLine(const String &line) {
  int ttsHexPos = line.indexOf("NET:TTSHEX:");
  if (ttsHexPos < 0) {
    return line;
  }

  String prefix = line.substring(0, ttsHexPos);
  String payload = line.substring(ttsHexPos + strlen("NET:TTSHEX:"));
  return prefix + "NET:TTSHEX:[" + String(payload.length() / 2) + " bytes]";
}

void sendToStm32(const String &line, uint16_t postDelayMs) {
  Serial.print("[STM32 TX] ");
  Serial.println(stm32LogLine(line));
  stm32Tx.println(line);
  stm32Tx.flush();
  lastStm32TxMs = millis();
  if (postDelayMs > 0) {
    delay(postDelayMs);
  }
}

void sendUartKeepalive() {
  if (millis() - lastUartKeepaliveMs < UART_KEEPALIVE_INTERVAL_MS) {
    return;
  }
  if (lastStm32TxMs > 0 && millis() - lastStm32TxMs < UART_TX_QUIET_MS) {
    return;
  }
  if (lastAckMs > 0 && millis() - lastAckMs < UART_OK_WINDOW_MS / 2) {
    return;
  }
  lastUartKeepaliveMs = millis();
  sendToStm32("NET:UART?");
}

void pollStm32() {
  while (stm32Rx.available()) {
    char ch = (char)stm32Rx.read();
    if (ch == '\n' || ch == '\r') {
      stm32Line.trim();
      if (!stm32Line.startsWith("BT:") && !stm32Line.startsWith("{")) {
        int btPos = stm32Line.indexOf('B');
        int jsonPos = stm32Line.indexOf('{');
        int keepPos = -1;
        if (btPos >= 0 && jsonPos >= 0) {
          keepPos = btPos < jsonPos ? btPos : jsonPos;
        } else if (btPos >= 0) {
          keepPos = btPos;
        } else if (jsonPos >= 0) {
          keepPos = jsonPos;
        }
        if (keepPos > 0) {
          stm32Line = stm32Line.substring(keepPos);
        }
      }
      if (stm32Line.length() > 0) {
        Serial.print("[STM32 RX] ");
        Serial.println(stm32Line);
        if (stm32Line.startsWith("BT:ACK:")) {
          lastAckMs = millis();
          if (stm32Line.startsWith("BT:ACK:act_")) {
            sendAckToBackend(stm32Line);
          }
        } else if (stm32Line.startsWith("BT:PONG:")) {
          lastAckMs = millis();
        } else if (stm32Line.startsWith("BT:BTN:")) {
          handleStm32ButtonEvent(stm32Line);
        } else if (stm32Line.startsWith("BT:{") || stm32Line.startsWith("{")) {
          sendTelemetryToBackend(stm32Line);
        }
      }
      stm32Line = "";
    } else {
      stm32Line += ch;
    }
  }
}
