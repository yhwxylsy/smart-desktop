// 智能桌面终端 STM32 执行器固件 —— 调度骨架
//
// 本文件仅保留全局行缓冲、setup()/loop() 调度，功能已按职责拆分到 src/ 下独立模块
// （config.h + 20 个模块，含字库迁入 src/ui/fonts/）。模块间依赖方向严格自上而下，
// 全局量由唯一归属模块以 extern 暴露。纯搬运阶段不改任何协议字符串、引脚号、波特率、
// 时序常量与初始化/轮询顺序（详见 docs/REBUILD_GUARDRAILS.md）。
//
// 编译：arduino-cli compile -b STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8
//       （配合 ARDUINO_DATA_DIR 指向实际数据目录；ESP32 侧见 edge/esp32s3/README.md）

#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "src/core/board.h"
#include "src/core/text_util.h"
#include "src/protocol/protocol.h"
#include "src/protocol/command_line.h"
#include "src/protocol/dispatcher.h"
#include "src/ui/ui_state.h"
#include "src/ui/oled.h"
#include "src/ui/oled_screens.h"
#include "src/ui/rgb.h"
#include "src/audio/tts.h"
#include "src/audio/buzzer.h"
#include "src/sensors/aht20.h"
#include "src/sensors/ultrasonic.h"
#include "src/sensors/encoder.h"
#include "src/sensors/telemetry.h"
#include "src/actuators/fan.h"
#include "src/actuators/servo.h"
#include "src/input/buttons.h"
#include "src/system/i2c_bus.h"
#include "src/system/ui_demo.h"
#include "src/system/user_context.h"

// 行缓冲：由 pollSerial 消费，board 模块 extern 引用。
String usbLine;
String espLine;

void setup() {
  usbConsole.begin(115200);
  espCommandSerial.begin(ESP_IN_BAUD);
  espAckSerial.begin(ESP_ACK_BAUD);
  delay(20);
  while (espCommandSerial.available()) {
    espCommandSerial.read();
  }

  if (drv8833Connected) {
    pinMode(PIN_DRV8833_IN1, OUTPUT);
    pinMode(PIN_DRV8833_IN2, OUTPUT);
  }
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_RGB_BLUE, OUTPUT);
  pinMode(PIN_RGB_GREEN, OUTPUT);
  pinMode(PIN_RGB_RED, OUTPUT);
  pinMode(PIN_SERVO, OUTPUT);
  pinMode(PIN_ULTRASONIC_TRIG, OUTPUT);
  pinMode(PIN_ULTRASONIC_ECHO, INPUT_PULLDOWN);
  pinMode(PIN_NTC, INPUT_ANALOG);
  pinMode(PIN_POTENTIOMETER, INPUT_ANALOG);
  pinMode(PIN_TRACKING_SENSOR, INPUT);
  pinMode(PIN_ENCODER_A, INPUT_PULLUP);
  pinMode(PIN_ENCODER_B, INPUT_PULLUP);
  pinMode(PIN_ENCODER_BUTTON, INPUT_PULLUP);
  pinMode(PIN_INFO_BUTTON, INPUT_PULLUP);
  pinMode(PIN_DEMO_BUTTON, INPUT_PULLUP);

  Wire.begin();
  Wire.setClock(400000);
  aht20Initialized = initializeAht20();

  setRgb(false, false, true);
  stopDrv8833();
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_SERVO, LOW);
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  initEncoder();
  beginUiEvent(UI_EVENT_BOOT, "STM32 EXECUTOR", "", false, millis());
  initializeOled();
  renderSystemOled();
  writeBack("BT:BOOT:STM32_EXECUTOR");
}

void loop() {
  pollSerial(usbConsole, usbLine, "USB");
  pollSerial(espCommandSerial, espLine, "ESP");
  updateEncoder();
  announceVolumeWhenSettled();
  updateServoPulse();
  updateMusicPlayer();
  updateInfoButton();
  updateKey2Button();
  updateUiDemo();
  updateSystemUi();
  if (millis() - lastTelemetryMs >= TELEMETRY_INTERVAL_MS && canSendPeriodicTelemetry()) {
    lastTelemetryMs = millis();
    sendTelemetrySnapshot();
  }
}
