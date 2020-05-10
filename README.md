# MsgPacketizer

[msgpack](https://github.com/msgpack/msgpack-c) based serializer / deserializer + packetize for Arduino and more


## Feature

- one-line serialize / deserialize + packetize + send / receive
- serializer / deserializer based on [MsgPack v0.1.3](https://github.com/hideakitai/MsgPack)
- packetize based on [Packetizer v0.3.3](https://github.com/hideakitai/Packetizer)


## Packet Protocol


| header | index  | msgpack | crc8   | footer |
|--------|--------|---------|--------|--------|
| 1 byte | 1 byte | N bytes | 1 byte | 1 byte |


- 1 byte header
- 1 byte index (packet index can be used to identify packet)
- __N byte serialized msgpack data__
- 1 byte crc8 (for received data check)
- 1 byte footer


## Usage

### Direct Data Receive + One-Line Send

``` C++
#include <MsgPacketizer.h>

// input to msgpack
int i;
float f;
String s;
std::vector<int> v;
std::map<String, float> m;

uint8_t recv_index = 0x12;
uint8_t send_index = 0x34;

void setup()
{
    Serial.begin(115200);

    // update received data directly
    MsgPacketizer::subscribe(Serial, recv_index, i, f, s, v, m);
}

void loop()
{
    // must be called to receive
    MsgPacketizer::parse();

    // send received data back
    MsgPacketizer::send(Serial, send_index, i, f, s, v, m);
}

```


### Callback with Received Objects + One-Line Send

``` C++
#include <MsgPacketizer.h>

uint8_t recv_index = 0x12;
uint8_t send_index = 0x34;

void setup()
{
    Serial.begin(115200);

    // handle received data with lambda
    // which has incoming argument types/data

    MsgPacketizer::subscribe(Serial, recv_index,
        [](int i, float f, String s, std::vector<int> v, std::map<String, float> m)
        {
            // send received data back
            MsgPacketizer::send(Serial, send_index, i, f, s, v, m);
        }
    );
}

void loop()
{
    // must be called to trigger callback
    MsgPacketizer::parse();
}

```

## APIs

``` C++
namespace MsgPacketizer
{
    // bind variables directly to specified index packet
    template <typename... Args>
    inline void subscribe(StreamType& stream, const uint8_t index, Args&... args);

    // bind callback to specified index packet
    template <typename F>
    inline void subscribe(StreamType& stream, const uint8_t index, const F& callback);

    // bind callback which is always called regardless of index
    template <typename F>
    inline void subscribe(StreamType& stream, const F& callback);

    // send arguments dilectly with variable types
    template <typename... Args>
    inline void send(StreamType& stream, const uint8_t index, Args&&... args);

    // send binary data
    inline void send(StreamType& stream, const uint8_t index, const uint8_t* data, const uint8_t size);

    // must be called to receive packets
    inline void parse(bool b_exec_cb = true);
}
```


## For NO-STL Boards

For following archtectures, several storage size for packets are limited.

- AVR
- megaAVR
- SAMD
- SPRESENSE


### API Limitation

There is limitation to `subscribe` packet for such boards.
Only direct variable binding can be used.

``` C++
namespace MsgPacketizer
{
    // bind variables directly to specified index packet
    template <typename... Args>
    inline void subscribe(StreamType& stream, const uint8_t index, Args&... args);

    // send arguments dilectly with variable types
    template <typename... Args>
    inline void send(StreamType& stream, const uint8_t index, Args&&... args);

    // send binary data
    inline void send(StreamType& stream, const uint8_t index, const uint8_t* data, const uint8_t size);

    // must be called to receive packets
    inline void parse(bool b_exec_cb = true);
}
```

If you want to add callback as with STL enabled boards, please follow this way.

``` C++
// handle received data depeneding on index
Packetizer::subscribe(Serial, recv_index, [&](const uint8_t* data, const uint8_t size)
{
    // unpack msgpack objects
    MsgPack::Unpacker unpacker;
    unpacker.feed(data, size);
    unpacker.decode(i, f, s, v, m);

    // send received data back
    MsgPacketizer::send(Serial, send_back_index, i, f, s, v, m);
});
```


### Memory Management (only for NO-STL Boards)

As mentioned above, for such boards like Arduino Uno, the storage sizes are limited.
And of course you can manage them by defining following macros.
But these default values are optimized for such boards, please be careful not to excess your boards storage/memory.
These macros have no effect for STL enabled boards.


#### MsgPack

``` C++
// msgpack serialized binary size
#define MSGPACK_MAX_PACKET_BYTE_SIZE  128
// max size of MsgPack::arr_t
#define MSGPACK_MAX_ARRAY_SIZE          8
// max size of MsgPack::map_t
#define MSGPACK_MAX_MAP_SIZE            8
// msgpack objects size in one packet
#define MSGPACK_MAX_OBJECT_SIZE        32
```

#### Packetizer

``` C++
// max number of decoded packet queues
#define PACKETIZER_MAX_PACKET_QUEUE_SIZE     2
// max data bytes in packet
#define PACKETIZER_MAX_PACKET_BINARY_SIZE  128
// max number of callback for one stream
#define PACKETIZER_MAX_CALLBACK_QUEUE_SIZE   8
// max number of streams
#define PACKETIZER_MAX_STREAM_MAP_SIZE       2
```


## Embedded Libraries

- [MsgPack v0.1.3](https://github.com/hideakitai/MsgPack)
- [Packetizer v0.3.3](https://github.com/hideakitai/Packetizer)


## License

MIT