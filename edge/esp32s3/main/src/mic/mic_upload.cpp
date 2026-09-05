#include "mic_upload.h"
#include <ArduinoJson.h>
#include "../../config.h"
#include "../config/config_store.h"

bool writeAllToClient(
    WiFiClient &client,
    const uint8_t *data,
    size_t length,
    uint32_t timeoutMs,
    size_t *writtenOut) {
  size_t offset = 0;
  uint32_t deadline = millis() + timeoutMs;
  while (offset < length) {
    if (!client.connected()) {
      if (writtenOut != nullptr) {
        *writtenOut = offset;
      }
      return false;
    }

    size_t chunk = min(MIC_UPLOAD_CHUNK_SIZE, length - offset);
    int writable = client.availableForWrite();
    if (writable > 0) {
      chunk = min(chunk, (size_t)writable);
    }

    size_t written = client.write(data + offset, chunk);
    if (written > 0) {
      offset += written;
      deadline = millis() + timeoutMs;
      delay(MIC_UPLOAD_INTER_CHUNK_DELAY_MS);
      continue;
    }

    if ((int32_t)(millis() - deadline) > 0) {
      if (writtenOut != nullptr) {
        *writtenOut = offset;
      }
      return false;
    }
    delay(10);
  }
  if (writtenOut != nullptr) {
    *writtenOut = offset;
  }
  return true;
}

String formatWriteFailure(const char *part, size_t written, size_t expected) {
  String error = String(part) + " write failed at ";
  error += String((uint32_t)written);
  error += "/";
  error += String((uint32_t)expected);
  error += " rssi=";
  error += String(WiFi.RSSI());
  error += " status=";
  error += String((int)WiFi.status());
  return error;
}

int readHttpResponse(WiFiClient &client, String &response) {
  String statusLine = client.readStringUntil('\n');
  int code = 0;
  int firstSpace = statusLine.indexOf(' ');
  if (firstSpace >= 0) {
    code = statusLine.substring(firstSpace + 1, firstSpace + 4).toInt();
  }
  int contentLength = -1;
  while (client.connected()) {
    String header = client.readStringUntil('\n');
    header.trim();
    if (header.length() == 0) {
      break;
    }
    String lowerHeader = header;
    lowerHeader.toLowerCase();
    if (lowerHeader.startsWith("content-length:")) {
      contentLength = lowerHeader.substring(strlen("content-length:")).toInt();
    }
  }
  response = "";
  if (contentLength >= 0) {
    response.reserve(contentLength + 1);
    uint32_t bodyDeadline = millis() + 30000;
    while ((int)response.length() < contentLength && (client.connected() || client.available())) {
      while (client.available() && (int)response.length() < contentLength) {
        response += (char)client.read();
        bodyDeadline = millis() + 30000;
      }
      if ((int32_t)(millis() - bodyDeadline) > 0) {
        break;
      }
      delay(5);
    }
  } else {
    response = client.readString();
  }
  return code;
}

AsrUploadResult parseAsrUploadResponse(int code, const String &response) {
  AsrUploadResult result;
  result.requestOk = code >= 200 && code < 300;
  if (!result.requestOk) {
    result.error = "HTTP " + String(code);
    return result;
  }

  JsonDocument filter;
  filter["ok"] = true;
  filter["provider"] = true;
  filter["text"] = true;
  filter["error"] = true;
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(filter));
  if (error) {
    result.asrOk = true;
    result.error = "";
    return result;
  }

  result.asrOk = doc["ok"] | false;
  result.provider = String((const char *)(doc["provider"] | ""));
  result.text = String((const char *)(doc["text"] | ""));
  result.error = String((const char *)(doc["error"] | ""));
  return result;
}

void appendMultipartField(String &body, const String &boundary, const char *name, const String &value) {
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"";
  body += name;
  body += "\"\r\n\r\n";
  body += value;
  body += "\r\n";
}

