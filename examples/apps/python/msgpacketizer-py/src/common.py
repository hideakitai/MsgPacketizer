import time

SEND_INDEX_SIMPLE = 0x01
RECV_INDEX_SIMPLE = 0x02
SEND_INDEX_ARR = 0x11
RECV_INDEX_ARR = 0x12
SEND_INDEX_MAP = 0x21
RECV_INDEX_MAP = 0x22
SEND_INDEX_CUSTOM = 0x31
RECV_INDEX_CUSTOM = 0x32

_TIME_START = time.perf_counter()


def generate_current_data():
    seconds: float = time.perf_counter() - _TIME_START
    millis: float = seconds * 1000.0
    micros: int = int(millis * 1000.0)

    simple = [micros, millis, f"{seconds}[sec]"]
    simple_arr = [micros, int(millis), int(seconds)]
    simple_map = {
        "micros": micros,
        "millis": int(millis),
        "seconds": int(seconds),
    }
    custom = {
        "ka": simple,
        "km": simple_map,
    }

    return (simple, simple_arr, simple_map, custom)
