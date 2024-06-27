import logging

import msgpack

from ...packetizerpy import packetizer
from .common import Message, to_hex


def decode(data: bytes) -> Message | None:
    logging.debug(f"decode input: {to_hex(data)}")
    decoded = packetizer.decode(data)
    if decoded is None:
        return None
    else:
        logging.debug(f"decoded: {to_hex(decoded.data)}")
        msg = msgpack.unpackb(decoded.data, use_list=True, raw=False)  # type: ignore
        logging.debug(f"unpacked: {msg}")
        return Message(decoded.index, msg)
