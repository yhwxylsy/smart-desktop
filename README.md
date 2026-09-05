# 智能桌面 AI 终端（Smart Desktop AI Terminal）

基于 **STM32F103C8T6 + XIAO ESP32S3 Sense + SYN6288 + RC522** 的桌面级智能 AI 终端。
用户通过文本/语音提问 → FastAPI 后端（AI + 实时会话）→ 生成硬件动作指令 → ESP32S3
桥接 → STM32 执行（OLED 状态 / RGB 灯效 / TTS 播报 / 风扇 / 舵机 / 蜂鸣器 / 旋律）→
动作 ACK 回执闭环。同时提供 Web 控制台与微信小程序作为 PC/移动端应用。

> 定位：一个把"语言模型 + 状态机 + 单片机外设 + 实时总线"串起来的完整端到端示例工程。

---

## 目录

- [1. 项目背景与动机](#1-项目背景与动机)
- [2. 功能特性](#2-功能特性)
- [3. 技术架构与设计](#3-技术架构与设计)
- [4. 目录结构说明](#4-目录结构说明)
- [5. 安装与使用](#5-安装与使用)
- [6. API 文档](#6-api-文档)
- [7. 配置项说明](#7-配置项说明)
- [8. 测试方法与示例](#8-测试方法与示例)
- [9. 贡献指南](#9-贡献指南)
- [10. 许可证](#10-许可证)
- [11. 常见问题（FAQ）](#11-常见问题faq)

---

## 1. 项目背景与动机

本项目源于物联网工程综合课程设计的"智能桌面终端"。初版工程为演示功能进行大量补丁式开发，
导致引脚、协议、状态灯语义彼此冲突、难以维护。本仓库是对整条产品链路的**重建基线**：

- **统一协议**：跨 ESP32S3/STM32 的 `NET:CMD:`/`NET:` 命令协议与 `BT:*` 回执协议只保留一份事实来源。
- **统一状态机**：UI/状态灯/OLED 全部来自一套会话状态（不再用"绿灯亮"代表系统健康）。
- **分层验收**：STM32 执行、ESP32 桥接、后端 AI 闭环、Web/小程序各层可独立验收后再联调。

主要动机：
1. 验证"用户自然输入 → 云端/本地大模型决策 → 单片机动作执行 → 执行 ACK 回执"的完整智能硬件闭环。
2. 覆盖常见课程设计评分点：PC 应用端（Web）+ 移动应用端（微信小程序）+ 物联网硬件接入。
3. 提供一个**无云依赖也能跑**的最小模式（`ai_provider=mock` + 本地规则动作规划），便于教学演示。

## 2. 功能特性

### 硬件执行端（STM32F103C8T6）
- OLED（128×64，SSD1306，I2C 400 kHz、4 ms 分页刷新）状态屏与 4 个子屏（用户/链路/传感/执行器）。
- RGB 三色灯状态动画（启动/就绪/聆听/思考/播报/执行/锁定/错误）。
- SYN6288 TTS 中文播报（UTF-8 → UTF-16BE → 0xFD 帧，音量为 0–16 级映射 0–100%）。
- 被动蜂鸣器：`NET:BEEP` 与五段非阻塞旋律（SUCCESS/ALERT/SCALE/STARTUP/BIRTHDAY）。
- DRV8833 风扇三档（约 85%/92%/100% PWM）与停转；舵机 0–180° 非阻塞脉冲；锁定/解锁电平。
- 传感器遥测：AHT20 温湿度、HC-SR04 超声波、NTC/电位器、红外跟踪、旋转编码器（旋钮调音量）。
- 双按键：KEY1 翻屏/回主屏，KEY2 打断/PTT；`BT:BTN:` 事件上报。
- `NET:UI:DEMO` 无阻塞全链路自检。

### 桥接端（XIAO ESP32S3 Sense）
- WiFi 连接与断线自动重连；配置存 NVS（SSID/服务器/Token，不在源码写死）。
- WebSocket 实时通道（`/api/realtime/ws`）+ HTTP 轮询兜底；UART 保活（命令 9600 / ACK 4800 非对称）。
- 双 HardwareSerial 桥接 STM32；`[EVT]` 事件时序日志；串口 CLI（`CFG:*`）。
- RC522 RFID 刷卡：上报 + 15 s 去重 + 读卡器健康自恢复。
- 保留编译开关：`SMARTDESK_IOTDA_ENABLED`（华为云 IoTDA MQTT 上报）、`MIC_PATH_ENABLED`（板载麦克风 ASR，默认关闭，语音入口以笔记本 PTT 为主）。

### 后端（FastAPI）
- 设备状态快照、RFID 用户注册/绑定/切换上下文；用户模式 study/rest/demo/admin。
- AI 动作规划：动作 `ActionSpec(type,payload)` → `NET:` 命令包装 `NET:CMD:<action_id>:...`；
  ACK 解析回执 `BT:ACK:<action_id>:OK/ERR`；支持 whitelist 工具集。
- 三种 AI 模式：`mock`（本地规则，无网可用）/ 兼容 OpenAI 协议的云端模型 / 实时工具调用。
- ASR：整段上传、分片上传（`/api/asr/transcribe/chunk`）、客户端识别文本直交；
  支持 DashScope Paraformer 实时与本地 FunASR 通道。
- WebSocket 实时会话：`ping/wake/text/tools/*/button/ack` 入站，`state/speak/assistant/stm32/commands/...` 广播。
- Web 控制台（`/console`）与移动控制台（`/mobile`）。

### 应用端
- Web：状态总览、实时控制台、动作下发、诊断。
- 微信小程序：单页控制端（总览/安防/控制/传感/对话/RFID/动作/诊断），1.5 s 轮询 + WebSocket 实时。

## 3. 技术架构与设计

```mermaid
flowchart LR
  U[用户 文本/语音] --> B[FastAPI 后端]
  B -->|AI 规划出 ActionSpec| A[动作/上下文引擎]
  A -->|NET:CMD:action_id:NET:xxx| WS[(WebSocket /api/realtime/ws)]
  WS --> E[ESP32S3 桥接]
  E <-->|UART 9600/4800 + BT:* 回执| S[STM32 执行器]
  S -->|OLED/RGB/SYN6288/风扇/舵机/蜂鸣/旋律| Hw[桌面硬件]
  S -->|BT:{telemetry}| B
  R[RC522 RFID] --> E --> B
  M[笔记本 PTT 语音] -->|/api/asr/*| B
  P[微信小程序 / Web] <-->|REST + WS| B
```

- **统一会话状态机**（STM32）：`S0 BOOT → S1 LOCKED → S2 READY → S3 LISTEN → S4 PROCESS → S5 SPEAK → S6 EXEC → S7 ERROR`；
  RGB/OLED/锁定逻辑全部派生自此状态机。`LOCKED` 只能被 `NET:LOCK:OFF` 解锁。
- **命令协议**（向下兼容，逐字保留）：见 [docs/PROTOCOL.md](docs/PROTOCOL.md)。
- **动作规划**（后端）：AI 输出结构化 `ActionSpec`，白名单工具集映射成 `NET:` 命令
  （`tts_speak→NET:TTSHEX:`、`fan_control→NET:FAN:ON:n`、`lock_control→NET:LOCK:ON/OFF`、`lamp_control→NET:AI:*` 等），
  见 [docs/ACTION_OUTLINE.md](docs/ACTION_OUTLINE.md)。
- **接线事实**：以 [docs/HARDWARE_WIRING.md](docs/HARDWARE_WIRING.md) 为准（STM32 引脚/串口复用：
  `espCommandSerial` 的 PB10 同时连接 ESP32 与 SYN6288）。
- **安全基线**：见 [docs/REBUILD_GUARDRAILS.md](docs/REBUILD_GUARDRAILS.md)（密钥不入固件、不改协议串等 9 条禁区）。

## 4. 目录结构说明

```text
smart-desktop/
├── backend/                 # FastAPI 后端
│   ├── app/                 # 主应用（main.py 路由、actions 动作规划、ai/asr/context…）
│   ├── tests/               # pytest（固件协议、动作协议、phase1 等）
│   ├── requirements.txt     # 依赖
│   └── data/                # 运行时数据（sqlite/json/audio，gitignore）
├── edge/esp32s3/            # ESP32S3 桥接固件（Arduino）
│   ├── main/                # main.ino 调度骨架 + config.h + src/（13 个模块）
│   └── README.md
├── firmware/stm32/          # STM32 执行器固件（Arduino/STM32duino）
│   ├── protocol/            # 主机侧协议参考实现（python）
│   ├── stm32_executor/      # .ino 调度骨架 + config.h + src/（20 个模块）+ 字库
│   └── README.md
├── miniprogram/             # 微信小程序控制端
├── deploy/                  # 部署样例：docker-compose.public.yml、Caddyfile、.env.example、huawei-cci
├── tools/                   # 启动/冒烟/本机语音 sidecar/一致性校验等脚本
├── docs/                    # 对外文档（架构/协议/接线/动作大纲/测试与重构归档…）
├── Dockerfile / render.yaml / runtime.txt
└── README.md
```

## 5. 安装与使用

### 5.1 后端（本地运行，Mock 模式无需任何 Key）

```powershell
python -m pip install -r backend/requirements.txt
python tools/start_backend.py        # 启动 uvicorn :8083 并自动健康检查
# 或前台：
cd backend
python -m uvicorn app.main:app --host 0.0.0.0 --port 8083
```

健康检查返回示例：

```json
{"status":"ok","protocol":"smart-desktop-realtime-v1","ai_provider":"mock","ai_model":"local-rules","cloud_ready":false,"device_id":"desktop-agent-001","edge_id":"esp32s3-sense-001"}
```

接入云端模型时创建 `backend/.env`（字段见 [§7 配置项](#7-配置项说明)）。

### 5.2 ESP32S3 桥接固件（Arduino-cli）

```powershell
$env:ARDUINO_DATA_DIR="D:\Arduino15"            # 按你的 arduino-cli 数据目录设置
arduino-cli compile -b esp32:esp32:XIAO_ESP32S3 edge/esp32s3/main
```

上电前经 USB 串口配置（写 NVS，勿写死进源码）：

```text
CFG:WIFI:<ssid>,<password>
CFG:SERVER:http://<后端局域网IP>:8083
CFG:TOKEN:<可选设备令牌>
```

### 5.3 STM32 执行器固件

```powershell
arduino-cli compile -b STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8 firmware/stm32/stm32_executor
```

接线与刷写细节见 [docs/HARDWARE_WIRING.md](docs/HARDWARE_WIRING.md) 与 `firmware/stm32/README.md`。

### 5.4 微信小程序

用微信开发者工具导入 `miniprogram/`（示例 AppID 已占位为 `wx0000000000000000`，请填自己的）。
在小程序内可输入后端地址 `apiBase`（存本地 storage）。

### 5.5 本机语音 sidecar（可选，笔记本麦克风当语音入口）

```powershell
python tools/laptop_wakeword_sidecar.py     # 唤醒词 → 录音对话
python tools/laptop_realtime_listener.py    # 实时监听 / PTT
python tools/laptop_mic_sidecar.py          # 录音上传转写
```

## 6. API 文档

### 6.1 HTTP 接口（FastAPI，`backend/app/main.py`）

| 方法 | 路径 | 说明 |
|---|---|---|
| GET | `/api/health` | 健康检查（provider/model/cloud_ready） |
| GET | `/api/state/{device_id}` | 设备实时状态快照 |
| GET/POST | `/api/users` | RFID 用户列表 / 创建用户 |
| POST | `/api/context/select` | 切换用户对话上下文 |
| POST | `/api/rfid/enroll/start` · GET | `/api/rfid/enroll/{id}` · `/cancel`：RFID 注册流程 |
| POST | `/api/hardware/telemetry` | 传感器遥测上报（ESP32 转发 STM32 `BT:{...}`） |
| POST | `/api/hardware/heartbeat` | 在线/串口保活上报 |
| GET | `/api/hardware/commands/{device_id}` | 轮询待执行命令（HTTP 兜底链路） |
| POST | `/api/hardware/action` | 手动下发一个动作 |
| POST | `/api/hardware/ack` | 动作 ACK 回执 |
| POST | `/api/hardware/button` | 按钮事件上报 |
| POST | `/api/rfid/register` · `/scan` | UID 绑定 / 刷卡（解锁或拒绝） |
| POST | `/api/chat` | 文本对话闭环入口 |
| POST | `/api/asr/transcribe` · `/chunk` · `/recognized` | 语音转写（整段/分片/客户端直交） |
| POST | `/api/realtime/inject` | 向实时会话注入文本 |
| GET | `/api/realtime/status` · `/diagnostics/{device_id}` | 实时连接与动作队列诊断 |
| GET | `/` · `/console` · `/mobile` | 首页/Web 控制台/移动控制台 |
| WS | `/api/realtime/ws?device_id=&edge_id=` | 实时双向通道 |

### 6.2 WebSocket 实时通道

- 连接后服务端先发 `{"type":"hello","protocol":...}`。
- 入站 `type`：`ping`（回 pong）、`wake`、`text`（触发对话回合）、`tools/list`、`tools/call`、`button`、`ack`/`stm32/ack`。
- 出站 `type`：`hello`、`state`、`speak`、`assistant`、`stm32/commands`、`button`、`button/ack`、`interrupt`、
  `rfid/scan`、`rfid/user`、`telemetry`、`heartbeat`、`ack`、`asr/result`、`error`。
- 可用动作工具：`tts_speak, volume_control, oled_display, fan_control, buzzer_alert, buzzer_music, focus_mode, servo_action, lock_control, lamp_control`。

### 6.3 固件侧协议（ESP32S3 ↔ STM32）

命令封装：`NET:CMD:<action_id>:NET:<COMMAND>`；直连调试命令：`NET:<COMMAND>`。
常用命令族（详见 `docs/PROTOCOL.md`）：

```text
NET:UI:LISTEN|THINK|ACTION|ACK|OUTPUT|IDLE|ERROR   # UI 状态提示
NET:UI:USER:<user>:<carduid>:<MODE>                # 用户上下文
NET:UART?  NET:I2C?  NET:TELEMETRY?  NET:UI:STATUS?
NET:TTS:<text>  NET:TTSHEX:<utf8hex>  NET:TTS:STOP  NET:VOLUME:<0-16|UP|DOWN>
NET:OLED:<text>  NET:RGB:MODE:SENSOR|EVENT  NET:RGB:STATUS?  NET:RGB:LEGEND?
NET:FAN:ON:<1-3>  NET:FAN:OFF  NET:MOTOR:OFF  NET:SERVO:<0-180>
NET:LOCK:ON|OFF  NET:AI:BUSY|IDLE|OFF  NET:BEEP  NET:MUSIC:<preset|STOP>
NET:RFID:<text>  NET:UI:DEMO  NET:UI:DEMO:STOP
```

回执：

```text
BT:ACK:<action_id>:OK|ERR       # 包装命令
BT:OK / BT:ERR                  # 直连命令
BT:PONG:<uptime_ms>             # UART 保活
BT:BTN:KEY1:* / KEY2:*          # 按键事件
BT:{...sensors...}              # 遥测 JSON（字段见 docs/PROTOCOL.md）
```

## 7. 配置项说明

### 7.1 后端 `.env`（放到 `backend/.env`，模板见 `deploy/.env.server.example`）

| 变量 | 默认 | 说明 |
|---|---|---|
| `APP_NAME` | Smart Desktop AI Terminal | 应用名 |
| `DEVICE_ID` / `EDGE_ID` | desktop-agent-001 / esp32s3-sense-001 | 设备/边缘标识 |
| `AI_PROVIDER` | mock | `mock`（本地规则）/ OpenAI 兼容云端 |
| `AI_BASE_URL` | dashscope 兼容端点 | OpenAI 兼容 base url |
| `AI_MODEL` | qwen-plus | 模型名 |
| `DASHSCOPE_API_KEY` | 空 | 云模型密钥 |
| `CONTROL_TOKEN` | 空 | 管理接口令牌 |
| `DEVICE_TOKEN` | 空 | 设备令牌 |
| `ASR_PROVIDER` | dashscope_paraformer | `dashscope_paraformer` / `funasr_local` 等 |
| `ASR_WS_URL` / `ASR_MODEL` / `ASR_LANGUAGE_HINT` | dashscope 实时 | 云端 ASR 参数 |
| `ASR_LOCAL_*` | paraformer-zh / fsmn-vad / ct-punc / cpu | 本地 FunASR 通道参数 |

### 7.2 ESP32 串口 CLI（写 NVS）

`CFG:WIFI:<ssid>,<pwd>`、`CFG:WIFI:SHOW`、`CFG:WIFI:SCAN`、`CFG:NET:TCP:<host>:<port>`、
`CFG:SERVER:<url>`、`CFG:TOKEN:<token>`、`CFG:RESET`、`CFG:UART:PING`、`CFG:RFID:STATUS|RESET`、
`CFG:MIC:*`（自检/录音）、`CFG:TTS:`、`CFG:OLED:`、`CHAT:<text>`。

固件侧编译开关（`edge/esp32s3/main/config.h`）：`SMARTDESK_IOTDA_ENABLED`（华为云 IoTDA MQTT）、
`MIC_PATH_ENABLED`（板载麦克风）；`local_config.h` 可本地覆盖（不入库）。

### 7.3 STM32

引脚/波特率/时序常量集中在 `firmware/stm32/stm32_executor/config.h`（命令 9600 / ACK 4800 非对称约定勿改）。
串口复用约束：`espCommandSerial` TX(PB10) 同时接 ESP32 与 SYN6288。

## 8. 测试方法与示例

```powershell
# 固件协议 + 命令知识主机回归（无需硬件）
python -m pytest backend/tests/test_firmware_protocol.py backend/tests/test_firmware_command_knowledge.py -q

# 固件一致性静态校验（引脚/波特率、命令表前缀、eventLabel 枚举序 ↔ 参考）
python tools/verify_firmware_consistency.py

# 双端编译门禁
arduino-cli compile -b esp32:esp32:XIAO_ESP32S3 edge/esp32s3/main
arduino-cli compile -b STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8 firmware/stm32/stm32_executor
```

后端相关测试见 `backend/tests/`（含动作协议与 phase1）；文本闭环可用 `tools/backend_smoke.py` 对运行中的
后端做 health→chat→命令轮询→ACK 冒烟。真机（串口）上板验收清单见 `docs/ACCEPTANCE_CHECKLIST.md`。

## 9. 贡献指南

1. Fork 本仓库并新建功能分支。
2. 遵守 [docs/REBUILD_GUARDRAILS.md](docs/REBUILD_GUARDRAILS.md) 行为保持红线（协议串/引脚/波特率/时序默认值改动需评审）。
3. 提交前运行 §8 的 pytest 与一致性校验；固件改动需给出 `arduino-cli compile` 全绿证据。
4. 保持"命令知识单一事实来源"：STM32 命令表在 `firmware/stm32/stm32_executor/src/protocol/`，
   主机侧参考在 `firmware/stm32/protocol/*.py`，两者变更需同步并更新黄金语料。
5. PR 描述请写明动机、改动范围、验证结果。

## 10. 许可证

本仓库**暂未附带开源许可证（All rights reserved）**——作者保留所有权利。这意味着：
默认情况下你**不能**在未经授权的情况下复制、修改、分发或商用本仓库代码（学习参考不受此限）。
如你有特定使用/合作意图，请先联系作者获取书面授权，或由作者按需为仓库补充开源许可证（如 MIT）。

## 11. 常见问题（FAQ）

**Q1：没有硬件能不能跑起来？**
能。后端默认 `AI_PROVIDER=mock`，`/api/health`、`/api/chat`（本地规则动作规划）无需任何 Key；
固件可用 `arduino-cli` 编译验证，协议可用主机侧 pytest 验证。

**Q2：语音入口是哪条？**
主链路由**笔记本麦克风 + tools 侧车（唤醒词/PTT/实时）**经 `/api/asr/*` 上传；
ESP32S3 板载麦克风默认关闭（`MIC_PATH_ENABLED`）。如需开启请同时开启对应后端 ASR 通道。

**Q3：为什么 STM32 命令口与 ACK 口波特率不同？**
历史接线原因（ESP32→STM32 命令 9600；STM32 软件串口回 ACK 4800）。改动会破坏链路，勿改。

**Q4：如何让 ESP32 连上我的后端？**
`CFG:WIFI:<ssid>,<pwd>` + `CFG:SERVER:http://<局域网IP>:8083`（或公网地址），保存后自动重连；
首选走 WebSocket `/api/realtime/ws`，断线自动切 HTTP 轮询兜底。

**Q5：RFID 刷卡后没反应？**
先 `CFG:RFID:STATUS` 看读卡器健康；`NET:I2C?`/后端日志确认用户是否已注册绑定；
同一 UID 15 秒内重复刷卡会被抑制。

**Q6：`NET:UI:DEMO` 是干嘛的？**
无阻塞桌面自检：OLED/灯/TTS/风扇/蜂鸣/锁/音频动效逐步演示，`NET:UI:DEMO:STOP` 停止。

**Q7：改了后端/协议后如何自证没破坏？**
运行 §8 全部命令；固件改动对照 [docs/FIRMWARE_MODULAR_REBUILD_2026-09-05.md](docs/FIRMWARE_MODULAR_REBUILD_2026-09-05.md)
与 `docs/TEST_REPORT_FIRMWARE_2026-09-05.md` 的验收清单与基线体积。

**Q8：密钥放哪里？**
一律放 `backend/.env` / 部署 secret / ESP32 `local_config.h`（均被 .gitignore 忽略），严禁写入固件源码。

---

更多细节请查阅 [docs/](docs/)（架构、协议、接线、动作大纲、测试与重构归档）。
