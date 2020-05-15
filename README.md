# MsgPacketizer

[msgpack](https://github.com/msgpack/msgpack-c) based serializer / deserializer + packetize for Arduino and more


## Feature

- one-line serialize / deserialize + packetize + robust send / receive
- serializer / deserializer supports almost all standard type of C++ same as [msgpack-c](https://github.com/msgpack/msgpack-c)
- supports custom class serialization / deserialization
- serializer / deserializer based on [MsgPack](https://github.com/hideakitai/MsgPack)
- packetize based on [Packetizer](https://github.com/hideakitai/Packetizer)


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
MsgPack::str_t s; // std::string or String
MsgPack::arr_t<int> v; // std::vector or arx::vector
MsgPack::map_t<String, float> m; // std::map or arx::map

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
        [](int i, float f, MsgPack::str_t s, MsgPack::arr_t<int> v, MsgPack::map_t<String, float> m)
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

### Custom Class Adaptation

To serialize / deserialize custom type you defined, please use `MSGPACK_DEFINE()` macro inside of your class.

``` C++
struct CustomClass
{
    int i;
    float f;
    MsgPack::str_t s;

    MSGPACK_DEFINE(i, f, s);
};
```

After that, you can serialize / deserialize your class completely same as other types.

``` C++
int i;
float f;
CustomClass c;
MsgPacketizer::send(Serial, send_index, i, f, c); // -> send(i, f, c.i, c.f, c.s)
MsgPacketizer::subscribe(Serial, recv_index, i, f, c); // this is also ok
```

Please see [MsgPack](https://github.com/hideakitai/MsgPack) for more detail.

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

    // get UnpackerRef = std::shared_ptr<MsgPack::Unpacker> of stream and handle it manually
    UnpackerRef getUnpackerRef(const StreamType& stream);

    // get map o unpackers and handle it manually
    UnpackerMap& getUnpackerMap();


    // send arguments dilectly with variable types
    template <typename... Args>
    inline void send(StreamType& stream, const uint8_t index, Args&&... args);

    // send binary data
    inline void send(StreamType& stream, const uint8_t index, const uint8_t* data, const uint8_t size);

    // send manually packed data
    inline void send(StreamType& stream, const uint8_t index);

    // get MsgPack::Packer and handle it manually
    const MsgPack::Packer& getPacker();

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
#define MSGPACK_MAX_OBJECT_SIZE        24
```

#### Packetizer

``` C++
// max number of decoded packet queues
#define PACKETIZER_MAX_PACKET_QUEUE_SIZE     1
// max data bytes in packet
#define PACKETIZER_MAX_PACKET_BINARY_SIZE  128
// max number of callback for one stream
#define PACKETIZER_MAX_CALLBACK_QUEUE_SIZE   4
// max number of streams
#define PACKETIZER_MAX_STREAM_MAP_SIZE       2
```


## Embedded Libraries

- [MsgPack v0.1.7](https://github.com/hideakitai/MsgPack)
- [Packetizer v0.3.6](https://github.com/hideakitai/Packetizer)


## License

MIT