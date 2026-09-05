#include "ultrasonic.h"
#include "../../config.h"

bool ultrasonicEnabled = ULTRASONIC_ENABLED_BY_DEFAULT != 0;

bool readDistanceCm(float &distanceCm) {
  if (!ultrasonicEnabled) {
    return false;
  }

  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);

  uint32_t durationUs = pulseIn(PIN_ULTRASONIC_ECHO, HIGH, ULTRASONIC_TIMEOUT_US);
  if (durationUs == 0) {
    return false;
  }

  distanceCm = durationUs / 58.0f;
  return true;
}
