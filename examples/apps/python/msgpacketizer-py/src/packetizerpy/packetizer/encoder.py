import logging

import crc8
from cobs import cobs

from .common import COBS_DELIMITER, to_hex


def encode(index: int, data: bytes) -> bytes:
    logging.debug(f"encode input: {index}, {data}")
    hash = crc8.crc8()
    hash.update(data)
    digest = hash.digest()
    raw_data = bytes([index]) + data + digest
    encoded = cobs.encode(raw_data) + COBS_DELIMITER
    logging.debug(f"encoded: {to_hex(encoded)}")
    return encoded
