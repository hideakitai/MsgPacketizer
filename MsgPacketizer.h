#pragma once

#ifndef HT_SERIAL_MSGPACKETIZER_H
#define HT_SERIAL_MSGPACKETIZER_H

#define PACKETIZER_USE_INDEX_AS_DEFAULT
#define PACKETIZER_USE_CRC_AS_DEFAULT

#include "util/MsgPack/MsgPack/util/DebugLog/DebugLog.h"
#ifdef MSGPACKETIZER_DEBUGLOG_ENABLE
#include "util/MsgPack/MsgPack/util/DebugLog/DebugLogEnable.h"
#define MSGPACK_DEBUGLOG_ENABLE
#else
#include "util/MsgPack/MsgPack/util/DebugLog/DebugLogDisable.h"
#endif

#include "util/Packetizer/Packetizer.h"
#include "util/MsgPack/MsgPack.h"

#if defined(ARDUINO) || defined(OF_VERSION_MAJOR)
#define MSGPACKETIZER_ENABLE_STREAM
#if defined(ESP_PLATFORM) || defined(ESP8266) || defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_MKRVIDOR4000) || defined(ARDUINO_SAMD_MKR1000) || defined(ARDUINO_SAMD_NANO_33_IOT)
#define MSGPACKETIZER_ENABLE_WIFI
#endif
#if defined(ESP_PLATFORM) || defined(ESP8266) || !defined(ARTNET_ENABLE_WIFI)
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

namespace ht {
namespace serial {
    namespace msgpacketizer {

#ifdef ARDUINO
        using StreamType = Stream;
#define ELAPSED_MICROS micros
#elif defined(OF_VERSION_MAJOR)
        using StreamType = ofSerial;
#define ELAPSED_MICROS ofGetElapsedTimeMicros
#endif

        namespace element {
            struct Base {
                uint32_t last_publish_us {0};
                uint32_t interval_us {33333};  // 30 fps

                bool next() const { return ELAPSED_MICROS() >= (last_publish_us + interval_us); }
                void setFrameRate(float fps) { interval_us = (uint32_t)(1000000.f / fps); }
                void setIntervalUsec(const uint32_t us) { interval_us = us; }
                void setIntervalMsec(const float ms) { interval_us = (uint32_t)(ms * 1000.f); }
                void setIntervalSec(const float sec) { interval_us = (uint32_t)(sec * 1000.f * 1000.f); }

                virtual ~Base() {}
                virtual void encodeTo(MsgPack::Packer& p) = 0;
            };

            using Ref = std::shared_ptr<Base>;
#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L  // Have libstdc++11
            using TupleRef = std::vector<Ref>;
#else
            using TupleRef = arx::vector<Ref, MSGPACKETIZER_MAX_PUBLISH_ELEMENT_SIZE>;
#endif

            template <typename T>
            class Value : public Base {
                T& t;

            public:
                Value(T& t)
                : t(t) {}
                virtual ~Value() {}
                virtual void encodeTo(MsgPack::Packer& p) override { p.pack(t); }
            };

            template <typename T>
            class Const : public Base {
                const T t;

            public:
                Const(const T& t)
                : t(t) {}
                virtual ~Const() {}
                virtual void encodeTo(MsgPack::Packer& p) override { p.pack(t); }
            };

            template <typename T>
            class Function : public Base {
                std::function<T()> getter;

            public:
                Function(const std::function<T()>& getter)
                : getter(getter) {}
                virtual ~Function() {}
                virtual void encodeTo(MsgPack::Packer& p) override { p.pack(getter()); }
            };

            class Tuple : public Base {
                TupleRef ts;

            public:
                Tuple(TupleRef&& ts)
                : ts(std::move(ts)) {}
                virtual ~Tuple() {}
                virtual void encodeTo(MsgPack::Packer& p) override {
                    for (auto& t : ts) t->encodeTo(p);
                }
            };

        }  // namespace element

        using PublishElementRef = element::Ref;
        using ElementTupleRef = element::TupleRef;

        template <typename T>
        inline auto make_element_ref(T& value)
            -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
            return PublishElementRef(new element::Value<T>(value));
        }

        template <typename T>
        inline auto make_element_ref(const T& value)
            -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
            return PublishElementRef(new element::Const<T>(value));
        }

        template <typename T>
        inline auto make_element_ref(const std::function<T()>& func)
            -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
            return PublishElementRef(new element::Function<T>(func));
        }

        template <typename Func>
        inline auto make_element_ref(const Func& func)
            -> std::enable_if_t<arx::is_callable<Func>::value, PublishElementRef> {
            return make_element_ref(arx::function_traits<Func>::cast(func));
        }

        // multiple parameters helper
        inline PublishElementRef make_element_ref(ElementTupleRef& t) {
            return PublishElementRef(new element::Tuple(std::move(t)));
        }

