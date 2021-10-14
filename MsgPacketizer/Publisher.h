#pragma once

#ifndef HT_SERIAL_MSGPACKETIZER_PUBLISHER_H
#define HT_SERIAL_MSGPACKETIZER_PUBLISHER_H

namespace arduino {
namespace msgpack {
    namespace msgpacketizer {

        namespace element {
            struct Base {
                uint32_t last_publish_us {0};
                uint32_t interval_us {33333};  // 30 fps

                bool next() const {
                    return MSGPACKETIZER_ELAPSED_MICROS() >= (last_publish_us + interval_us);
                }
                void setFrameRate(float fps) {
                    interval_us = (uint32_t)(1000000.f / fps);
                }
                void setIntervalUsec(const uint32_t us) {
                    interval_us = us;
                }
                void setIntervalMsec(const float ms) {
                    interval_us = (uint32_t)(ms * 1000.f);
                }
                void setIntervalSec(const float sec) {
                    interval_us = (uint32_t)(sec * 1000.f * 1000.f);
                }

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
                Value(T& t) : t(t) {}
                virtual ~Value() {}
                virtual void encodeTo(MsgPack::Packer& p) override {
                    p.pack(t);
                }
            };

            template <typename T>
            class Const : public Base {
                const T t;

            public:
                Const(const T& t) : t(t) {}
                virtual ~Const() {}
                virtual void encodeTo(MsgPack::Packer& p) override {
                    p.pack(t);
                }
            };

            template <typename T>
            class Function : public Base {
                std::function<T()> getter;

            public:
                Function(const std::function<T()>& getter) : getter(getter) {}
                virtual ~Function() {}
                virtual void encodeTo(MsgPack::Packer& p) override {
                    p.pack(getter());
                }
            };

            class Tuple : public Base {
                TupleRef ts;

            public:
                Tuple(TupleRef&& ts) : ts(std::move(ts)) {}
                virtual ~Tuple() {}
                virtual void encodeTo(MsgPack::Packer& p) override {
                    for (auto& t : ts) t->encodeTo(p);
                }
            };

        }  // namespace element

        using PublishElementRef = element::Ref;
        using ElementTupleRef = element::TupleRef;

        template <typename T>
        inline auto make_element_ref(T& value) -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
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

        struct Destination {
            StreamType* stream;
            TargetStreamType type;
            uint8_t index;
            str_t ip;
            uint16_t port;

            Destination() {}
            Destination(const Destination& dest)
            : stream(dest.stream), type(dest.type), index(dest.index), ip(dest.ip), port(dest.port) {}
            Destination(Destination&& dest)
            : stream(std::move(dest.stream))
            , type(std::move(dest.type))
            , index(std::move(dest.index))
            , ip(std::move(dest.ip))
            , port(std::move(dest.port)) {}
            Destination(const StreamType& stream, const TargetStreamType type, const uint8_t index)
            : stream((StreamType*)&stream), type(type), index(index), ip(), port() {}
            Destination(
                const StreamType& stream,
                const TargetStreamType type,
                const uint8_t index,
                const str_t& ip,
                const uint16_t port)
            : stream((StreamType*)&stream), type(type), index(index), ip(ip), port(port) {}

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
                     : (type != rhs.type)     ? (type < rhs.type)
                     : (index != rhs.index)   ? (index < rhs.index)
                     : (ip != rhs.ip)         ? (ip < rhs.ip)
                                              : (port < rhs.port);
            }
            inline bool operator==(const Destination& rhs) const {
                return (stream == rhs.stream) && (type == rhs.type) && (index == rhs.index) && (ip == rhs.ip)
                    && (port == rhs.port);
            }
            inline bool operator!=(const Destination& rhs) const {
                return !(*this == rhs);
            }
        };

#endif  // MSGPACKETIZER_ENABLE_STREAM

#ifdef MSGPACKETIZER_ENABLE_STREAM
#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L  // Have libstdc++11
        using PackerMap = std::map<Destination, PublishElementRef>;
#else
        using PackerMap = arx::map<Destination, PublishElementRef, MSGPACKETIZER_MAX_PUBLISH_DESTINATION_SIZE>;
#endif
#endif  // MSGPACKETIZER_ENABLE_STREAM

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
#ifdef MSGPACKETIZER_ENABLE_NETWORK
                    case TargetStreamType::STREAM_UDP:
                        Packetizer::send(
                            *reinterpret_cast<UDP*>(dest.stream),
                            dest.ip,
                            dest.port,
                            dest.index,
                            encoder.data(),
                            encoder.size());
                        break;
                    case TargetStreamType::STREAM_TCP:
                        Packetizer::send(
                            *reinterpret_cast<Client*>(dest.stream), dest.index, encoder.data(), encoder.size());
                        break;
#endif
                    default:
                        LOG_ERROR(F("This communication I/F is not supported"));
                        break;
                }
            }

            void post() {
                for (auto& mp : addr_map) {
                    if (mp.second->next()) {
                        mp.second->last_publish_us = MSGPACKETIZER_ELAPSED_MICROS();
                        send(mp.first, mp.second);
                    }
                }
            }

            // for Serial and TCP (Client)

            template <typename S>
            PublishElementRef publish(const S& stream, const uint8_t index, const char* const value) {
                return publish_impl(stream, index, make_element_ref(value));
            }

            template <typename S, typename T>
            auto publish(const S& stream, const uint8_t index, T& value)
                -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
                return publish_impl(stream, index, make_element_ref(value));
            }

            template <typename S, typename T>
            auto publish(const S& stream, const uint8_t index, const T& value)
                -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
                return publish_impl(stream, index, make_element_ref(value));
            }

            template <typename S, typename Func>
            auto publish(const S& stream, const uint8_t index, Func&& func)
                -> std::enable_if_t<arx::is_callable<Func>::value, PublishElementRef> {
                return publish(stream, index, arx::function_traits<Func>::cast(func));
            }

            template <typename S, typename T>
            PublishElementRef publish(const S& stream, const uint8_t index, std::function<T()>&& getter) {
                return publish_impl(stream, index, make_element_ref(getter));
            }

            template <typename S, typename... Args>
            PublishElementRef publish(const S& stream, const uint8_t index, Args&&... args) {
                ElementTupleRef v {make_element_ref(std::forward<Args>(args))...};
                return publish_impl(stream, index, make_element_ref(v));
            }

            template <typename S, typename... Args>
            PublishElementRef publish_arr(const S& stream, const uint8_t index, Args&&... args) {
                static MsgPack::arr_size_t s(sizeof...(args));
                return publish(stream, index, s, std::forward<Args>(args)...);
            }

            template <typename S, typename... Args>
            PublishElementRef publish_map(const S& stream, const uint8_t index, Args&&... args) {
                if ((sizeof...(args) % 2) == 0) {
                    static MsgPack::map_size_t s(sizeof...(args) / 2);
                    return publish(stream, index, s, std::forward<Args>(args)...);
                } else {
                    LOG_WARN(F("serialize arg size must be even for map :"), sizeof...(args));
                    return nullptr;
                }
            }

            template <typename S>
            void unpublish(const S& stream, const uint8_t index) {
                Destination dest = getDestination(stream, index);
                addr_map.erase(dest);
            }

            template <typename S>
            PublishElementRef getPublishElementRef(const S& stream, const uint8_t index) {
                Destination dest = getDestination(stream, index);
                return addr_map[dest];
            }

