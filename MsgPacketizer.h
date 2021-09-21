#pragma once

#ifndef HT_SERIAL_MSGPACKETIZER_H
#define HT_SERIAL_MSGPACKETIZER_H

#if defined(ARDUINO) || defined(OF_VERSION_MAJOR)
#define MSGPACKETIZER_ENABLE_STREAM
#if defined(ESP_PLATFORM) || defined(ESP8266) || defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_MKRVIDOR4000) || defined(ARDUINO_SAMD_MKR1000) || defined(ARDUINO_SAMD_NANO_33_IOT)
#define MSGPACKETIZER_ENABLE_WIFI
#endif
#if defined(ESP_PLATFORM) || defined(ESP8266) || !defined(MSGPACKETIZER_ENABLE_WIFI)
#define MSGPACKETIZER_ENABLE_ETHER
#endif
#endif

#if defined(MSGPACKETIZER_ENABLE_ETHER) || defined(MSGPACKETIZER_ENABLE_WIFI)
#define MSGPACKETIZER_ENABLE_NETWORK
#include <UDP.h>
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

namespace ht {
namespace serial {
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
#endif

        enum class TargetStreamType : uint8_t {
            STREAM_SERIAL,
            STREAM_UDP,
            STREAM_TCP,
        };

    }  // namespace msgpacketizer
}  // namespace serial
}  // namespace ht

#include "MsgPacketizer/Publisher.h"
#include "MsgPacketizer/Subscriber.h"

namespace MsgPacketizer = ht::serial::msgpacketizer;

#include "MsgPacketizer/util/MsgPack/MsgPack/util/DebugLog/DebugLogRestoreState.h"

#endif  // HT_SERIAL_MSGPACKETIZER_H
