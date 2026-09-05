# 双固件模块化重构归档与上板验收（2026-09-05）

> 范围：`edge/esp32s3/main`（ESP32S3 桥接）与 `firmware/stm32/stm32_executor`（STM32 执行器）
> 纯搬运 → 去耦合两阶段。行为保持硬约束见 `docs/REBUILD_GUARDRAILS.md`（协议串/引脚/波特率/
> 时序常量/setup-loop 顺序逐字未改）。

## 1. 重构结果

### ESP32S3（`edge/esp32s3/main`）
- `main.ino`：1994 行 → **72 行**调度骨架。
- `config.h`：设备标识/引脚/时序常量/`SMARTDESK_*` 编译宏（`iotda`/`mic` 死代码开关）。
- `src/`：`core`(types/hex_util) + `config/config_store`(configStore 命名空间访问器) +
  `net`(wifi/http/websocket/heartbeat/iotda) + `bridge`(stm32_link/backend/telemetry) +
  `rfid` + `mic`(capture/upload/pipeline) + `console/serial_cli`(表驱动 CLI)。
- 全局量唯一归属：`stm32Tx/Rx`、`webSocket`、`rfid`、`iotdaClient/Mqtt`、`voiceState`、
  `micReady/Busy`、`prefs`(已私有) 等均单点定义。

### STM32（`firmware/stm32/stm32_executor`）
- `stm32_executor.ino`：2702 行 → **94 行**调度骨架（setup 引脚初始化与 loop 轮询顺序保留）。
- `config.h`：引脚/波特率/时序常量全部**逐字**搬入（含修正：真实值如 `PIN_BUZZER=PB9`、
  `PIN_RGB_RED=PB0`、`TELEMETRY_INTERVAL_MS=4000`、`DRV8833_FAN_LEVEL1_DUTY=217`）。
- `src/`：`core`(board/text_util) + `protocol`(protocol/command_line/dispatcher) + `ui`
  (ui_state/oled/oled_screens/rgb + fonts 迁入) + `audio`(tts/buzzer) +
  `sensors`(aht20/ultrasonic/encoder/telemetry) + `actuators`(fan/servo) +
  `input`(buttons) + `system`(i2c_bus/ui_demo/user_context)。
- `executeNetCommand` 的 35+ 分支并入 `NET_COMMANDS[]` 单一命令表（行序=原 if 顺序）。
- 死代码标注（去耦合阶段删除项）：`announceVolumeWhenSettled`、`volumeAnnouncementPending`
  （只写不读）、`ttsInterrupted`（只写不读）——已加 `TODO(decouple)`，本次未删。

## 2. 构建与验证（真实工具链）

工具链位置：
- `arduino-cli`：`C:\Users\yanghao\arduino-cli\arduino-cli.exe`（不在 PATH）
- 数据目录：`D:\Arduino15`（调用前设 `ARDUINO_DATA_DIR=D:\Arduino15`）
- git：`D:\Git\bin\git.exe`

命令：

```text
# ESP32
arduino-cli compile -b esp32:esp32:XIAO_ESP32S3 edge/esp32s3/main
# STM32
arduino-cli compile -b STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8 firmware/stm32/stm32_executor
# 主机侧协议/命令知识回归
python -m pytest backend/tests/test_firmware_protocol.py backend/tests/test_firmware_command_knowledge.py -q
```

### 体积/内存结果（与重构阶段对比）

| 目标 | Flash | RAM | 说明 |
|---|---|---|---|
| ESP32 默认（拆分后） | 924177 B (27%) | 46316 B | 基线 |
| ESP32 默认（去耦合后） | 926053 B (27%) | 46316 B | +1.9KB（访问器/命令表） |
| ESP32 `SMARTDESK_IOTDA_ENABLED=1` | 927049 B (27%) | 46492 B | 宏双向门禁绿 |
| ESP32 `MIC_PATH_ENABLED=true` | 961937 B (28%) | 46372 B | 宏双向门禁绿 |
| STM32（拆分后基线） | 61776 B (94%) | 3688 B | — |
| STM32（执行层表驱动后） | 61944 B (94%) | 3688 B | +168B |

### 回归测试

- `test_firmware_protocol.py`（parse_line/ack_for 常规+边界）✅
- `test_firmware_command_knowledge.py`（classify/preview/前缀扫描语料冻结）✅
  - `command_knowledge_reference.py` 为合并前四源语义的黄金参考。

## 3. 问题整改归档

