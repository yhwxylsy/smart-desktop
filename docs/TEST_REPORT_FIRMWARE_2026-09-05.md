# 固件完整流程测试报告（ESP32S3 / STM32）2026-09-05

> 范围：按用户要求**不测网络后端/云 AI/小程序/部署**，聚焦两块固件可执行正确性。
> 环境：无真机硬件（不烧录、无串口），主机 Windows + arduino-cli 1.5.2 + pytest 9.1.1。
> 结论：**全部通过（编译矩阵 ✅ / 主机回归 10 passed ✅ / 一致性静态校验 89 PASS ✅）**，
> 未发现需修复的固件缺陷。

## 1. 编译门禁矩阵

工具链：`C:\Users\yanghao\arduino-cli\arduino-cli.exe`（`ARDUINO_DATA_DIR=D:\Arduino15`）。

| 项 | FQBN | Flash | RAM | 结果 |
|---|---|---|---|---|
| ESP32 默认态 | `esp32:esp32:XIAO_ESP32S3` | 926053 B (27%) | 46316 B (14%) | ✅ 与基线一致 |
| ESP32 `SMARTDESK_IOTDA_ENABLED=1` | 同上 | 928965 B (27%) | 46492 B (14%) | ✅ 死代码模块可编译 |
| ESP32 `MIC_PATH_ENABLED=true` | 同上 | 961937 B (28%) | 46372 B (14%) | ✅ 麦克风链路可编译 |
| ESP32 恢复默认态复编 | 同上 | 926053 B (27%) | 46316 B (14%) | ✅ 可复现 |
| STM32 默认态 | `STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8` | 61944 B (94%) | 3688 B (18%) | ✅ 与基线一致 |

说明：IOTDA=1 相比默认 +2.9KB（增量来自 MQTT/WiFiClientSecure 死代码实际参与链接）；
MIC=true 相比默认 +35.9KB（`static const bool` 折叠导致 I2S/采集实现被真正保留）。偏差均在预期，非误引入。

## 2. 主机侧协议回归（pytest）

```text
python -m pytest backend/tests/test_firmware_protocol.py backend/tests/test_firmware_command_knowledge.py -q
10 passed
```

- `test_firmware_protocol.py`（6 项）：parse_line/ack_for 常规 + 包装缺 `:NET:`、action_id 内含冒号、空串/空白、仅 `NET:CMD:`、空 action_id ack 回退。
- `test_firmware_command_knowledge.py`（4 项）：classify 37 条、preview 25 条、前缀扫描 8 条黄金语料（oracle：`firmware/stm32/protocol/command_knowledge_reference.py`）。

## 3. 一致性静态校验（tools/verify_firmware_consistency.py）

**89 PASS / 0 FAIL**（只读校验脚本，不修改固件）。

| 检查 | 内容 | 结果 |
|---|---|---|
| A | STM32 `config.h` 引脚/波特率 ↔ README 接线（buzzer PB9、DRV8833 PA0/PA1、超声 PA11/PA10、编码器 PA8/PA9/PB15、KEY1 PB12/KEY2 PB13、RGB PB0/PA7/PA6、串口 PB11/PB10+PB4/PB3+PA3/PA2、9600/4800） | 全 PASS |
| B | ESP32 `config.h` XIAO 引脚（D5=6、D7=44、D2=3、D3=4、D8=7、D9=8、D10=9、MIC 42/41） | 全 PASS |
| C | dispatcher `NET_COMMANDS[]` ↔ 参考前缀集：每条扫描前缀族有对应执行行（`NET:CMD:` 为 parse 层已知例外）；每条执行命令能被前缀扫描识别（无孤立/无隐形命令） | 全 PASS |
| D | `ui_state.cpp` eventLabel switch 返回序 ↔ `command_knowledge_reference.EVENT_LABELS`（枚举下标映射未漂移） | PASS |

注：脚本初版对 STM32 `config.h` 的 `#ifndef INFO_BUTTON_PIN/#else` 覆盖分支取值误取到宏符号（伪 FAIL 2 项），已修正为"优先字面值"后全绿；固件源码零改动。

## 4. 失败项与修复

- 本轮未发现固件源码缺陷，无源码改动。
- 唯一修正：校验脚本 `tools/verify_firmware_consistency.py` 的覆盖分支取值逻辑（属本次新增校验工具自身）。

## 5. 需真机人工验收的遗留项（本环境无硬件）

- [ ] STM32 上电 `BT:BOOT:STM32_EXECUTOR`、USB 115200 日志正常，无首行吞字。
- [ ] `NET:UART?`→`BT:PONG`；`NET:I2C?` 扫到 OLED（0x3D/0x3C）。
- [ ] `NET:UI:DEMO` 全链路自检与 `DEMO:STOP` 中断。
- [ ] 风扇档位/停转、蜂鸣 `NET:BEEP`、旋律、`NET:SERVO:0/90/180`。
- [ ] SYN6288 `NET:TTS:`/`NET:TTSHEX:`/`NET:VOLUME:` 发音与音量。
- [ ] KEY1/KEY2 按键事件 `BT:BTN:` 上报。
- [ ] 遥测 `BT:{...}` 周期上报与 `NET:TELEMETRY?` 即时触发。
- [ ] AHT20/超声/编码器传感数值；RFID 刷卡上报与 15s 去重。
- [ ] ESP32 WiFi/心跳/命令轮询/WebSocket 在线；`CFG:*` 系列命令。
- [ ] 死代码开关置 1 上板行为（IOTDA/MIC）不影响主流程。

详见 `docs/FIRMWARE_MODULAR_REBUILD_2026-09-05.md` §4（上板验收清单）。