#ifdef MSGPACKETIZER_ENABLE_STREAM

        enum class TargetStreamType : uint8_t {
            STREAM_SERIAL,
            STREAM_UDP,
            STREAM_TCP,
        };

        struct Destination {
            StreamType* stream;
            TargetStreamType type;
            uint8_t index;
            String ip;
            uint16_t port;

            Destination() {}
            Destination(const Destination& dest)
            : stream(dest.stream), type(dest.type), index(dest.index), ip(dest.ip), port(dest.port) {}
            Destination(Destination&& dest)
            : stream(std::move(dest.stream)), type(std::move(dest.type)), index(std::move(dest.index)), ip(std::move(dest.ip)), port(std::move(dest.port)) {}
            Destination(const StreamType& stream, const TargetStreamType type, const uint8_t index)
            : stream((StreamType*)&stream), type(type), index(index), ip(), port() {}
            Destination(const StreamType& stream, const TargetStreamType type, const uint8_t index, const String& ip, const uint16_t port)
            : stream((StreamType*)&stream), type(type), index(index), ip(ip), port(port) {}
            Destination(const StreamType& stream, const TargetStreamType type, const uint8_t index, const IPAddress& ip, const uint16_t port)
            : stream((StreamType*)&stream), type(type), index(index), ip(String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3])), port(port) {}

            Destination& operator=(const Destination& dest) {
                stream = dest.stream;
                type = dest.type;
                index = dest.index;
                ip = dest.ip;
                port = dest.port;
                return *this;
            }
            Destination& operator=(Destination&& dest) {
                stream = std::move(dest.stream);
                type = std::move(dest.type);
                index = std::move(dest.index);
                ip = std::move(dest.ip);
                port = std::move(dest.port);
                return *this;
            }
            inline bool operator<(const Destination& rhs) const {
                return (stream != rhs.stream) ? (stream < rhs.stream)
                       : (type != rhs.type)   ? (type < rhs.type)
                       : (index != rhs.index) ? (index < rhs.index)
                       : (ip != rhs.ip)       ? (ip < rhs.ip)
                                              : (port < rhs.port);
            }
            inline bool operator==(const Destination& rhs) const {
                return (stream == rhs.stream) && (type == rhs.type) && (index == rhs.index) && (ip == rhs.ip) && (port == rhs.port);
            }
            inline bool operator!=(const Destination& rhs) const {
                return !(*this == rhs);
            }
        };

#endif  // MSGPACKETIZER_ENABLE_STREAM

        class DecodeTargetStream;
#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L  // Have libstdc++11
        using UnpackerRef = std::shared_ptr<MsgPack::Unpacker>;
#ifdef MSGPACKETIZER_ENABLE_STREAM
        using UnpackerMap = std::map<DecodeTargetStream, UnpackerRef>;
        using PackerMap = std::map<Destination, PublishElementRef>;
#endif  // MSGPACKETIZER_ENABLE_STREAM
        using namespace std;
#else
        using UnpackerRef = std::shared_ptr<MsgPack::Unpacker>;
#ifdef MSGPACKETIZER_ENABLE_STREAM
        using UnpackerMap = arx::map<DecodeTargetStream, UnpackerRef, PACKETIZER_MAX_STREAM_MAP_SIZE>;
        using PackerMap = arx::map<Destination, PublishElementRef, MSGPACKETIZER_MAX_PUBLISH_DESTINATION_SIZE>;
#endif  // MSGPACKETIZER_ENABLE_STREAM
        using namespace arx;
#endif

        class PackerManager {
            PackerManager() {}
            PackerManager(const PackerManager&) = delete;
            PackerManager& operator=(const PackerManager&) = delete;

            MsgPack::Packer encoder;
#ifdef MSGPACKETIZER_ENABLE_STREAM
            PackerMap addr_map;
#endif  // MSGPACKETIZER_ENABLE_STREAM

        public:
            static PackerManager& getInstance() {
                static PackerManager m;
                return m;
            }

            const MsgPack::Packer& getPacker() const {
                return encoder;
            }

            MsgPack::Packer& getPacker() {
                return encoder;
            }

#ifdef MSGPACKETIZER_ENABLE_STREAM

            void send(const Destination& dest, PublishElementRef elem) {
                encoder.clear();
                elem->encodeTo(encoder);
                switch (dest.type) {
                    case TargetStreamType::STREAM_SERIAL:
                        Packetizer::send(*dest.stream, dest.index, encoder.data(), encoder.size());
                        break;
                    case TargetStreamType::STREAM_UDP:
#ifdef MSGPACKETIZER_ENABLE_NETWORK
                        Packetizer::send(*reinterpret_cast<UDP*>(dest.stream), dest.ip, dest.port, dest.index, encoder.data(), encoder.size());
#endif
                        break;
                    case TargetStreamType::STREAM_TCP:
#ifdef MSGPACKETIZER_ENABLE_NETWORK
                        Packetizer::send(*reinterpret_cast<Client*>(dest.stream), dest.index, encoder.data(), encoder.size());
#endif
                        break;
                    default:
                        break;
                }
            }

            void post() {
                for (auto& mp : addr_map) {
                    if (mp.second->next()) {
                        mp.second->last_publish_us = ELAPSED_MICROS();
                        send(mp.first, mp.second);
                    }
                }
            }

            PublishElementRef publish(const StreamType& stream, const uint8_t index, const char* const value) {
                return publish_impl(stream, index, make_element_ref(value));
            }

            template <typename T>
            auto publish(const StreamType& stream, const uint8_t index, T& value)
                -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
                return publish_impl(stream, index, make_element_ref(value));
            }

            template <typename T>
            auto publish(const StreamType& stream, const uint8_t index, const T& value)
                -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
                return publish_impl(stream, index, make_element_ref(value));
            }

            template <typename Func>
            auto publish(const StreamType& stream, const uint8_t index, Func&& func)
                -> std::enable_if_t<arx::is_callable<Func>::value, PublishElementRef> {
                return publish(stream, index, arx::function_traits<Func>::cast(func));
            }

            template <typename T>
            PublishElementRef publish(const StreamType& stream, const uint8_t index, std::function<T()>&& getter) {
                return publish_impl(stream, index, make_element_ref(getter));
            }

            template <typename... Args>
            PublishElementRef publish(const StreamType& stream, const uint8_t index, Args&&... args) {
                ElementTupleRef v {make_element_ref(std::forward<Args>(args))...};
                return publish_impl(stream, index, make_element_ref(v));
            }

            template <typename... Args>
            PublishElementRef publish_arr(const StreamType& stream, const uint8_t index, Args&&... args) {
                static MsgPack::arr_size_t s(sizeof...(args));
                return publish(stream, index, s, std::forward<Args>(args)...);
            }

            template <typename... Args>
            PublishElementRef publish_map(const StreamType& stream, const uint8_t index, Args&&... args) {
                if ((sizeof...(args) % 2) == 0) {
                    static MsgPack::map_size_t s(sizeof...(args) / 2);
                    return publish(stream, index, s, std::forward<Args>(args)...);
                } else {
                    LOG_WARNING("serialize arg size must be even for map :", sizeof...(args));
                    return nullptr;
                }
            }

            void unpublish(const StreamType& stream, const uint8_t index) {
                Destination dest = getDestination(stream, index);
                addr_map.erase(dest);
            }

            PublishElementRef getPublishElementRef(const StreamType& stream, const uint8_t index) {
                Destination dest = getDestination(stream, index);
                return addr_map[dest];
            }

