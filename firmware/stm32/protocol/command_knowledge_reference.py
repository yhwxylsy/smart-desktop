# -*- coding: utf-8 -*-
# 冻结自拆分后 C++ 实现的"命令知识"四源语义（合并 command_table 前的黄金参考）。
# 与以下 C++ 逐字对应：
#   - isKnownNetCommandStart / stringMatchesAt / findKnownNetCommandStart (src/protocol/command_line.cpp)
#   - classifyNetCommand                                     (src/protocol/command_line.cpp)
#   - commandPreview                                         (src/protocol/command_line.cpp)
#   - eventLabel(UiEventType) 的枚举序                        (src/ui/ui_state.cpp)
# 用途：1) 作为主机侧回归规格锁定旧行为；
#       2) command_table 合并完成后，用它做新旧等价性判定的 oracle。
from __future__ import annotations

from dataclasses import dataclass

# UiEventType 枚举序（见 ui_state.cpp eventLabel 的 switch 顺序）
EVENT_LABELS = [
    "BOOT",       # UI_EVENT_BOOT
    "UI DEMO",    # UI_EVENT_DEMO
    "LISTEN",     # UI_EVENT_LISTEN
    "THINK",      # UI_EVENT_THINK
    "ACTION",     # UI_EVENT_ACTION
    "ACK",        # UI_EVENT_ACK
    "UART",       # UI_EVENT_UART
    "I2C SCAN",   # UI_EVENT_I2C
    "TELEMETRY",  # UI_EVENT_TELEMETRY
    "OLED",       # UI_EVENT_OLED
    "SYN6288 TTS",  # UI_EVENT_TTS
    "FAN ON",     # UI_EVENT_FAN_ON
    "FAN OFF",    # UI_EVENT_FAN_OFF
    "BEEP",       # UI_EVENT_BEEP
    "MUSIC",      # UI_EVENT_MUSIC
    "LOCK ON",    # UI_EVENT_LOCK_ON
    "LOCK OFF",   # UI_EVENT_LOCK_OFF
    "AI BUSY",    # UI_EVENT_AI_BUSY
    "AI IDLE",    # UI_EVENT_AI_IDLE
    "AI OFF",     # UI_EVENT_AI_OFF
    "SERVO",      # UI_EVENT_SERVO
    "RFID",       # UI_EVENT_RFID
    "ERROR",      # UI_EVENT_ERROR
]
# classify 返回的枚举值即上述列表下标；本模块直接用标签名便于断言。
_EVENT_INDEX = {name: i for i, name in enumerate(EVENT_LABELS)}
ERROR_INDEX = _EVENT_INDEX["ERROR"]
RFID_INDEX = _EVENT_INDEX["RFID"]
LISTEN_INDEX = _EVENT_INDEX["LISTEN"]
THINK_INDEX = _EVENT_INDEX["THINK"]
ACTION_INDEX = _EVENT_INDEX["ACTION"]
ACK_INDEX = _EVENT_INDEX["ACK"]
AI_IDLE_INDEX = _EVENT_INDEX["AI IDLE"]
TELEMETRY_INDEX = _EVENT_INDEX["TELEMETRY"]
UART_INDEX = _EVENT_INDEX["UART"]
I2C_INDEX = _EVENT_INDEX["I2C SCAN"]
TTS_INDEX = _EVENT_INDEX["SYN6288 TTS"]
OLED_INDEX = _EVENT_INDEX["OLED"]
BEEP_INDEX = _EVENT_INDEX["BEEP"]
MUSIC_INDEX = _EVENT_INDEX["MUSIC"]
FAN_ON_INDEX = _EVENT_INDEX["FAN ON"]
FAN_OFF_INDEX = _EVENT_INDEX["FAN OFF"]
LOCK_ON_INDEX = _EVENT_INDEX["LOCK ON"]
LOCK_OFF_INDEX = _EVENT_INDEX["LOCK OFF"]
AI_BUSY_INDEX = _EVENT_INDEX["AI BUSY"]
AI_OFF_INDEX = _EVENT_INDEX["AI OFF"]
SERVO_INDEX = _EVENT_INDEX["SERVO"]
DEMO_INDEX = _EVENT_INDEX["UI DEMO"]


def upper_copy(text: str) -> str:
    return text.upper()


def contains_upper_token(text: str, token: str) -> bool:
    # C++: upperCopy(text).indexOf(token) >= 0（token 不转大写，调用点传大写 "RFID"）
    return upper_copy(text).find(token) >= 0


def compact(text: str, max_len: int) -> str:
    # 复刻 C++ compactForDisplay：可打印 ASCII(32..126) 保留；
    # 非可打印折叠为单个 '?'（连续折叠）；达到 maxLen 即停止。
    # 语料为 ASCII 命令，逐字符处理与 C++ 逐字节一致。
    out: list[str] = []
    last_replacement = False
    for raw in text:
        if len(out) >= max_len:
            break
        code = ord(raw) & 0xFF
        if 32 <= code <= 126:
            out.append(chr(code))
            last_replacement = False
        elif not last_replacement:
            out.append("?")
            last_replacement = True
    return "".join(out)


# ---- 前缀表（isKnownNetCommandStart 的 20 条）----
KNOWN_PREFIXES = [
    "NET:CMD:",
    "NET:UART?",
    "NET:I2C?",
    "NET:TELEMETRY?",
    "NET:ULTRASONIC:",
    "NET:MOTOR:",
    "NET:TTS:",
    "NET:TTSHEX:",
    "NET:AUDIO:",
    "NET:VOLUME:",
    "NET:OLED:",
    "NET:BEEP",
    "NET:MUSIC:",
    "NET:FAN:",
    "NET:LOCK:",
    "NET:AI:",
    "NET:UI:",
    "NET:RGB:",
    "NET:SERVO:",
    "NET:RFID:",
]


