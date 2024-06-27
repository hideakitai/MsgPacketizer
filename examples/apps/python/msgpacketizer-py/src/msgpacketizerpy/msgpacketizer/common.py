from dataclasses import dataclass
from typing import Any, List


@dataclass(frozen=True)
class Message:
    index: int
    msg: List[Any]


def to_hex(data: bytes) -> str:
    return " ".join([f"0x{b:02x}" for b in data])
