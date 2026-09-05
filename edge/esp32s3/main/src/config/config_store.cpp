#include "config_store.h"
#include "../../config.h"
#include <Preferences.h>

namespace configStore {

static Preferences prefs;
static String gWifiSsid;
static String gWifiPassword;
static String gServerHost;
static String gDeviceToken = String(SMARTDESK_DEVICE_TOKEN);
static uint16_t gServerPort = 8082;
static bool gServerSecure = false;

void load() {
  prefs.begin("smartdesk", true);
  gWifiSsid = prefs.getString("wifi_ssid", "");
  gWifiPassword = prefs.getString("wifi_pass", "");
  gServerHost = prefs.getString("server_host", "");
  gServerPort = prefs.getUShort("server_port", 8082);
  gServerSecure = prefs.getBool("server_secure", false);
  gDeviceToken = prefs.getString("device_token", SMARTDESK_DEVICE_TOKEN);
  prefs.end();
}

void save() {
  prefs.begin("smartdesk", false);
  prefs.putString("wifi_ssid", gWifiSsid);
  prefs.putString("wifi_pass", gWifiPassword);
  prefs.putString("server_host", gServerHost);
  prefs.putUShort("server_port", gServerPort);
  prefs.putBool("server_secure", gServerSecure);
  prefs.putString("device_token", gDeviceToken);
  prefs.end();
}

void reset() {
  prefs.begin("smartdesk", false);
  prefs.clear();
  prefs.end();
  gWifiSsid = "";
  gWifiPassword = "";
  gServerHost = "";
  gDeviceToken = String(SMARTDESK_DEVICE_TOKEN);
  gServerSecure = false;
}

bool parseServerUrl(String value) {
  value.trim();
  bool secure = false;
  if (value.startsWith("https://")) {
    secure = true;
    value.remove(0, strlen("https://"));
  } else if (value.startsWith("wss://")) {
    secure = true;
    value.remove(0, strlen("wss://"));
  } else if (value.startsWith("http://")) {
    value.remove(0, strlen("http://"));
  } else if (value.startsWith("ws://")) {
    value.remove(0, strlen("ws://"));
  }
  int slash = value.indexOf('/');
  if (slash >= 0) {
    value = value.substring(0, slash);
  }
  int colon = value.lastIndexOf(':');
  if (colon >= 0) {
    gServerHost = value.substring(0, colon);
    gServerPort = (uint16_t)value.substring(colon + 1).toInt();
  } else {
    gServerHost = value;
    gServerPort = secure ? 443 : 8082;
  }
  gServerHost.trim();
  gServerSecure = secure;
  return gServerHost.length() > 0 && gServerPort > 0;
}

const String &host() {
  return gServerHost;
}

uint16_t port() {
  return gServerPort;
}

bool secure() {
  return gServerSecure;
}

const String &deviceToken() {
  return gDeviceToken;
}

const String &wifiSsid() {
  return gWifiSsid;
}

const String &wifiPassword() {
  return gWifiPassword;
}

String httpBase() {
  return String(gServerSecure ? "https://" : "http://") + gServerHost + ":" + String(gServerPort);
}

String wsBase() {
  return String(gServerSecure ? "wss://" : "ws://") + gServerHost + ":" + String(gServerPort);
}

String wsPath() {
  return "/api/realtime/ws?device_id=" + String(DEVICE_ID) + "&edge_id=" + String(EDGE_ID);
}

void setWifi(const String &ssid, const String &password) {
  gWifiSsid = ssid;
  gWifiPassword = password;
}

void setToken(const String &token) {
  gDeviceToken = token;
}

}  // namespace configStore
