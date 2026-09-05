#include "servo.h"
#include "../../config.h"

static bool servoActive = false;
static bool servoPulseHigh = false;
static uint16_t servoPulseWidthUs = 1500;
static uint32_t servoPulseStartedUs = 0;
static uint32_t nextServoPulseUs = 0;
static uint32_t lastServoCommandMs = 0;

bool parseServoAngle(const String &value, uint8_t &angle) {
  String text = value;
  text.trim();
  if (text.length() == 0) {
    return false;
  }
  for (uint16_t i = 0; i < text.length(); ++i) {
    if (!isDigit(text.charAt(i))) {
      return false;
    }
  }
  long parsed = text.toInt();
  if (parsed < 0 || parsed > 180) {
    return false;
  }
  angle = (uint8_t)parsed;
  return true;
}

void setServoAngle(uint8_t angle) {
  servoPulseWidthUs = SERVO_MIN_PULSE_US +
                      ((uint32_t)(SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * angle) / 180;
  lastServoCommandMs = millis();
  nextServoPulseUs = micros();
  servoPulseHigh = false;
  servoActive = true;
  digitalWrite(PIN_SERVO, LOW);
}

void updateServoPulse() {
  if (!servoActive) {
    return;
  }

  uint32_t nowUs = micros();
  if (servoPulseHigh) {
    if ((uint32_t)(nowUs - servoPulseStartedUs) >= servoPulseWidthUs) {
      digitalWrite(PIN_SERVO, LOW);
      servoPulseHigh = false;
      nextServoPulseUs = servoPulseStartedUs + SERVO_PULSE_PERIOD_US;
    }
  } else if ((int32_t)(nowUs - nextServoPulseUs) >= 0) {
    digitalWrite(PIN_SERVO, HIGH);
    servoPulseStartedUs = nowUs;
    servoPulseHigh = true;
  }

  if (!servoPulseHigh && millis() - lastServoCommandMs > SERVO_HOLD_MS) {
    servoActive = false;
    digitalWrite(PIN_SERVO, LOW);
  }
}
