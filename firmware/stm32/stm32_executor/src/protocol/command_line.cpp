#include "command_line.h"
#include <string.h>
#include "../../config.h"
#include "../core/board.h"
#include "../core/text_util.h"
#include "../ui/rgb.h"
#include "../ui/ui_state.h"
#include "dispatcher.h"
#include "protocol.h"

UiEventType classifyNetCommand(const String &command, const String &actionId) {
  if (containsUpperToken(command, "RFID") || containsUpperToken(actionId, "RFID")) {
    return UI_EVENT_RFID;
  }
  if (command == "NET:UI:LISTEN") {
    return UI_EVENT_LISTEN;
  }
  if (command == "NET:UI:THINK") {
    return UI_EVENT_THINK;
  }
  if (command == "NET:UI:ACTION") {
    return UI_EVENT_ACTION;
  }
  if (command == "NET:UI:ACK") {
    return UI_EVENT_ACK;
  }
  if (command == "NET:UI:OUTPUT") {
    return UI_EVENT_AI_IDLE;
  }
  if (command == "NET:UI:IDLE") {
    return UI_EVENT_AI_IDLE;
  }
  if (command == "NET:UI:ERROR") {
    return UI_EVENT_ERROR;
  }
  if (command == "NET:UI:DEMO") {
    return UI_EVENT_DEMO;
  }
  if (command == "NET:UI:DEMO:STOP" || command == "NET:UI:STATUS?") {
    return UI_EVENT_TELEMETRY;
  }
  if (command.startsWith("NET:UI:USER:")) {
    return UI_EVENT_RFID;
  }
  if (command.startsWith("NET:UI:")) {
    return UI_EVENT_ERROR;
  }
  if (command == "NET:UART?") {
    return UI_EVENT_UART;
  }
  if (command == "NET:I2C?") {
    return UI_EVENT_I2C;
  }
  if (command == "NET:TELEMETRY?") {
    return UI_EVENT_TELEMETRY;
  }
  if (command.startsWith("NET:RGB:")) {
    return UI_EVENT_TELEMETRY;
  }
  if (command == "NET:TTS:STOP" || command == "NET:AUDIO:STOP" ||
      command.startsWith("NET:TTS:") || command.startsWith("NET:TTSHEX:") ||
      command.startsWith("NET:VOLUME:")) {
    return UI_EVENT_TTS;
  }
  if (command.startsWith("NET:OLED:")) {
    return UI_EVENT_OLED;
  }
  if (command == "NET:BEEP") {
    return UI_EVENT_BEEP;
  }
  if (command.startsWith("NET:MUSIC:")) {
    return UI_EVENT_MUSIC;
  }
  if (command.startsWith("NET:FAN:ON")) {
    return UI_EVENT_FAN_ON;
  }
  if (command == "NET:FAN:OFF") {
    return UI_EVENT_FAN_OFF;
  }
  if (command == "NET:LOCK:ON") {
    return UI_EVENT_LOCK_ON;
  }
  if (command == "NET:LOCK:OFF") {
    return UI_EVENT_LOCK_OFF;
  }
  if (command == "NET:AI:BUSY") {
    return UI_EVENT_AI_BUSY;
  }
  if (command == "NET:AI:IDLE") {
    return UI_EVENT_AI_IDLE;
  }
  if (command == "NET:AI:OFF") {
    return UI_EVENT_AI_OFF;
  }
  if (command.startsWith("NET:SERVO:")) {
    return UI_EVENT_SERVO;
  }
  return UI_EVENT_ERROR;
}

String commandPreview(const String &command) {
  if (command == "NET:TTS:STOP" || command == "NET:AUDIO:STOP") {
    return "TTS STOP";
  }
  if (command.startsWith("NET:UI:USER:")) {
    return String("USER ") + compactForDisplay(command.substring(strlen("NET:UI:USER:")), 14);
  }
  if (command.startsWith("NET:VOLUME:")) {
    return String("VOLUME ") + compactForDisplay(command.substring(strlen("NET:VOLUME:")), 4);
  }
  if (command.startsWith("NET:TTSHEX:")) {
    uint16_t hexChars = command.length() - strlen("NET:TTSHEX:");
    return String("TTSHEX ") + String(hexChars / 2) + "B";
  }
  if (command.startsWith("NET:TTS:")) {
    return String("TTS ") + compactForDisplay(command.substring(strlen("NET:TTS:")), 16);
  }
  if (command.startsWith("NET:OLED:")) {
    return String("OLED ") + compactForDisplay(command.substring(strlen("NET:OLED:")), 16);
  }
  if (command.startsWith("NET:FAN:ON")) {
    return "FAN ON";
  }
  if (command == "NET:FAN:OFF") {
    return "FAN OFF";
  }
  if (command == "NET:BEEP") {
    return "BEEP";
  }
  if (command.startsWith("NET:MUSIC:")) {
    return String("MUSIC ") + compactForDisplay(command.substring(strlen("NET:MUSIC:")), 12);
  }
  if (command.startsWith("NET:LOCK:")) {
    return command.substring(strlen("NET:"));
  }
  if (command.startsWith("NET:AI:")) {
    return command.substring(strlen("NET:"));
  }
  if (command.startsWith("NET:UI:")) {
    return command.substring(strlen("NET:"));
  }
  if (command == "NET:UART?") {
    return "UART PING";
  }
  if (command == "NET:I2C?") {
    return "I2C SCAN";
  }
  if (command == "NET:TELEMETRY?") {
    return "TELEMETRY";
  }
  if (command.startsWith("NET:RGB:")) {
    return String("RGB ") + compactForDisplay(command.substring(strlen("NET:RGB:")), 12);
  }
  if (command.startsWith("NET:ULTRASONIC:")) {
    return String("ULTRASONIC ") + compactForDisplay(command.substring(strlen("NET:ULTRASONIC:")), 10);
  }
  if (command.startsWith("NET:MOTOR:")) {
    return String("MOTOR ") + compactForDisplay(command.substring(strlen("NET:MOTOR:")), 12);
  }
  if (command.startsWith("NET:SERVO:")) {
    return String("SERVO ") + compactForDisplay(command.substring(strlen("NET:SERVO:")), 12);
  }
  return compactForDisplay(command, 20);
}

