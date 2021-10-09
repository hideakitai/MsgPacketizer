#pragma once
#ifndef HT_SERIAL_PACKETIZER_ENCODER_H
#define HT_SERIAL_PACKETIZER_ENCODER_H

namespace arduino {
    namespace packetizer {

        template <typename Encoding>
        class Encoder {
            EncoderBaseRef encoder {encoder_t<Encoding>::create()};

        public:
            size_t encode(const uint8_t index, const uint8_t* src, const size_t size, bool b_crc) {
                return encoder->encode(index, src, size, b_crc);
            }
            size_t encode(const uint8_t index, const uint8_t* src, const size_t size) {
                return encoder->encode(index, src, size);
            }
            size_t encode(const uint8_t* src, const size_t size, bool b_crc) {
                return encoder->encode(src, size, b_crc);
            }
            size_t encode(const uint8_t* src, const size_t size) {
                return encoder->encode(src, size);
            }

            const uint8_t* data() const { return encoder->data(); };
            size_t size() const { return encoder->size(); };
            const Packet& packet() const { return encoder->packet(); }

            void verifying(bool b) { encoder->verifying(b); }
            bool verifying() const { return encoder->verifying(); }
        };

        template <typename Encoding>
        class EncodeManager {
            EncodeManager()
            : encoder(std::make_shared<Encoder<Encoding>>()) {}
            EncodeManager(const EncodeManager&) = delete;
            EncodeManager& operator=(const EncodeManager&) = delete;

            EncoderRef<Encoding> encoder;

        public:
            static EncodeManager<Encoding>& getInstance() {
                static EncodeManager<Encoding> m;
                return m;
            }

            EncoderRef<Encoding> getEncoder() {
                return encoder;
            }
        };

        template <typename Encoding = DefaultEncoding>
        inline const Packet& encode(const uint8_t index, const uint8_t* data, const size_t size, const bool b_crc) {
            auto e = EncodeManager<Encoding>::getInstance().getEncoder();
            e->verifying(b_crc);
            e->encode(index, data, size);
            return e->packet();
        }

        template <typename Encoding = DefaultEncoding>
        inline const Packet& encode(const uint8_t index, const uint8_t* data, const size_t size) {
            auto e = EncodeManager<Encoding>::getInstance().getEncoder();
            e->encode(index, data, size);
            return e->packet();
        }

        template <typename Encoding = DefaultEncoding>
        inline const Packet& encode(const uint8_t* data, const size_t size, const bool b_crc) {
            auto e = EncodeManager<Encoding>::getInstance().getEncoder();
            e->verifying(b_crc);
            e->encode(data, size);
            return e->packet();
        }

        template <typename Encoding = DefaultEncoding>
        inline const Packet& encode(const uint8_t* data, const size_t size) {
            auto e = EncodeManager<Encoding>::getInstance().getEncoder();
            e->encode(data, size);
            return e->packet();
        }

        template <typename Encoding = DefaultEncoding>
        inline void encode_option(const bool b_crc) {
            auto e = EncodeManager<Encoding>::getInstance().getEncoder();
            e->verifying(b_crc);
        }

#ifdef PACKETIZER_ENABLE_STREAM

        template <typename Encoding = DefaultEncoding>
        inline void send(StreamType& stream, const uint8_t index, const uint8_t* data, const size_t size, const bool b_crc) {
            const auto& packet = encode<Encoding>(index, data, size, b_crc);
            PACKETIZER_STREAM_WRITE(stream, packet.data.data(), packet.data.size());
        }

        template <typename Encoding = DefaultEncoding>
        inline void send(StreamType& stream, const uint8_t index, const uint8_t* data, const size_t size) {
            const auto& packet = encode<Encoding>(index, data, size);
            PACKETIZER_STREAM_WRITE(stream, packet.data.data(), packet.data.size());
        }

        template <typename Encoding = DefaultEncoding>
        inline void send(StreamType& stream, const uint8_t* data, const size_t size, const bool b_crc) {
            const auto& packet = encode<Encoding>(data, size, b_crc);
            PACKETIZER_STREAM_WRITE(stream, packet.data.data(), packet.data.size());
        }

        template <typename Encoding = DefaultEncoding>
        inline void send(StreamType& stream, const uint8_t* data, const size_t size) {
            const auto& packet = encode<Encoding>(data, size);
            PACKETIZER_STREAM_WRITE(stream, packet.data.data(), packet.data.size());
        }

#ifdef PACKETIZER_ENABLE_NETWORK