ChunkUploadResponse parseChunkUploadAck(int code, const String &response, bool finalPart) {
  ChunkUploadResponse result;
  result.requestOk = code >= 200 && code < 300;
  if (!result.requestOk) {
    result.error = "HTTP " + String(code);
    return result;
  }

  if (finalPart) {
    JsonDocument filter;
    filter["ok"] = true;
    filter["complete"] = true;
    filter["received"] = true;
    filter["error"] = true;
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(filter));
    if (!error && !doc["complete"].isNull()) {
      result.chunkOk = doc["ok"] | false;
      result.complete = doc["complete"] | false;
      result.received = (size_t)((uint32_t)(doc["received"] | 0));
      result.error = String((const char *)(doc["error"] | ""));
      return result;
    }
    result.asr = parseAsrUploadResponse(code, response);
    result.chunkOk = result.asr.requestOk;
    result.complete = result.asr.requestOk;
    result.error = result.asr.error;
    return result;
  }

  JsonDocument filter;
  filter["ok"] = true;
  filter["complete"] = true;
  filter["received"] = true;
  filter["error"] = true;
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(filter));
  if (error) {
    result.error = "chunk ack parse failed: " + String(error.c_str());
    return result;
  }

  result.chunkOk = doc["ok"] | false;
  result.complete = doc["complete"] | false;
  result.received = (size_t)((uint32_t)(doc["received"] | 0));
  result.error = String((const char *)(doc["error"] | ""));
  return result;
}

ChunkUploadResponse postMicWavChunk(
    const String &uploadId,
    const uint8_t *data,
    size_t chunkSize,
    size_t offset,
    size_t totalAudioSize,
    bool finalPart,
    const String &source,
    bool inject) {
  ChunkUploadResponse result;
  result.received = offset;
  String boundary = "----SmartDeskChunk" + String((uint32_t)millis(), HEX) + String((uint32_t)offset, HEX);
  String prefix;
  prefix.reserve(768);
  appendMultipartField(prefix, boundary, "upload_id", uploadId);
  appendMultipartField(prefix, boundary, "offset", String((uint32_t)offset));
  appendMultipartField(prefix, boundary, "total_size", String((uint32_t)totalAudioSize));
  appendMultipartField(prefix, boundary, "final", finalPart ? "true" : "false");
  appendMultipartField(prefix, boundary, "device_id", DEVICE_ID);
  appendMultipartField(prefix, boundary, "inject", inject ? "true" : "false");
  appendMultipartField(prefix, boundary, "source", source);
  appendMultipartField(prefix, boundary, "audio_format", "wav");
  appendMultipartField(prefix, boundary, "sample_rate", String(MIC_SAMPLE_RATE));
  prefix += "--" + boundary + "\r\n";
  prefix += "Content-Disposition: form-data; name=\"chunk\"; filename=\"esp32-mic.part\"\r\n";
  prefix += "Content-Type: application/octet-stream\r\n\r\n";
  String suffix = "\r\n--" + boundary + "--\r\n";
  size_t requestSize = prefix.length() + chunkSize + suffix.length();

  WiFiClient client;
  client.setTimeout(120000);
  if (!client.connect(configStore::host().c_str(), configStore::port())) {
    result.error = "chunk connect failed rssi=" + String(WiFi.RSSI()) + " status=" + String((int)WiFi.status());
    return result;
  }

  Serial.printf(
      "[MIC] chunk upload offset=%u size=%u final=%u rssi=%d status=%d\n",
      (unsigned int)offset,
      (unsigned int)chunkSize,
      finalPart ? 1 : 0,
      WiFi.RSSI(),
      (int)WiFi.status());
  client.printf("POST /api/asr/transcribe/chunk HTTP/1.1\r\n");
  client.printf("Host: %s:%u\r\n", configStore::host().c_str(), configStore::port());
  client.printf("Connection: close\r\n");
  client.printf("Content-Type: multipart/form-data; boundary=%s\r\n", boundary.c_str());
  client.printf("Content-Length: %u\r\n\r\n", (unsigned int)requestSize);
  size_t written = 0;
  if (!writeAllToClient(client, (const uint8_t *)prefix.c_str(), prefix.length(), 15000, &written)) {
    result.error = formatWriteFailure("chunk prefix", written, prefix.length());
    client.stop();
    return result;
  }
  if (!writeAllToClient(client, data, chunkSize, 15000, &written)) {
    result.error = formatWriteFailure("chunk audio", written, chunkSize);
    client.stop();
    return result;
  }
  if (!writeAllToClient(client, (const uint8_t *)suffix.c_str(), suffix.length(), 15000, &written)) {
    result.error = formatWriteFailure("chunk suffix", written, suffix.length());
    client.stop();
    return result;
  }
  client.flush();

  String response;
  int code = readHttpResponse(client, response);
  Serial.printf("[MIC] POST /api/asr/transcribe/chunk -> %d\n", code);
  client.stop();
  return parseChunkUploadAck(code, response, finalPart);
}

