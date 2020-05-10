# MsgPacketizer
[MessagePack](https://github.com/msgpack/msgpack-c) based serializer / deserializer mainly for the communication between openFrameworks and Arduino




## Feature

- simple serialize / deserialize interface using [msgpack-c/c++](https://github.com/msgpack/msgpack-c)
- simple packet check option using header / escape sequence / checksum / crc8 support
- for the detail of serialize / deserialize,  see [msgpack-c/c++ Wiki](https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_configure)
- you can use this library both in oF and Arduino (see Dependencies)



## Packet Protocol

- 1 byte header 
- 1 byte index
- 1 byte size ( = N)
- N byte data (msgpack protocol)
- 1 byte footer (crc8, simple some, or none)

1 byte index can be used to identify the type of data, or as you like. 

You can also choose the type of footer from None / Simple Sum / CRC8.




## Dependencies
- [msgpack-arduino](https://github.com/hideakitai/msgpack-arduino) (for Arduino)
- [PlatformIO](http://platformio.org/) (for Arduino)




## Notation

If you want to use projectGenerator of openFrameworks, rename directory to ```ofxMsgPacketizer```



## Usage

### serialize & write

``` C++
MsgPacketizer::Packer packer; // default packet checker is crc8
struct Message
{
    int id;
    float time;
    MSGPACK_DEFINE(id, time); // custom type declaration
};

Message msg {1, 2.3};
packer << msg;
serial.writeBytes(packer.data(), packer.size());
```

## read & deserialize

``` c++
MsgPacketizer::Unacker unpacker; // default packet checker is crc8

serial.readBytes(serial_buffer, size);
unpacker.feed(serial_buffer, size);

while (unpacker.available())
{
    Message msg;
    unpacker >> msg;
  
    // do something with message
  
    unpacker.pop();
}
```

### Footer

set at constructor

```
MsgPacketizer::Reader reader; // default = CRC8
MsgPacketizer::Reader reader(MsgPacketizer::Checker::None);
MsgPacketizer::Reader reader(MsgPacketizer::Checker::Sum);
MsgPacketizer::Reader reader(MsgPacketizer::Checker::CRC8);

MsgPacketizer::Sender sender; // default = CRC8
MsgPacketizer::Sender sender(MsgPacketizer::Checker::None);
MsgPacketizer::Sender sender(MsgPacketizer::Checker::Sum);
MsgPacketizer::Sender sender(MsgPacketizer::Checker::CRC8);
```

or set after constructor

```c++
MsgPacketizer::Reader::setCheckMode(MsgPacketizer::Checker::None);
MsgPacketizer::Reader::setCheckMode(MsgPacketizer::Checker::Sum);
MsgPacketizer::Reader::setCheckMode(MsgPacketizer::Checker::CRC8); // default

MsgPacketizer::Sender::setCheckMode(MsgPacketizer::Checker::None);
MsgPacketizer::Sender::setCheckMode(MsgPacketizer::Checker::Sum);
MsgPacketizer::Sender::setCheckMode(MsgPacketizer::Checker::CRC8); // default
```



## Memory Management

Required memory can arbitrarily be managed by defining following macros. See detail at [msgpack-c/c++ Wiki](https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_configure).

```c++
#define MSGPACK_EMBED_STACK_SIZE 32
#define MSGPACK_PACKER_MAX_BUFFER_SIZE 9
#define MSGPACK_UNPACKER_INIT_BUFFER_SIZE (64*1024)
#define MSGPACK_UNPACKER_RESERVE_SIZE (32*1024)
#define MSGPACK_ZONE_CHUNK_SIZE 8192
#define MSGPACK_SBUFFER_INIT_SIZE 8192
#define MSGPACK_VREFBUFFER_REF_SIZE 32
#define MSGPACK_VREFBUFFER_CHUNK_SIZE 8192
#define MSGPACK_ZBUFFER_INIT_SIZE 8192
#define MSGPACK_ZBUFFER_RESERVE_SIZE 512
```


## Embedded Libraries

- [MsgPack v0.1.3](https://github.com/hideakitai/MsgPack)
- [Packetizer v0.3.3](https://github.com/hideakitai/Packetizer)


## License

MIT. Boost Software License, Version 1.0 for MessagePack itself. See the [LICENSE\_1\_0.txt](LICENSE_1_0.txt) file for details.