def string_matches_at(text: str, index: int, prefix: str) -> bool:
    if index < 0 or index + len(prefix) > len(text):
        return False
    return text[index : index + len(prefix)] == prefix


def is_known_net_command_start(text: str, index: int) -> bool:
    if index < 0 or index >= len(text):
        return False
    if index > 0 and text[index - 1] == ":":
        return False
    return any(string_matches_at(text, index, prefix) for prefix in KNOWN_PREFIXES)


def find_known_net_command_start(text: str, from_index: int) -> int:
    for i in range(from_index, len(text)):
        if is_known_net_command_start(text, i):
            return i
    return -1


def classify(command: str, action_id: str) -> int:
    """返回 UiEventType 枚举下标，复刻 classifyNetCommand 分支顺序。"""
    if contains_upper_token(command, "RFID") or contains_upper_token(action_id, "RFID"):
        return RFID_INDEX
    if command == "NET:UI:LISTEN":
        return LISTEN_INDEX
    if command == "NET:UI:THINK":
        return THINK_INDEX
    if command == "NET:UI:ACTION":
        return ACTION_INDEX
    if command == "NET:UI:ACK":
        return ACK_INDEX
    if command == "NET:UI:OUTPUT":
        return AI_IDLE_INDEX
    if command == "NET:UI:IDLE":
        return AI_IDLE_INDEX
    if command == "NET:UI:ERROR":
        return ERROR_INDEX
    if command == "NET:UI:DEMO":
        return DEMO_INDEX
    if command == "NET:UI:DEMO:STOP" or command == "NET:UI:STATUS?":
        return TELEMETRY_INDEX
    if command.startswith("NET:UI:USER:"):
        return RFID_INDEX
    if command.startswith("NET:UI:"):
        return ERROR_INDEX
    if command == "NET:UART?":
        return UART_INDEX
    if command == "NET:I2C?":
        return I2C_INDEX
    if command == "NET:TELEMETRY?":
        return TELEMETRY_INDEX
    if command.startswith("NET:RGB:"):
        return TELEMETRY_INDEX
    if command in ("NET:TTS:STOP", "NET:AUDIO:STOP") or command.startswith(("NET:TTS:", "NET:TTSHEX:", "NET:VOLUME:")):
        return TTS_INDEX
    if command.startswith("NET:OLED:"):
        return OLED_INDEX
    if command == "NET:BEEP":
        return BEEP_INDEX
    if command.startswith("NET:MUSIC:"):
        return MUSIC_INDEX
    if command.startswith("NET:FAN:ON"):
        return FAN_ON_INDEX
    if command == "NET:FAN:OFF":
        return FAN_OFF_INDEX
    if command == "NET:LOCK:ON":
        return LOCK_ON_INDEX
    if command == "NET:LOCK:OFF":
        return LOCK_OFF_INDEX
    if command == "NET:AI:BUSY":
        return AI_BUSY_INDEX
    if command == "NET:AI:IDLE":
        return AI_IDLE_INDEX
    if command == "NET:AI:OFF":
        return AI_OFF_INDEX
    if command.startswith("NET:SERVO:"):
        return SERVO_INDEX
    return ERROR_INDEX


def preview(command: str) -> str:
    """复刻 commandPreview 返回显示串。"""
    if command == "NET:TTS:STOP" or command == "NET:AUDIO:STOP":
        return "TTS STOP"
    if command.startswith("NET:UI:USER:"):
        return "USER " + compact(command[len("NET:UI:USER:"):], 14)
    if command.startswith("NET:VOLUME:"):
        return "VOLUME " + compact(command[len("NET:VOLUME:"):], 4)
    if command.startswith("NET:TTSHEX:"):
        hex_chars = len(command) - len("NET:TTSHEX:")
        return "TTSHEX " + str(hex_chars // 2) + "B"
    if command.startswith("NET:TTS:"):
        return "TTS " + compact(command[len("NET:TTS:"):], 16)
    if command.startswith("NET:OLED:"):
        return "OLED " + compact(command[len("NET:OLED:"):], 16)
    if command.startswith("NET:FAN:ON"):
        return "FAN ON"
    if command == "NET:FAN:OFF":
        return "FAN OFF"
    if command == "NET:BEEP":
        return "BEEP"
    if command.startswith("NET:MUSIC:"):
        return "MUSIC " + compact(command[len("NET:MUSIC:"):], 12)
    if command.startswith("NET:LOCK:"):
        return command[len("NET:"):]
    if command.startswith("NET:AI:"):
        return command[len("NET:"):]
    if command.startswith("NET:UI:"):
        return command[len("NET:"):]
    if command == "NET:UART?":
        return "UART PING"
    if command == "NET:I2C?":
        return "I2C SCAN"
    if command == "NET:TELEMETRY?":
        return "TELEMETRY"
    if command.startswith("NET:RGB:"):
        return "RGB " + compact(command[len("NET:RGB:"):], 12)
    if command.startswith("NET:ULTRASONIC:"):
        return "ULTRASONIC " + compact(command[len("NET:ULTRASONIC:"):], 10)
    if command.startswith("NET:MOTOR:"):
        return "MOTOR " + compact(command[len("NET:MOTOR:"):], 12)
    if command.startswith("NET:SERVO:"):
        return "SERVO " + compact(command[len("NET:SERVO:"):], 12)
    return compact(command, 20)
