"""命令知识黄金回归：冻结 command_knowledge_reference.py（复刻 C++ 合并前语义）。

command_table 合并后，任何改动都必须使此处语料断言保持成立（等价性安全网）。
"""
import importlib.util
import sys
from pathlib import Path


def load_knowledge_module():
    path = (
        Path(__file__).resolve().parents[2]
        / "firmware"
        / "stm32"
        / "protocol"
        / "command_knowledge_reference.py"
    )
    spec = importlib.util.spec_from_file_location("command_knowledge_reference", path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def _event_label(module, index):
    return module.EVENT_LABELS[index]


# (command, action_id, 期望 classify 事件标签)
CLASSIFY_CORPUS = [
    ("NET:UI:LISTEN", "", "LISTEN"),
    ("NET:UI:THINK", "", "THINK"),
    ("NET:UI:ACTION", "", "ACTION"),
    ("NET:UI:ACK", "", "ACK"),
    ("NET:UI:OUTPUT", "", "AI IDLE"),
    ("NET:UI:IDLE", "", "AI IDLE"),
    ("NET:UI:ERROR", "", "ERROR"),
    ("NET:UI:DEMO", "", "UI DEMO"),
    ("NET:UI:DEMO:STOP", "", "TELEMETRY"),
    ("NET:UI:STATUS?", "", "TELEMETRY"),
    ("NET:UI:USER:alice-001", "", "RFID"),
    ("NET:UI:ACTION2", "", "ERROR"),
    ("NET:UART?", "", "UART"),
    ("NET:I2C?", "", "I2C SCAN"),
    ("NET:TELEMETRY?", "", "TELEMETRY"),
    ("NET:RGB:STATUS?", "", "TELEMETRY"),
    ("NET:TTS:STOP", "", "SYN6288 TTS"),
    ("NET:AUDIO:STOP", "", "SYN6288 TTS"),
    ("NET:TTS:nihao", "", "SYN6288 TTS"),
    ("NET:TTSHEX:E4BDA0E5A5BD", "", "SYN6288 TTS"),
    ("NET:VOLUME:5", "", "SYN6288 TTS"),
    ("NET:OLED:hello", "", "OLED"),
    ("NET:BEEP", "", "BEEP"),
    ("NET:MUSIC:SUCCESS", "", "MUSIC"),
    ("NET:FAN:ON:2", "", "FAN ON"),
    ("NET:FAN:OFF", "", "FAN OFF"),
    ("NET:LOCK:ON", "", "LOCK ON"),
    ("NET:LOCK:OFF", "", "LOCK OFF"),
    ("NET:AI:BUSY", "", "AI BUSY"),
    ("NET:AI:IDLE", "", "AI IDLE"),
    ("NET:AI:OFF", "", "AI OFF"),
    ("NET:SERVO:90", "", "SERVO"),
    ("NET:RFID:SCAN:ABC", "", "RFID"),
    # 未归类命令落入默认 ERROR（含可执行但无分类的 MOTOR/ULTRASONIC）
    ("NET:MOTOR:OFF", "", "ERROR"),
    ("NET:ULTRASONIC:ON", "", "ERROR"),
    ("NOT_A_COMMAND", "", "ERROR"),
    # action_id 含 RFID 关键字 -> RFID
    ("NET:BEEP", "rfid_scan_act", "RFID"),
]


def test_classify_corpus():
    module = load_knowledge_module()
    for command, action_id, expected in CLASSIFY_CORPUS:
        result = module.classify(command, action_id)
        assert _event_label(module, result) == expected, (command, action_id)


# (command, 期望 preview 显示串)
PREVIEW_CORPUS = [
    ("NET:UI:LISTEN", "UI:LISTEN"),
    ("NET:UI:USER:alice-001", "USER alice-001"),
    ("NET:UI:USER:1234567890abcdef", "USER 1234567890abcd"),
    ("NET:TTS:STOP", "TTS STOP"),
    ("NET:AUDIO:STOP", "TTS STOP"),
    ("NET:TTS:nihao", "TTS nihao"),
    ("NET:TTSHEX:E4BDA0E5A5BD", "TTSHEX 6B"),
    ("NET:VOLUME:5", "VOLUME 5"),
    ("NET:OLED:hello", "OLED hello"),
    ("NET:BEEP", "BEEP"),
    ("NET:MUSIC:SUCCESS", "MUSIC SUCCESS"),
    ("NET:FAN:ON:2", "FAN ON"),
    ("NET:FAN:OFF", "FAN OFF"),
    ("NET:LOCK:ON", "LOCK:ON"),
    ("NET:AI:BUSY", "AI:BUSY"),
    ("NET:SERVO:90", "SERVO 90"),
    ("NET:RGB:MODE:SENSOR", "RGB MODE:SENSOR"),
    ("NET:ULTRASONIC:ON", "ULTRASONIC ON"),
    ("NET:MOTOR:OFF", "MOTOR OFF"),
    ("NET:UART?", "UART PING"),
    ("NET:I2C?", "I2C SCAN"),
    ("NET:TELEMETRY?", "TELEMETRY"),
    ("NET:RFID:SCAN:ABC", "NET:RFID:SCAN:ABC"),
    ("NOT_A_COMMAND", "NOT_A_COMMAND"),
    ("THIS_IS_A_VERY_LONG_COMMAND_STRING_XYZ", "THIS_IS_A_VERY_LONG_"),
]


def test_preview_corpus():
    module = load_knowledge_module()
    for command, expected in PREVIEW_CORPUS:
        assert module.preview(command) == expected, command


def test_prefix_start_scanning():
    module = load_knowledge_module()
    # 单独命令：起始位置命中
    assert module.is_known_net_command_start("NET:BEEP", 0) is True
    assert module.is_known_net_command_start("NET:UART?", 0) is True
    assert module.is_known_net_command_start("NET:BEEP", 1) is False
    # 前导垃圾字节被丢弃
    assert module.find_known_net_command_start("junk>NET:BEEP", 0) == 5
    # 紧邻 ':' 的 NET 不视为起始（粘包拆行的冒号保护）
    assert module.is_known_net_command_start("act:NET:BEEP", 4) is False
    assert module.find_known_net_command_start("NET:CMD:a:NET:BEEP", 1) == -1
    # 拼接命令能定位第二条
    assert module.find_known_net_command_start("NET:BEEP" "NET:UI:ACK", 1) == 8
    # 空串/越界安全
    assert module.is_known_net_command_start("", 0) is False
    assert module.find_known_net_command_start("NET:BEEP", 99) == -1


def test_known_prefixes_match_command_preview_families():
    module = load_knowledge_module()
    for command, _ in PREVIEW_CORPUS:
        if command.startswith("NET:"):
            assert module.find_known_net_command_start(command, 0) == 0, command