#ifdef MSGPACKETIZER_ENABLE_NETWORK

            PublishElementRef publish(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index, const char* const value) {
                return publish_impl(stream, ip, port, index, make_element_ref(value));
            }
            PublishElementRef publish(const Client& stream, const uint8_t index, const char* const value) {
                return publish_impl(stream, index, make_element_ref(value));
            }

            template <typename T>
            auto publish(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index, T& value)
                -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
                return publish_impl(stream, ip, port, index, make_element_ref(value));
            }
            template <typename T>
            auto publish(const Client& stream, const uint8_t index, T& value)
                -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
                return publish_impl(stream, index, make_element_ref(value));
            }

            template <typename T>
            auto publish(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index, const T& value)
                -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
                return publish_impl(stream, ip, port, index, make_element_ref(value));
            }
            template <typename T>
            auto publish(const Client& stream, const uint8_t index, const T& value)
                -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
                return publish_impl(stream, index, make_element_ref(value));
            }

            template <typename Func>
            auto publish(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index, Func&& func)
                -> std::enable_if_t<arx::is_callable<Func>::value, PublishElementRef> {
                return publish(stream, ip, port, index, arx::function_traits<Func>::cast(func));
            }
            template <typename Func>
            auto publish(const Client& stream, const uint8_t index, Func&& func)
                -> std::enable_if_t<arx::is_callable<Func>::value, PublishElementRef> {
                return publish(stream, index, arx::function_traits<Func>::cast(func));
            }

            template <typename T>
            PublishElementRef publish(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index, std::function<T()>&& getter) {
                return publish_impl(stream, ip, port, index, make_element_ref(getter));
            }
            template <typename T>
            PublishElementRef publish(const Client& stream, const uint8_t index, std::function<T()>&& getter) {
                return publish_impl(stream, index, make_element_ref(getter));
            }

            template <typename... Args>
            PublishElementRef publish(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index, Args&&... args) {
                ElementTupleRef v {make_element_ref(std::forward<Args>(args))...};
                return publish_impl(stream, ip, port, index, make_element_ref(v));
            }
            template <typename... Args>
            PublishElementRef publish(const Client& stream, const uint8_t index, Args&&... args) {
                ElementTupleRef v {make_element_ref(std::forward<Args>(args))...};
                return publish_impl(stream, index, make_element_ref(v));
            }

            template <typename... Args>
            PublishElementRef publish_arr(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index, Args&&... args) {
                static MsgPack::arr_size_t s(sizeof...(args));
                return publish(stream, ip, port, index, s, std::forward<Args>(args)...);
            }
            template <typename... Args>
            PublishElementRef publish_arr(const Client& stream, const uint8_t index, Args&&... args) {
                static MsgPack::arr_size_t s(sizeof...(args));
                return publish(stream, index, s, std::forward<Args>(args)...);
            }

            template <typename... Args>
            PublishElementRef publish_map(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index, Args&&... args) {
                if ((sizeof...(args) % 2) == 0) {
                    static MsgPack::map_size_t s(sizeof...(args) / 2);
                    return publish(stream, ip, port, index, s, std::forward<Args>(args)...);
                } else {
                    LOG_WARNING("serialize arg size must be even for map :", sizeof...(args));
                    return nullptr;
                }
            }
            template <typename... Args>
            PublishElementRef publish_map(const Client& stream, const uint8_t index, Args&&... args) {
                if ((sizeof...(args) % 2) == 0) {
                    static MsgPack::map_size_t s(sizeof...(args) / 2);
                    return publish(stream, index, s, std::forward<Args>(args)...);
                } else {
                    LOG_WARNING("serialize arg size must be even for map :", sizeof...(args));
                    return nullptr;
                }
            }

            void unpublish(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index) {
                Destination dest = getDestination(stream, ip, port, index);
                addr_map.erase(dest);
            }
            void unpublish(const Client& stream, const uint8_t index) {
                Destination dest = getDestination(stream, index);
                addr_map.erase(dest);
            }

            PublishElementRef getPublishElementRef(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index) {
                Destination dest = getDestination(stream, ip, port, index);
                return addr_map[dest];
            }
            PublishElementRef getPublishElementRef(const Client& stream, const uint8_t index) {
                Destination dest = getDestination(stream, index);
                return addr_map[dest];
            }