bool stringMatchesAt(const String &text, int index, const char *prefix) {
  size_t prefixLen = strlen(prefix);
  if (index < 0 || index + (int)prefixLen > (int)text.length()) {
    return false;
  }
  for (size_t i = 0; i < prefixLen; ++i) {
    if (text.charAt(index + i) != prefix[i]) {
      return false;
    }
  }
  return true;
}

bool isKnownNetCommandStart(const String &text, int index) {
  if (index < 0 || index >= (int)text.length()) {
    return false;
  }
  if (index > 0 && text.charAt(index - 1) == ':') {
    return false;
  }

  static const char *prefixes[] = {
    "NET:CMD:",
    "NET:UART?",
    "NET:I2C?",
    "NET:TELEMETRY?",
    "NET:ULTRASONIC:",
    "NET:MOTOR:",
    "NET:TTS:",
    "NET:TTSHEX:",
    "NET:AUDIO:",
    "NET:VOLUME:",
    "NET:OLED:",
    "NET:BEEP",
    "NET:MUSIC:",
    "NET:FAN:",
    "NET:LOCK:",
    "NET:AI:",
    "NET:UI:",
    "NET:RGB:",
    "NET:SERVO:",
    "NET:RFID:"
  };

  for (uint8_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i) {
    if (stringMatchesAt(text, index, prefixes[i])) {
      return true;
    }
  }
  return false;
}

int findKnownNetCommandStart(const String &text, int fromIndex) {
  for (int i = fromIndex; i < (int)text.length(); ++i) {
    if (isKnownNetCommandStart(text, i)) {
      return i;
    }
  }
  return -1;
}

void handleSingleCommandLine(String line, const char *source) {
  uint32_t receivedMs = millis();
  if (strcmp(source, "ESP") == 0) {
    lastEspRxActivityMs = receivedMs;
  }
  line.trim();
  if (line.length() == 0) {
    return;
  }

  usbConsole.print("[");
  usbConsole.print(source);
  usbConsole.print("] ");
  usbConsole.println(line);

  uint32_t parseStartedMs = millis();
  ParsedCommand parsed = parseLine(line);
  uint32_t parseMs = millis() - parseStartedMs;
  if (parsed.command.length() == 0) {
    commandLightMeasureActive = false;
    commandHadLight = false;
    commandLightResponseMs = 0;
    beginUiEvent(UI_EVENT_ERROR, "PARSE ERR", parsed.actionId, parsed.wrapped, receivedMs);
    sendAck(parsed, false);
    finishUiEvent(parseMs, 0, millis() - receivedMs, false);
    logEventTiming(source, parsed, false);
    return;
  }

  beginUiEvent(classifyNetCommand(parsed.command, parsed.actionId), commandPreview(parsed.command),
               parsed.actionId, parsed.wrapped, receivedMs);
  commandHadLight = false;
  commandLightResponseMs = 0;
  commandLightMeasureActive = true;
  RgbState cueRgb = uiMachineBaseRgb();
  setRgb(cueRgb.red, cueRgb.green, cueRgb.blue);
  uint32_t actionStartedMs = millis();
  bool ok = executeNetCommand(parsed.command);
  uint32_t actionMs = millis() - actionStartedMs;
  commandLightMeasureActive = false;
  sendAck(parsed, ok);
  finishUiEvent(parseMs, actionMs, millis() - receivedMs, ok);
  logEventTiming(source, parsed, ok);
}

void handleLine(String line, const char *source) {
  line.trim();
  if (line.length() == 0) {
    return;
  }

  int firstStart = findKnownNetCommandStart(line, 0);
  if (firstStart > 0) {
    usbConsole.print("[WARN] dropping serial prefix bytes=");
    usbConsole.println(firstStart);
    line = line.substring(firstStart);
  }

  int nextStart = findKnownNetCommandStart(line, 1);
  if (nextStart < 0) {
    handleSingleCommandLine(line, source);
    return;
  }

  usbConsole.println("[WARN] split concatenated NET commands");
  int currentStart = 0;
  while (currentStart >= 0 && currentStart < (int)line.length()) {
    int followingStart = findKnownNetCommandStart(line, currentStart + 1);
    String segment = followingStart >= 0 ? line.substring(currentStart, followingStart) : line.substring(currentStart);
    segment.trim();
    if (segment.length() > 0) {
      handleSingleCommandLine(segment, source);
    }
    if (followingStart < 0) {
      break;
    }
    currentStart = followingStart;
  }
}
