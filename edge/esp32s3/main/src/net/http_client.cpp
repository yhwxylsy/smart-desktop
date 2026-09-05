#include "http_client.h"
#include "../../config.h"
#include "../config/config_store.h"

bool postJson(const String &path, const String &body, String *responseOut) {
  if (WiFi.status() != WL_CONNECTED || configStore::host().isEmpty()) {
    return false;
  }
  HTTPClient http;
  WiFiClientSecure secureClient;
  bool began = false;
  if (configStore::secure()) {
    secureClient.setInsecure();
    began = http.begin(secureClient, configStore::httpBase() + path);
  } else {
    began = http.begin(configStore::httpBase() + path);
  }
  if (!began) {
    Serial.printf("[HTTP] POST %s begin failed\n", path.c_str());
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  if (configStore::deviceToken().length() > 0) {
    http.addHeader("X-Device-Token", configStore::deviceToken());
  }
  int code = http.POST(body);
  bool ok = code >= 200 && code < 300;
  if (responseOut != nullptr) {
    *responseOut = http.getString();
  }
  Serial.printf("[HTTP] POST %s -> %d\n", path.c_str(), code);
  http.end();
  return ok;
}

bool getJson(const String &path, String *responseOut) {
  if (WiFi.status() != WL_CONNECTED || configStore::host().isEmpty()) {
    return false;
  }
  HTTPClient http;
  WiFiClientSecure secureClient;
  bool began = false;
  if (configStore::secure()) {
    secureClient.setInsecure();
    began = http.begin(secureClient, configStore::httpBase() + path);
  } else {
    began = http.begin(configStore::httpBase() + path);
  }
  if (!began) {
    Serial.printf("[HTTP] GET %s begin failed\n", path.c_str());
    return false;
  }
  int code = http.GET();
  bool ok = code >= 200 && code < 300;
  if (responseOut != nullptr) {
    *responseOut = http.getString();
  }
  Serial.printf("[HTTP] GET %s -> %d\n", path.c_str(), code);
  http.end();
  return ok;
}
