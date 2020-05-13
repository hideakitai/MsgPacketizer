# Packetizer

binary data packetization encoder / decoder


## Feature

- encode / decode binary arrays to simple packet protocol
- simple packet check using header / escape sequence / crc8
- one-line packetizing and sending with Serial (or other Stream class)
- callback registration with lambda and automatic execution
- mainly developed for serial communication between Arduino and other apps (oF, Max, etc.)
- this library is embeded and used inside of my other libraries
  - [ArduinoOSC](https://github.com/hideakitai/ArduinoOSC) (only for OscSerial)
  - [MsgPacketizer](https://github.com/hideakitai/MsgPacketizer)


## Packet Protocol

| header | index  | data    | crc8   | footer |
|--------|--------|---------|--------|--------|
| 1 byte | 1 byte | N bytes | 1 byte | 1 byte |


- 1 byte header
- 1 byte index (packet index can be used to identify packet)
- N byte data (arbitrary binary arrays)
- 1 byte crc8 (for received data check)
- 1 byte footer

All of the bytes (excluding header and footer) can be escaped and whole packet size can be increased.


## Quick Start


``` C++
#include <Packetizer.h>

Packetizer::Decoder decoder;

uint8_t recv_index = 0x12;
uint8_t send_index = 0x34;

void setup()
{
    Serial.begin(115200);
    decoder.attach(Serial);

    decoder.subscribe(recv_index, [](const uint8_t* data, const uint8_t size)
    {
        Packetizer::send(Serial, send_index, data, size); // send back packet
    });
}

void loop()
{
    decoder.parse(); // must be called to trigger callback
}
```

## Quick API Example

### Simple One-Line Encode + Send

``` C++
// send array
uint8_t index {0xAB};
uint8_t arr[10] {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
Packetizer::send(Serial, index, arr, sizeof(arr));

// send variable sized args
// note: only 1 byte integral (0-255) is available for each args
Packetizer::send(Serial1, index, 1, 2, 3);
Packetizer::send(Serial2, index, 1, 2, 3, 4, 5, 6);
Packetizer::send(Serial3, index, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
```

### Simple Receive with Attached Serial + Decode with Lambda

``` C++
#include <Packetizer.h>

Packetizer::Decoder decoder;

uint8_t index_lambda = 0xAB;
uint8_t index_ext_cb = 0xCD;
void callback(const uint8_t* data, const uint8_t size)
{
    /* do something here */
}

void setup()
{
    Serial.begin(115200);
    decoder.attach(Serial);

    // these function will be called if the packet with this index has come

    // set callback with lambda
    decoder.subscribe(index_lambda, [](const uint8_t* data, const uint8_t size)
    {
        // do something here
    });

    // set pre-defined callback
    decoder.subscribe(index_ext_cb, callback);

    // set callback without index
    decoder.subscribe([](const uint8_t index, const uint8_t* data, const uint8_t size)
    {
        // this function will be called everytime when packet has come
    });
}

void loop()
{
    // parse(): read stream, decode packet, and execute callbacks
    // if no Stream object is attached, this function do nothing
    decoder.parse();
}
```


## Dependencies

### Teensy 3.x on Arduino IDE

- Please follow this instruction ([TeensyDirtySTLErrorSolution](https://github.com/hideakitai/TeensyDirtySTLErrorSolution)) to enable STL


### Other Platforms

None


## Embedded Libraries

- [ArxTypeTraits v0.1.6](https://github.com/hideakitai/ArxTypeTraits)
- [ArxContainer v0.3.4](https://github.com/hideakitai/ArxContainer)
- [ArxSmartPtr v0.1.1](https://github.com/hideakitai/ArxSmartPtr)
- [CRCx v0.2.1](https://github.com/hideakitai/CRCx)
- [TeensyDirtySTLErrorSolution v0.1.0](https://github.com/hideakitai/TeensyDirtySTLErrorSolution)


## Decoding Details

There are two main procedure to decode packets.

### A. Auto Receive + Decode + Callback

- attach Stream object
- add callbacks
- call parse()

This example is already shown above.


### B. Manual Data Feed + Manual Decode

- no Stream object
- no callbacks
- feed() data and handle packet data manually


``` c++
Packetizer::Decoder decoder;

void setup()
{
    // no Serial object attachment
    // no callbacks
}

void loop()
{
    // manual binary data feed to decoder
    while (const int size = Serial.available())
    {
        uint8_t data[size];
        Serial.readBytes((char*)data, size);

        // do something before decoding

        // if 3rd argument is false, callbacks won't be called
        // even if callbacks are registered
        decoder.feed(data, size, false);
    }

    // manual packet handling
    if (decoder.available())
    {
        // get decoded packet data
        const auto& packet = decoder.packet();
        const uint8_t* data = packet.data();
        const uint8_t size = packet.size();

        uint8_t index = 0xEF;
        if (decoder.index() == index)
        {
            // do something with data which has non-registered index

            // if data processing has done, pop current data
            decoder.pop();
        }
    }
}
```


### Callback at Every Packet

If you register callback without index, it will be called every time when packet has come. Note that the function arguments are different from that with index.

``` C++
decoder.subscribe([](const uint8_t index, const uint8_t* data, const uint8_t size)
{
    // this function will be called every time when packet has come
});
```




## Encoding Details

### Encode and Send Separately

``` C++
const auto& packet = Packetizer::encode(index, arr, sizeof(arr));
Serial.write(packet.data(), packet.size());
```

or

``` C++
const auto& packet = Packetizer::encode(index, 1, 2, 3, 4, 5);
Serial.write(packet.data(), packet.size());
```

### Encoding methods

There are three ways to encode data.

#### 1. variable size arguments

Packetizer::Encoder can pack variable sized arguments.

``` c++
Packetizer::Encoder encoder;
encoder.pack(1, 2, 3); // you can pack variable sized arguments
Serial.write(encoder.data(), encoder.size());
```

Index can be set in constructer.

``` c++
Packetizer::Encoder encoder(10); // you can set index in constructor (default is 0x00)
encoder.pack(11, 12, 13, 14, 15);
Serial.write(encoder.data(), encoder.size());
```

If you want to re-use Packetizer::Encoder instance, please call init() before you pack() data.

``` c++
// 1st use
Packetizer::Encoder encoder;
encoder.pack(31, 32);
Serial.write(encoder.data(), encoder.size());

// 2nd use
encoder.init(); // please call init() if you re-use Encoder instance
encoder.pack(41, 42, 43);
Serial.write(encoder.data(), encoder.size());

// 3rd use
encoder.init(50); // you can set index by init() (default is 0x00)
encoder.pack(51, 52, 53, 54);
Serial.write(encoder.data(), encoder.size());
```


#### 2. insertion operator

You can pack data using insertion operator. If you finish packing, please use Packer::endp().

``` c++
Packetizer::Encoder encoder;
encoder << 1 << 2;                  // add bytes like this
encoder << 3;                       // you can add any bytes
encoder << 4 << Packetizer::endp(); // finally you should add Packer::endp
Serial.write(encoder.data(), encoder.size());
```

If you re-use Packer instance, please use init() before you pack().

``` c++
// 1st use
Packetizer::Encoder encoder;
encoder << 1 << 2 << 3 << 4 << Packetizer::endp();
Serial.write(encoder.data(), encoder.size());

// 2nd use
encoder.init();
encoder << 11 << 12 << 13 << Packetizer::endp;
Serial.write(packer.data(), packer.size());

// 3rd use
encoder.init(20); // you can set index by init() (default is 0x00)
encoder << 21 << 22 << 23 << Packetizer::endp;
Serial.write(encoder.data(), encoder.size());
```

#### 3. pointer to array and size

``` c++
uint8_t test_array[5] = {0x7C, 0x7D, 0x7E, 0x7F, 0x80};

Packetizer::Encoder encoder;
encoder.pack(test_array, sizeof(test_array));
Serial.write(encoder.data(), encoder.size());

encoder.init(0x01); // you can change index number
encoder.pack(test_array, sizeof(test_array));
Serial.write(encoder.data(), encoder.size());
```




## API List


### Global


``` C++
// send
template <typename ...Rest>
void Packetizer::send(Stream& stream, const uint8_t index, const uint8_t first, Rest&& ...args)
void Packetizer::send(Stream& stream, const uint8_t index, const uint8_t* data, const uint8_t size)

// encode
template <typename ...Rest>
const Packet& Packetizer::encode(const uint8_t index, const uint8_t first, Rest&& ...args)
const Packet& Packetizer::encode(const uint8_t index, const uint8_t* data, const uint8_t size)
```


### Encoder Class

``` C++
explicit Encoder_(const uint8_t idx = 0)

void init(const uint8_t index = 0)

template <typename ...Rest>
void pack(const uint8_t first, Rest&& ...args)
void pack(const uint8_t* const sbuf, const uint8_t size)

const endp& operator<< (const endp& e)
Encoder_& operator<< (const uint8_t arg)

const Buffer& packet() const
size_t size() const
const uint8_t* data() const
```


### Decoder Class

``` C++
void attach(Stream& s)
void subscribe(const uint8_t index, const callback_t& func)
void subscribe(const cb_always_t& func)
void unsubscribe(uint8_t index)
void unsubscribe()
void parse(bool b_exec_cb = true)

void feed(const uint8_t* const data, const size_t size, bool b_exec_cb = true)
void feed(const uint8_t d, bool b_exec_cb = true)
void callback()

bool isParsing() const
size_t available() const
void pop()
void pop_back()

uint8_t index() const
uint8_t size() const
uint8_t data(const uint8_t i) const
const uint8_t* data() const

uint8_t index_back() const
uint8_t size_back() const
uint8_t data_back(const uint8_t i) const
const uint8_t* data_back() const

uint32_t errors() const
```


## Other Options

### Packet Data Storage Class Inside

STL is used to handle packet data by default, but for following boards/architectures, [ArxContainer](https://github.com/hideakitai/ArxContainer) is used to store the packet data because STL can not be used for such boards.
The storage size of such boards for packets, queue of packets, max packet binary size, and callbacks are limited.

- AVR
- megaAVR
- SAMD
- SPRESENSE


### Memory Management (for NO-STL Boards)

As mentioned above, for such boards like Arduino Uno, the storage sizes are limited.
And of course you can manage them by defining following macros.
But these default values are optimized for such boards, please be careful not to excess your boards storage/memory.

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

For other STL enabled boards, only max packet queue size can be changed.
Default value is 0 and not limited.

``` C++
#define PACKETIZER_MAX_PACKET_QUEUE_SIZE 3 // default: 0
```


### Packet Header

You can also change the packet header value.
If you need it, define it like:

``` C++
#define PACKETIZER_START_BYTE 0xAB // default: 0xC1
```


## License

MIT
