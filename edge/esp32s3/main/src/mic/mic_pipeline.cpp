#include "mic_pipeline.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "../../config.h"
#include "../config/config_store.h"
#include "../core/hex_util.h"
#include "../net/heartbeat.h"
#include "../net/websocket_link.h"
#include "../net/http_client.h"
#include "../bridge/backend_bridge.h"
#include "../bridge/stm32_link.h"
#include "mic_capture.h"
#include "mic_upload.h"

bool rejectMicPath(const char *trigger) {
  Serial.print("[MIC] disabled; ignored ");
  Serial.println(trigger);
  voiceState = "mic_disabled";
  return false;
}

String normalizeSelfTestText(String value) {
  value.toLowerCase();
  value.replace(" ", "");
  value.replace("\t", "");
  value.replace("\r", "");
  value.replace("\n", "");
  value.replace(",", "");
  value.replace(".", "");
  value.replace("!", "");
  value.replace("?", "");
  value.replace(":", "");
  value.replace(";", "");
  return value;
}

bool selfTestTextMatches(const String &recognized, const String &expected) {
  String actual = normalizeSelfTestText(recognized);
  String target = normalizeSelfTestText(expected);
  return target.length() > 0 && actual.indexOf(target) >= 0;
}

void sendMicSelfTestTelemetry(const String &expected, const AsrUploadResult &result, bool passed) {
  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  doc["edge_id"] = EDGE_ID;
  doc["voice_state"] = passed ? "mic_selftest_ok" : "mic_selftest_fail";
  JsonObject sensors = doc["sensors"].to<JsonObject>();
  sensors["mic_selftest_expected"] = expected;
  sensors["mic_selftest_text"] = result.text;
  sensors["mic_selftest_ok"] = passed;
  sensors["mic_selftest_provider"] = result.provider;
  sensors["mic_selftest_error"] = result.error;
  String body;
  serializeJson(doc, body);
  postJson("/api/hardware/telemetry", body);
}

bool captureAndUploadMic(bool inject, const String &source) {
  if (!MIC_PATH_ENABLED) {
    return rejectMicPath("capture request");
  }
  if (micBusy) {
    Serial.println("[MIC] busy");
    sendToStm32("NET:UI:ERROR", 0);
    return false;
  }
  if (!micReady) {
    Serial.println("[MIC] not ready");
    sendToStm32("NET:UI:ERROR", 0);
    return false;
  }
  if (WiFi.status() != WL_CONNECTED || configStore::host().isEmpty()) {
    Serial.println("[MIC] WiFi/server not ready");
    sendToStm32("NET:UI:ERROR", 0);
    return false;
  }

  micBusy = true;
  setVoiceState("recording", true);
  sendToStm32("NET:UI:LISTEN", 0);
  Serial.printf("[MIC] recording %u seconds...\n", MIC_RECORD_SECONDS);
  MicCapture capture = captureMicWav();
  if (capture.wavBuffer == nullptr || capture.wavSize == 0) {
    micBusy = false;
    setVoiceState("asr_error", true);
    sendToStm32("NET:UI:ERROR", 0);
    return false;
  }

  setVoiceState("uploading", true);
  sendToStm32("NET:UI:THINK", 0);
  Serial.printf("[MIC] captured %u bytes\n", (unsigned int)capture.wavSize);
  bool resumeWsAfterUpload = pauseWebSocketForMicUpload();
  AsrUploadResult result = uploadMicWav(capture.wavBuffer, capture.wavSize, source, inject);
  free(capture.wavBuffer);
  micBusy = false;

  if (result.requestOk && result.asrOk) {
    Serial.printf("[MIC] ASR OK provider=%s text=%s\n", result.provider.c_str(), result.text.c_str());
    setVoiceState("text_bridge", true);
    if (!inject) {
      sendToStm32("NET:UI:IDLE", 0);
    } else {
      String response;
      if (getJson("/api/hardware/commands/" + String(DEVICE_ID), &response)) {
        forwardCommandsFromJson(response);
      }
    }
    if (resumeWsAfterUpload) {
      startWebSocket();
    }
    return true;
  }

  Serial.printf(
      "[MIC] ASR FAIL transport=%s provider=%s error=%s\n",
      result.requestOk ? "ok" : "bad",
      result.provider.c_str(),
      result.error.c_str());
  setVoiceState("asr_error", true);
  sendToStm32("NET:UI:ERROR", 0);
  if (resumeWsAfterUpload) {
    startWebSocket();
  }
  return false;
}

bool captureAndUploadMicAfterCue(String cueText, bool inject, const String &source) {
  if (!MIC_PATH_ENABLED) {
    return rejectMicPath("cue capture request");
  }
  cueText.trim();
  if (cueText.isEmpty()) {
    cueText = "三。二。一。";
  }
  if (micBusy) {
    Serial.println("[MIC] busy");
    return false;
  }
  if (!micReady) {
    Serial.println("[MIC] not ready");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED || configStore::host().isEmpty()) {
    Serial.println("[MIC] WiFi/server not ready");
    return false;
  }

  setVoiceState("recording_cue", true);
  Serial.printf("[MIC] cue phrase=%s delay=%u ms\n", cueText.c_str(), MIC_COUNTDOWN_CUE_DELAY_MS);
  sendToStm32("NET:TTSHEX:" + utf8Hex(cueText), 0);
  delay(MIC_COUNTDOWN_CUE_DELAY_MS);
  return captureAndUploadMic(inject, source);
}

bool runMicSelfTest(String phrase) {
  if (!MIC_PATH_ENABLED) {
    return rejectMicPath("self-test request");
  }
  phrase.trim();
  if (phrase.isEmpty()) {
    phrase = "AI TEST OK";
  }
  if (micBusy) {
    Serial.println("[MIC] busy");
    return false;
  }
  if (!micReady) {
    Serial.println("[MIC] not ready");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED || configStore::host().isEmpty()) {
    Serial.println("[MIC] WiFi/server not ready");
    return false;
  }

  micBusy = true;
  setVoiceState("mic_selftest_recording", true);
  Serial.printf("[MIC] selftest phrase=%s\n", phrase.c_str());
  sendToStm32("NET:TTSHEX:" + utf8Hex(phrase), 0);
  MicCapture capture = captureMicWav();
  if (capture.wavBuffer == nullptr || capture.wavSize == 0) {
    micBusy = false;
    setVoiceState("mic_selftest_fail", true);
    AsrUploadResult failed;
    failed.error = "recording failed";
    sendMicSelfTestTelemetry(phrase, failed, false);
    return false;
  }

  setVoiceState("mic_selftest_uploading", true);
  Serial.printf("[MIC] selftest captured %u bytes\n", (unsigned int)capture.wavSize);
  AsrUploadResult result = uploadMicWav(capture.wavBuffer, capture.wavSize, "esp32_mic_selftest", false);
  free(capture.wavBuffer);
  micBusy = false;

  bool passed = result.requestOk && result.asrOk && selfTestTextMatches(result.text, phrase);
  Serial.printf(
      "[MIC] SELFTEST %s expected=%s recognized=%s provider=%s error=%s\n",
      passed ? "PASS" : "FAIL",
      phrase.c_str(),
      result.text.c_str(),
      result.provider.c_str(),
      result.error.c_str());
  sendMicSelfTestTelemetry(phrase, result, passed);
  setVoiceState(passed ? "text_bridge" : "mic_selftest_fail", true);
  return passed;
}
