import logging
from typing import Callable, Dict

import msgpack

from ...packetizerpy import packetizer
from .common import Message, to_hex


class Subscriber:
    def __init__(self):
        self._packetizer = packetizer.Subscriber()
        self._callbacks: Dict[int, Callable[[Message], None]] = {}

    def feed(self, buffer: bytes):
        self._packetizer.feed(buffer)

    def subscribe(self, index: int, callback: Callable[[Message], None]):
        self._callbacks[index] = callback
        self._packetizer.subscribe(index, self._callback)

    def unsubscribe(self, index: int):
        del self._callbacks[index]
        self._packetizer.unsubscribe(index)

    def _callback(self, m: packetizer.Message):
        logging.debug(f"callback input index: {m.index}, data: {to_hex(m.data)}")
        unpacked = msgpack.unpackb(m.data, use_list=True, raw=False)  # type: ignore
        logging.debug(f"unpacked: {unpacked}")
        msg = Message(m.index, unpacked)

        if msg.index in self._callbacks:
            self._callbacks[msg.index](msg)
        else:
            logging.warn(f"callback not found for index {msg.index}")
