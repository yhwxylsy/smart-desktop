#pragma once
#include <Arduino.h>
#include <MFRC522.h>
#include "../../config.h"

// RC522 读卡器与扫卡上报（原 main.ino L1637-1952 原样搬运）。
extern MFRC522 rfid;

bool rfidVersionHealthy(byte version);
byte initializeRfidReader();
void printRfidStatus(bool probeCard);
String uidToString(MFRC522::Uid *uid);
bool rfidCardReady();
void pollRfid();
