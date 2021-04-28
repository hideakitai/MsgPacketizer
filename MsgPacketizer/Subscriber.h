#pragma once

#ifndef HT_SERIAL_MSGPACKETIZER_SUBSCRIBER_H
#define HT_SERIAL_MSGPACKETIZER_SUBSCRIBER_H

namespace ht {
namespace serial {
    namespace msgpacketizer {

        class DecodeTargetStream;
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

            const UnpackerMap& getUnpackerMap() const {
                return decoders;
            }

            UnpackerMap& getUnpackerMap() {
                return decoders;
            }

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
#endif  // MSGPACKETIZER_ENABLE_STREAM
        };

#ifdef ARDUINOJSON_VERSION

        namespace detail {
            template <size_t N>
            inline void subscribe_staticjson(const uint8_t* data, const std::function<void(const StaticJsonDocument<N>&)>& callback) {
                StaticJsonDocument<N> doc;
                auto err = deserializeMsgPack(doc, data);
                if (err) {
                    LOG_ERROR(F("deserializeJson() faled: "), err.c_str());
                } else {
                    callback(doc);
                }
            }
            inline void deserialize_dynamicjson(const uint8_t* data, const size_t size, const std::function<void(const DynamicJsonDocument&)>& callback) {
                DynamicJsonDocument doc(size * MSGPACKETIZER_ARDUINOJSON_DESERIALIZE_BUFFER_SCALE);
                auto err = deserializeMsgPack(doc, data);
                if (err) {
                    LOG_ERROR(F("deserializeJson() faled: "), err.c_str());
                } else {
                    callback(doc);
                }
            }
            template <size_t N>
            inline void subscribe_staticjson_index(const uint8_t index, const uint8_t* data, const std::function<void(uint8_t, const StaticJsonDocument<N>&)>& callback) {
                StaticJsonDocument<N> doc;
                auto err = deserializeMsgPack(doc, data);
                if (err) {
                    LOG_ERROR(F("deserializeJson() faled: "), err.c_str());
                } else {
                    callback(index, doc);
                }
            }
            inline void deserialize_dynamicjson_index(const uint8_t index, const uint8_t* data, const size_t size, const std::function<void(uint8_t, const DynamicJsonDocument&)>& callback) {
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

        // for unsupported communication interface with manual operation

        template <typename... Args>
        inline void subscribe(const uint8_t index, Args&&... args) {
            Packetizer::subscribe(index, [&](const uint8_t* data, const size_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef();
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(std::forward<Args>(args)...);
            });
        }

        template <typename... Args>
        inline void subscribe_arr(const uint8_t index, Args&&... args) {
            static MsgPack::arr_size_t sz;
            Packetizer::subscribe(index, [&](const uint8_t* data, const size_t size) {
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
                Packetizer::subscribe(index, [&](const uint8_t* data, const size_t size) {
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef();
                    unpacker->clear();
                    unpacker->feed(data, size);
                    unpacker->deserialize(sz, std::forward<Args>(args)...);
                });
            } else {
                LOG_WARNING(F("deserialize arg size must be even for map :"), sizeof...(args));
            }
        }

        namespace detail {
            template <typename R, typename... Args>
            inline void subscribe(const uint8_t index, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(index,
                    [&, cb {std::move(callback)}](const uint8_t* data, const size_t size) {
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
                    [&, cb {std::move(callback)}](const uint8_t index, const uint8_t* data, const size_t size) {
                        auto unpacker = UnpackerManager::getInstance().getUnpackerRef();
                        unpacker->clear();
                        unpacker->feed(data, size);
                        cb(index, *unpacker);
                    });
            }

#ifdef ARDUINOJSON_VERSION

            template <size_t N>
            inline void subscribe(const uint8_t index, std::function<void(const StaticJsonDocument<N>&)>&& callback) {
                Packetizer::subscribe(index,
                    [&, callback](const uint8_t* data, const size_t) {
                        subscribe_staticjson(data, callback);
                    });
            }
            inline void subscribe(const uint8_t index, std::function<void(const DynamicJsonDocument&)>&& callback) {
                Packetizer::subscribe(index,
                    [&, callback](const uint8_t* data, const size_t size) {
                        deserialize_dynamicjson(data, size, callback);
                    });
            }

            template <size_t N>
            inline void subscribe(std::function<void(const uint8_t, const StaticJsonDocument<N>&)>&& callback) {
                Packetizer::subscribe(
                    [&, callback](const uint8_t index, const uint8_t* data, const size_t) {
                        subscribe_staticjson_index(index, data, callback);
                    });
            }
            inline void subscribe(std::function<void(const uint8_t, const DynamicJsonDocument&)>&& callback) {
                Packetizer::subscribe(
                    [&, callback](const uint8_t index, const uint8_t* data, const size_t size) {
                        deserialize_dynamicjson_index(index, data, size, callback);
                    });
            }

#endif  // ARDUINOJSON_VERSION

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

#ifdef MSGPACKETIZER_ENABLE_STREAM

        // for supported communication interface (Arduino, oF)

        template <typename... Args>
        inline void subscribe(StreamType& stream, const uint8_t index, Args&&... args) {
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const size_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(std::forward<Args>(args)...);
            });
        }

        template <typename... Args>
        inline void subscribe_arr(StreamType& stream, const uint8_t index, Args&&... args) {
            static MsgPack::arr_size_t sz;
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const size_t size) {
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
                Packetizer::subscribe(stream, index, [&](const uint8_t* data, const size_t size) {
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                    unpacker->clear();
                    unpacker->feed(data, size);
                    unpacker->deserialize(sz, std::forward<Args>(args)...);
                });
            } else {
                LOG_WARNING(F("deserialize arg size must be even for map :"), sizeof...(args));
            }
        }

        namespace detail {
            template <typename R, typename... Args>
            inline void subscribe(StreamType& stream, const uint8_t index, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(stream, index,
                    [&, cb {std::move(callback)}](const uint8_t* data, const size_t size) {
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
                    [&, cb {std::move(callback)}](const uint8_t index, const uint8_t* data, const size_t size) {
                        auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                        unpacker->clear();
                        unpacker->feed(data, size);
                        cb(index, *unpacker);
                    });
            }

#ifdef ARDUINOJSON_VERSION

            template <size_t N>
            inline void subscribe(StreamType& stream, const uint8_t index, std::function<void(const StaticJsonDocument<N>&)>&& callback) {
                Packetizer::subscribe(stream, index,
                    [&, callback](const uint8_t* data, const size_t) {
                        subscribe_staticjson(data, callback);
                    });
            }
            inline void subscribe(StreamType& stream, const uint8_t index, std::function<void(const DynamicJsonDocument&)>&& callback) {
                Packetizer::subscribe(stream, index,
                    [&, callback](const uint8_t* data, const size_t size) {
                        deserialize_dynamicjson(data, size, callback);
                    });
            }

            template <size_t N>
            inline void subscribe(StreamType& stream, std::function<void(const uint8_t, const StaticJsonDocument<N>&)>&& callback) {
                Packetizer::subscribe(stream,
                    [&, callback](const uint8_t index, const uint8_t* data, const size_t) {
                        subscribe_staticjson_index(index, data, callback);
                    });
            }
            inline void subscribe(StreamType& stream, std::function<void(const uint8_t, const DynamicJsonDocument&)>&& callback) {
                Packetizer::subscribe(stream,
                    [&, callback](const uint8_t index, const uint8_t* data, const size_t size) {
                        deserialize_dynamicjson_index(index, data, size, callback);
                    });
            }

#endif  // ARDUINOJSON_VERSION

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
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const size_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(std::forward<Args>(args)...);
            });
        }
        template <typename... Args>
        inline void subscribe(Client& stream, const uint8_t index, Args&&... args) {
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const size_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(std::forward<Args>(args)...);
            });
        }

