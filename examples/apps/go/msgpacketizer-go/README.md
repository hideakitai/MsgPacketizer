### prepare

```shell
make go-mod-tidy
```

### run

```shell
make run-serialport /dev/ttyUSB0 [-v] [-b 115200]
make run-network [-tcp] [-v] [-remote-ip 192.168.0.201] [-remote-port 55555] [-local-port 54321]
```

### help

```shell
make help
make help-serial
make help-network
```
