#include "serial_cli.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "../../config.h"
#include "../config/config_store.h"
#include "../core/hex_util.h"
#include "../net/wifi_manager.h"
#include "../net/websocket_link.h"
#include "../rfid/rfid_reader.h"
#include "../bridge/stm32_link.h"
#include "../mic/mic_pipeline.h"

static String usbLine;

// ---- 命令处理器（每个函数逐字保留原 if 分支体，仅改造成独立函数）----

static void handleCfgWifi(const String &line) {
  String value = line.substring(strlen("CFG:WIFI:"));
  if (value == "SHOW") {
    Serial.print("[CFG] ssid=");
    Serial.print(configStore::wifiSsid());
    Serial.print(" server=");
    Serial.print(configStore::secure() ? "https://" : "http://");
    Serial.print(configStore::host());
    Serial.print(":");
    Serial.println(configStore::port());
  } else if (value == "SCAN") {
    scanWifi();
  } else {
    int comma = value.indexOf(',');
    if (comma < 0) {
      Serial.println("[CFG] use CFG:WIFI:<ssid>,<password>");
    } else {
      configStore::setWifi(value.substring(0, comma), value.substring(comma + 1));
      configStore::save();
      Serial.println("[CFG] wifi saved");
      WiFi.disconnect(true);
      delay(200);
      connectWifi();
      startWebSocket();
    }
  }
}

static void handleCfgNetTcp(const String &line) {
  probeTcp(line.substring(strlen("CFG:NET:TCP:")));
}

static void handleCfgServer(const String &line) {
  if (configStore::parseServerUrl(line.substring(strlen("CFG:SERVER:")))) {
    configStore::save();
    wsStarted = false;
    webSocket.disconnect();
    Serial.println("[CFG] server saved");
    startWebSocket();
  } else {
    Serial.println("[CFG] invalid server");
  }
}

static void handleCfgToken(const String &line) {
  String token = line.substring(strlen("CFG:TOKEN:"));
  token.trim();
  configStore::setToken(token);
  configStore::save();
  Serial.println(configStore::deviceToken().length() > 0 ? "[CFG] device token saved" : "[CFG] device token cleared");
}

static void handleCfgReset(const String &line) {
  configStore::reset();
  Serial.println("[CFG] cleared");
}

static void handleCfgUartPing(const String &line) {
  sendToStm32("NET:UART?");
}

static void handleCfgRfidStatus(const String &line) {
  printRfidStatus(true);
}

static void handleCfgRfidReset(const String &line) {
  byte version = initializeRfidReader();
  Serial.printf("[RFID] reinitialized version=0x%02X\n", version);
  printRfidStatus(true);
}

static void handleMicRecCue(const String &line) {
  captureAndUploadMicAfterCue("");
}

static void handleMicRecCueAsrOnly(const String &line) {
  captureAndUploadMicAfterCue("", false, "esp32_mic_asr_test");
}

static void handleMicRecCueArg(const String &line) {
  captureAndUploadMicAfterCue(line.substring(strlen("CFG:MIC:REC:CUE:")));
}

static void handleMicRecAsrOnly(const String &line) {
  captureAndUploadMic(false, "esp32_mic_asr_test");
}

static void handleMicRec(const String &line) {
  captureAndUploadMic();
}

static void handleMicSelfTest(const String &line) {
  String phrase = "";
  if (line.startsWith("CFG:MIC:SELFTEST:")) {
    phrase = line.substring(strlen("CFG:MIC:SELFTEST:"));
  }
  runMicSelfTest(phrase);
}

static void handleCfgTts(const String &line) {
  sendToStm32("NET:TTSHEX:" + utf8Hex(line.substring(strlen("CFG:TTS:"))));
}

static void handleCfgOled(const String &line) {
  sendToStm32("NET:OLED:" + line.substring(strlen("CFG:OLED:")));
}

static void handleChat(const String &line) {
  JsonDocument doc;
  doc["type"] = "text";
  doc["text"] = line.substring(strlen("CHAT:"));
  String body;
  serializeJson(doc, body);
  if (wsConnected) {
    webSocket.sendTXT(body);
  } else {
    Serial.println("[CHAT] websocket not connected");
  }
}

// ---- 表驱动分发（顺序 = 原 if 链顺序；exact=false 表示前缀匹配）----

struct CliEntry {
  const char *prefix;
  bool exact;
  void (*run)(const String &line);
};

static const CliEntry CLI_COMMANDS[] = {
    {"CFG:WIFI:", false, handleCfgWifi},
    {"CFG:NET:TCP:", false, handleCfgNetTcp},
    {"CFG:SERVER:", false, handleCfgServer},
    {"CFG:TOKEN:", false, handleCfgToken},
    {"CFG:RESET", true, handleCfgReset},
    {"CFG:UART:PING", true, handleCfgUartPing},
    {"CFG:RFID:STATUS", true, handleCfgRfidStatus},
    {"CFG:RFID:RESET", true, handleCfgRfidReset},
    {"CFG:MIC:REC:CUE", true, handleMicRecCue},
    {"CFG:MIC:REC:CUE:ASRONLY", true, handleMicRecCueAsrOnly},
    {"CFG:MIC:REC:CUE:", false, handleMicRecCueArg},
    {"CFG:MIC:REC:ASRONLY", true, handleMicRecAsrOnly},
    {"CFG:MIC:REC", true, handleMicRec},
    {"CFG:MIC:SELFTEST", false, handleMicSelfTest},
    {"CFG:TTS:", false, handleCfgTts},
    {"CFG:OLED:", false, handleCfgOled},
    {"CHAT:", false, handleChat},
};

void handleSerialCommand(String line) {
  line.trim();
  if (line.isEmpty()) {
    return;
  }

  for (const CliEntry &entry : CLI_COMMANDS) {
    bool matched = entry.exact ? (line == entry.prefix) : line.startsWith(entry.prefix);
    if (matched) {
      entry.run(line);
      return;
    }
  }

  Serial.println("[CFG] unknown command");
}

void pollUsbSerial() {
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\n' || ch == '\r') {
      handleSerialCommand(usbLine);
      usbLine = "";
    } else {
      usbLine += ch;
    }
  }
}