        template <typename... Args>
        inline void subscribe_arr(UDP& stream, const uint8_t index, Args&&... args) {
            static MsgPack::arr_size_t sz;
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const size_t size) {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(sz, std::forward<Args>(args)...);
            });
        }
        template <typename... Args>
        inline void subscribe_arr(Client& stream, const uint8_t index, Args&&... args) {
            static MsgPack::arr_size_t sz;
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const size_t size) {
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
                Packetizer::subscribe(stream, index, [&](const uint8_t* data, const size_t size) {
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                    unpacker->clear();
                    unpacker->feed(data, size);
                    unpacker->deserialize(sz, std::forward<Args>(args)...);
                });
            } else {
                LOG_WARNING(F("deserialize arg size must be even for map :"), sizeof...(args));
            }
        }
        template <typename... Args>
        inline void subscribe_map(Client& stream, const uint8_t index, Args&&... args) {
            if ((sizeof...(args) % 2) == 0) {
                static MsgPack::map_size_t sz;
                Packetizer::subscribe(stream, index, [&](const uint8_t* data, const size_t size) {
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                    unpacker->clear();
                    unpacker->feed(data, size);
                    unpacker->deserialize(sz, std::forward<Args>(args)...);
                });
            } else {
                LOG_WARNING(F("deserialize arg size must be even for map :"), sizeof...(args));
            }
        }

        namespace detail {
            template <typename R, typename... Args>
            inline void subscribe(UDP& stream, const uint8_t index, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(stream, index,
                    [&, cb {std::move(callback)}](const uint8_t* data, const size_t size) {
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
                    [&, cb {std::move(callback)}](const uint8_t* data, const size_t size) {
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
                    [&, cb {std::move(callback)}](const uint8_t index, const uint8_t* data, const size_t size) {
                        auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                        unpacker->clear();
                        unpacker->feed(data, size);
                        cb(index, *unpacker);
                    });
            }
            template <typename R, typename... Args>
            inline void subscribe(Client& stream, std::function<R(Args...)>&& callback) {
                Packetizer::subscribe(stream,
                    [&, cb {std::move(callback)}](const uint8_t index, const uint8_t* data, const size_t size) {
                        auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                        unpacker->clear();
                        unpacker->feed(data, size);
                        cb(index, *unpacker);
                    });
            }

#ifdef ARDUINOJSON_VERSION

            template <size_t N>
            inline void subscribe(UDP& stream, const uint8_t index, std::function<void(const StaticJsonDocument<N>&)>&& callback) {
                Packetizer::subscribe(stream, index,
                    [&, callback](const uint8_t* data, const size_t) {
                        subscribe_staticjson(data, callback);
                    });
            }
            inline void subscribe(UDP& stream, const uint8_t index, std::function<void(const DynamicJsonDocument&)>&& callback) {
                Packetizer::subscribe(stream, index,
                    [&, callback](const uint8_t* data, const size_t size) {
                        deserialize_dynamicjson(data, size, callback);
                    });
            }
            template <size_t N>
            inline void subscribe(Client& stream, const uint8_t index, std::function<void(const StaticJsonDocument<N>&)>&& callback) {
                Packetizer::subscribe(stream, index,
                    [&, callback](const uint8_t* data, const size_t) {
                        subscribe_staticjson(data, callback);
                    });
            }
            inline void subscribe(Client& stream, const uint8_t index, std::function<void(const DynamicJsonDocument&)>&& callback) {
                Packetizer::subscribe(stream, index,
                    [&, callback](const uint8_t* data, const size_t size) {
                        deserialize_dynamicjson(data, size, callback);
                    });
            }

            template <size_t N>
            inline void subscribe(UDP& stream, std::function<void(const uint8_t, const StaticJsonDocument<N>&)>&& callback) {
                Packetizer::subscribe(stream,
                    [&, callback](const uint8_t index, const uint8_t* data, const size_t) {
                        subscribe_staticjson_index(index, data, callback);
                    });
            }
            inline void subscribe(UDP& stream, std::function<void(const uint8_t, const DynamicJsonDocument&)>&& callback) {
                Packetizer::subscribe(stream,
                    [&, callback](const uint8_t index, const uint8_t* data, const size_t size) {
                        deserialize_dynamicjson_index(index, data, size, callback);
                    });
            }
            template <size_t N>
            inline void subscribe(Client& stream, std::function<void(const uint8_t, const StaticJsonDocument<N>&)>&& callback) {
                Packetizer::subscribe(stream,
                    [&, callback](const uint8_t index, const uint8_t* data, const size_t) {
                        subscribe_staticjson_index(index, data, callback);
                    });
            }
            inline void subscribe(Client& stream, std::function<void(const uint8_t, const DynamicJsonDocument&)>&& callback) {
                Packetizer::subscribe(stream,
                    [&, callback](const uint8_t index, const uint8_t* data, const size_t size) {
                        deserialize_dynamicjson_index(index, data, size, callback);
                    });
            }

#endif
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

    }  // namespace msgpacketizer
}  // namespace serial
}  // namespace ht

#endif  // HT_SERIAL_MSGPACKETIZER_SUBSCRIBER_H
