# MsgPacketizer

[msgpack](https://github.com/msgpack/msgpack-c) based serializer / deserializer + packetize for Arduino and more


## Feature

- one-line serialize / deserialize or publish / subscribe + packetize + robust send / receive
- serializer / deserializer supports almost all standard type of C++ same as [msgpack-c](https://github.com/msgpack/msgpack-c)
- supports custom class serialization / deserialization
- serializer / deserializer based on [MsgPack](https://github.com/hideakitai/MsgPack)
- packetize based on [Packetizer](https://github.com/hideakitai/Packetizer)


## Packet Protocol

| index  | msgpack | crc8   |
| ------ | ------- | ------ |
| 1 byte | N bytes | 1 byte |


- 1 byte index (packet index can be used to identify packet)
- __N byte serialized msgpack data__
- 1 byte crc8 (for received data check)
- these bytes are encoded to COBS encoding based on [Packetizer](https://github.com/hideakitai/Packetizer)


## Usage

### Direct Data Receive + Data Publishing

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

    // publish varibales periodically (default 30[times/sec])
    MsgPacketizer::publish(Serial, send_index, i, f, s, v, m);
}

void loop()
{
    // must be called to trigger callback and publish data
    MsgPacketizer::update();
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
            // send received data back in one-line
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


### Nested Data with Custom Class

To serialize / deserialize nested data, defining custom class is recommended. For example, to make `{"k1": v, "k2":[i, f, s]}`:

``` C++
struct ArrayData
{
    int i; float f; MsgPack::str_t s;
    MSGPACK_DEFINE(i, f, s); // [i, f, s]
};
struct NestedData
{
    MsgPack::str_t k1, k2; int v;
    ArrayData a;
    MSGPACK_DEFINE_MAP(k1, v, k2, a); // {"k1": v, "k2":[i, f, s]}
};
```

and you can serialize / deserialize your class completely same as other types.

``` C++
NestedData n;
MsgPacketizer::publish(Serial, send_index, n);
MsgPacketizer::subscribe(Serial, recv_index, n);
```

Please see examples and [MsgPack](https://github.com/hideakitai/MsgPack) for more detail.


## APIs

``` C++
namespace MsgPacketizer
{
    // bind variables directly to specified index packet
    template <typename... Args>
    inline void subscribe(StreamType& stream, const uint8_t index, Args&... args);
    // bind variables directly to specified index packet with array format
    template <typename... Args>
    inline void subscribe_arr(StreamType& stream, const uint8_t index, Args&... args);
    // bind variables directly to specified index packet with map format
    template <typename... Args>
    inline void subscribe_map(StreamType& stream, const uint8_t index, Args&... args);
    // bind callback to specified index packet
    template <typename F>
    inline void subscribe(StreamType& stream, const uint8_t index, const F& callback);
    // bind callback which is always called regardless of index
    template <typename F>
    inline void subscribe(StreamType& stream, const F& callback);
    // must be called to receive packets
    inline void parse(bool b_exec_cb = true);
    // get UnpackerRef = std::shared_ptr<MsgPack::Unpacker> of stream and handle it manually
    inline UnpackerRef getUnpackerRef(const StreamType& stream);
    // get map o unpackers and handle it manually
    inline UnpackerMap& getUnpackerMap();

    // publish arguments periodically
    template <typename... Args>
    inline PublishElementRef publish(const StreamType& stream, const uint8_t index, Args&&... args);
    // publish arguments periodically as array format
    template <typename... Args>
    inline PublishElementRef publish_arr(const StreamType& stream, const uint8_t index, Args&&... args);
    // publish arguments periodically as map format
    template <typename... Args>
    inline PublishElementRef publish_map(const StreamType& stream, const uint8_t index, Args&&... args);
    // must be called to publish data
    inline void post();
    // send arguments dilectly with variable types
    template <typename... Args>
    inline void send(StreamType& stream, const uint8_t index, Args&&... args);
    // send binary data
    inline void send(StreamType& stream, const uint8_t index, const uint8_t* data, const uint8_t size);
    // send manually packed data
    inline void send(StreamType& stream, const uint8_t index);
    // get registerd publish element class
    inline PublishElementRef getPublishElementRef(const StreamType& stream, const uint8_t index);
    // send args as array format
    template <typename... Args>
    inline void send_arr(StreamType& stream, const uint8_t index, Args&&... args);
    // send args as map format
    template <typename... Args>
    inline void send_map(StreamType& stream, const uint8_t index, Args&&... args);
    // get MsgPack::Packer and handle it manually
    inline const MsgPack::Packer& getPacker();

    // call parse() and post()
    inline void update();
}
```

## Other Options

```C++
#define MSGPACKETIZER_ENABLE_DEBUG_LOG
```



## For NO-STL Boards

For following archtectures, several storage size for packets are limited.

- AVR
- megaAVR


### Memory Management (only for NO-STL Boards)

As mentioned above, for such boards like Arduino Uno, the storage sizes are limited.
And of course you can manage them by defining following macros.
But these default values are optimized for such boards, please be careful not to excess your boards storage/memory.
These macros have no effect for STL enabled boards.

#### MsgPacketizer

```C++
// max publishing elemtnt size in one destination
#define MSGPACKETIZER_MAX_PUBLISH_ELEMENT_SIZE 5
// max destinations to publish
#define MSGPACKETIZER_MAX_PUBLISH_DESTINATION_SIZE 1
```

#### MsgPack

``` C++
// msgpack serialized binary size
#define MSGPACK_MAX_PACKET_BYTE_SIZE 96
// max size of MsgPack::arr_t
#define MSGPACK_MAX_ARRAY_SIZE 3
// max size of MsgPack::map_t
#define MSGPACK_MAX_MAP_SIZE 3
// msgpack objects size in one packet
#define MSGPACK_MAX_OBJECT_SIZE 16
```

#### Packetizer

``` C++
// max number of decoded packet queues
#define PACKETIZER_MAX_PACKET_QUEUE_SIZE 1
// max data bytes in packet
#define PACKETIZER_MAX_PACKET_BINARY_SIZE 96
// max number of callback for one stream
#define PACKETIZER_MAX_CALLBACK_QUEUE_SIZE 3
// max number of streams
#define PACKETIZER_MAX_STREAM_MAP_SIZE 1
```


## Embedded Libraries

- [MsgPack v0.3.1](https://github.com/hideakitai/MsgPack)
- [Packetizer v0.5.3](https://github.com/hideakitai/Packetizer)


## License

MIT
