#include "rfid_reader.h"
#include <SPI.h>
#include <ArduinoJson.h>
#include "../../config.h"
#include "../config/config_store.h"
#include "../net/http_client.h"
#include "../bridge/stm32_link.h"

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

static unsigned long lastRfidMs = 0;
static unsigned long lastRfidSeenMs = 0;
static unsigned long lastRfidHealthMs = 0;
static String lastRfidUid;

bool rfidVersionHealthy(byte version) {
  return version != 0x00 && version != 0xFF;
}

byte initializeRfidReader() {
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  rfid.PCD_Init();
  delay(4);
  rfid.PCD_AntennaOn();
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);
  return rfid.PCD_ReadRegister(MFRC522::VersionReg);
}

void printRfidStatus(bool probeCard) {
  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  byte gain = rfid.PCD_GetAntennaGain();
  byte txControl = rfid.PCD_ReadRegister(MFRC522::TxControlReg);
  bool cardReady = probeCard && rfidCardReady();
  Serial.printf(
      "[RFID] status version=0x%02X healthy=%s antenna_gain=0x%02X tx_control=0x%02X card_ready=%s last_uid=%s\n",
      version,
      rfidVersionHealthy(version) ? "true" : "false",
      gain,
      txControl,
      cardReady ? "true" : "false",
      lastRfidUid.length() > 0 ? lastRfidUid.c_str() : "-");
}

String uidToString(MFRC522::Uid *uid) {
  String value;
  for (byte i = 0; i < uid->size; i++) {
    if (uid->uidByte[i] < 0x10) {
      value += "0";
    }
    value += String(uid->uidByte[i], HEX);
  }
  value.toUpperCase();
  return value;
}

bool rfidCardReady() {
  if (rfid.PICC_IsNewCardPresent()) {
    return true;
  }

  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);
  MFRC522::StatusCode status = rfid.PICC_WakeupA(bufferATQA, &bufferSize);
  return status == MFRC522::STATUS_OK || status == MFRC522::STATUS_COLLISION;
}

void pollRfid() {
  unsigned long now = millis();
  if (now - lastRfidHealthMs >= RFID_HEALTH_INTERVAL_MS) {
    lastRfidHealthMs = now;
    byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
    byte txControl = rfid.PCD_ReadRegister(MFRC522::TxControlReg);
    byte gain = rfid.PCD_GetAntennaGain();
    if (!rfidVersionHealthy(version) || (txControl & 0x03) != 0x03 || gain != MFRC522::RxGain_max) {
      Serial.printf(
          "[RFID] reader recovery version=0x%02X tx_control=0x%02X antenna_gain=0x%02X\n",
          version,
          txControl,
          gain);
      initializeRfidReader();
    }
  }
  if (now - lastRfidMs < RFID_POLL_INTERVAL_MS) {
    return;
  }
  lastRfidMs = now;
  if (!rfidCardReady() || !rfid.PICC_ReadCardSerial()) {
    return;
  }
  String uid = uidToString(&rfid.uid);
  if (uid == lastRfidUid && now - lastRfidSeenMs < RFID_REPEAT_SUPPRESS_MS) {
    Serial.print("[RFID] repeat skipped ");
    Serial.println(uid);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }
  lastRfidUid = uid;
  lastRfidSeenMs = now;

  Serial.print("[RFID] ");
  Serial.println(uid);
  sendToStm32("NET:RFID:SCAN:" + uid, 0);
  sendToStm32("NET:BEEP", 0);

  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  doc["uid"] = uid;
  doc["source"] = "rc522";
  String body;
  serializeJson(doc, body);
  String response;
  bool ok = postJson("/api/rfid/scan", body, &response);
  if (!ok) {
    Serial.println("[RFID] scan post failed");
    sendToStm32("NET:RFID:NETWORK_ERROR", 0);
  } else {
    Serial.println("[RFID] scan accepted by backend");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