#endif  // MSGPACKETIZER_ENABLE_NETWORK
#endif  // MSGPACKETIZER_ENABLE_STREAM

        private:
#ifdef MSGPACKETIZER_ENABLE_STREAM

            Destination getDestination(const StreamType& stream, const uint8_t index) {
                Destination s;
                s.stream = (StreamType*)&stream;
                s.type = TargetStreamType::STREAM_SERIAL;
                s.index = index;
                return s;
            }

            template <typename S>
            PublishElementRef publish_impl(const S& stream, const uint8_t index, PublishElementRef ref) {
                Destination dest = getDestination(stream, index);
                addr_map.insert(make_pair(dest, ref));
                return ref;
            }

#ifdef MSGPACKETIZER_ENABLE_NETWORK

            Destination getDestination(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index) {
                Destination s;
                s.stream = (StreamType*)&stream;
                s.type = TargetStreamType::STREAM_UDP;
                s.index = index;
                s.ip = ip;
                s.port = port;
                return s;
            }
            Destination getDestination(const Client& stream, const uint8_t index) {
                Destination s;
                s.stream = (StreamType*)&stream;
                s.type = TargetStreamType::STREAM_TCP;
                s.index = index;
                return s;
            }

            PublishElementRef publish_impl(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index, PublishElementRef ref) {
                Destination dest = getDestination(stream, ip, port, index);
                addr_map.insert(make_pair(dest, ref));
                return ref;
            }
            PublishElementRef publish_impl(const Client& stream, const uint8_t index, PublishElementRef ref) {
                Destination dest = getDestination(stream, index);
                addr_map.insert(make_pair(dest, ref));
                return ref;
            }

#endif
#endif  // MSGPACKETIZER_ENABLE_STREAM
        };

        template <typename... Args>
        inline const Packetizer::Packet& encode(const uint8_t index, Args&&... args) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.serialize(std::forward<Args>(args)...);
            return Packetizer::encode(index, packer.data(), packer.size());
        }

        inline const Packetizer::Packet& encode(const uint8_t index, const uint8_t* data, const uint8_t size) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.pack(data, size);
            return Packetizer::encode(index, packer.data(), packer.size());
        }

        inline const Packetizer::Packet& encode(const uint8_t index) {
            auto& packer = PackerManager::getInstance().getPacker();
            return Packetizer::encode(index, packer.data(), packer.size());
        }

        template <typename... Args>
        inline const Packetizer::Packet& encode_arr(const uint8_t index, Args&&... args) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.serialize(MsgPack::arr_size_t(sizeof...(args)), std::forward<Args>(args)...);
            return Packetizer::encode(index, packer.data(), packer.size());
        }

        template <typename... Args>
        inline const Packetizer::Packet& encode_map(const uint8_t index, Args&&... args) {
            if ((sizeof...(args) % 2) == 0) {
                auto& packer = PackerManager::getInstance().getPacker();
                packer.clear();
                packer.serialize(MsgPack::arr_size_t(sizeof...(args) / 2), std::forward<Args>(args)...);
                return Packetizer::encode(index, packer.data(), packer.size());
            } else {
                LOG_WARNING("serialize arg size must be even for map :", sizeof...(args));
                return Packetizer::encode(index, nullptr, 0);
            }
        }

#ifdef MSGPACKETIZER_ENABLE_STREAM

        template <typename... Args>
        inline void send(StreamType& stream, const uint8_t index, Args&&... args) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.serialize(std::forward<Args>(args)...);
            Packetizer::send(stream, index, packer.data(), packer.size());
        }

        inline void send(StreamType& stream, const uint8_t index, const uint8_t* data, const uint8_t size) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.pack(data, size);
            Packetizer::send(stream, index, packer.data(), packer.size());
        }

        inline void send(StreamType& stream, const uint8_t index) {
            auto& packer = PackerManager::getInstance().getPacker();
            Packetizer::send(stream, index, packer.data(), packer.size());
        }

        template <typename... Args>
        inline void send_arr(StreamType& stream, const uint8_t index, Args&&... args) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.serialize(MsgPack::arr_size_t(sizeof...(args)), std::forward<Args>(args)...);
            Packetizer::send(stream, index, packer.data(), packer.size());
        }

        template <typename... Args>
        inline void send_map(StreamType& stream, const uint8_t index, Args&&... args) {
            if ((sizeof...(args) % 2) == 0) {
                auto& packer = PackerManager::getInstance().getPacker();
                packer.clear();
                packer.serialize(MsgPack::arr_size_t(sizeof...(args) / 2), std::forward<Args>(args)...);
                Packetizer::send(stream, index, packer.data(), packer.size());
            } else {
                LOG_WARNING("serialize arg size must be even for map :", sizeof...(args));
            }
        }