| # | 问题 | 根因 | 修复 |
|---|---|---|---|
| 1 | ESP32 `src/` 相对 include 解析失败 | `stm32_link/backend_bridge/telemetry_bridge` 同处 `bridge/`，跨目录路径写成 `../stm32_link/...` | 改为同目录 `stm32_link.h`/跨目录 `../bridge/...` |
| 2 | `heartbeat.cpp` 找不到 `http_client.h` | 同目录被写成 `../http_client.h` | 改 `"http_client.h"` |
| 3 | `config_store.cpp` 宏未定义/找不到根 `config.h` | 裸 `"config.h"` 在独立 TU 解析不到根文件 | 相对 `../../config.h` |
| 4 | `lastAckMs` extern+static 冲突 | cpp 定义误加 `static` | 去掉 `static` |
| 5 | `SMARTDESK_IOTDA_ENABLED=1` 链接 undefined | `iotda_client.cpp` 在包含 `config.h` **之前**就 `#if SMARTDESK_IOTDA_ENABLED`，未定义宏按 0 → 整文件成空 TU；另 `iotda_client.h` 缺 `<WiFiClientSecure.h>` | include 前置 + 补头 |
| 6 | STM32 `config.h` 曾用臆造引脚/时序值 | 按计划描述而非源码 | 逐字重搬 L8-130 常量并复核 |
| 7 | 个别大文件 `write_to_file` 落空为空文件 | 写入偶发未持久化 | 重写并逐一校验非空（已复查无空 cpp） |
| 8 | STM32 首次编译 Flash 溢出 | 误用 `GenF1` 默认 pnum(32K C6) | 用真实 FQBN `GenF1:pnum=BLUEPILL_F103C8` |
| 9 | 项目目录无 `.git` | 目录从未 init | 在 `smart-desktop-2` 内 `git init` + 首提快照 |

## 4. 上板验收清单（无硬件验证后需人工执行）

- [ ] **编译门禁**：两侧 `arduino-cli compile` 零错误（命令见上），Flash/RAM 与上表一致。
- [ ] **diff 纪律**：确认本次改动只含代码块位移/表驱动/访问器，无协议字符串与日志格式改动。
- [ ] USB 串口 `115200`：上电见 `[BOOT]`/`BT:BOOT:STM32_EXECUTOR`，无首行吞字。
- [ ] `NET:UART?` → `BT:PONG:<ms>`；`NET:I2C?` 能扫到 OLED（`0x3D` 或 `0x3C`）。
- [ ] 发送 `NET:UI:DEMO`，走完 OLED/灯/TTS/风扇/蜂鸣/锁/音效各步，`[DEMO] step=` 日志逐条出现；`NET:UI:DEMO:STOP` 可中断。
- [ ] `NET:FAN:ON:1/2/3`、`NET:FAN:OFF`、`NET:MOTOR:OFF` 档位与停转正确。
- [ ] `NET:TTS:nihao`、`NET:TTSHEX:E4BDA0E5A5BD`（你好）、`NET:VOLUME:8`、`NET:MUSIC:SUCCESS` 发音正常。
- [ ] `NET:SERVO:0/90/180` 转动范围正常；`NET:BEEP` 响。
- [ ] KEY1 翻页/长按回主屏；KEY2 短按/长按事件 `BT:BTN:` 上报正确。
- [ ] 遥测行 `BT:{...}` 周期上报；`NET:TELEMETRY?` 可即时触发；字段与旧版一致。
- [ ] 环境传感：AHT20/超声/编码器数值合理；`NET:ULTRASONIC:OFF` 生效。
- [ ] ESP32 侧：WiFi/后端心跳/命令轮询/WebSocket 在线，`[EVT]` 时序行字段不变。
- [ ] RFID 刷卡 → `NET:RFID:SCAN:` 通知 + 后端上报；重复卡 15s 抑制。
- [ ] `CFG:WIFI:<ssid>,<pwd>`、`CFG:SERVER:`、`CFG:TOKEN:`、`CFG:RESET` 行为符合预期。
- [ ] 死代码开关：`SMARTDESK_IOTDA_ENABLED`、`MIC_PATH_ENABLED` 各自置 1 后仍可编译。

## 5. 已知后续项（留待硬件回归/后续迭代）

- STM32 四源命令知识“真正单表合一”（classify/preview/前缀扫描并入 `NET_COMMANDS[]`）：当前
  执行层已表驱动，其余为单点纯函数并被黄金语料冻结；合并属负收益高风险，留待硬件回归后评估。
- 确证死代码删除：`announceVolumeWhenSettled` 等（加 TODO，未删，避免无硬件时行为漂移）。
- `local_config.h` 覆盖机制已生效（gitignore）；部署时经它注入 SSID/TOKEN/IoTDA 凭据。
