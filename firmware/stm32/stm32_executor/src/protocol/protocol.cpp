#include "protocol.h"
#include "../../config.h"
#include "../core/board.h"

void sendAck(const ParsedCommand &parsed, bool ok) {
  if (parsed.wrapped && parsed.actionId.length() > 0) {
    writeBack("BT:ACK:" + parsed.actionId + ":" + (ok ? "OK" : "ERR"));
  } else {
    writeBack(ok ? "BT:OK" : "BT:ERR");
  }
}

ParsedCommand parseLine(String line) {
  line.trim();
  ParsedCommand parsed;
  parsed.actionId = "";
  parsed.command = line;
  parsed.wrapped = false;

  if (!line.startsWith("NET:CMD:")) {
    return parsed;
  }

  int idStart = strlen("NET:CMD:");
  int commandStart = line.indexOf(":NET:", idStart);
  if (commandStart < 0) {
    parsed.command = "";
    return parsed;
  }

  parsed.wrapped = true;
  parsed.actionId = line.substring(idStart, commandStart);
  parsed.command = line.substring(commandStart + 1);
  return parsed;
}
