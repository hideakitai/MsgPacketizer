#pragma once
#ifndef HT_SERIAL_PACKETIZER_DECODER_H
#define HT_SERIAL_PACKETIZER_DECODER_H

namespace arduino {
    namespace packetizer {

        template <typename Encoding = DefaultEncoding>
        class Decoder {
            DecoderBaseRef decoder {decoder_t<Encoding>::create()};
            PacketQueue packets;
            CallbackType cb_no_index;
            CallbackAlwaysType cb_always;
            CallbackMap callbacks;

        public:
            void subscribe(const CallbackType& func) {
                cb_no_index = func;
            }
            void subscribe(const CallbackAlwaysType& func) {
                cb_always = func;
            }
            void subscribe(const uint8_t index, const CallbackType& func) {
                callbacks.emplace(make_pair(index, func));
            }

            void unsubscribe() {
                if (indexing())
                    cb_always = nullptr;
                else
                    cb_no_index = nullptr;
            }
            void unsubscribe(uint8_t index) {
                callbacks.erase(index);
            }

            void feed(const uint8_t* const data, const size_t size, bool b_exec_cb = true) {
                for (size_t i = 0; i < size; ++i) {
                    decoder->feed(data[i], packets);
                    if (PACKETIZER_MAX_PACKET_QUEUE_SIZE != 0)
                        if (available() > PACKETIZER_MAX_PACKET_QUEUE_SIZE)
                            pop();

                    if (available() && b_exec_cb) callback();
                }
            }

            void callback() {
                if (indexing()) {
                    if (!cb_always && callbacks.empty()) return;
                    while (available()) {
                        if (cb_always) cb_always(index(), data(), size());
                        auto it = callbacks.find(index());
                        if (it != callbacks.end()) it->second(data(), size());
                        pop();
                    }
                } else {
                    if (!cb_no_index) return;
                    while (available()) {
                        cb_no_index(data(), size());
                        pop();
                    }
                }
            }

            void reset() {
                packets.clear();
                decoder->reset();
            }
            bool parsing() const { return decoder->parsing(); }
            size_t available() const { return packets.size(); }
            void pop() { pop_front(); }
            void pop_front() { packets.pop_front(); }
            void pop_back() { packets.pop_back(); }

            const Packet& packet() const { return packets.front(); }
            uint8_t index() const { return packets.front().index; }
            size_t size() const { return packet().data.size(); }
            const uint8_t* data() const { return packet().data.data(); }
            uint8_t data(const uint8_t i) const { return packet().data[i]; }

            const Packet& packet_latest() const { return packets.back(); }
            uint8_t index_latest() const { return packets.back().index; }
            size_t size_latest() const { return packet_latest().data.size(); }
            const uint8_t* data_latest() const { return packet_latest().data.data(); }
            uint8_t data_latest(const uint8_t i) const { return packet_latest().data[i]; }

            uint32_t errors() const { return decoder->errors(); }

            void options(bool b_index, bool b_crc) {
                indexing(b_index);
                verifying(b_crc);
            }
            void indexing(bool b) { decoder->indexing(b); }
            void verifying(bool b) { decoder->verifying(b); }

            bool indexing() const { return decoder->indexing(); }
            bool verifying() const { return decoder->verifying(); }
        };

#ifdef PACKETIZER_ENABLE_STREAM

        struct DecodeTargetStream {
            StreamType* stream;
            DecodeTargetStreamType type;

            DecodeTargetStream()
            : stream(nullptr), type(DecodeTargetStreamType::STREAM_SERIAL) {}
            DecodeTargetStream(const DecodeTargetStream& dest)
            : stream(dest.stream), type(dest.type) {}
            DecodeTargetStream(DecodeTargetStream&& dest)
            : stream(std::move(dest.stream)), type(std::move(dest.type)) {}
            DecodeTargetStream(const StreamType& stream, const DecodeTargetStreamType type)
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

#endif  // PACKETIZER_ENABLE_STREAM

        template <typename Encoding = DefaultEncoding>
        class DecodeManager {
            DecodeManager() {}
            DecodeManager(const DecodeManager&) = delete;
            DecodeManager& operator=(const DecodeManager&) = delete;

            DecoderRef<Encoding> decoder;

        public:
            static DecodeManager& getInstance() {
                static DecodeManager m;
                return m;
            }

            DecoderRef<Encoding> getDecoderRef() {
                if (!decoder) decoder = std::make_shared<Decoder<Encoding>>();
                return decoder;
            }

#ifdef PACKETIZER_ENABLE_STREAM

            template <typename S>
            auto getDecoderRef(const S& stream)
                -> typename std::enable_if<is_base_of<StreamType, S>::value, DecoderRef<Encoding>>::type {
                auto s = getDecodeTargetStream(stream);
                if (decoders.find(s) == decoders.end())
                    decoders.insert(make_pair(s, std::make_shared<Decoder<Encoding>>()));
                return decoders[s];
            }

