#pragma once
#ifndef HT_SERIAL_PACKETIZER_TYPES_H
#define HT_SERIAL_PACKETIZER_TYPES_H

#ifdef ARDUINO
    #include <Arduino.h>
#endif

#include "util/ArxTypeTraits/ArxTypeTraits.h"
#include "util/ArxContainer/ArxContainer.h"
#include "util/ArxSmartPtr/ArxSmartPtr.h"


namespace ht {
namespace serial {
namespace packetizer {


#ifdef PACKETIZER_ENABLE_STREAM
#ifdef ARDUINO
    using StreamType = Stream;
    #define PACKETIZER_STREAM_WRITE(stream, data, size) stream.write(data, size);
#elif defined(OF_VERSION_MAJOR)
    using StreamType = ofSerial;
    #define PACKETIZER_STREAM_WRITE(stream, data, size) stream.writeBytes(data, size);
#endif
#endif // PACKETIZER_ENABLE_STREAM

#ifdef TEENSYDUINO
    #include "util/TeensyDirtySTLErrorSolution/TeensyDirtySTLErrorSolution.h"
#endif


template <typename Encoding> class Decoder;

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11

    #ifndef PACKETIZER_MAX_PACKET_QUEUE_SIZE
    #define PACKETIZER_MAX_PACKET_QUEUE_SIZE 0
    #endif // PACKETIZER_MAX_PACKET_QUEUE_SIZE

    using PacketData = std::vector<uint8_t>;
    struct Packet { uint8_t index; PacketData data; };
    using PacketQueue = std::deque<Packet>;

    using CallbackType = std::function<void(const uint8_t* data, const size_t size)>;
    using CallbackMap = std::map<uint8_t, CallbackType>;
    using CallbackAlwaysType = std::function<void(const uint8_t index, const uint8_t* data, const size_t size)>;

    template <typename Encoding>
    using DecoderRef = std::shared_ptr<Decoder<Encoding>>;
    #ifdef PACKETIZER_ENABLE_STREAM
        template <typename Encoding>
        using DecoderMap = std::map<StreamType*, DecoderRef<Encoding>>;
    #endif

    using namespace std;

#else // Do not have libstdc++11

    #ifndef PACKETIZER_MAX_PACKET_QUEUE_SIZE
    #define PACKETIZER_MAX_PACKET_QUEUE_SIZE 1
    #endif // PACKETIZER_MAX_PACKET_QUEUE_SIZE

    #ifndef PACKETIZER_MAX_PACKET_BINARY_SIZE
    #define PACKETIZER_MAX_PACKET_BINARY_SIZE 128
    #endif // PACKETIZER_MAX_PACKET_BINARY_SIZE

    #ifndef PACKETIZER_MAX_CALLBACK_QUEUE_SIZE
    #define PACKETIZER_MAX_CALLBACK_QUEUE_SIZE 4
    #endif // PACKETIZER_MAX_CALLBACK_QUEUE_SIZE

    #ifndef PACKETIZER_MAX_STREAM_MAP_SIZE
    #define PACKETIZER_MAX_STREAM_MAP_SIZE 2
    #endif // PACKETIZER_MAX_STREAM_MAP_SIZE

    using PacketData = arx::vector<uint8_t, PACKETIZER_MAX_PACKET_BINARY_SIZE>;
    struct Packet { uint8_t index; PacketData data; };
    using PacketQueue = arx::deque<Packet, PACKETIZER_MAX_PACKET_QUEUE_SIZE>;

    using CallbackType = std::function<void(const uint8_t* data, const size_t size)>;
    using CallbackAlwaysType = std::function<void(const uint8_t index, const uint8_t* data, const size_t size)>;
    using CallbackMap = arx::map<uint8_t, CallbackType, PACKETIZER_MAX_CALLBACK_QUEUE_SIZE>;

    template <typename Encoding>
    using DecoderRef = std::shared_ptr<Decoder<Encoding>>;
    #ifdef PACKETIZER_ENABLE_STREAM
        template <typename Encoding>
        using DecoderMap = arx::map<StreamType*, DecoderRef<Encoding>, PACKETIZER_MAX_STREAM_MAP_SIZE>;
    #endif

    using namespace arx;

#endif

    namespace encoding
    {
        struct COBS {};
        struct SLIP {};
    } // namespace encoder

#ifdef PACKETIZER_USE_INDEX_AS_DEFAULT
    #define PACKETIZER_DEFAULT_INDEX_SETTING true
#else
    #define PACKETIZER_DEFAULT_INDEX_SETTING false
#endif
#ifdef PACKETIZER_USE_CRC_AS_DEFAULT
    #define PACKETIZER_DEFAULT_CRC_SETTING true
#else
    #define PACKETIZER_DEFAULT_CRC_SETTING false
#endif

#ifdef PACKETIZER_SET_DEFAULT_ENCODING_SLIP
    using DefaultEncoding = encoding::SLIP;
#else
    using DefaultEncoding = encoding::COBS;
#endif

} // packetizer
} // serial
} // ht

#endif // HT_SERIAL_PACKETIZER_TYPES_H
