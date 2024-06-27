import logging
from typing import Any

import msgpack

from ...packetizerpy import packetizer
from .common import to_hex


def encode(index: int, msgs: Any) -> bytes:
    logging.debug(f"encode input: index = {index}, msgs = {msgs}")
    packed = msgpack.packb(msgs, use_bin_type=True)
    logging.debug(f"packed: {to_hex(packed)}")
    encoded = packetizer.encode(index, packed)
    logging.debug(f"encoded: {to_hex(encoded)}")
    return encoded