AsrUploadResult uploadMicWavChunked(const uint8_t *wavBuffer, size_t wavSize, const String &source, bool inject) {
  AsrUploadResult result;
  if (WiFi.status() != WL_CONNECTED || configStore::host().isEmpty()) {
    result.error = "WiFi/server not ready";
    return result;
  }

  String uploadId = String(EDGE_ID) + "-" + String((uint32_t)millis(), HEX) + "-" + String((uint32_t)wavSize, HEX);
  Serial.printf(
      "[MIC] chunked upload start id=%s total=%u part=%u\n",
      uploadId.c_str(),
      (unsigned int)wavSize,
      (unsigned int)MIC_UPLOAD_CHUNKED_PART_SIZE);

  size_t offset = 0;
  while (offset < wavSize) {
    size_t partSize = min(MIC_UPLOAD_CHUNKED_PART_SIZE, wavSize - offset);
    bool finalPart = offset + partSize >= wavSize;
    bool advanced = false;
    for (uint8_t attempt = 1; attempt <= MIC_UPLOAD_CHUNKED_PART_ATTEMPTS; attempt++) {
      ChunkUploadResponse chunk = postMicWavChunk(
          uploadId,
          wavBuffer + offset,
          partSize,
          offset,
          wavSize,
          finalPart,
          source,
          inject);
      if (finalPart && chunk.requestOk && chunk.complete) {
        Serial.println("[MIC] chunked upload complete");
        return chunk.asr;
      }
      if (chunk.requestOk && chunk.chunkOk && chunk.received > offset && chunk.received <= wavSize) {
        offset = chunk.received;
        advanced = true;
        delay(40);
        break;
      }

      result.error = chunk.error;
      if (result.error.isEmpty()) {
        result.error = "chunk upload did not advance";
      }
      Serial.printf(
          "[MIC] chunk offset %u attempt %u failed: %s received=%u\n",
          (unsigned int)offset,
          attempt,
          result.error.c_str(),
          (unsigned int)chunk.received);
      delay(400 + attempt * 200);
    }
    if (!advanced) {
      return result;
    }
  }

  result.error = "chunked upload ended without final response";
  return result;
}