            void options(const bool b_index, const bool b_crc) {
                decoder->indexing(b_index);
                decoder->verifying(b_crc);
                for (auto& d : decoders) {
                    d.second->indexing(b_index);
                    d.second->verifying(b_crc);
                }
            }

            void parse(const bool b_exec_cb = true) {
                for (auto& d : decoders) {
                    const size_t size = stream_available(d.first.stream, d.first.type);
                    if (size) {
                        uint8_t* data = new uint8_t[size];
                        stream_read_to(d.first.stream, d.first.type, data, size);
                        d.second->feed(data, size, b_exec_cb);
                        delete[] data;
                    }
                }
            }

            void reset() {
                decoder->reset();
                for (auto& d : decoders)
                    d.second->reset();
            }

        private:
            DecoderMap<Encoding> decoders;

            DecodeTargetStream getDecodeTargetStream(const StreamType& stream) {
                DecodeTargetStream s;
                s.stream = (StreamType*)&stream;
                s.type = DecodeTargetStreamType::STREAM_SERIAL;
                return s;
            }

#ifdef PACKETIZER_ENABLE_NETWORK

            DecodeTargetStream getDecodeTargetStream(const UDP& stream) {
                DecodeTargetStream s;
                s.stream = (StreamType*)&stream;
                s.type = DecodeTargetStreamType::STREAM_UDP;
                return s;
            }
            DecodeTargetStream getDecodeTargetStream(const Client& stream) {
                DecodeTargetStream s;
                s.stream = (StreamType*)&stream;
                s.type = DecodeTargetStreamType::STREAM_TCP;
                return s;
            }

#endif

            size_t stream_available(StreamType* stream, const DecodeTargetStreamType type) const {
                switch (type) {
                    case DecodeTargetStreamType::STREAM_SERIAL:
                        return stream_available(stream);
                    case DecodeTargetStreamType::STREAM_UDP:
#ifdef PACKETIZER_ENABLE_NETWORK
                        return stream_available(reinterpret_cast<UDP*>(stream));
#else
                        return 0;
#endif
                    case DecodeTargetStreamType::STREAM_TCP:
#ifdef PACKETIZER_ENABLE_NETWORK
                        return stream_available(reinterpret_cast<Client*>(stream));
#else
                        return 0;
#endif
                    default:
                        return 0;
                }
            }
            size_t stream_available(StreamType* stream) const {
                return stream->available();
            }
#ifdef PACKETIZER_ENABLE_NETWORK
            size_t stream_available(UDP* stream) const {
                return stream->parsePacket();
            }
            size_t stream_available(Client* stream) const {
                return stream->available();
            }
#endif

            void stream_read_to(StreamType* stream, const DecodeTargetStreamType type, uint8_t* data, const size_t size) {
                switch (type) {
                    case DecodeTargetStreamType::STREAM_SERIAL:
                        stream_read_to(stream, data, size);
                        break;
                    case DecodeTargetStreamType::STREAM_UDP:
#ifdef PACKETIZER_ENABLE_NETWORK
                        stream_read_to(reinterpret_cast<UDP*>(stream), data, size);
#endif
                        break;
                    case DecodeTargetStreamType::STREAM_TCP:
#ifdef PACKETIZER_ENABLE_NETWORK
                        stream_read_to(reinterpret_cast<Client*>(stream), data, size);
#endif
                        break;
                    default:
                        break;
                }
            }
            void stream_read_to(StreamType* stream, uint8_t* data, const size_t size) {
#ifdef SERIAL_H  // serial for catkin (ROS)
                stream->read(data, size);
#else
                stream->readBytes((char*)data, size);
#endif
            }
#ifdef PACKETIZER_ENABLE_NETWORK
            void stream_read_to(UDP* stream, uint8_t* data, const size_t size) {
                stream->read(data, size);
            }
            void stream_read_to(Client* stream, uint8_t* data, const size_t size) {
                stream->readBytes(data, size);
            }
#endif

#endif  // PACKETIZER_ENABLE_STREAM
        };

        template <typename Encoding = DefaultEncoding>
        inline const Packet& decode(const uint8_t* data, const size_t size) {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
            decoder->reset();
            decoder->indexing(PACKETIZER_DEFAULT_INDEX_SETTING);
            decoder->verifying(PACKETIZER_DEFAULT_CRC_SETTING);
            decoder->feed(data, size);
            return decoder->packet();
        }

        template <typename Encoding = DefaultEncoding>
        inline const Packet& decode(const uint8_t* data, const size_t size, const bool b_crc) {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
            decoder->reset();
            decoder->indexing(PACKETIZER_DEFAULT_INDEX_SETTING);
            decoder->verifying(b_crc);
            decoder->feed(data, size);
            return decoder->packet();
        }