#ifdef MSGPACKETIZER_ENABLE_NETWORK

        template <typename... Args>
        inline void send(UDP& stream, const String& ip, const uint16_t port, const uint8_t index, Args&&... args) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.serialize(std::forward<Args>(args)...);
            Packetizer::send(stream, ip, port, index, packer.data(), packer.size());
        }
        template <typename... Args>
        inline void send(Client& stream, const uint8_t index, Args&&... args) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.serialize(std::forward<Args>(args)...);
            Packetizer::send(stream, index, packer.data(), packer.size());
        }

        inline void send(UDP& stream, const String& ip, const uint16_t port, const uint8_t index, const uint8_t* data, const uint8_t size) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.pack(data, size);
            Packetizer::send(stream, ip, port, index, packer.data(), packer.size());
        }
        inline void send(Client& stream, const uint8_t index, const uint8_t* data, const uint8_t size) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.pack(data, size);
            Packetizer::send(stream, index, packer.data(), packer.size());
        }

        inline void send(UDP& stream, const String& ip, const uint16_t port, const uint8_t index) {
            auto& packer = PackerManager::getInstance().getPacker();
            Packetizer::send(stream, ip, port, index, packer.data(), packer.size());
        }
        inline void send(Client& stream, const uint8_t index) {
            auto& packer = PackerManager::getInstance().getPacker();
            Packetizer::send(stream, index, packer.data(), packer.size());
        }

        template <typename... Args>
        inline void send_arr(UDP& stream, const String& ip, const uint16_t port, const uint8_t index, Args&&... args) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.serialize(MsgPack::arr_size_t(sizeof...(args)), std::forward<Args>(args)...);
            Packetizer::send(stream, ip, port, index, packer.data(), packer.size());
        }
        template <typename... Args>
        inline void send_arr(Client& stream, const uint8_t index, Args&&... args) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.serialize(MsgPack::arr_size_t(sizeof...(args)), std::forward<Args>(args)...);
            Packetizer::send(stream, index, packer.data(), packer.size());
        }

        template <typename... Args>
        inline void send_map(UDP& stream, const String& ip, const uint16_t port, const uint8_t index, Args&&... args) {
            if ((sizeof...(args) % 2) == 0) {
                auto& packer = PackerManager::getInstance().getPacker();
                packer.clear();
                packer.serialize(MsgPack::arr_size_t(sizeof...(args) / 2), std::forward<Args>(args)...);
                Packetizer::send(stream, ip, port, index, packer.data(), packer.size());
            } else {
                LOG_WARNING("serialize arg size must be even for map :", sizeof...(args));
            }
        }
        template <typename... Args>
        inline void send_map(Client& stream, const uint8_t index, Args&&... args) {
            if ((sizeof...(args) % 2) == 0) {
                auto& packer = PackerManager::getInstance().getPacker();
                packer.clear();
                packer.serialize(MsgPack::arr_size_t(sizeof...(args) / 2), std::forward<Args>(args)...);
                Packetizer::send(stream, index, packer.data(), packer.size());
            } else {
                LOG_WARNING("serialize arg size must be even for map :", sizeof...(args));
            }
        }

#endif  // MSGPACKETIZER_ENABLE_NETWORK

        template <typename... Args>
        inline PublishElementRef publish(const StreamType& stream, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish(stream, index, std::forward<Args>(args)...);
        }

        template <typename... Args>
        inline PublishElementRef publish_arr(const StreamType& stream, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish_arr(stream, index, std::forward<Args>(args)...);
        }

        template <typename... Args>
        inline PublishElementRef publish_map(const StreamType& stream, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish_map(stream, index, std::forward<Args>(args)...);
        }

        inline void unpublish(const StreamType& stream, const uint8_t index) {
            PackerManager::getInstance().unpublish(stream, index);
        };

        inline PublishElementRef getPublishElementRef(const StreamType& stream, const uint8_t index) {
            return PackerManager::getInstance().getPublishElementRef(stream, index);
        }