AsrUploadResult uploadMicWav(const uint8_t *wavBuffer, size_t wavSize, const String &source, bool inject) {
  AsrUploadResult result;
  if (WiFi.status() != WL_CONNECTED || configStore::host().isEmpty()) {
    result.error = "WiFi/server not ready";
    return result;
  }

  String boundary = "----SmartDeskBoundary" + String((uint32_t)millis(), HEX);
  String prefix;
  prefix.reserve(512);
  prefix += "--" + boundary + "\r\n";
  prefix += "Content-Disposition: form-data; name=\"device_id\"\r\n\r\n";
  prefix += DEVICE_ID;
  prefix += "\r\n--" + boundary + "\r\n";
  prefix += "Content-Disposition: form-data; name=\"inject\"\r\n\r\n";
  prefix += inject ? "true" : "false";
  prefix += "\r\n";
  prefix += "--" + boundary + "\r\n";
  prefix += "Content-Disposition: form-data; name=\"source\"\r\n\r\n";
  prefix += source;
  prefix += "\r\n";
  prefix += "--" + boundary + "\r\n";
  prefix += "Content-Disposition: form-data; name=\"audio_format\"\r\n\r\nwav\r\n";
  prefix += "--" + boundary + "\r\n";
  prefix += "Content-Disposition: form-data; name=\"sample_rate\"\r\n\r\n";
  prefix += String(MIC_SAMPLE_RATE);
  prefix += "\r\n--" + boundary + "\r\n";
  prefix += "Content-Disposition: form-data; name=\"audio\"; filename=\"esp32-mic.wav\"\r\n";
  prefix += "Content-Type: audio/wav\r\n\r\n";
  String suffix = "\r\n--" + boundary + "--\r\n";

  size_t totalSize = prefix.length() + wavSize + suffix.length();
  Serial.printf("[MIC] upload body %u bytes\n", (unsigned int)totalSize);

  for (uint8_t attempt = 1; attempt <= 3; attempt++) {
    result = AsrUploadResult();
    WiFiClient client;
    client.setTimeout(120000);
    if (!client.connect(configStore::host().c_str(), configStore::port())) {
      result.error = "connect failed rssi=" + String(WiFi.RSSI()) + " status=" + String((int)WiFi.status());
      Serial.printf("[MIC] upload attempt %u failed: %s\n", attempt, result.error.c_str());
      delay(800);
      continue;
    }

    Serial.printf(
        "[MIC] upload attempt %u rssi=%d status=%d target=%s:%u\n",
        attempt,
        WiFi.RSSI(),
        (int)WiFi.status(),
        configStore::host().c_str(),
        configStore::port());
    client.printf("POST /api/asr/transcribe HTTP/1.1\r\n");
    client.printf("Host: %s:%u\r\n", configStore::host().c_str(), configStore::port());
    client.printf("Connection: close\r\n");
    client.printf("Content-Type: multipart/form-data; boundary=%s\r\n", boundary.c_str());
    client.printf("Content-Length: %u\r\n\r\n", (unsigned int)totalSize);
    size_t written = 0;
    if (!writeAllToClient(client, (const uint8_t *)prefix.c_str(), prefix.length(), 15000, &written)) {
      result.error = formatWriteFailure("prefix", written, prefix.length());
      Serial.printf("[MIC] upload attempt %u failed: %s\n", attempt, result.error.c_str());
      client.stop();
      delay(800);
      continue;
    }
    if (!writeAllToClient(client, wavBuffer, wavSize, 15000, &written)) {
      result.error = formatWriteFailure("audio", written, wavSize);
      Serial.printf("[MIC] upload attempt %u failed: %s\n", attempt, result.error.c_str());
      client.stop();
      delay(800);
      continue;
    }
    if (!writeAllToClient(client, (const uint8_t *)suffix.c_str(), suffix.length(), 15000, &written)) {
      result.error = formatWriteFailure("suffix", written, suffix.length());
      Serial.printf("[MIC] upload attempt %u failed: %s\n", attempt, result.error.c_str());
      client.stop();
      delay(800);
      continue;
    }
    client.flush();

    String response;
    int code = readHttpResponse(client, response);
    Serial.printf("[MIC] POST /api/asr/transcribe -> %d\n", code);
    client.stop();
    result = parseAsrUploadResponse(code, response);

    if (!result.requestOk) {
      if (code <= 0 || code >= 500) {
        Serial.printf("[MIC] upload attempt %u failed: %s\n", attempt, result.error.c_str());
        delay(800);
        continue;
      }
      return result;
    }

    return result;
  }

  Serial.printf("[MIC] direct upload failed, trying chunked fallback: %s\n", result.error.c_str());
  AsrUploadResult chunked = uploadMicWavChunked(wavBuffer, wavSize, source, inject);
  if (chunked.error.isEmpty() && !chunked.requestOk) {
    chunked.error = result.error;
  }
  return chunked;
}
