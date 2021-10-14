#pragma once

#ifndef HT_SERIAL_MSGPACKETIZER_SUBSCRIBER_H
#define HT_SERIAL_MSGPACKETIZER_SUBSCRIBER_H

namespace arduino {
namespace msgpack {
    namespace msgpacketizer {

        struct DecodeTargetStream;
        using UnpackerRef = std::shared_ptr<MsgPack::Unpacker>;

#ifdef MSGPACKETIZER_ENABLE_STREAM
#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L  // Have libstdc++11
        using UnpackerMap = std::map<DecodeTargetStream, UnpackerRef>;
#else
        using UnpackerMap = arx::map<DecodeTargetStream, UnpackerRef, PACKETIZER_MAX_STREAM_MAP_SIZE>;
#endif
#endif  // MSGPACKETIZER_ENABLE_STREAM

#ifdef ARDUINOJSON_VERSION
#ifndef MSGPACKETIZER_ARDUINOJSON_DESERIALIZE_BUFFER_SCALE
#define MSGPACKETIZER_ARDUINOJSON_DESERIALIZE_BUFFER_SCALE 3
#endif
#endif

#ifdef MSGPACKETIZER_ENABLE_STREAM

        struct DecodeTargetStream {
            StreamType* stream;
            TargetStreamType type;

            DecodeTargetStream() : stream(nullptr), type(TargetStreamType::STREAM_SERIAL) {}
            DecodeTargetStream(const DecodeTargetStream& dest) : stream(dest.stream), type(dest.type) {}
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
                if (!decoder) decoder = std::make_shared<MsgPack::Unpacker>();
                return decoder;
            }

#ifdef MSGPACKETIZER_ENABLE_STREAM

            const UnpackerMap& getUnpackerMap() const {
                return decoders;
            }

            UnpackerMap& getUnpackerMap() {
                return decoders;
            }

            UnpackerRef getUnpackerRef(const StreamType& stream) {
                auto s = getDecodeTargetStream(stream);
                if (decoders.find(s) == decoders.end())
                    decoders.insert(make_pair(s, std::make_shared<MsgPack::Unpacker>()));
                return decoders[s];
            }

            DecodeTargetStream getDecodeTargetStream(const StreamType& stream) {
                DecodeTargetStream s;
                s.stream = (StreamType*)&stream;
                s.type = TargetStreamType::STREAM_SERIAL;
                return s;
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

#endif  // MSGPACKETIZER_ENABLE_NETWORK
#endif  // MSGPACKETIZER_ENABLE_STREAM
        };

#ifdef ARDUINOJSON_VERSION

        namespace detail {
            template <size_t N>
            inline void subscribe_staticjson(
                const uint8_t* data, const std::function<void(const StaticJsonDocument<N>&)>& callback) {
                StaticJsonDocument<N> doc;
                auto err = deserializeMsgPack(doc, data);
                if (err) {
                    LOG_ERROR(F("deserializeJson() faled: "), err.c_str());
                } else {
                    callback(doc);
                }
            }
            inline void deserialize_dynamicjson(
                const uint8_t* data,
                const size_t size,
                const std::function<void(const DynamicJsonDocument&)>& callback) {
                DynamicJsonDocument doc(size * MSGPACKETIZER_ARDUINOJSON_DESERIALIZE_BUFFER_SCALE);
                auto err = deserializeMsgPack(doc, data);
                if (err) {
                    LOG_ERROR(F("deserializeJson() faled: "), err.c_str());
                } else {
                    callback(doc);
                }
            }
            template <size_t N>
            inline void subscribe_staticjson_index(
                const uint8_t index,
                const uint8_t* data,
                const std::function<void(uint8_t, const StaticJsonDocument<N>&)>& callback) {
                StaticJsonDocument<N> doc;
                auto err = deserializeMsgPack(doc, data);
                if (err) {
                    LOG_ERROR(F("deserializeJson() faled: "), err.c_str());
                } else {
                    callback(index, doc);
                }
            }
            inline void deserialize_dynamicjson_index(
                const uint8_t index,
                const uint8_t* data,
                const size_t size,
                const std::function<void(uint8_t, const DynamicJsonDocument&)>& callback) {
                DynamicJsonDocument doc(size * MSGPACKETIZER_ARDUINOJSON_DESERIALIZE_BUFFER_SCALE);
                auto err = deserializeMsgPack(doc, data);
                if (err) {
                    LOG_ERROR(F("deserializeJson() faled: "), err.c_str());
                } else {
                    callback(index, doc);
                }
            }
        }  // namespace detail

#endif  // ARDUINOJSON_VERSION