#ifdef MSGPACKETIZER_ENABLE_NETWORK

        template <typename... Args>
        inline PublishElementRef publish(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish(stream, ip, port, index, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline PublishElementRef publish(const Client& stream, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish(stream, index, std::forward<Args>(args)...);
        }

        template <typename... Args>
        inline PublishElementRef publish_arr(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish_arr(stream, ip, port, index, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline PublishElementRef publish_arr(const Client& stream, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish_arr(stream, index, std::forward<Args>(args)...);
        }

        template <typename... Args>
        inline PublishElementRef publish_map(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish_map(stream, ip, port, index, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline PublishElementRef publish_map(const Client& stream, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish_map(stream, index, std::forward<Args>(args)...);
        }

        inline void unpublish(const UDP& stream, const String& ip, const uint16_t port, const uint8_t index) {
            PackerManager::getInstance().unpublish(stream, ip, port, index);
        };
        inline void unpublish(const Client& stream, const uint8_t index) {
            PackerManager::getInstance().unpublish(stream, index);
        };

        inline PublishElementRef getPublishElementRef(const UDP& stream, const uint8_t index) {
            return PackerManager::getInstance().getPublishElementRef(stream, index);
        }
        inline PublishElementRef getPublishElementRef(const Client& stream, const uint8_t index) {
            return PackerManager::getInstance().getPublishElementRef(stream, index);
        }

#endif  // MSGPACKETIZER_ENABLE_NETWORK

        inline void post() {
            PackerManager::getInstance().post();
        }

#endif  // MSGPACKETIZER_ENABLE_STREAM

        inline const MsgPack::Packer& getPacker() {
            return PackerManager::getInstance().getPacker();
        }

#ifdef MSGPACKETIZER_ENABLE_STREAM

        struct DecodeTargetStream {
            StreamType* stream;
            TargetStreamType type;

            DecodeTargetStream()
            : stream(nullptr), type(TargetStreamType::STREAM_SERIAL) {}
            DecodeTargetStream(const DecodeTargetStream& dest)
            : stream(dest.stream), type(dest.type) {}
            DecodeTargetStream(DecodeTargetStream&& dest)
            : stream(std::move(dest.stream)), type(std::move(dest.type)) {}
            DecodeTargetStream(const StreamType& stream, const TargetStreamType type)
            : stream((StreamType*)&stream), type(type) {}

            DecodeTargetStream& operator=(const DecodeTargetStream& dest) {
                stream = dest.stream;
                type = dest.type;
                return *this;
            }
            DecodeTargetStream& operator=(DecodeTargetStream&& dest) {
                stream = std::move(dest.stream);
                type = std::move(dest.type);
                return *this;
            }
            inline bool operator<(const DecodeTargetStream& rhs) const {
                return (stream != rhs.stream) ? (stream < rhs.stream) : (type < rhs.type);
            }
            inline bool operator==(const DecodeTargetStream& rhs) const {
                return (stream == rhs.stream) && (type == rhs.type);
            }
            inline bool operator!=(const DecodeTargetStream& rhs) const {
                return !(*this == rhs);
            }
        };

#endif  // MSGPACKETIZER_ENABLE_STREAM

        class UnpackerManager {
            UnpackerManager() {}
            UnpackerManager(const UnpackerManager&) = delete;
            UnpackerManager& operator=(const UnpackerManager&) = delete;

            UnpackerRef decoder;  // for non-stream usage
#ifdef MSGPACKETIZER_ENABLE_STREAM
            UnpackerMap decoders;
#endif  // MSGPACKETIZER_ENABLE_STREAM

        public:
            static UnpackerManager& getInstance() {
                static UnpackerManager m;
                return m;
            }

            UnpackerRef getUnpackerRef() {
                if (!decoder)
                    decoder = std::make_shared<MsgPack::Unpacker>();
                return decoder;
            }

#ifdef MSGPACKETIZER_ENABLE_STREAM

            DecodeTargetStream getDecodeTargetStream(const StreamType& stream) {
                DecodeTargetStream s;
                s.stream = (StreamType*)&stream;
                s.type = TargetStreamType::STREAM_SERIAL;
                return s;
            }

            UnpackerRef getUnpackerRef(const StreamType& stream) {
                auto s = getDecodeTargetStream(stream);
                if (decoders.find(s) == decoders.end())
                    decoders.insert(make_pair(s, std::make_shared<MsgPack::Unpacker>()));
                return decoders[s];
            }

#ifdef MSGPACKETIZER_ENABLE_NETWORK

            DecodeTargetStream getDecodeTargetStream(const UDP& stream) {
                DecodeTargetStream s;
                s.stream = (StreamType*)&stream;
                s.type = TargetStreamType::STREAM_UDP;
                return s;
            }
            DecodeTargetStream getDecodeTargetStream(const Client& stream) {
                DecodeTargetStream s;
                s.stream = (StreamType*)&stream;
                s.type = TargetStreamType::STREAM_TCP;
                return s;
            }

            UnpackerRef getUnpackerRef(const UDP& stream) {
                auto s = getDecodeTargetStream(stream);
                if (decoders.find(s) == decoders.end())
                    decoders.insert(make_pair(s, std::make_shared<MsgPack::Unpacker>()));
                return decoders[s];
            }
            UnpackerRef getUnpackerRef(const Client& stream) {
                auto s = getDecodeTargetStream(stream);
                if (decoders.find(s) == decoders.end())
                    decoders.insert(make_pair(s, std::make_shared<MsgPack::Unpacker>()));
                return decoders[s];
            }

#endif  // MSGPACKETIZER_ENABLE_NETWORK

            const UnpackerMap& getUnpackerMap() const {
                return decoders;
            }

            UnpackerMap& getUnpackerMap() {
                return decoders;
            }

#endif  // MSGPACKETIZER_ENABLE_STREAM
        };

#ifdef MSGPACKETIZER_ENABLE_STREAM

        // for supported communication interface (Arduino, oF)

        template <typename... Args>
        inline void subscribe(StreamType& stream, const uint8_t index, Args&&... args) {
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(std::forward<Args>(args)...);
            });
        }

        template <typename... Args>
        inline void subscribe_arr(StreamType& stream, const uint8_t index, Args&&... args) {
            static MsgPack::arr_size_t sz;
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(sz, std::forward<Args>(args)...);
            });
        }

        template <typename... Args>
        inline void subscribe_map(StreamType& stream, const uint8_t index, Args&&... args) {
            if ((sizeof...(args) % 2) == 0) {
                static MsgPack::map_size_t sz;
                Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size) {
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                    unpacker->clear();
                    unpacker->feed(data, size);
                    unpacker->deserialize(sz, std::forward<Args>(args)...);
                });
            } else {
                LOG_WARNING("deserialize arg size must be even for map :", sizeof...(args));
            }
        }

        namespace detail {
            template <typename R, typename... Args>
            inline void subscribe(StreamType& stream, const uint8_t index, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(stream, index,
                    [&, cb {std::move(callback)}](const uint8_t* data, const uint8_t size) {
                        auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                        unpacker->clear();
                        unpacker->feed(data, size);
                        std::tuple<std::remove_cvref_t<Args>...> t;
                        unpacker->to_tuple(t);
                        std::apply(cb, t);
                    });
            }

            template <typename R, typename... Args>
            inline void subscribe(StreamType& stream, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(stream,
                    [&, cb {std::move(callback)}](const uint8_t index, const uint8_t* data, const uint8_t size) {
                        auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                        unpacker->clear();
                        unpacker->feed(data, size);
                        cb(index, *unpacker);
                    });
            }
        }  // namespace detail

        template <typename F>
        inline auto subscribe(StreamType& stream, const uint8_t index, F&& callback)
            -> std::enable_if_t<arx::is_callable<F>::value> {
            detail::subscribe(stream, index, arx::function_traits<F>::cast(std::move(callback)));
        }

        template <typename F>
        inline auto subscribe(StreamType& stream, F&& callback)
            -> std::enable_if_t<arx::is_callable<F>::value> {
            detail::subscribe(stream, arx::function_traits<F>::cast(std::move(callback)));
        }

        void unsubscribe(const StreamType& stream, const uint8_t index) {
            Packetizer::unsubscribe(stream, index);
        }

        void unsubscribe(const StreamType& stream) {
            Packetizer::unsubscribe(stream);
        }

        inline UnpackerRef getUnpackerRef(const StreamType& stream) {
            return UnpackerManager::getInstance().getUnpackerRef(stream);
        }

