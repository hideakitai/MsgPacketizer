# MsgPacketizer Example App (Python)

## serialport

```shell
make run_serialport
```

```shell
usage: serialport.py [-h] [-b BAUDRATE] [-v] port

positional arguments:
  port                  serial port

options:
  -h, --help            show this help message and exit
  -b BAUDRATE, --baudrate BAUDRATE
                        baudrate
  -v, --verbose         verbose mode
```

## network (UDP/TCP)

```shell
make run_network
```

```shell
usage: network.py [-h] [--remote-ip REMOTE_IP] [--remote-port REMOTE_PORT] [--local-port LOCAL_PORT] [--tcp] [-v]

options:
  -h, --help            show this help message and exit
  --remote-ip REMOTE_IP
                        remote client ip
  --remote-port REMOTE_PORT
                        remote client port
  --local-port LOCAL_PORT
                        local port to listen
  --tcp                 use tcp instead of udp
  -v, --verbose         verbose mode
```

## For `rye` User

```shell
rye run serialport /your/serialport
```

```shell
rye run network
```

## Export `requirements.txt`

```shell
make export
```