        // ----- for unsupported communication interface with manual operation -----

        // bind variables directly to specified index packet
        template <typename... Args>
        inline void subscribe_manual(const uint8_t index, Args&&... args) {
            Packetizer::subscribe(index, [&](const uint8_t* data, const size_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef();
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(std::forward<Args>(args)...);
            });
        }

        // bind variables directly to specified index packet with array format
        template <typename... Args>
        inline void subscribe_manual_arr(const uint8_t index, Args&&... args) {
            static MsgPack::arr_size_t sz;
            Packetizer::subscribe(index, [&](const uint8_t* data, const size_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef();
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(sz, std::forward<Args>(args)...);
            });
        }

        // bind variables directly to specified index packet with map format
        template <typename... Args>
        inline void subscribe_manual_map(const uint8_t index, Args&&... args) {
            if ((sizeof...(args) % 2) == 0) {
                static MsgPack::map_size_t sz;
                Packetizer::subscribe(index, [&](const uint8_t* data, const size_t size) {
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef();
                    unpacker->clear();
                    unpacker->feed(data, size);
                    unpacker->deserialize(sz, std::forward<Args>(args)...);
                });
            } else {
                LOG_WARN(F("deserialize arg size must be even for map :"), sizeof...(args));
            }
        }

        namespace detail {
            template <typename R, typename... Args>
            inline void subscribe_manual(const uint8_t index, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(index, [&, callback](const uint8_t* data, const size_t size) {
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef();
                    unpacker->clear();
                    unpacker->feed(data, size);
                    std::tuple<std::remove_cvref_t<Args>...> t;
                    unpacker->to_tuple(t);
                    std::apply(callback, t);
                });
            }

            template <typename R, typename... Args>
            inline void subscribe_manual(std::function<R(Args...)>&& callback) {
                Packetizer::subscribe([&, callback](const uint8_t index, const uint8_t* data, const size_t size) {
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef();
                    unpacker->clear();
                    unpacker->feed(data, size);
                    callback(index, *unpacker);
                });
            }

#ifdef ARDUINOJSON_VERSION

            template <size_t N>
            inline void subscribe_manual(
                const uint8_t index, std::function<void(const StaticJsonDocument<N>&)>&& callback) {
                Packetizer::subscribe(
                    index, [&, callback](const uint8_t* data, const size_t) { subscribe_staticjson(data, callback); });
            }
            inline void subscribe_manual(
                const uint8_t index, std::function<void(const DynamicJsonDocument&)>&& callback) {
                Packetizer::subscribe(index, [&, callback](const uint8_t* data, const size_t size) {
                    deserialize_dynamicjson(data, size, callback);
                });
            }

            template <size_t N>
            inline void subscribe_manual(std::function<void(const uint8_t, const StaticJsonDocument<N>&)>&& callback) {
                Packetizer::subscribe([&, callback](const uint8_t index, const uint8_t* data, const size_t) {
                    subscribe_staticjson_index(index, data, callback);
                });
            }
            inline void subscribe_manual(std::function<void(const uint8_t, const DynamicJsonDocument&)>&& callback) {
                Packetizer::subscribe([&, callback](const uint8_t index, const uint8_t* data, const size_t size) {
                    deserialize_dynamicjson_index(index, data, size, callback);
                });
            }

#endif  // ARDUINOJSON_VERSION

        }  // namespace detail

        // bind callback to specified index packet
        template <typename F>
        inline auto subscribe_manual(const uint8_t index, F&& callback)
            -> std::enable_if_t<arx::is_callable<F>::value> {
            detail::subscribe_manual(index, arx::function_traits<F>::cast(std::move(callback)));
        }

        // bind callback which is always called regardless of index
        template <typename F>
        inline auto subscribe_manual(F&& callback) -> std::enable_if_t<arx::is_callable<F>::value> {
            detail::subscribe_manual(arx::function_traits<F>::cast(std::move(callback)));
        }

        // unsubscribe
        inline void unsubscribe_manual(const uint8_t index) {
            Packetizer::unsubscribe(index);
        }

        // feed packet manually: must be called to manual decoding
        inline void feed(const uint8_t* data, const size_t size) {
            Packetizer::feed(data, size);
        }

#ifdef MSGPACKETIZER_ENABLE_STREAM

        // ----- for supported communication interface (Arduino, oF, ROS) -----

        template <typename S, typename... Args>
        inline void subscribe(S& stream, const uint8_t index, Args&&... args) {
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const size_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(std::forward<Args>(args)...);
            });
        }

