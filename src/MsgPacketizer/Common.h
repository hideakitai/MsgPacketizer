#pragma once

#include <cstdint>

extern "C"
{
    int _getpid(){ return -1;}
    int _kill(int pid, int sig){ return -1; }
    int _write(){return -1;}
}

// default sizes
//#define MSGPACK_EMBED_STACK_SIZE 32
//#define MSGPACK_PACKER_MAX_BUFFER_SIZE 9
//#define MSGPACK_UNPACKER_INIT_BUFFER_SIZE (64*1024)
//#define MSGPACK_UNPACKER_RESERVE_SIZE (32*1024)
//#define MSGPACK_ZONE_CHUNK_SIZE 8192
//#define MSGPACK_SBUFFER_INIT_SIZE 8192
//#define MSGPACK_VREFBUFFER_REF_SIZE 32
//#define MSGPACK_VREFBUFFER_CHUNK_SIZE 8192
//#define MSGPACK_ZBUFFER_INIT_SIZE 8192
//#define MSGPACK_ZBUFFER_RESERVE_SIZE 512

// modified sizes
#ifndef MSGPACK_EMBED_STACK_SIZE
#define MSGPACK_EMBED_STACK_SIZE 32 // default : j32
#endif
#ifndef MSGPACK_PACKER_MAX_BUFFER_SIZE
#define MSGPACK_PACKER_MAX_BUFFER_SIZE 9 // default : 9
#endif
#ifndef MSGPACK_UNPACKER_INIT_BUFFER_SIZE
#define MSGPACK_UNPACKER_INIT_BUFFER_SIZE 64 * 256 // default : (64*1024)
#endif
#ifndef MSGPACK_UNPACKER_RESERVE_SIZE
#define MSGPACK_UNPACKER_RESERVE_SIZE 32 * 256 // default : (32*1024)
#endif
#ifndef MSGPACK_ZONE_CHUNK_SIZE
#define MSGPACK_ZONE_CHUNK_SIZE 512 // default : 8192
#endif
#ifndef MSGPACK_SBUFFER_INIT_SIZE
#define MSGPACK_SBUFFER_INIT_SIZE 512 // default : 8192
#endif
#ifndef MSGPACK_VREFBUFFER_REF_SIZE
#define MSGPACK_VREFBUFFER_REF_SIZE 32 // default : 32
#endif
#ifndef MSGPACK_VREFBUFFER_CHUNK_SIZE
#define MSGPACK_VREFBUFFER_CHUNK_SIZE 512 // default : 8192
#endif
#ifndef MSGPACK_ZBUFFER_INIT_SIZE
#define MSGPACK_ZBUFFER_INIT_SIZE 512 // default : 8192
#endif
#ifndef MSGPACK_ZBUFFER_RESERVE_SIZE
#define MSGPACK_ZBUFFER_RESERVE_SIZE 512 // default : 512
#endif

#define MSGPACK_DISABLE_LEGACY_NIL
#define MSGPACK_DISABLE_LEGACY_CONVERT

namespace MsgPacketizer
{
    enum class State { Start, Index, Size, Data, Checksum };
    enum class Checker { None, Sum, CRC8 };

    const uint8_t START_BYTE = 0x7E;
    const uint8_t ESCAPE_BYTE = 0x7D;
    const uint8_t ESCAPE_MASK = 0x20;
    const uint16_t ESCAPE_MARKER = 0xFFFF;

	#define MSGPACKPRTCL_READ_BUFFER_SIZE 256
}

