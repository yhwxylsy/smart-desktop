import importlib.util
import sys
from pathlib import Path


def load_protocol_module():
    path = Path(__file__).resolve().parents[2] / "firmware" / "stm32" / "protocol" / "protocol_reference.py"
    spec = importlib.util.spec_from_file_location("protocol_reference", path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def test_wrapped_stm32_command_parser_contract():
    protocol = load_protocol_module()

    parsed = protocol.parse_line("NET:CMD:act_123:NET:TTSHEX:E4BDA0E5A5BD")

    assert parsed.wrapped is True
    assert parsed.action_id == "act_123"
    assert parsed.command == "NET:TTSHEX:E4BDA0E5A5BD"
    assert protocol.ack_for("NET:CMD:act_123:NET:TTSHEX:E4BDA0E5A5BD") == "BT:ACK:act_123:OK"


def test_direct_debug_command_parser_contract():
    protocol = load_protocol_module()

    parsed = protocol.parse_line("NET:UART?")

    assert parsed.wrapped is False
    assert parsed.action_id is None
    assert parsed.command == "NET:UART?"
    assert protocol.ack_for("NET:UART?", ok=False) == "BT:ERR"


def test_wrapped_command_missing_inner_net_marker():
    protocol = load_protocol_module()

    # 仅 "NET:CMD:" 前缀、无 ":NET:" 分隔 —— 视为解析失败（command 置空）
    parsed = protocol.parse_line("NET:CMD:")
    assert parsed.wrapped is True
    assert parsed.action_id is None
    assert parsed.command == ""
    assert protocol.ack_for("NET:CMD:", ok=False) == "BT:ERR"

    parsed = protocol.parse_line("NET:CMD:act_9")
    assert parsed.wrapped is True
    assert parsed.action_id is None
    assert parsed.command == ""


def test_action_id_may_contain_colons():
    protocol = load_protocol_module()

    # action_id 内部可含冒号（":" 后不跟 "NET"），ack 原样带回 action_id
    parsed = protocol.parse_line("NET:CMD:act:user:42:NET:VOLUME:8")
    assert parsed.wrapped is True
    assert parsed.action_id == "act:user:42"
    assert parsed.command == "NET:VOLUME:8"
    assert protocol.ack_for("NET:CMD:act:user:42:NET:VOLUME:8") == "BT:ACK:act:user:42:OK"


def test_parser_empty_and_whitespace_edges():
    protocol = load_protocol_module()

    parsed = protocol.parse_line("")
    assert parsed.wrapped is False
    assert parsed.action_id is None
    assert parsed.command == ""

    # 首尾空白被 trim（parse_line 内 value.strip()）
    parsed = protocol.parse_line("  NET:UART?  ")
    assert parsed.wrapped is False
    assert parsed.command == "NET:UART?"

    # 空行/纯空白 ack 走非包装路径
    assert protocol.ack_for("   ", ok=False) == "BT:ERR"
    assert protocol.ack_for("", ok=True) == "BT:OK"


def test_ack_wrapped_only_when_action_id_present():
    protocol = load_protocol_module()

    # wrapped 但 action_id 为空 -> 非包装 ack
    parsed = protocol.parse_line("NET:CMD::NET:TTSHEX:E4BDA0E5A5BD")
    assert parsed.wrapped is True
    assert parsed.action_id == ""
    assert protocol.ack_for("NET:CMD::NET:TTSHEX:E4BDA0E5A5BD") == "BT:OK"

    # wrapped 且有 action_id -> 包装 ack，失败态 ERR
    assert (
        protocol.ack_for("NET:CMD:a1:NET:BEEP", ok=False)
        == "BT:ACK:a1:ERR"
    )