        template <typename Encoding = DefaultEncoding>
        inline const Packet& decode(const uint8_t* data, const size_t size, const bool b_index, const bool b_crc) {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
            decoder->reset();
            decoder->indexing(b_index);
            decoder->verifying(b_crc);
            decoder->feed(data, size);
            return decoder->packet();
        }

        template <typename Encoding = DefaultEncoding>
        inline DecoderRef<Encoding> decode_option(const bool b_index, const bool b_crc) {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
            decoder->indexing(b_index);
            decoder->verifying(b_crc);
            return decoder;
        }

        template <typename Encoding = DefaultEncoding>
        inline DecoderRef<Encoding> subscribe(const CallbackType& func) {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
            decoder->subscribe(func);
            return decoder;
        }

        template <typename Encoding = DefaultEncoding>
        inline DecoderRef<Encoding> subscribe(const CallbackAlwaysType& func) {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
            decoder->subscribe(func);
            return decoder;
        }

        template <typename Encoding = DefaultEncoding>
        inline DecoderRef<Encoding> subscribe(const uint8_t index, const CallbackType& func) {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
            decoder->subscribe(index, func);
            return decoder;
        }

        template <typename Encoding = DefaultEncoding>
        inline DecoderRef<Encoding> unsubscribe() {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
            decoder->unsubscribe();
            return decoder;
        }

        template <typename Encoding = DefaultEncoding>
        inline DecoderRef<Encoding> unsubscribe(const uint8_t index) {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
            decoder->unsubscribe(index);
            return decoder;
        }

        template <typename Encoding = DefaultEncoding>
        inline DecoderRef<Encoding> feed(const uint8_t* data, const size_t size, bool b_exec_cb = true) {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
            decoder->feed(data, size, b_exec_cb);
            return decoder;
        }

        template <typename Encoding = DefaultEncoding>
        inline DecoderRef<Encoding> reset() {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
            decoder->reset();
            return decoder;
        }

        template <typename Encoding = DefaultEncoding>
        inline DecoderRef<Encoding> getDecoderRef() {
            return DecodeManager<Encoding>::getInstance().getDecoderRef();
        }

#ifdef PACKETIZER_ENABLE_STREAM

        template <typename S, typename Encoding = DefaultEncoding>
        inline auto options(const S& stream, const bool b_index, const bool b_crc)
            -> typename std::enable_if<is_base_of<StreamType, S>::value, DecoderRef<Encoding>>::type {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
            decoder->indexing(b_index);
            decoder->verifying(b_crc);
            return decoder;
        }

        template <typename S, typename Encoding = DefaultEncoding>
        inline auto subscribe(const S& stream, const CallbackType& func)
            -> typename std::enable_if<is_base_of<StreamType, S>::value, DecoderRef<Encoding>>::type {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
            decoder->subscribe(func);
            return decoder;
        }

        template <typename S, typename Encoding = DefaultEncoding>
        inline auto subscribe(const S& stream, const CallbackAlwaysType& func)
            -> typename std::enable_if<is_base_of<StreamType, S>::value, DecoderRef<Encoding>>::type {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
            decoder->subscribe(func);
            return decoder;
        }

        template <typename S, typename Encoding = DefaultEncoding>
        inline auto subscribe(const S& stream, const uint8_t index, const CallbackType& func)
            -> typename std::enable_if<is_base_of<StreamType, S>::value, DecoderRef<Encoding>>::type {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
            decoder->subscribe(index, func);
            return decoder;
        }

        template <typename S, typename Encoding = DefaultEncoding>
        inline auto unsubscribe(const S& stream)
            -> typename std::enable_if<is_base_of<StreamType, S>::value, DecoderRef<Encoding>>::type {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
            decoder->unsubscribe();
            return decoder;
        }

        template <typename S, typename Encoding = DefaultEncoding>
        inline auto unsubscribe(const S& stream, const uint8_t index)
            -> typename std::enable_if<is_base_of<StreamType, S>::value, DecoderRef<Encoding>>::type {
            auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
            decoder->unsubscribe(index);
            return decoder;
        }

        template <typename S, typename Encoding = DefaultEncoding>
        inline auto getDecoderRef(const S& stream)
            -> typename std::enable_if<is_base_of<StreamType, S>::value, DecoderRef<Encoding>>::type {
            return DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
        }

        template <typename Encoding = DefaultEncoding>
        inline void parse(bool b_exec_cb = true) {
            DecodeManager<Encoding>::getInstance().parse(b_exec_cb);
        }

#endif  // PACKETIZER_ENABLE_STREAM

    }  // namespace packetizer
}  // namespace arduino

#endif  // HT_SERIAL_PACKETIZER_DECODER_H
