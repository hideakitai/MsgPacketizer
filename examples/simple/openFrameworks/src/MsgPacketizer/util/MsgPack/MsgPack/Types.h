#pragma once
#ifndef HT_SERIAL_MSGPACK_TYPES_H
#define HT_SERIAL_MSGPACK_TYPES_H

#include <stddef.h>
#ifdef ARDUINO
    #include <Arduino.h>
#else
    #include <string>
#endif

#ifdef HT_SERIAL_MSGPACK_DISABLE_STL
    #include "util/ArxContainer/ArxContainer.h"
    #ifdef HT_SERIAL_MSGPACK_DISABLE_STL
        #ifndef MSGPACK_MAX_PACKET_BYTE_SIZE
            #define MSGPACK_MAX_PACKET_BYTE_SIZE 128
        #endif // MSGPACK_MAX_PACKET_BYTE_SIZE
        #ifndef MSGPACK_MAX_ARRAY_SIZE
            #define MSGPACK_MAX_ARRAY_SIZE 8
        #endif // MSGPACK_MAX_ARRAY_SIZE
        #ifndef MSGPACK_MAX_MAP_SIZE
            #define MSGPACK_MAX_MAP_SIZE 8
        #endif // MSGPACK_MAX_MAP_SIZE
        #ifndef MSGPACK_MAX_OBJECT_SIZE
            #define MSGPACK_MAX_OBJECT_SIZE 32
        #endif // MSGPACK_MAX_OBJECT_SIZE
    #endif // HT_SERIAL_MSGPACK_DISABLE_STL
#else
    #include <vector>
    #include <map>
#endif // HT_SERIAL_MSGPACK_DISABLE_STL

#include "util/ArxTypeTraits/ArxTypeTraits.h"

namespace ht {
namespace serial {
namespace msgpack {

#ifdef HT_SERIAL_MSGPACK_DISABLE_STL
    using idx_t = arx::vector<size_t, MSGPACK_MAX_OBJECT_SIZE>;
    template <typename T, size_t N = MSGPACK_MAX_ARRAY_SIZE>
    using arr_t = arx::vector<T, N>;
    template <typename T, typename U, size_t N = MSGPACK_MAX_MAP_SIZE>
    using map_t = arx::map<T, U, N>;
    template <typename T, size_t N = MSGPACK_MAX_PACKET_BYTE_SIZE>
    using bin_t = arx::vector<
        typename std::enable_if<
            std::is_same<T, uint8_t>::value || std::is_same<T, char>::value, T
        >::type,
        N
    >;
#else
    using idx_t = std::vector<size_t>;
    template <typename T>
    using arr_t = std::vector<T>;
    template <typename T, typename U>
    using map_t = std::map<T, U>;
    template <typename T>
    using bin_t = std::vector<
        typename std::enable_if<
            std::is_same<T, uint8_t>::value || std::is_same<T, char>::value, T
        >::type,
        std::allocator<T>
    >;
#endif
#ifdef ARDUINO
    using str_t = String;
#else
    using str_t = std::string;
#endif


    namespace object
    {
        class NIL
        {
            bool is_nil {false};
            NIL& operator=(const NIL& rhs) { this->is_nil = rhs.is_nil; return *this; }
            NIL& operator=(const bool b) { this->is_nil = b; return *this; }
            bool operator()() const { return this->is_nil; }
        };
    }
    enum class Type : uint8_t
    {
        NA = 0xC1, // never used
        NIL = 0xC0,
        BOOL = 0xC2,
        UINT7 = 0x00, // same as POSITIVE_FIXINT
        UINT8 = 0xCC,
        UINT16 = 0xCD,
        UINT32 = 0xCE,
        UINT64 = 0xCF,
        INT5 = 0xE0, // same as NEGATIVE_FIXINT
        INT8 = 0xD0,
        INT16 = 0xD1,
        INT32 = 0xD2,
        INT64 = 0xD3,
        FLOAT32 = 0xCA,
        FLOAT64 = 0xCB,
        STR5 = 0xA0, // same as FIXSTR
        STR8 = 0xD9,
        STR16 = 0xDA,
        STR32 = 0xDB,
        BIN8 = 0xC4,
        BIN16 = 0xC5,
        BIN32 = 0xC6,
        ARRAY4 = 0x90, // same as FIXARRAY
        ARRAY16 = 0xDC,
        ARRAY32 = 0xDD,
        MAP4 = 0x80, // same as FIXMAP
        MAP16 = 0xDE,
        MAP32 = 0xDF,
        FIXEXT1 = 0xD4,
        FIXEXT2 = 0xD5,
        FIXEXT4 = 0xD6,
        FIXEXT8 = 0xD7,
        FIXEXT16 = 0xD8,
        EXT8 = 0xC7,
        EXT16 = 0xC8,
        EXT32 = 0xC9,
        TIMESTAMP32 = 0xD6,
        TIMESTAMP64 = 0xD7,
        TIMESTAMP96 = 0xC7,

        POSITIVE_FIXINT = 0x00,
        NEGATIVE_FIXINT = 0xE0,
        FIXSTR = 0xA0,
        FIXARRAY = 0x90,
        FIXMAP = 0x80,
    };

    enum class BitMask : uint8_t
    {
        BOOL = 0x01,
        POSITIVE_FIXINT = 0x7F,
        UINT7 = 0x7F, // same as POSITIVE_FIXINT
        NEGATIVE_FIXINT = 0x1F,
        INT5 = 0x1F, // same as NEGATIVE_FIXINT
        FIXSTR = 0x1F,
        STR5 = 0x1F, // same as FIXSTR
        FIXARRAY = 0x0F,
        ARRAY4 = 0x0F, // same as FIXARRAY
        FIXMAP = 0x0F,
        MAP4 = 0x0F, // same as FIXMAP
    };

} // msgpack
} // serial
} // ht

#endif // HT_SERIAL_MSGPACK_TYPES_H
