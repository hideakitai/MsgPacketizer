# MsgPacketizer Examples

## Description

### Desktop Apps

- sends **all of the packets defined below** periodically
- prints **all of the received msgpack defined below** If they receive the packets

### Arduino

- **update variables based on received msgpack from app** If they receive the packets
- **sends updated variables back to app** periodically (loop back)

As a result, your app prints the data as same as it sent before. The device receives, updates, and sends back the data from your app.

## MsgPack/JSON Definitions

### simple (App → Arduino : `0x01`, Arduino → App: `0x02`)

```json
[int, float, str] // micros, millis, seconds[sec]
```

### arr (App → Arduino : `0x11`, Arduino → App: `0x12`)

```json
[int, int, int] // micros, millis (rounded), seconds (rounded)
```

### map (App → Arduino : `0x21`, Arduino → App: `0x22`)

```json
{"micros": int, "millis": int, "seconds": int}
```

### custom/custom_arduinojson (App → Arduino : `0x31`, Arduino → App: `0x32`)

```json
{
  "ka": [int, float, str], // micros, millis, seconds[sec]
  "km": {"micros": int, "millis": int, "seconds": int}
}
```