#ifdef MSGPACKETIZER_ENABLE_NETWORK

            PublishElementRef publish(
                const UDP& stream,
                const str_t& ip,
                const uint16_t port,
                const uint8_t index,
                const char* const value) {
                return publish_impl(stream, ip, port, index, make_element_ref(value));
            }

            template <typename T>
            auto publish(const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, T& value)
                -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
                return publish_impl(stream, ip, port, index, make_element_ref(value));
            }

            template <typename T>
            auto publish(const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, const T& value)
                -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef> {
                return publish_impl(stream, ip, port, index, make_element_ref(value));
            }

            template <typename Func>
            auto publish(const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Func&& func)
                -> std::enable_if_t<arx::is_callable<Func>::value, PublishElementRef> {
                return publish(stream, ip, port, index, arx::function_traits<Func>::cast(func));
            }

            template <typename T>
            PublishElementRef publish(
                const UDP& stream,
                const str_t& ip,
                const uint16_t port,
                const uint8_t index,
                std::function<T()>&& getter) {
                return publish_impl(stream, ip, port, index, make_element_ref(getter));
            }

            template <typename... Args>
            PublishElementRef publish(
                const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args) {
                ElementTupleRef v {make_element_ref(std::forward<Args>(args))...};
                return publish_impl(stream, ip, port, index, make_element_ref(v));
            }

            template <typename... Args>
            PublishElementRef publish_arr(
                const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args) {
                static MsgPack::arr_size_t s(sizeof...(args));
                return publish(stream, ip, port, index, s, std::forward<Args>(args)...);
            }

            template <typename... Args>
            PublishElementRef publish_map(
                const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args) {
                if ((sizeof...(args) % 2) == 0) {
                    static MsgPack::map_size_t s(sizeof...(args) / 2);
                    return publish(stream, ip, port, index, s, std::forward<Args>(args)...);
                } else {
                    LOG_WARN(F("serialize arg size must be even for map :"), sizeof...(args));
                    return nullptr;
                }
            }

            void unpublish(const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index) {
                Destination dest = getDestination(stream, ip, port, index);
                addr_map.erase(dest);
            }

            PublishElementRef getPublishElementRef(
                const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index) {
                Destination dest = getDestination(stream, ip, port, index);
                return addr_map[dest];
            }

#endif  // MSGPACKETIZER_ENABLE_NETWORK

        private:
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

            Destination getDestination(const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index) {
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

            PublishElementRef publish_impl(
                const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, PublishElementRef ref) {
                Destination dest = getDestination(stream, ip, port, index);
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
                LOG_WARN(F("serialize arg size must be even for map :"), sizeof...(args));
                return Packetizer::encode(index, nullptr, 0);
            }
        }

