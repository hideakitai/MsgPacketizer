import logging

import crc8
from cobs import cobs

from .common import COBS_DELIMITER, Message, to_hex


def decode(chunk: bytes) -> Message | None:
    logging.debug(f"cobs decode input: {to_hex(chunk)}")

    # decode cobs (NOTE: 0x00 on last byte is not required)
    decoded = cobs.decode(chunk.rstrip(COBS_DELIMITER))

    if len(decoded) < 3:
        logging.warn(f"decoded length is too short: {decoded}")
        return None

    # devide into idnex, data, crc
    index = decoded[0]
    data = decoded[1:-1]
    crc = decoded[-1].to_bytes(1, byteorder="big")

    # calcurate crc8
    hash = crc8.crc8()
    hash.update(data)
    digest = hash.digest()

    logging.debug(
        f"cobs decoded to index: {index}, data: {to_hex(data)}, crc: {crc}, crc calc: {digest}"
    )

    # check crc8
    if digest == crc:
        return Message(index, data)
    else:
        logging.warn(f"crc mismatch (recv: {crc}, calc: {digest})")
        return None
