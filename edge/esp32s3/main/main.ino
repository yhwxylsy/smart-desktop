// 智能桌面终端 ESP32S3 边缘桥接固件 —— 调度骨架
//
// 本文件仅保留 setup()/loop() 调度，所有功能已按职责拆分到 src/ 下独立模块。
// 模块间依赖方向严格自上而下（L5 主文件 -> L4 应用 -> L3 状态 -> L2 驱动 -> L1 核心），
// 任何全局量均由唯一归属模块以 extern 暴露。纯搬运阶段不改任何协议字符串、引脚、
// 波特率、时序常量与初始化/轮询顺序（详见 docs/REBUILD_GUARDRAILS.md）。
//
// 编译：arduino-cli compile -b esp32:esp32:XIAO_ESP32S3

#include <Arduino.h>

#include "config.h"
#include "src/core/types.h"
#include "src/core/hex_util.h"
#include "src/config/config_store.h"

#include "src/net/wifi_manager.h"
#include "src/net/http_client.h"
#include "src/net/websocket_link.h"
#include "src/net/heartbeat.h"
#include "src/net/iotda_client.h"

#include "src/bridge/stm32_link.h"
#include "src/bridge/backend_bridge.h"
#include "src/bridge/telemetry_bridge.h"

#include "src/rfid/rfid_reader.h"

#include "src/mic/mic_capture.h"
#include "src/mic/mic_upload.h"
#include "src/mic/mic_pipeline.h"

#include "src/console/serial_cli.h"

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println();
  Serial.println("[BOOT] smart desktop ESP32S3 bridge");

  stm32Tx.begin(9600, SERIAL_8N1, -1, STM32_TX_PIN);
  stm32Rx.begin(4800, SERIAL_8N1, STM32_RX_PIN, -1);

  byte rfidVersion = initializeRfidReader();
  Serial.print("[RFID] reader version=0x");
  Serial.println(rfidVersion, HEX);
  initMicrophone();

  configStore::load();
  if (strlen(SMARTDESK_BOOTSTRAP_SERVER_URL) > 0 &&
      configStore::parseServerUrl(SMARTDESK_BOOTSTRAP_SERVER_URL)) {
    configStore::save();
    Serial.print("[CFG] bootstrap server=");
    Serial.println(configStore::httpBase());
  }
  connectWifi();
  startWebSocket();
}

void loop() {
  pollUsbSerial();
  pollStm32();
  pollRfid();
  if (wsStarted) {
    webSocket.loop();
  } else {
    connectWifi();
    startWebSocket();
  }
#if SMARTDESK_IOTDA_ENABLED
  iotdaLoop();
#endif
  pollBackendCommands();
  sendUartKeepalive();
  sendHeartbeat();
}
