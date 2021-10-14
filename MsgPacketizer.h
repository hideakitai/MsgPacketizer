#pragma once

#ifndef HT_SERIAL_MSGPACKETIZER_H
#define HT_SERIAL_MSGPACKETIZER_H

#if defined(ARDUINO) || defined(OF_VERSION_MAJOR) || defined(SERIAL_H)
#define MSGPACKETIZER_ENABLE_STREAM
#ifdef ARDUINO  // TODO: support more platforms
#if defined(ESP_PLATFORM) || defined(ESP8266) || defined(ARDUINO_AVR_UNO_WIFI_REV2)                             \
    || defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_MKRVIDOR4000) || defined(ARDUINO_SAMD_MKR1000) \
    || defined(ARDUINO_SAMD_NANO_33_IOT)
#define MSGPACKETIZER_ENABLE_WIFI
#endif
#if defined(ESP_PLATFORM) || defined(ESP8266) || !defined(MSGPACKETIZER_ENABLE_WIFI)
#define MSGPACKETIZER_ENABLE_ETHER
#endif
#endif
#endif

#if defined(MSGPACKETIZER_ENABLE_ETHER) || defined(MSGPACKETIZER_ENABLE_WIFI)
#define MSGPACKETIZER_ENABLE_NETWORK
#include <Udp.h>
#include <Client.h>
#endif

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L  // Have libstdc++11
// use standard c++ libraries
#else
#ifndef MSGPACKETIZER_MAX_PUBLISH_ELEMENT_SIZE
#define MSGPACKETIZER_MAX_PUBLISH_ELEMENT_SIZE 5
#endif
#ifndef MSGPACKETIZER_MAX_PUBLISH_DESTINATION_SIZE
#define MSGPACKETIZER_MAX_PUBLISH_DESTINATION_SIZE 2
#endif
#ifndef MSGPACK_MAX_PACKET_BYTE_SIZE
#define MSGPACK_MAX_PACKET_BYTE_SIZE 96
#endif
#ifndef MSGPACK_MAX_ARRAY_SIZE
#define MSGPACK_MAX_ARRAY_SIZE 3
#endif
#ifndef MSGPACK_MAX_MAP_SIZE
#define MSGPACK_MAX_MAP_SIZE 3
#endif
#ifndef MSGPACK_MAX_OBJECT_SIZE
#define MSGPACK_MAX_OBJECT_SIZE 16
#endif
#ifndef PACKETIZER_MAX_PACKET_QUEUE_SIZE
#define PACKETIZER_MAX_PACKET_QUEUE_SIZE 1
#endif
#ifndef PACKETIZER_MAX_PACKET_BINARY_SIZE
#define PACKETIZER_MAX_PACKET_BINARY_SIZE 96
#endif
#ifndef PACKETIZER_MAX_CALLBACK_QUEUE_SIZE
#define PACKETIZER_MAX_CALLBACK_QUEUE_SIZE 3
#endif
#ifndef PACKETIZER_MAX_STREAM_MAP_SIZE
#define PACKETIZER_MAX_STREAM_MAP_SIZE 1
#endif
#endif  // have libstdc++11

#define PACKETIZER_USE_INDEX_AS_DEFAULT
#define PACKETIZER_USE_CRC_AS_DEFAULT

#include "MsgPacketizer/util/MsgPack/MsgPack/util/DebugLog/DebugLog.h"
#ifdef MSGPACKETIZER_DEBUGLOG_ENABLE
#include "MsgPacketizer/util/MsgPack/MsgPack/util/DebugLog/DebugLogEnable.h"
#define MSGPACK_DEBUGLOG_ENABLE
#else
#include "MsgPacketizer/util/MsgPack/MsgPack/util/DebugLog/DebugLogDisable.h"
#endif

#include "MsgPacketizer/util/Packetizer/Packetizer.h"
#include "MsgPacketizer/util/MsgPack/MsgPack.h"

namespace arduino {
namespace msgpack {
    namespace msgpacketizer {

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L  // Have libstdc++11
        using namespace std;
#else
        using namespace arx;
#endif

#ifdef ARDUINO
        using StreamType = Stream;
#define MSGPACKETIZER_ELAPSED_MICROS micros
#elif defined(OF_VERSION_MAJOR)
        using StreamType = ofSerial;
#define MSGPACKETIZER_ELAPSED_MICROS ofGetElapsedTimeMicros
#elif defined(SERIAL_H)
#include "serial/serial.h"
        using StreamType = ::serial::Serial;
#include <chrono>
        using namespace std::chrono;
        steady_clock::time_point time_start {steady_clock::now()};
#define MSGPACKETIZER_ELAPSED_MICROS() duration_cast<microseconds>(steady_clock::now() - time_start).count()
#endif

#ifdef ARDUINO
        using str_t = String;
#else
        using str_t = std::string;
#ifndef F
        inline const char* F(const char* c) {
            return c;
        }
#endif
#endif

        enum class TargetStreamType : uint8_t {
            STREAM_SERIAL,
            STREAM_UDP,
            STREAM_TCP,
        };

    }  // namespace msgpacketizer
}  // namespace msgpack
}  // namespace arduino

#include "MsgPacketizer/Publisher.h"
#include "MsgPacketizer/Subscriber.h"

namespace MsgPacketizer = arduino::msgpack::msgpacketizer;

#include "MsgPacketizer/util/MsgPack/MsgPack/util/DebugLog/DebugLogRestoreState.h"

#endif  // HT_SERIAL_MSGPACKETIZER_H
