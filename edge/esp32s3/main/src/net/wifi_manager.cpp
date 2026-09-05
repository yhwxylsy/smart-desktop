#include "wifi_manager.h"
#include <WiFi.h>
#include "../../config.h"
#include "../config/config_store.h"

static unsigned long lastWifiMissingLogMs = 0;

bool scanTargetWifi(int32_t *channelOut, uint8_t bssidOut[6], int32_t *rssiOut, bool logResult) {
  int count = WiFi.scanNetworks(false, true);
  int bestIndex = -1;
  int32_t bestRssi = -127;
  int32_t bestChannel = 0;
  bool found = false;
  for (int i = 0; i < count; i++) {
    if (WiFi.SSID(i) == configStore::wifiSsid()) {
      found = true;
      if (WiFi.RSSI(i) > bestRssi) {
        bestIndex = i;
        bestRssi = WiFi.RSSI(i);
        bestChannel = WiFi.channel(i);
      }
    }
  }

  if (found && bestIndex >= 0) {
    uint8_t *bssid = WiFi.BSSID(bestIndex);
    if (bssid != nullptr && bssidOut != nullptr) {
      memcpy(bssidOut, bssid, 6);
    }
    if (channelOut != nullptr) {
      *channelOut = bestChannel;
    }
    if (rssiOut != nullptr) {
      *rssiOut = bestRssi;
    }
    if (logResult) {
      Serial.printf(
          "[WIFI] scan target rssi=%d channel=%d bssid=%s enc=%d\n",
          bestRssi,
          bestChannel,
          WiFi.BSSIDstr(bestIndex).c_str(),
          (int)WiFi.encryptionType(bestIndex));
    }
  } else if (logResult) {
    Serial.printf("[WIFI] scan target not found count=%d\n", count);
  }
  WiFi.scanDelete();
  return found && bestIndex >= 0;
}

void connectWifi() {
  if (configStore::wifiSsid().isEmpty()) {
    if (millis() - lastWifiMissingLogMs > 5000) {
      lastWifiMissingLogMs = millis();
      Serial.println("[WIFI] not configured; use CFG:WIFI:<ssid>,<password>");
    }
    return;
  }
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.disconnect(false, false);
  delay(200);
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  uint8_t targetBssid[6] = {0};
  int32_t targetChannel = 0;
  int32_t targetRssi = 0;
  bool hasTarget = scanTargetWifi(&targetChannel, targetBssid, &targetRssi);
  if (hasTarget && targetChannel > 0) {
    WiFi.begin(configStore::wifiSsid().c_str(), configStore::wifiPassword().c_str(), targetChannel, targetBssid);
  } else {
    WiFi.begin(configStore::wifiSsid().c_str(), configStore::wifiPassword().c_str());
  }
  Serial.print("[WIFI] connecting");
  unsigned long deadline = millis() + WIFI_CONNECT_TIMEOUT_MS;
  while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WIFI] ip=");
    Serial.println(WiFi.localIP());
    Serial.printf("[WIFI] rssi=%d bssid=%s\n", WiFi.RSSI(), WiFi.BSSIDstr().c_str());
  } else {
    Serial.printf("[WIFI] connect timeout status=%d rssi=%d\n", (int)WiFi.status(), WiFi.RSSI());
  }
}

void scanWifi() {
  int count = WiFi.scanNetworks();
  Serial.printf("[WIFI] scan count=%d\n", count);
  for (int i = 0; i < count; i++) {
    Serial.printf("  %s rssi=%d channel=%d\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
  }
}

void probeTcp(String target) {
  target.trim();
  int colon = target.lastIndexOf(':');
  if (colon <= 0 || colon >= (int)target.length() - 1) {
    Serial.println("[NET] use CFG:NET:TCP:<host>:<port>");
    return;
  }

  String host = target.substring(0, colon);
  uint16_t port = (uint16_t)target.substring(colon + 1).toInt();
  String localIp = WiFi.localIP().toString();
  String gatewayIp = WiFi.gatewayIP().toString();
  String dnsIp = WiFi.dnsIP().toString();
  Serial.printf("[NET] probe host=%s port=%u wifi=%d rssi=%d local=%s gateway=%s dns=%s\n",
                host.c_str(),
                port,
                (int)WiFi.status(),
                WiFi.RSSI(),
                localIp.c_str(),
                gatewayIp.c_str(),
                dnsIp.c_str());

  IPAddress ip;
  if (WiFi.hostByName(host.c_str(), ip)) {
    String resolved = ip.toString();
    Serial.printf("[NET] resolved %s -> %s\n", host.c_str(), resolved.c_str());
  } else {
    Serial.printf("[NET] resolve failed %s\n", host.c_str());
  }

  WiFiClient probeClient;
  probeClient.setTimeout(5000);
  bool ok = probeClient.connect(host.c_str(), port);
  Serial.printf("[NET] tcp %s\n", ok ? "ok" : "failed");
  probeClient.stop();
}
