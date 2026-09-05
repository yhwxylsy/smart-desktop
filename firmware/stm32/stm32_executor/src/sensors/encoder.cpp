#include "encoder.h"
#include "../../config.h"
#include "../audio/tts.h"

long encoderPosition = 0;
long encoderDeltaSinceTelemetry = 0;

static uint8_t encoderLastState = 0;
static int8_t encoderVolumeTicks = 0;

uint8_t readEncoderState() {
  uint8_t state = 0;
  if (digitalRead(PIN_ENCODER_A) == HIGH) {
    state |= 0x01;
  }
  if (digitalRead(PIN_ENCODER_B) == HIGH) {
    state |= 0x02;
  }
  return state;
}

void initEncoder() {
  encoderLastState = readEncoderState();
}

void updateEncoder() {
  static const int8_t TRANSITIONS[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0
  };
  uint8_t state = readEncoderState();
  uint8_t index = (encoderLastState << 2) | state;
  int8_t delta = TRANSITIONS[index];
  if (delta != 0) {
    encoderPosition += delta;
    encoderDeltaSinceTelemetry += delta;
    encoderVolumeTicks += delta;
    if (encoderVolumeTicks >= ENCODER_STEPS_PER_DETENT) {
      encoderVolumeTicks = 0;
      setSpeechVolume("UP", true);
    } else if (encoderVolumeTicks <= -ENCODER_STEPS_PER_DETENT) {
      encoderVolumeTicks = 0;
      setSpeechVolume("DOWN", true);
    }
  }
  encoderLastState = state;
}
