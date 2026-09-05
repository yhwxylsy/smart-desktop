#include "buzzer.h"
#include "../../config.h"
#include "../core/board.h"
#include "../core/text_util.h"

static const MelodyNote MELODY_SUCCESS[] = {
  {523, 120}, {659, 120}, {784, 160}, {1047, 260}
};

static const MelodyNote MELODY_ALERT[] = {
  {988, 100}, {0, 70}, {988, 100}, {0, 70}, {784, 240}
};

static const MelodyNote MELODY_SCALE[] = {
  {262, 110}, {294, 110}, {330, 110}, {349, 110},
  {392, 110}, {440, 110}, {494, 110}, {523, 220}
};

static const MelodyNote MELODY_STARTUP[] = {
  {392, 100}, {523, 100}, {659, 120}, {784, 220}
};

static const MelodyNote MELODY_BIRTHDAY[] = {
  {392, 170}, {392, 170}, {440, 340}, {392, 340}, {523, 340}, {494, 520},
  {0, 120},
  {392, 170}, {392, 170}, {440, 340}, {392, 340}, {587, 340}, {523, 520}
};

static const MelodyNote *activeMelody = nullptr;
static uint8_t activeMelodyLength = 0;
static uint8_t activeMelodyIndex = 0;
static uint32_t musicStepStartedMs = 0;
static uint16_t musicStepTotalMs = 0;
static bool musicPlaying = false;

void stopMusic() {
  noTone(PIN_BUZZER);
  activeMelody = nullptr;
  activeMelodyLength = 0;
  activeMelodyIndex = 0;
  musicStepStartedMs = 0;
  musicStepTotalMs = 0;
  musicPlaying = false;
}

void startMelodyStep() {
  if (!musicPlaying || activeMelody == nullptr || activeMelodyIndex >= activeMelodyLength) {
    stopMusic();
    return;
  }

  const MelodyNote &note = activeMelody[activeMelodyIndex];
  if (note.frequency == 0) {
    noTone(PIN_BUZZER);
  } else {
    tone(PIN_BUZZER, note.frequency, note.durationMs);
  }
  musicStepStartedMs = millis();
  musicStepTotalMs = note.durationMs + MUSIC_NOTE_GAP_MS;
}

bool startMusic(const MelodyNote *melody, uint8_t length) {
  if (melody == nullptr || length == 0) {
    return false;
  }
  stopMusic();
  activeMelody = melody;
  activeMelodyLength = length;
  activeMelodyIndex = 0;
  musicPlaying = true;
  startMelodyStep();
  return true;
}

void updateMusicPlayer() {
  if (!musicPlaying) {
    return;
  }
  if (musicStepStartedMs == 0) {
    startMelodyStep();
    return;
  }
  if (millis() - musicStepStartedMs < musicStepTotalMs) {
    return;
  }
  activeMelodyIndex++;
  if (activeMelodyIndex >= activeMelodyLength) {
    stopMusic();
    return;
  }
  startMelodyStep();
}

bool startMusicByPreset(String preset) {
  preset.trim();
  String name = upperCopy(preset);
  if (name == "STOP" || name == "OFF") {
    stopMusic();
    return true;
  }
  if (name == "LIST?") {
    writeBack("BT:MUSIC:LIST:SUCCESS,ALERT,SCALE,STARTUP,BIRTHDAY");
    return true;
  }
  if (name.length() == 0 || name == "SUCCESS" || name == "OK") {
    return startMusic(MELODY_SUCCESS, (uint8_t)(sizeof(MELODY_SUCCESS) / sizeof(MELODY_SUCCESS[0])));
  }
  if (name == "ALERT" || name == "WARN" || name == "WARNING") {
    return startMusic(MELODY_ALERT, (uint8_t)(sizeof(MELODY_ALERT) / sizeof(MELODY_ALERT[0])));
  }
  if (name == "SCALE") {
    return startMusic(MELODY_SCALE, (uint8_t)(sizeof(MELODY_SCALE) / sizeof(MELODY_SCALE[0])));
  }
  if (name == "STARTUP" || name == "BOOT") {
    return startMusic(MELODY_STARTUP, (uint8_t)(sizeof(MELODY_STARTUP) / sizeof(MELODY_STARTUP[0])));
  }
  if (name == "BIRTHDAY" || name == "HAPPY") {
    return startMusic(MELODY_BIRTHDAY, (uint8_t)(sizeof(MELODY_BIRTHDAY) / sizeof(MELODY_BIRTHDAY[0])));
  }
  return false;
}

bool playBeep(uint16_t frequency, uint16_t durationMs) {
  stopMusic();
  tone(PIN_BUZZER, frequency, durationMs);
  return true;
}
