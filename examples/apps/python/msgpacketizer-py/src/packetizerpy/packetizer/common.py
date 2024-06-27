from dataclasses import dataclass

COBS_DELIMITER = b"\x00"


@dataclass(frozen=True)
class Message:
    index: int
    data: bytes


def to_hex(data: bytes) -> str:
    return " ".join([f"0x{b:02x}" for b in data])
