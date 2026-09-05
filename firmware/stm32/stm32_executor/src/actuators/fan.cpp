#include "fan.h"
#include "../../config.h"

uint8_t currentFanLevel = 0;
bool drv8833Connected = DRV8833_CONNECTED_BY_DEFAULT != 0;

void stopDrv8833() {
  currentFanLevel = 0;
  if (!drv8833Connected) {
    return;
  }
  analogWrite(PIN_DRV8833_IN1, 0);
  analogWrite(PIN_DRV8833_IN2, 0);
  digitalWrite(PIN_DRV8833_IN1, LOW);
  digitalWrite(PIN_DRV8833_IN2, LOW);
}

uint8_t drv8833FanDutyForLevel(uint8_t level) {
  if (level <= 1) {
    return DRV8833_FAN_LEVEL1_DUTY;
  }
  if (level == 2) {
    return DRV8833_FAN_LEVEL2_DUTY;
  }
  return DRV8833_FAN_LEVEL3_DUTY;
}

uint8_t fanLevelFromCommand(const String &command) {
  int lastColon = command.lastIndexOf(':');
  if (lastColon < 0 || lastColon + 1 >= (int)command.length()) {
    return 2;
  }
  int level = command.substring(lastColon + 1).toInt();
  if (level < 1) {
    return 1;
  }
  if (level > 3) {
    return 3;
  }
  return (uint8_t)level;
}

bool driveFanOn(uint8_t level) {
  if (!drv8833Connected) {
    return false;
  }
  currentFanLevel = level < 1 ? 1 : (level > 3 ? 3 : level);
  uint8_t duty = drv8833FanDutyForLevel(level);
  // Current fan wiring spins the useful direction with IN2 PWM and IN1 LOW.
  analogWrite(PIN_DRV8833_IN1, 0);
  digitalWrite(PIN_DRV8833_IN1, LOW);
  analogWrite(PIN_DRV8833_IN2, duty);
  return true;
}
