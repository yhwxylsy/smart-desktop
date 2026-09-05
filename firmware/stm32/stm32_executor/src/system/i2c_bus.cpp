#include "i2c_bus.h"
#include <Wire.h>
#include "../../config.h"
#include "../core/board.h"
#include "../ui/oled.h"
#include "../ui/ui_state.h"

uint8_t scanI2cBus() {
  uint8_t found = 0;
  usbConsole.println("[I2C] scan start");
  for (uint8_t address = 1; address < 0x78; ++address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      found++;
      usbConsole.print("[I2C] found 0x");
      if (address < 0x10) {
        usbConsole.print("0");
      }
      usbConsole.println(address, HEX);
    } else if (error == 4) {
      usbConsole.print("[I2C] unknown error at 0x");
      if (address < 0x10) {
        usbConsole.print("0");
      }
      usbConsole.println(address, HEX);
    }
  }
  if (found == 0) {
    usbConsole.println("[I2C] no devices found");
  }
  usbConsole.print("[I2C] scan done count=");
  usbConsole.println(found);
  return found;
}

bool handleI2cScanCommand() {
  uint8_t found = scanI2cBus();
  if (!oledAvailable && (oledProbe(OLED_ADDR_PRIMARY) || oledProbe(OLED_ADDR_FALLBACK))) {
    initializeOled();
    oledRenderPending = true;
  }
  uiEvent.detail = String("I2C FOUND ") + String(found);
  oledRenderPending = true;
  return found > 0;
}
