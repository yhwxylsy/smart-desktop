#include "mic_capture.h"
#include <I2S.h>
#include "../../config.h"

bool micReady = false;
bool micBusy = false;

uint8_t *allocLargeBuffer(size_t size) {
  uint8_t *buffer = (uint8_t *)ps_malloc(size);
  if (buffer == nullptr) {
    buffer = (uint8_t *)malloc(size);
  }
  return buffer;
}

bool initMicrophone() {
  if (!MIC_PATH_ENABLED) {
    micReady = false;
    Serial.println("[MIC] disabled by firmware config; browser/laptop speech path stays available");
    return false;
  }
  I2S.setAllPins(-1, MIC_CLK_PIN, MIC_DATA_PIN, -1, -1);
  micReady = I2S.begin(PDM_MONO_MODE, MIC_SAMPLE_RATE, MIC_SAMPLE_BITS);
  if (micReady) {
    Serial.println("[MIC] ready at 16 kHz mono");
  } else {
    Serial.println("[MIC] init failed; browser speech fallback stays available");
  }
  return micReady;
}

void generateWavHeader(uint8_t *wavHeader, uint32_t wavSize, uint32_t sampleRate) {
  uint32_t fileSize = wavSize + MIC_WAV_HEADER_SIZE - 8;
  uint32_t byteRate = sampleRate * MIC_SAMPLE_BITS / 8;
  const uint8_t templateHeader[] = {
      'R', 'I', 'F', 'F',
      (uint8_t)(fileSize), (uint8_t)(fileSize >> 8), (uint8_t)(fileSize >> 16), (uint8_t)(fileSize >> 24),
      'W', 'A', 'V', 'E',
      'f', 'm', 't', ' ',
      0x10, 0x00, 0x00, 0x00,
      0x01, 0x00,
      0x01, 0x00,
      (uint8_t)(sampleRate), (uint8_t)(sampleRate >> 8), (uint8_t)(sampleRate >> 16), (uint8_t)(sampleRate >> 24),
      (uint8_t)(byteRate), (uint8_t)(byteRate >> 8), (uint8_t)(byteRate >> 16), (uint8_t)(byteRate >> 24),
      0x02, 0x00,
      0x10, 0x00,
      'd', 'a', 't', 'a',
      (uint8_t)(wavSize), (uint8_t)(wavSize >> 8), (uint8_t)(wavSize >> 16), (uint8_t)(wavSize >> 24),
  };
  memcpy(wavHeader, templateHeader, sizeof(templateHeader));
}

void conditionPcm16(uint8_t *pcmBuffer, size_t pcmSize) {
  int16_t *samples = (int16_t *)pcmBuffer;
  size_t sampleCount = pcmSize / sizeof(int16_t);
  if (sampleCount == 0) {
    return;
  }

  int64_t sum = 0;
  for (size_t i = 0; i < sampleCount; i++) {
    sum += samples[i];
  }
  int32_t dcOffset = (int32_t)(sum / (int64_t)sampleCount);

  for (size_t i = 0; i < sampleCount; i++) {
    int32_t value = ((int32_t)samples[i] - dcOffset) * MIC_DIGITAL_GAIN;
    if (value > 32767) {
      value = 32767;
    } else if (value < -32768) {
      value = -32768;
    }
    samples[i] = (int16_t)value;
  }
}

MicCapture captureMicWav() {
  MicCapture capture;
  size_t sampleSize = 0;
  uint32_t pcmSize = MIC_SAMPLE_RATE * (MIC_SAMPLE_BITS / 8) * MIC_RECORD_SECONDS;
  size_t wavSize = MIC_WAV_HEADER_SIZE + pcmSize;
  uint8_t *wavBuffer = allocLargeBuffer(wavSize);
  if (wavBuffer == nullptr) {
    Serial.println("[MIC] buffer alloc failed");
    return capture;
  }

  generateWavHeader(wavBuffer, pcmSize, MIC_SAMPLE_RATE);
  esp_i2s::i2s_read(esp_i2s::I2S_NUM_0, wavBuffer + MIC_WAV_HEADER_SIZE, pcmSize, &sampleSize, portMAX_DELAY);
  if (sampleSize == 0) {
    Serial.println("[MIC] recording failed");
    free(wavBuffer);
    return capture;
  }
  conditionPcm16(wavBuffer + MIC_WAV_HEADER_SIZE, sampleSize);
  generateWavHeader(wavBuffer, sampleSize, MIC_SAMPLE_RATE);
  capture.wavBuffer = wavBuffer;
  capture.wavSize = MIC_WAV_HEADER_SIZE + sampleSize;
  return capture;
}
