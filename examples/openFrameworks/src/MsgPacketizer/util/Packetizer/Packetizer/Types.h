#pragma once
#ifndef HT_SERIAL_PACKETIZER_TYPES_H
#define HT_SERIAL_PACKETIZER_TYPES_H

#ifdef ARDUINO
    #include <Arduino.h>
#endif

#ifdef PACKETIZER_DISABLE_STL
    #include "util/ArxTypeTraits/ArxTypeTraits.h"
    #include "util/ArxContainer/ArxContainer.h"
    #include "util/ArxSmartPtr/ArxSmartPtr.h"
#else
    #include <vector>
    #include <deque>
    #include <map>
    #include <functional>
    #include <memory>
#endif // PACKETIZER_DISABLE_STL


namespace ht {
namespace serial {
namespace packetizer {


#ifdef PACKETIZER_ENABLE_STREAM
#ifdef ARDUINO
    using StreamType = Stream;
#elif defined (OF_VERSION_MAJOR)
    using StreamType = ofSerial;
#endif
#endif // PACKETIZER_ENABLE_STREAM

#ifdef PACKETIZER_ENABLE_STREAM
#ifdef ARDUINO
    #define PACKETIZER_STREAM_WRITE(stream, data, size) stream.write(data, size);
#elif defined(OF_VERSION_MAJOR)
    #define PACKETIZER_STREAM_WRITE(stream, data, size) stream.writeBytes(data, size);
#endif
#endif // PACKETIZER_ENABLE_STREAM

#ifdef TEENSYDUINO
    #include "util/TeensyDirtySTLErrorSolution/TeensyDirtySTLErrorSolution.h"
#endif


template <typename Encoding> class Decoder;
#ifdef PACKETIZER_DISABLE_STL

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

    using Packet = arx::vector<uint8_t, PACKETIZER_MAX_PACKET_BINARY_SIZE>;
    using PacketQueue = arx::deque<Packet, PACKETIZER_MAX_PACKET_QUEUE_SIZE>;
    using IndexQueue = arx::deque<uint8_t, PACKETIZER_MAX_PACKET_QUEUE_SIZE>;

    using CallbackType = std::function<void(const uint8_t* data, const size_t size)>;
    using CallbackAlwaysType = std::function<void(const uint8_t index, const uint8_t* data, const size_t size)>;
    using CallbackMap = arx::map<uint8_t, CallbackType, PACKETIZER_MAX_CALLBACK_QUEUE_SIZE>;

    template <typename Encoding>
    using DecoderRef = arx::shared_ptr<Decoder<Encoding>>;
    #ifdef PACKETIZER_ENABLE_STREAM
        template <typename Encoding>
        using DecoderMap = arx::map<StreamType*, DecoderRef<Encoding>, PACKETIZER_MAX_STREAM_MAP_SIZE>;
    #endif

    using namespace arx;

#else

    #ifndef PACKETIZER_MAX_PACKET_QUEUE_SIZE
    #define PACKETIZER_MAX_PACKET_QUEUE_SIZE 0
    #endif // PACKETIZER_MAX_PACKET_QUEUE_SIZE

    using Packet = std::vector<uint8_t>;
    using PacketQueue = std::deque<Packet>;
    using IndexQueue = std::deque<uint8_t>;

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

#endif

    namespace encoding
    {
        struct COBS {};
        struct SLIP {};

        struct DefaultOption {
#ifdef PACKETIZER_USE_INDEX_AS_DEFAULT
            bool b_index {true};
#else
            bool b_index {false};
#endif
#ifdef PACKETIZER_USE_CRC_AS_DEFAULT
            bool b_crc {true};
#else
            bool b_crc {false};
#endif
        } default_option;

    } // namespace encoder

#ifdef PACKETIZER_SET_DEFAULT_ENCODING_SLIP
    using DefaultEncoding = encoding::SLIP;
#else
    using DefaultEncoding = encoding::COBS;
#endif

} // packetizer
} // serial
} // ht

#endif // HT_SERIAL_PACKETIZER_TYPES_H
