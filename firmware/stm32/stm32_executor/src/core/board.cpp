#include "board.h"
#include "../../config.h"
#include "../protocol/command_line.h"

SoftwareSerial espAckSerial(PB4, PB3);  // RX pin is unused. PB3 is the documented software TX.
HardwareSerial usbConsole(PA3, PA2);    // HardwareSerial(0); USB/debug console (SWD/USB CDC)
HardwareSerial espCommandSerial(PB11, PB10);  // HardwareSerial(3); ESP32 command link + SYN6288

unsigned long lastEspRxActivityMs = 0;
static uint32_t espEmptyDelimiterCount = 0;

void writeBack(const String &line) {
  usbConsole.println(line);
  espAckSerial.println(line);
}

void pollSerial(Stream &stream, String &buffer, const char *source) {
  while (stream.available()) {
    char ch = (char)stream.read();
    if (strcmp(source, "ESP") == 0) {
      lastEspRxActivityMs = millis();
    }
    if (ch == '\n' || ch == '\r') {
      if (buffer.length() == 0 && strcmp(source, "ESP") == 0) {
        espEmptyDelimiterCount++;
        if (espEmptyDelimiterCount <= 5 || (espEmptyDelimiterCount % 20) == 0) {
          usbConsole.print("[ESP] empty delimiter count=");
          usbConsole.println(espEmptyDelimiterCount);
        }
        continue;
      }
      handleLine(buffer, source);
      buffer = "";
    } else {
      buffer += ch;
    }
  }
}