        template <typename S, typename... Args>
        inline void subscribe_arr(S& stream, const uint8_t index, Args&&... args) {
            static MsgPack::arr_size_t sz;
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const size_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(sz, std::forward<Args>(args)...);
            });
        }

        template <typename S, typename... Args>
        inline void subscribe_map(S& stream, const uint8_t index, Args&&... args) {
            if ((sizeof...(args) % 2) == 0) {
                static MsgPack::map_size_t sz;
                Packetizer::subscribe(stream, index, [&](const uint8_t* data, const size_t size) {
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                    unpacker->clear();
                    unpacker->feed(data, size);
                    unpacker->deserialize(sz, std::forward<Args>(args)...);
                });
            } else {
                LOG_WARN(F("deserialize arg size must be even for map :"), sizeof...(args));
            }
        }

        namespace detail {
            template <typename S, typename R, typename... Args>
            inline void subscribe(S& stream, const uint8_t index, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(stream, index, [&, callback](const uint8_t* data, const size_t size) {
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                    unpacker->clear();
                    unpacker->feed(data, size);
                    std::tuple<std::remove_cvref_t<Args>...> t;
                    unpacker->to_tuple(t);
                    std::apply(callback, t);
                });
            }

            template <typename S, typename R, typename... Args>
            inline void subscribe(S& stream, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(
                    stream, [&, callback](const uint8_t index, const uint8_t* data, const size_t size) {
                        auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                        unpacker->clear();
                        unpacker->feed(data, size);
                        callback(index, *unpacker);
                    });
            }

#ifdef ARDUINOJSON_VERSION

            template <typename S, size_t N>
            inline void subscribe(
                S& stream, const uint8_t index, std::function<void(const StaticJsonDocument<N>&)>&& callback) {
                Packetizer::subscribe(stream, index, [&, callback](const uint8_t* data, const size_t) {
                    subscribe_staticjson(data, callback);
                });
            }
            template <typename S>
            inline void subscribe(
                S& stream, const uint8_t index, std::function<void(const DynamicJsonDocument&)>&& callback) {
                Packetizer::subscribe(stream, index, [&, callback](const uint8_t* data, const size_t size) {
                    deserialize_dynamicjson(data, size, callback);
                });
            }

            template <typename S, size_t N>
            inline void subscribe(
                S& stream, std::function<void(const uint8_t, const StaticJsonDocument<N>&)>&& callback) {
                Packetizer::subscribe(stream, [&, callback](const uint8_t index, const uint8_t* data, const size_t) {
                    subscribe_staticjson_index(index, data, callback);
                });
            }
            template <typename S>
            inline void subscribe(
                S& stream, std::function<void(const uint8_t, const DynamicJsonDocument&)>&& callback) {
                Packetizer::subscribe(
                    stream, [&, callback](const uint8_t index, const uint8_t* data, const size_t size) {
                        deserialize_dynamicjson_index(index, data, size, callback);
                    });
            }

#endif  // ARDUINOJSON_VERSION

        }  // namespace detail

        template <typename S, typename F>
        inline auto subscribe(S& stream, const uint8_t index, F&& callback)
            -> std::enable_if_t<arx::is_callable<F>::value> {
            detail::subscribe(stream, index, arx::function_traits<F>::cast(std::move(callback)));
        }

        template <typename S, typename F>
        inline auto subscribe(S& stream, F&& callback) -> std::enable_if_t<arx::is_callable<F>::value> {
            detail::subscribe(stream, arx::function_traits<F>::cast(std::move(callback)));
        }

        template <typename S>
        inline void unsubscribe(const S& stream, const uint8_t index) {
            Packetizer::unsubscribe(stream, index);
        }

        template <typename S>
        inline void unsubscribe(const S& stream) {
            Packetizer::unsubscribe(stream);
        }

        template <typename S>
        inline UnpackerRef getUnpackerRef(const S& stream) {
            return UnpackerManager::getInstance().getUnpackerRef(stream);
        }

        inline UnpackerMap& getUnpackerMap() {
            return UnpackerManager::getInstance().getUnpackerMap();
        }

        inline void parse(bool b_exec_cb = true) {
            Packetizer::parse(b_exec_cb);
        }

        inline void update(bool b_exec_cb = true) {
            Packetizer::parse(b_exec_cb);
            PackerManager::getInstance().post();
        }

#endif  // MSGPACKETIZER_ENABLE_STREAM

    }  // namespace msgpacketizer
}  // namespace msgpack
}  // namespace arduino

#endif  // HT_SERIAL_MSGPACKETIZER_SUBSCRIBER_H
