import argparse
import logging
import socket
import threading
import time
import traceback

import common

# from msgpacketizer import msgpacketizer
from src.msgpacketizerpy import msgpacketizer

# args
argparser = argparse.ArgumentParser()
argparser.add_argument("--remote-ip", default="192.168.0.201", help="remote server ip")
argparser.add_argument("--remote-port", default=55555, help="remote server port")
argparser.add_argument("--local-port", default=54321, help="local port to listen")
argparser.add_argument("--tcp", action="store_true", help="use tcp instead of udp")
argparser.add_argument("-v", "--verbose", action="store_true", help="verbose mode")
args = argparser.parse_args()

# logging
format = "%(levelname)s: [%(filename)s %(lineno)d] %(message)s"
if args.verbose:
    logging.basicConfig(format=format, level=logging.DEBUG)
else:
    logging.basicConfig(format=format, level=logging.INFO)

# udp/tcp
BUFFER_SIZE = 1024
LOCAL_IP = socket.gethostbyname(socket.gethostname())
LOCAL_ADDRESS = (LOCAL_IP, args.local_port)
REMOTE_ADDRESS = (args.remote_ip, args.remote_port)
if args.tcp:
    sock = socket.socket(socket.AF_INET, type=socket.SOCK_STREAM)
    sock.connect(REMOTE_ADDRESS)
else:
    sock = socket.socket(socket.AF_INET, type=socket.SOCK_DGRAM)
    sock.bind(LOCAL_ADDRESS)

# msgpacketizer
subscriber = msgpacketizer.Subscriber()
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
def read_network(stop_event: threading.Event):
    try:
        while not stop_event.is_set():
            (buffer, addr) = sock.recvfrom(BUFFER_SIZE)
            if len(buffer) > 0:
                logging.debug(f"received {len(buffer)} bytes from {addr}")
                subscriber.feed(buffer)

    except Exception:
        traceback.print_exc()


stop_event = threading.Event()
thread = threading.Thread(target=read_network, args=(stop_event,))
thread.daemon = True
thread.start()


try:
    while True:
        # send
        (simple, simple_arr, simple_map, custom) = common.generate_current_data()
        msg_simple = msgpacketizer.encode(common.SEND_INDEX_SIMPLE, simple)
        msg_simple_arr = msgpacketizer.encode(common.SEND_INDEX_ARR, simple_arr)
        msg_simple_map = msgpacketizer.encode(common.SEND_INDEX_MAP, simple_map)
        msg_custom = msgpacketizer.encode(common.SEND_INDEX_CUSTOM, custom)

        if args.tcp:
            sock.send(msg_simple)
            sock.send(msg_simple_arr)
            sock.send(msg_simple_map)
            sock.send(msg_custom)
        else:
            sock.sendto(msg_simple, REMOTE_ADDRESS)
            sock.sendto(msg_simple_arr, REMOTE_ADDRESS)
            sock.sendto(msg_simple_map, REMOTE_ADDRESS)
            sock.sendto(msg_custom, REMOTE_ADDRESS)

        time.sleep(0.1)

except KeyboardInterrupt:
    print("KeyboardInterrupt")

except Exception:
    traceback.print_exc()

finally:
    stop_event.set()
    sock.close()
