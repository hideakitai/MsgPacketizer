# MsgPacketizer

[msgpack](https://github.com/msgpack/msgpack-c) based serializer / deserializer + packetize for Arduino, ROS, and more

## Feature

- one-line [serialize/deserialize] or [publish/subscribe] + packetize + robust [send/receive]
- [serializer/deserializer] supports almost all standard type of C++ same as [msgpack-c](https://github.com/msgpack/msgpack-c)
- support custom class [serialization/deserialization]
- support one-line manual [serialization/deserialization] to work with any communication interface
- support working with [ArduinoJSON](https://github.com/bblanchon/ArduinoJson)
- [serializer/deserializer] based on [MsgPack](https://github.com/hideakitai/MsgPack)
- packetize based on [Packetizer](https://github.com/hideakitai/Packetizer)
- working also in ROS with [serial](https://github.com/wjwwood/serial) and [serial-ros2](https://github.com/RoverRobotics-forks/serial-ros2)

## Packet Protocol

| index  | msgpack | crc8   |
| ------ | ------- | ------ |
| 1 byte | N bytes | 1 byte |

- 1 byte index (packet index can be used to identify packet)
- **N byte serialized msgpack data**
- 1 byte crc8 (for received data check)
- these bytes are encoded to COBS encoding based on [Packetizer](https://github.com/hideakitai/Packetizer)

## Usage

### Direct Data Receive + Data Publishing

```C++
#include <MsgPacketizer.h>

// input to msgpack
int i;
float f;
MsgPack::str_t s; // std::string or String
MsgPack::arr_t<int> v; // std::vector or arx::vector
MsgPack::map_t<String, float> m; // std::map or arx::map

uint8_t recv_index = 0x12;
uint8_t send_index = 0x34;

void setup() {
    Serial.begin(115200);

    // update received data directly
    MsgPacketizer::subscribe(Serial, recv_index, i, f, s, v, m);

    // publish varibales periodically (default 30[times/sec])
    MsgPacketizer::publish(Serial, send_index, i, f, s, v, m);
}

void loop() {
    // must be called to trigger callback and publish data
    MsgPacketizer::update();
}

```

### Callback with Received Objects + One-Line Send

```C++
#include <MsgPacketizer.h>

uint8_t recv_index = 0x12;
uint8_t send_index = 0x34;

void setup() {
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

void loop() {
    // must be called to trigger callback
    MsgPacketizer::parse();
}

```

### Nested Data with Custom Class

To serialize / deserialize nested data, defining custom class is recommended. For example, to make `{"k1": v, "k2":[i, f, s]}`:

```C++
struct ArrayData {
    int i; float f; MsgPack::str_t s;
    MSGPACK_DEFINE(i, f, s); // [i, f, s]
};
struct NestedData {
    MsgPack::str_t k1, k2; int v;
    ArrayData a;
    MSGPACK_DEFINE_MAP(k1, v, k2, a); // {"k1": v, "k2":[i, f, s]}
};
```

and you can serialize / deserialize your class completely same as other types.

```C++
NestedData n;
MsgPacketizer::publish(Serial, send_index, n);
MsgPacketizer::subscribe(Serial, recv_index, n);
```

Please see examples and [MsgPack](https://github.com/hideakitai/MsgPack) for more detail.

### Manual Encode / Decode with Any Communication I/F

You can just encode / decode data manually to use it with any communication interface.
Please note:

- only one unsupoprted interface (serial, udp, tcp, etc.) is available for manual subscription because MsgPacketizer cannot indetify which data is from which device
- `publish` is not available for unsupported data stream

```C++
#include <MsgPacketizer.h>

const uint8_t recv_index = 0x12;
const uint8_t send_index = 0x34;

void setup() {
    Serial.begin(115200);
    delay(2000);

    // subscribe the data for manual operation
    MsgPacketizer::subscribe(recv_index,
        [&](const int i, const float f, const String& s) {
            // just encode your data manually and get binary packet from MsgPacketizer
            const auto& packet = MsgPacketizer::encode(send_index, i, f, s);
            // send the packet data with your interface
            Serial.write(packet.data.data(), packet.data.size());
        }
    );
}

void loop() {
    // you should feed the received data manually to MsgPacketizer
    const size_t size = Serial.available();
    if (size) {
        uint8_t* data = new uint8_t[size];
        // you can get binary data from any communication interface
        Serial.readBytes(data, size);
        // feed your binary data to MsgPacketizer manually
        // if data has successfully received and decoded, callback will be called
        MsgPacketizer::feed(data, size);
        delete[] data;
    }
}
```

### UDP and TCP Support

#### TCP Support

- start client first
- everything else can be used in the same way

```C++
#include <MsgPacketizer.h>
#include <WiFi.h>

const uint8_t index = 0x12;
int i; float f; String s;

// WiFi stuff
WiFiClient client;
const char* host = "192.168.0.10";
const int port = 54321;

void setup() {
    WiFi.begin("your-ssid", "your-password");

    // start client
    client.connect(host, port);

    // everything else can be used in the same way
    MsgPacketizer::publish(client, index, i, f, s)->setFrameRate(1);
    MsgPacketizer::subscribe(client, index,
        [&](const int i, const float f, const String& s) {
            // do something with received data
        }
    );
}

void loop() {
    // do something with your variables i, f, s

    // must be called to trigger callback and publish data
    MsgPacketizer::update();
}
```

#### UDP Support

- start client first
- set ip and port when you publish or send messages
- everything else can be used in the same way

```C++
#include <MsgPacketizer.h>
#include <WiFi.h>

const uint8_t index = 0x12;
int i; float f; String s;

// WiFi stuff
WiFiUDP client;
const char* host = "192.168.0.10";
const int port = 54321;

void setup() {
    WiFi.begin("your-ssid", "your-password");

    // start client first
    client.begin(port);

    // set host and port when publishing or sending messages
    MsgPacketizer::publish(client, host, port, index, i, f, s)->setFrameRate(1);
    MsgPacketizer::subscribe(client, index,
        [&](const int i, const float f, const String& s) {
            // do something with received data
        }
    );
}

void loop() {
    // do something with your variables i, f, s

    // must be called to trigger callback and publish data
    MsgPacketizer::update();
}
```

### ArduinoJson Support

- supports only version > 6.x
- supports `StaticJsonDocument<N>` and `DynamicJsonDocument`
- supports only `send`, `decode`, and `subscribe` with callbacks
  - you can use `publish` and `subscribe` directly by reference, but not reccomended
  - please see [this official document](https://arduinojson.org/v6/how-to/reuse-a-json-document/) for the detail

```C++
#include <ArduinoJson.h>  // include before MsgPacketizer.h
#include <MsgPacketizer.h>
#include <WiFi.h>

const uint8_t msg_index = 0x12;
const char* host = "192.168.0.10";
const int port = 54321;
WiFiUDP client;

void setup() {
    WiFi.begin("your-ssid", "your-password");
    client.begin(port);

    MsgPacketizer::subscribe(client, msg_index,
        [&](const StaticJsonDocument<200>& doc) {
            // do something with your json
        }
    );
}

void loop() {
    StaticJsonDocument<200> doc;
    // make your json here
    MsgPacketizer::send(client, host, port, msg_index, doc);

    delay(1000);

    // must be called to trigger callback and publish data
    MsgPacketizer::update();
}
```

#### Buffer Size Adjustment when Subscribing `DynamicJsonDocument`

Currently we cannot calculate the json size from msgpack format before deserialization. Please adjust buffer size by defining following macro before including `MsgPacketizer.h`. Default is `size_of_msgpack_bytes * 3`.

```C++
#define MSGPACKETIZER_ARDUINOJSON_DESERIALIZE_BUFFER_SCALE 3  // default
#include <MsgPacketizer.h>
```

## APIs

### Subscriber

```C++
namespace MsgPacketizer {

    // ----- for unsupported communication interface with manual operation -----

    // bind variables directly to specified index packet
    template <typename... Args>
    inline void subscribe_manual(const uint8_t index, Args&&... args);
    // bind variables directly to specified index packet with array format
    template <typename... Args>
    inline void subscribe_manual_arr(const uint8_t index, Args&&... args);
    // bind variables directly to specified index packet with map format
    template <typename... Args>
    inline void subscribe_manual_map(const uint8_t index, Args&&... args);
    // bind callback to specified index packet
    template <typename F>
    inline void subscribe_manual(const uint8_t index, F&& callback);
    // bind callback which is always called regardless of index
    template <typename F>
    inline void subscribe_manual(F&& callback);
    // unsubscribe
    inline void unsubscribe_manual(const uint8_t index);

    // feed packet manually: must be called to manual decoding
    inline void feed(const uint8_t* data, const size_t size);


    // ----- for supported communication interface (Arduino, oF, ROS) -----

    template <typename S, typename... Args>
    inline void subscribe(S& stream, const uint8_t index, Args&&... args);
    template <typename S, typename... Args>
    inline void subscribe_arr(S& stream, const uint8_t index, Args&&... args);
    template <typename S, typename... Args>
    inline void subscribe_map(S& stream, const uint8_t index, Args&&... args);
    template <typename S, typename F>
    inline void subscribe(S& stream, const uint8_t index, F&& callback);
    template <typename S, typename F>
    inline void subscribe(S& stream, F&& callback);
    template <typename S>
    inline void unsubscribe(const S& stream, const uint8_t index);
    template <typename S>
    inline void unsubscribe(const S& stream);
    template <typename S>

    // get UnpackerRef = std::shared_ptr<MsgPack::Unpacker> of stream and handle it manually
    inline UnpackerRef getUnpackerRef(const S& stream);
    // get map of unpackers and handle it manually
    inline UnpackerMap& getUnpackerMap();

    // must be called to receive packets
    inline void parse(bool b_exec_cb = true);
    inline void update(bool b_exec_cb = true);
}
```

### Publisher

```C++
namespace MsgPacketizer {

    // ----- for unsupported communication interface with manual operation -----

    // encode arguments directly with variable types
    template <typename... Args>
    inline const Packetizer::Packet& encode(const uint8_t index, Args&&... args);
    // encode binary data
    inline const Packetizer::Packet& encode(const uint8_t index, const uint8_t* data, const uint8_t size);
    // encode manually packed data
    inline const Packetizer::Packet& encode(const uint8_t index);
    // encode args as array format
    template <typename... Args>
    inline const Packetizer::Packet& encode_arr(const uint8_t index, Args&&... args);
    // encode args as map format
    template <typename... Args>
    inline const Packetizer::Packet& encode_map(const uint8_t index, Args&&... args);


    // ----- for supported communication interface (Arduino, oF, ROS) -----

    // send arguments dilectly with variable types
    template <typename S, typename... Args>
    inline void send(S& stream, const uint8_t index, Args&&... args);
    // send binary data
    template <typename S>
    inline void send(S& stream, const uint8_t index, const uint8_t* data, const uint8_t size);
    // send manually packed data
    template <typename S>
    inline void send(S& stream, const uint8_t index);
    // send args as array format
    template <typename S, typename... Args>
    inline void send_arr(S& stream, const uint8_t index, Args&&... args);
    // send args as map format
    template <typename S, typename... Args>
    inline void send_map(S& stream, const uint8_t index, Args&&... args);

    // UDP version of send
    template <typename... Args>
    inline void send(UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args);
    inline void send(UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, const uint8_t* data, const uint8_t size);
    inline void send(UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index);
    template <typename... Args>
    inline void send_arr(UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args);
    template <typename... Args>
    inline void send_map(UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args);

    // publish arguments periodically
    template <typename S, typename... Args>
    inline PublishElementRef publish(const S& stream, const uint8_t index, Args&&... args);
    // publish arguments periodically as array format
    template <typename S, typename... Args>
    inline PublishElementRef publish_arr(const S& stream, const uint8_t index, Args&&... args);
    // publish arguments periodically as map format
    template <typename S, typename... Args>
    inline PublishElementRef publish_map(const S& stream, const uint8_t index, Args&&... args);
    // unpublish
    template <typename S>
    inline void unpublish(const S& stream, const uint8_t index);
    // get registerd publish element class
    template <typename S>
    inline PublishElementRef getPublishElementRef(const S& stream, const uint8_t index);

    // UDP version of publish
    template <typename... Args>
    inline PublishElementRef publish(const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args);
    template <typename... Args>
    inline PublishElementRef publish_arr(const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args);
    template <typename... Args>
    inline PublishElementRef publish_map(const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args);
    inline void unpublish(const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index);
    inline PublishElementRef getPublishElementRef(const UDP& stream, const uint8_t index);

    // must be called to publish data
    inline void post();
    // get MsgPack::Packer and handle it manually
    inline const MsgPack::Packer& getPacker();
}
```

## Other Options

```C++
#define MSGPACKETIZER_DEBUGLOG_ENABLE
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

```C++
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

```C++
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

- [MsgPack v0.3.18](https://github.com/hideakitai/MsgPack)
- [Packetizer v0.7.0](https://github.com/hideakitai/Packetizer)

## License

MIT
