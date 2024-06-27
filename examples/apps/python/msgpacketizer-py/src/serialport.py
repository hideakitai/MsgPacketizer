import argparse
import logging
import threading
import time
import traceback

import common
import serial

from src.msgpacketizerpy import msgpacketizer

# parse command line arguments
argparser = argparse.ArgumentParser()
argparser.add_argument("port", help="serial port")
argparser.add_argument("-b", "--baudrate", default=115200, help="baudrate")
argparser.add_argument("-v", "--verbose", action="store_true", help="verbose mode")
args = argparser.parse_args()

# create logger based on verbose mode
format = "%(levelname)s: [%(filename)s %(lineno)d] %(message)s"
if args.verbose:
    logging.basicConfig(format=format, level=logging.DEBUG)
else:
    logging.basicConfig(format=format, level=logging.INFO)

# create and open serial port
arduino = serial.Serial(args.port, args.baudrate)

# create msgpacketizer.subscriber
subscriber = msgpacketizer.Subscriber()

# subscribe messages
subscriber.subscribe(
    common.RECV_INDEX_SIMPLE,
    lambda msg: print(f"[simple] index = {hex(msg.index)}, msg = {msg.msg}"),
)
subscriber.subscribe(
    common.RECV_INDEX_ARR,
    lambda msg: print(f"[simple_arr] index = {hex(msg.index)}, msg = {msg.msg}"),
)
subscriber.subscribe(
    common.RECV_INDEX_MAP,
    lambda msg: print(f"[simple_map] index = {hex(msg.index)}, msg = {msg.msg}"),
)
subscriber.subscribe(
    common.RECV_INDEX_CUSTOM,
    lambda msg: print(f"[custom] index = {hex(msg.index)}, msg = {msg.msg}"),
)


# read
def read_serial(stop_event: threading.Event):
    try:
        while not stop_event.is_set():
            # read data from serial port
            num_bytes = arduino.in_waiting
            if num_bytes > 0:
                logging.debug(f"bytes in_waiting: {num_bytes}")
                subscriber.feed(arduino.read(num_bytes))

    except Exception:
        traceback.print_exc()


stop_event = threading.Event()
thread = threading.Thread(target=read_serial, args=(stop_event,))
thread.start()


# write
try:
    while True:
        # send data to serial port periodically
        (simple, simple_arr, simple_map, custom) = common.generate_current_data()
        arduino.write(
            msgpacketizer.encode(
                common.SEND_INDEX_SIMPLE,
                simple,
            )
        )
        arduino.write(
            msgpacketizer.encode(
                common.SEND_INDEX_ARR,
                simple_arr,
            )
        )
        arduino.write(
            msgpacketizer.encode(
                common.SEND_INDEX_MAP,
                simple_map,
            )
        )
        arduino.write(
            msgpacketizer.encode(
                common.SEND_INDEX_CUSTOM,
                custom,
            )
        )

        time.sleep(0.1)

except KeyboardInterrupt:
    print("KeyboardInterrupt")

except Exception:
    traceback.print_exc()

finally:
    stop_event.set()
    thread.join()
    arduino.close()
