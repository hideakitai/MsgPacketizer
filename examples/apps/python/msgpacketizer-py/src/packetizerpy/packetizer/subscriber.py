import logging
from typing import Callable, Dict

from .common import COBS_DELIMITER, Message, to_hex
from .decoder import decode


class Subscriber:
    def __init__(self):
        self._buffer = bytearray()
        self._callbacks: Dict[int, Callable[[Message], None]] = {}

    def feed(self, buffer: bytes):
        logging.debug(f"feed input: {to_hex(buffer)}")
        self._buffer += buffer
        msgs: list[Message] = []

        # find delimiter (0x00)
        while self._buffer.find(COBS_DELIMITER) != -1:
            # if delimiter found, split and process buffer one by one
            [chunk, self._buffer] = self._buffer.split(COBS_DELIMITER, maxsplit=1)
            logging.debug(f"buffer splited to chunk: {chunk}, rest: {self._buffer}")

            # decode packet
            decoded = decode(bytes(chunk))
            if decoded is not None:
                msgs.append(decoded)

        # iterate over received msgs and call callbacks
        for msg in msgs:
            logging.debug(f"received msg = index: {msg.index}, msg: {msg.data}")
            # do some stuff in user-defined callback function
            if msg.index in self._callbacks:
                self._callbacks[msg.index](msg)
            else:
                logging.warn(f"callback for index {msg.index} not found")

    def subscribe(self, index: int, callback: Callable[[Message], None]):
        self._callbacks[index] = callback

    def unsubscribe(self, index: int):
        del self._callbacks[index]
