# MsgPacketizer Example App (Node)

## Preparation

```shell
npm insall
```

## Usage

### Check / Build / Watch & Rebuild

```shell
npm run check
npm run build
npm run watch
```

### Build and Run Once

```shell
npm run serial /your/serialport
npm run udp
npm run tcp
```

### Hot Reload (`tsx`)

```shell
npm run dev-serial /your/serialport
npm run dev-udp
npm run dev-tcp
```

## Options

### `serial`

```shell
❯ npm run serial -- -h

> msgpacketizer-ts@1.0.0 serial
> tsc && node dist/serial.js -h

Usage: serial [options] <port>

Arguments:
  port                     serial port

Options:
  -b, --baudrate <number>  baudrate (default: "115200")
  -v, --verbose            verbose mode (default: false)
  -h, --help               display help for command
```

### `udp`

```shell
❯ npm run udp -- -h

> msgpacketizer-ts@1.0.0 udp
> tsc && node dist/udp.js -h

Usage: udp [options]

Options:
  --remote-ip <ip>      remote client ip (default: "192.168.0.201")
  --remote-port <port>  remote client port (default: "55555")
  --local-port <port>   local port to listen (default: "54321")
  -v, --verbose         verbose mode (default: false)
  -h, --help            display help for command
```

### `tcp`

```shell
❯ npm run tcp -- -h

> msgpacketizer-ts@1.0.0 tcp
> tsc && node dist/tcp.js -h

Usage: tcp [options]

Options:
  --remote-ip <ip>      remote client ip (default: "192.168.0.201")
  --remote-port <port>  remote client port (default: "55555")
  -v, --verbose         verbose mode (default: false)
  -h, --help            display help for command
```
