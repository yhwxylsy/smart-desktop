#include "aht20.h"
#include <Wire.h>
#include "../../config.h"

bool aht20Initialized = false;

bool initializeAht20() {
  Wire.beginTransmission(AHT20_I2C_ADDRESS);
  Wire.write(AHT20_CMD_INIT);
  Wire.write(0x08);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  delay(10);
  return true;
}

bool readAht20(float &temperatureC, float &humidityPct) {
  if (!aht20Initialized) {
    aht20Initialized = initializeAht20();
  }
  if (!aht20Initialized) {
    return false;
  }

  Wire.beginTransmission(AHT20_I2C_ADDRESS);
  Wire.write(AHT20_CMD_MEASURE);
  Wire.write(0x33);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    aht20Initialized = false;
    return false;
  }

  delay(85);
  uint8_t received = Wire.requestFrom(AHT20_I2C_ADDRESS, (uint8_t)7);
  if (received != 7) {
    aht20Initialized = false;
    return false;
  }

  uint8_t buffer[7];
  for (uint8_t i = 0; i < 7; ++i) {
    buffer[i] = Wire.read();
  }
  if ((buffer[0] & 0x80) != 0) {
    return false;
  }

  uint32_t rawHumidity = ((uint32_t)buffer[1] << 12) | ((uint32_t)buffer[2] << 4) | (buffer[3] >> 4);
  uint32_t rawTemperature = ((uint32_t)(buffer[3] & 0x0F) << 16) | ((uint32_t)buffer[4] << 8) | buffer[5];
  humidityPct = (rawHumidity * 100.0f) / 1048576.0f;
  temperatureC = ((rawTemperature * 200.0f) / 1048576.0f) - 50.0f;
  return true;
}
