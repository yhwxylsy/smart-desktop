#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "../../config.h"

// HTTP 后端客户端（原 main.ino L484-540 原样搬运）。
// 端点路径串保留在各自调用方模块，本模块仅提供通用 post/get 封装。
bool postJson(const String &path, const String &body, String *responseOut = nullptr);
bool getJson(const String &path, String *responseOut = nullptr);