        namespace detail {
            template <typename Encoding = DefaultEncoding>
            inline void send_impl(UDP* stream, const String& ip, const uint16_t port, const uint8_t* data, const size_t size) {
                stream->beginPacket(ip.c_str(), port);
                stream->write(data, size);
                stream->endPacket();
            }
            template <typename Encoding = DefaultEncoding>
            inline void send_impl(UDP* stream, const IPAddress& ip, const uint16_t port, const uint8_t* data, const size_t size) {
                stream->beginPacket(ip, port);
                stream->write(data, size);
                stream->endPacket();
            }
            template <typename Encoding = DefaultEncoding>
            inline void send_impl(Client* stream, const uint8_t* data, const size_t size) {
                stream->write(data, size);
            }
        }  // namespace detail

        template <typename Encoding = DefaultEncoding>
        inline void send(UDP& stream, const String& ip, const uint16_t port, const uint8_t index, const uint8_t* data, const size_t size, const bool b_crc) {
            const auto& packet = encode<Encoding>(index, data, size, b_crc);
            detail::send_impl(&stream, ip, port, packet.data.data(), packet.data.size());
        }
        template <typename Encoding = DefaultEncoding>
        inline void send(UDP& stream, const String& ip, const uint16_t port, const uint8_t index, const uint8_t* data, const size_t size) {
            const auto& packet = encode<Encoding>(index, data, size);
            detail::send_impl(&stream, ip, port, packet.data.data(), packet.data.size());
        }
        template <typename Encoding = DefaultEncoding>
        inline void send(UDP& stream, const String& ip, const uint16_t port, const uint8_t* data, const size_t size, const bool b_crc) {
            const auto& packet = encode<Encoding>(data, size, b_crc);
            detail::send_impl(&stream, ip, port, packet.data.data(), packet.data.size());
        }
        template <typename Encoding = DefaultEncoding>
        inline void send(UDP& stream, const String& ip, const uint16_t port, const uint8_t* data, const size_t size) {
            const auto& packet = encode<Encoding>(data, size);
            detail::send_impl(&stream, ip, port, packet.data.data(), packet.data.size());
        }

        template <typename Encoding = DefaultEncoding>
        inline void send(UDP& stream, const IPAddress& ip, const uint16_t port, const uint8_t index, const uint8_t* data, const size_t size, const bool b_crc) {
            const auto& packet = encode<Encoding>(index, data, size, b_crc);
            detail::send_impl(&stream, ip, port, packet.data.data(), packet.data.size());
        }
        template <typename Encoding = DefaultEncoding>
        inline void send(UDP& stream, const IPAddress& ip, const uint16_t port, const uint8_t index, const uint8_t* data, const size_t size) {
            const auto& packet = encode<Encoding>(index, data, size);
            detail::send_impl(&stream, ip, port, packet.data.data(), packet.data.size());
        }
        template <typename Encoding = DefaultEncoding>
        inline void send(UDP& stream, const IPAddress& ip, const uint16_t port, const uint8_t* data, const size_t size, const bool b_crc) {
            const auto& packet = encode<Encoding>(data, size, b_crc);
            detail::send_impl(&stream, ip, port, packet.data.data(), packet.data.size());
        }
        template <typename Encoding = DefaultEncoding>
        inline void send(UDP& stream, const IPAddress& ip, const uint16_t port, const uint8_t* data, const size_t size) {
            const auto& packet = encode<Encoding>(data, size);
            detail::send_impl(&stream, ip, port, packet.data.data(), packet.data.size());
        }

        template <typename Encoding = DefaultEncoding>
        inline void send(Client& stream, const uint8_t index, const uint8_t* data, const size_t size, const bool b_crc) {
            const auto& packet = encode<Encoding>(index, data, size, b_crc);
            detail::send_impl(&stream, packet.data.data(), packet.data.size());
        }
        template <typename Encoding = DefaultEncoding>
        inline void send(Client& stream, const uint8_t index, const uint8_t* data, const size_t size) {
            const auto& packet = encode<Encoding>(index, data, size);
            detail::send_impl(&stream, packet.data.data(), packet.data.size());
        }
        template <typename Encoding = DefaultEncoding>
        inline void send(Client& stream, const uint8_t* data, const size_t size, const bool b_crc) {
            const auto& packet = encode<Encoding>(data, size, b_crc);
            detail::send_impl(&stream, packet.data.data(), packet.data.size());
        }
        template <typename Encoding = DefaultEncoding>
        inline void send(Client& stream, const uint8_t* data, const size_t size) {
            const auto& packet = encode<Encoding>(data, size);
            detail::send_impl(&stream, packet.data.data(), packet.data.size());
        }

#endif  // PACKETIZER_ENABLE_NETWORK
#endif  // PACKETIZER_ENABLE_STREAM

    }  // namespace packetizer
}  // namespace arduino

#endif  // HT_SERIAL_PACKETIZER_ENCODER_H
