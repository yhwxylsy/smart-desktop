#pragma once
#include <Arduino.h>

// 麦克风 ASR / 分块上传相关的轻量数据结构（原 main.ino L124-144 原样搬运）
struct AsrUploadResult {
  bool requestOk = false;
  bool asrOk = false;
  String provider;
  String text;
  String error;
};

struct MicCapture {
  uint8_t *wavBuffer = nullptr;
  size_t wavSize = 0;
};

struct ChunkUploadResponse {
  bool requestOk = false;
  bool chunkOk = false;
  bool complete = false;
  size_t received = 0;
  String error;
  AsrUploadResult asr;
};