#ifdef MSGPACKETIZER_ENABLE_NETWORK

        template <typename... Args>
        inline void subscribe(UDP& stream, const uint8_t index, Args&&... args) {
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(std::forward<Args>(args)...);
            });
        }
        template <typename... Args>
        inline void subscribe(Client& stream, const uint8_t index, Args&&... args) {
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(std::forward<Args>(args)...);
            });
        }

        template <typename... Args>
        inline void subscribe_arr(UDP& stream, const uint8_t index, Args&&... args) {
            static MsgPack::arr_size_t sz;
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(sz, std::forward<Args>(args)...);
            });
        }
        template <typename... Args>
        inline void subscribe_arr(Client& stream, const uint8_t index, Args&&... args) {
            static MsgPack::arr_size_t sz;
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(sz, std::forward<Args>(args)...);
            });
        }

        template <typename... Args>
        inline void subscribe_map(UDP& stream, const uint8_t index, Args&&... args) {
            if ((sizeof...(args) % 2) == 0) {
                static MsgPack::map_size_t sz;
                Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size) {
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                    unpacker->clear();
                    unpacker->feed(data, size);
                    unpacker->deserialize(sz, std::forward<Args>(args)...);
                });
            } else {
                LOG_WARNING("deserialize arg size must be even for map :", sizeof...(args));
            }
        }
        template <typename... Args>
        inline void subscribe_map(Client& stream, const uint8_t index, Args&&... args) {
            if ((sizeof...(args) % 2) == 0) {
                static MsgPack::map_size_t sz;
                Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size) {
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                    unpacker->clear();
                    unpacker->feed(data, size);
                    unpacker->deserialize(sz, std::forward<Args>(args)...);
                });
            } else {
                LOG_WARNING("deserialize arg size must be even for map :", sizeof...(args));
            }
        }

        namespace detail {
            template <typename R, typename... Args>
            inline void subscribe(UDP& stream, const uint8_t index, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(stream, index,
                    [&, cb {std::move(callback)}](const uint8_t* data, const uint8_t size) {
                        auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                        unpacker->clear();
                        unpacker->feed(data, size);
                        std::tuple<std::remove_cvref_t<Args>...> t;
                        unpacker->to_tuple(t);
                        std::apply(cb, t);
                    });
            }
            template <typename R, typename... Args>
            inline void subscribe(Client& stream, const uint8_t index, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(stream, index,
                    [&, cb {std::move(callback)}](const uint8_t* data, const uint8_t size) {
                        auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                        unpacker->clear();
                        unpacker->feed(data, size);
                        std::tuple<std::remove_cvref_t<Args>...> t;
                        unpacker->to_tuple(t);
                        std::apply(cb, t);
                    });
            }

            template <typename R, typename... Args>
            inline void subscribe(UDP& stream, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(stream,
                    [&, cb {std::move(callback)}](const uint8_t index, const uint8_t* data, const uint8_t size) {
                        auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                        unpacker->clear();
                        unpacker->feed(data, size);
                        cb(index, *unpacker);
                    });
            }
            template <typename R, typename... Args>
            inline void subscribe(Client& stream, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(stream,
                    [&, cb {std::move(callback)}](const uint8_t index, const uint8_t* data, const uint8_t size) {
                        auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                        unpacker->clear();
                        unpacker->feed(data, size);
                        cb(index, *unpacker);
                    });
            }
        }  // namespace detail

        template <typename F>
        inline auto subscribe(UDP& stream, const uint8_t index, F&& callback)
            -> std::enable_if_t<arx::is_callable<F>::value> {
            detail::subscribe(stream, index, arx::function_traits<F>::cast(std::move(callback)));
        }
        template <typename F>
        inline auto subscribe(Client& stream, const uint8_t index, F&& callback)
            -> std::enable_if_t<arx::is_callable<F>::value> {
            detail::subscribe(stream, index, arx::function_traits<F>::cast(std::move(callback)));
        }

        template <typename F>
        inline auto subscribe(UDP& stream, F&& callback)
            -> std::enable_if_t<arx::is_callable<F>::value> {
            detail::subscribe(stream, arx::function_traits<F>::cast(std::move(callback)));
        }
        template <typename F>
        inline auto subscribe(Client& stream, F&& callback)
            -> std::enable_if_t<arx::is_callable<F>::value> {
            detail::subscribe(stream, arx::function_traits<F>::cast(std::move(callback)));
        }

        template <typename F>
        void unsubscribe(const UDP& stream, const uint8_t index) {
            Packetizer::unsubscribe(stream, index);
        }
        template <typename F>
        void unsubscribe(const Client& stream, const uint8_t index) {
            Packetizer::unsubscribe(stream, index);
        }

        template <typename F>
        void unsubscribe(const UDP& stream) {
            Packetizer::unsubscribe(stream);
        }
        template <typename F>
        void unsubscribe(const Client& stream) {
            Packetizer::unsubscribe(stream);
        }

        inline UnpackerRef getUnpackerRef(const UDP& stream) {
            return UnpackerManager::getInstance().getUnpackerRef(stream);
        }
        inline UnpackerRef getUnpackerRef(const Client& stream) {
            return UnpackerManager::getInstance().getUnpackerRef(stream);
        }

