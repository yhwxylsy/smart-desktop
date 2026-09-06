#pragma once

// ============================================================================
// 编译期配置、设备标识与引脚/时序常量
// 从 main.ino 原样搬运（原 L14-16、L18-62、L104-122）。
// 本文件必须最先被包含：SMARTDESK_* 宏需在任何模块头之前定义，
// local_config.h（本地覆盖，不提交）也在此统一引入。
// ============================================================================

#if __has_include("local_config.h")
#include "local_config.h"
#endif

#ifndef SMARTDESK_BOOTSTRAP_SERVER_URL
#define SMARTDESK_BOOTSTRAP_SERVER_URL ""
#endif

#ifndef SMARTDESK_DEVICE_TOKEN
#define SMARTDESK_DEVICE_TOKEN ""
#endif

#ifndef SMARTDESK_IOTDA_ENABLED
#define SMARTDESK_IOTDA_ENABLED 0
#endif

#ifndef SMARTDESK_IOTDA_HOST
#define SMARTDESK_IOTDA_HOST ""
#endif

#ifndef SMARTDESK_IOTDA_PORT
#define SMARTDESK_IOTDA_PORT 8883
#endif

#ifndef SMARTDESK_IOTDA_DEVICE_ID
#define SMARTDESK_IOTDA_DEVICE_ID ""
#endif

#ifndef SMARTDESK_IOTDA_SECRET
#define SMARTDESK_IOTDA_SECRET ""
#endif

#ifndef SMARTDESK_IOTDA_TIMESTAMP
#define SMARTDESK_IOTDA_TIMESTAMP "2026062100"
#endif

static const char *DEVICE_ID = "desktop-agent-001";
static const char *EDGE_ID = "esp32s3-sense-001";

static const int STM32_TX_PIN = 6;   // XIAO D5 -> STM32 PB11, 9600 baud
static const int STM32_RX_PIN = 44;  // XIAO D7 <- STM32 PB3, 9600 baud

static const int RFID_RST_PIN = 3;   // XIAO D2
static const int RFID_SS_PIN = 4;    // XIAO D3
static const int RFID_SCK_PIN = 7;   // XIAO D8
static const int RFID_MISO_PIN = 8;  // XIAO D9
static const int RFID_MOSI_PIN = 9;  // XIAO D10
static const int MIC_CLK_PIN = 42;   // XIAO ESP32S3 Sense PDM CLK
static const int MIC_DATA_PIN = 41;  // XIAO ESP32S3 Sense PDM DATA

static const unsigned long UART_OK_WINDOW_MS = 15000;
static const unsigned long UART_KEEPALIVE_INTERVAL_MS = 10000;
static const unsigned long UART_TX_QUIET_MS = 7000;
static const unsigned long COMMAND_POLL_INTERVAL_MS = 800;
static const unsigned long RFID_POLL_INTERVAL_MS = 200;
static const unsigned long RFID_REPEAT_SUPPRESS_MS = 15000;
static const unsigned long RFID_HEALTH_INTERVAL_MS = 5000;
static const unsigned long WIFI_CONNECT_TIMEOUT_MS = 30000;
static const bool MIC_PATH_ENABLED = false;
static const uint32_t MIC_RECORD_SECONDS = 6;
static const uint32_t MIC_SAMPLE_RATE = 16000;
static const uint32_t MIC_SAMPLE_BITS = 16;
static const uint32_t MIC_WAV_HEADER_SIZE = 44;
static const int32_t MIC_DIGITAL_GAIN = 16;
static const uint16_t MIC_COUNTDOWN_CUE_DELAY_MS = 1450;
static const size_t MIC_UPLOAD_CHUNK_SIZE = 512;
static const uint8_t MIC_UPLOAD_INTER_CHUNK_DELAY_MS = 2;
static const size_t MIC_UPLOAD_CHUNKED_PART_SIZE = 8192;
static const uint8_t MIC_UPLOAD_CHUNKED_PART_ATTEMPTS = 4;