#ifdef MSGPACKETIZER_ENABLE_STREAM

        template <typename S, typename... Args>
        inline void send(S& stream, const uint8_t index, Args&&... args) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.serialize(std::forward<Args>(args)...);
            Packetizer::send(stream, index, packer.data(), packer.size());
        }

        template <typename S>
        inline void send(S& stream, const uint8_t index, const uint8_t* data, const uint8_t size) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.pack(data, size);
            Packetizer::send(stream, index, packer.data(), packer.size());
        }

        template <typename S>
        inline void send(S& stream, const uint8_t index) {
            auto& packer = PackerManager::getInstance().getPacker();
            Packetizer::send(stream, index, packer.data(), packer.size());
        }

        template <typename S, typename... Args>
        inline void send_arr(S& stream, const uint8_t index, Args&&... args) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.serialize(MsgPack::arr_size_t(sizeof...(args)), std::forward<Args>(args)...);
            Packetizer::send(stream, index, packer.data(), packer.size());
        }

        template <typename S, typename... Args>
        inline void send_map(S& stream, const uint8_t index, Args&&... args) {
            if ((sizeof...(args) % 2) == 0) {
                auto& packer = PackerManager::getInstance().getPacker();
                packer.clear();
                packer.serialize(MsgPack::arr_size_t(sizeof...(args) / 2), std::forward<Args>(args)...);
                Packetizer::send(stream, index, packer.data(), packer.size());
            } else {
                LOG_WARN(F("serialize arg size must be even for map :"), sizeof...(args));
            }
        }

#ifdef MSGPACKETIZER_ENABLE_NETWORK

        template <typename... Args>
        inline void send(UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.serialize(std::forward<Args>(args)...);
            Packetizer::send(stream, ip, port, index, packer.data(), packer.size());
        }

        inline void send(
            UDP& stream,
            const str_t& ip,
            const uint16_t port,
            const uint8_t index,
            const uint8_t* data,
            const uint8_t size) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.pack(data, size);
            Packetizer::send(stream, ip, port, index, packer.data(), packer.size());
        }

        inline void send(UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index) {
            auto& packer = PackerManager::getInstance().getPacker();
            Packetizer::send(stream, ip, port, index, packer.data(), packer.size());
        }

        template <typename... Args>
        inline void send_arr(UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args) {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.serialize(MsgPack::arr_size_t(sizeof...(args)), std::forward<Args>(args)...);
            Packetizer::send(stream, ip, port, index, packer.data(), packer.size());
        }

        template <typename... Args>
        inline void send_map(UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args) {
            if ((sizeof...(args) % 2) == 0) {
                auto& packer = PackerManager::getInstance().getPacker();
                packer.clear();
                packer.serialize(MsgPack::arr_size_t(sizeof...(args) / 2), std::forward<Args>(args)...);
                Packetizer::send(stream, ip, port, index, packer.data(), packer.size());
            } else {
                LOG_WARN(F("serialize arg size must be even for map :"), sizeof...(args));
            }
        }

#endif  // MSGPACKETIZER_ENABLE_NETWORK

        template <typename S, typename... Args>
        inline PublishElementRef publish(const S& stream, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish(stream, index, std::forward<Args>(args)...);
        }

        template <typename S, typename... Args>
        inline PublishElementRef publish_arr(const S& stream, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish_arr(stream, index, std::forward<Args>(args)...);
        }

        template <typename S, typename... Args>
        inline PublishElementRef publish_map(const S& stream, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish_map(stream, index, std::forward<Args>(args)...);
        }

        template <typename S>
        inline void unpublish(const S& stream, const uint8_t index) {
            PackerManager::getInstance().unpublish(stream, index);
        }

        template <typename S>
        inline PublishElementRef getPublishElementRef(const S& stream, const uint8_t index) {
            return PackerManager::getInstance().getPublishElementRef(stream, index);
        }

#ifdef MSGPACKETIZER_ENABLE_NETWORK

        template <typename... Args>
        inline PublishElementRef publish(
            const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish(stream, ip, port, index, std::forward<Args>(args)...);
        }

        template <typename... Args>
        inline PublishElementRef publish_arr(
            const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish_arr(stream, ip, port, index, std::forward<Args>(args)...);
        }

        template <typename... Args>
        inline PublishElementRef publish_map(
            const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index, Args&&... args) {
            return PackerManager::getInstance().publish_map(stream, ip, port, index, std::forward<Args>(args)...);
        }

        inline void unpublish(const UDP& stream, const str_t& ip, const uint16_t port, const uint8_t index) {
            PackerManager::getInstance().unpublish(stream, ip, port, index);
        };

        inline PublishElementRef getPublishElementRef(const UDP& stream, const uint8_t index) {
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

    }  // namespace msgpacketizer
}  // namespace msgpack
}  // namespace arduino

#endif  // HT_SERIAL_MSGPACKETIZER_PUBLISHER_H