#endif  // MSGPACKETIZER_ENABLE_NETWORK

        inline void parse(bool b_exec_cb = true) {
            Packetizer::parse(b_exec_cb);
        }

        inline UnpackerMap& getUnpackerMap() {
            return UnpackerManager::getInstance().getUnpackerMap();
        }

        inline void update(bool b_exec_cb = true) {
            Packetizer::parse(b_exec_cb);
            PackerManager::getInstance().post();
        }

#endif  // MSGPACKETIZER_ENABLE_STREAM

        // for unsupported communication interface with manual operation

        template <typename... Args>
        inline void subscribe(const uint8_t index, Args&&... args) {
            Packetizer::subscribe(index, [&](const uint8_t* data, const uint8_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef();
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(std::forward<Args>(args)...);
            });
        }

        template <typename... Args>
        inline void subscribe_arr(const uint8_t index, Args&&... args) {
            static MsgPack::arr_size_t sz;
            Packetizer::subscribe(index, [&](const uint8_t* data, const uint8_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef();
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(sz, std::forward<Args>(args)...);
            });
        }

        template <typename... Args>
        inline void subscribe_map(const uint8_t index, Args&&... args) {
            if ((sizeof...(args) % 2) == 0) {
                static MsgPack::map_size_t sz;
                Packetizer::subscribe(index, [&](const uint8_t* data, const uint8_t size) {
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef();
                    unpacker->clear();
                    unpacker->feed(data, size);
                    unpacker->deserialize(sz, std::forward<Args>(args)...);
                });
            } else {
                LOG_WARNING("deserialize arg size must be even for map :", sizeof...(args));
            }
        }

        namespace detail {
            template <typename R, typename... Args>
            inline void subscribe(const uint8_t index, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(index,
                    [&, cb {std::move(callback)}](const uint8_t* data, const uint8_t size) {
                        auto unpacker = UnpackerManager::getInstance().getUnpackerRef();
                        unpacker->clear();
                        unpacker->feed(data, size);
                        std::tuple<std::remove_cvref_t<Args>...> t;
                        unpacker->to_tuple(t);
                        std::apply(cb, t);
                    });
            }

            template <typename R, typename... Args>
            inline void subscribe(std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(
                    [&, cb {std::move(callback)}](const uint8_t index, const uint8_t* data, const uint8_t size) {
                        auto unpacker = UnpackerManager::getInstance().getUnpackerRef();
                        unpacker->clear();
                        unpacker->feed(data, size);
                        cb(index, *unpacker);
                    });
            }
        }  // namespace detail

        template <typename F>
        inline auto subscribe(const uint8_t index, F&& callback)
            -> std::enable_if_t<arx::is_callable<F>::value> {
            detail::subscribe(index, arx::function_traits<F>::cast(std::move(callback)));
        }

        template <typename F>
        inline auto subscribe(F&& callback)
            -> std::enable_if_t<arx::is_callable<F>::value> {
            detail::subscribe(arx::function_traits<F>::cast(std::move(callback)));
        }

        void unsubscribe(const uint8_t index) {
            Packetizer::unsubscribe(index);
        }

        inline void feed(const uint8_t* data, const size_t size) {
            Packetizer::feed(data, size);
        }
    }  // namespace msgpacketizer
}  // namespace serial
}  // namespace ht

namespace MsgPacketizer = ht::serial::msgpacketizer;

#include "util/MsgPack/MsgPack/util/DebugLog/DebugLogRestoreState.h"

#endif  // HT_SERIAL_MSGPACKETIZER_H
