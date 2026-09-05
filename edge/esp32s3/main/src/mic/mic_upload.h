#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "../../config.h"
#include "../core/types.h"

// 麦克风录音上传（原 main.ino L937-1358 原样搬运）。
bool writeAllToClient(
    WiFiClient &client,
    const uint8_t *data,
    size_t length,
    uint32_t timeoutMs = 15000,
    size_t *writtenOut = nullptr);
String formatWriteFailure(const char *part, size_t written, size_t expected);
int readHttpResponse(WiFiClient &client, String &response);
AsrUploadResult parseAsrUploadResponse(int code, const String &response);
void appendMultipartField(String &body, const String &boundary, const char *name, const String &value);
ChunkUploadResponse parseChunkUploadAck(int code, const String &response, bool finalPart);
ChunkUploadResponse postMicWavChunk(
    const String &uploadId,
    const uint8_t *data,
    size_t chunkSize,
    size_t offset,
    size_t totalAudioSize,
    bool finalPart,
    const String &source,
    bool inject);
AsrUploadResult uploadMicWavChunked(const uint8_t *wavBuffer, size_t wavSize, const String &source, bool inject);
AsrUploadResult uploadMicWav(const uint8_t *wavBuffer, size_t wavSize, const String &source, bool inject);
