#pragma once
#ifndef HT_SERIAL_PACKETIZER
#define HT_SERIAL_PACKETIZER

#ifdef ARDUINO
    #include <Arduino.h>
#endif

#if defined(ARDUINO)\
 || defined(OF_VERSION_MAJOR)
    #define PACKETIZER_ENABLE_STREAM
#endif

#include "Packetizer/util/ArxTypeTraits/ArxTypeTraits.h"
#include "Packetizer/util/ArxContainer/ArxContainer.h"
#include "Packetizer/util/ArxSmartPtr/ArxSmartPtr.h"
#include "Packetizer/Types.h"
#include "Packetizer/Encoding.h"

namespace ht {
namespace serial {
namespace packetizer {

    template <typename Encoding>
    class Encoder
    {
        EncoderBaseRef encoder {encoder_t<Encoding>::create()};

    public:

        size_t encode(const uint8_t index, const uint8_t* src, const size_t size, bool b_crc)
        {
            return encoder->encode(index, src, size, b_crc);
        }
        size_t encode(const uint8_t index, const uint8_t* src, const size_t size)
        {
            return encoder->encode(index, src, size);
        }
        size_t encode(const uint8_t* src, const size_t size, bool b_crc)
        {
            return encoder->encode(src, size, b_crc);
        }
        size_t encode(const uint8_t* src, const size_t size)
        {
            return encoder->encode(src, size);
        }

        const uint8_t* data() const { return encoder->data(); };
        size_t size() const { return encoder->size(); };
        const Packet& packet() const { return encoder->packet(); }

        void verifying(bool b)  { encoder->verifying(b); }
        bool verifying() const { return encoder->verifying(); }
    };


    template <typename Encoding = DefaultEncoding>
    class Decoder
    {
        DecoderBaseRef decoder {decoder_t<Encoding>::create()};
        PacketQueue packets;
        CallbackType cb_no_index;
        CallbackAlwaysType cb_always;
        CallbackMap callbacks;

    public:

        void subscribe(const CallbackType& func)
        {
            cb_no_index = func;
        }

        void subscribe(const CallbackAlwaysType& func)
        {
            cb_always = func;
        }

        void subscribe(const uint8_t index, const CallbackType& func)
        {
            callbacks.emplace(make_pair(index, func));
        }

        void unsubscribe()
        {
            if (indexing()) cb_always = nullptr;
            else            cb_no_index = nullptr;
        }

        void unsubscribe(uint8_t index)
        {
            callbacks.erase(index);
        }

        void feed(const uint8_t* const data, const size_t size, bool b_exec_cb = true)
        {
            for (size_t i = 0; i < size; ++i)
            {
                decoder->feed(data[i], packets);
                if (PACKETIZER_MAX_PACKET_QUEUE_SIZE != 0)
                    if (available() > PACKETIZER_MAX_PACKET_QUEUE_SIZE)
                        pop();

                if (available() && b_exec_cb) callback();
            }
        }

        void callback()
        {
            if (indexing())
            {
                if (!cb_always && callbacks.empty()) return;
                while(available())
                {
                    if (cb_always) cb_always(index(), data(), size());
                    auto it = callbacks.find(index());
                    if (it != callbacks.end()) it->second(data(), size());
                    pop();
                }
            }
            else
            {
                if (!cb_no_index) return;
                while(available())
                {
                    cb_no_index(data(), size());
                    pop();
                }
            }
        }

        void reset() { packets.clear(); decoder->reset(); }
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

        void options(bool b_index, bool b_crc) { indexing(b_index); verifying(b_crc); }
        void indexing(bool b)  { decoder->indexing(b); }
        void verifying(bool b)  { decoder->verifying(b); }

        bool indexing() const { return decoder->indexing(); }
        bool verifying() const { return decoder->verifying(); }
    };

    template <typename Encoding>
    using EncoderRef = std::shared_ptr<Encoder<Encoding>>;
    template <typename Encoding>
    using DecoderRef = std::shared_ptr<Decoder<Encoding>>;


    template <typename Encoding>
    class EncodeManager
    {
        EncodeManager() : encoder(std::make_shared<Encoder<Encoding>>()) {}
        EncodeManager(const EncodeManager&) = delete;
        EncodeManager& operator=(const EncodeManager&) = delete;

        EncoderRef<Encoding> encoder;

    public:

        static EncodeManager<Encoding>& getInstance()
        {
            static EncodeManager<Encoding> m;
            return m;
        }

        EncoderRef<Encoding> getEncoder()
        {
            return encoder;
        }
    };


    template <typename Encoding = DefaultEncoding>
    inline const Packet& encode(const uint8_t index, const uint8_t* data, const size_t size, const bool b_crc)
    {
        auto e = EncodeManager<Encoding>::getInstance().getEncoder();
        e->verifying(b_crc);
        e->encode(index, data, size);
        return e->packet();
    }

    template <typename Encoding = DefaultEncoding>
    inline const Packet& encode(const uint8_t index, const uint8_t* data, const size_t size)
    {
        auto e = EncodeManager<Encoding>::getInstance().getEncoder();
        e->encode(index, data, size);
        return e->packet();
    }

    template <typename Encoding = DefaultEncoding>
    inline const Packet& encode(const uint8_t* data, const size_t size, const bool b_crc)
    {
        auto e = EncodeManager<Encoding>::getInstance().getEncoder();
        e->verifying(b_crc);
        e->encode(data, size);
        return e->packet();
    }

    template <typename Encoding = DefaultEncoding>
    inline const Packet& encode(const uint8_t* data, const size_t size)
    {
        auto e = EncodeManager<Encoding>::getInstance().getEncoder();
        e->encode(data, size);
        return e->packet();
    }

    template <typename Encoding = DefaultEncoding>
    inline void encode_option(const bool b_crc)
    {
        auto e = EncodeManager<Encoding>::getInstance().getEncoder();
        e->verifying(b_crc);
    }

#ifdef PACKETIZER_ENABLE_STREAM

    template <typename Encoding = DefaultEncoding>
    inline void send(StreamType& stream, const uint8_t index, const uint8_t* data, const size_t size, const bool b_crc)
    {
        const auto& packet = encode<Encoding>(index, data, size, b_crc);
        PACKETIZER_STREAM_WRITE(stream, packet.data.data(), packet.data.size());
    }

    template <typename Encoding = DefaultEncoding>
    inline void send(StreamType& stream, const uint8_t index, const uint8_t* data, const size_t size)
    {
        const auto& packet = encode<Encoding>(index, data, size);
        PACKETIZER_STREAM_WRITE(stream, packet.data.data(), packet.data.size());
    }

    template <typename Encoding = DefaultEncoding>
    inline void send(StreamType& stream, const uint8_t* data, const size_t size, const bool b_crc)
    {
        const auto& packet = encode<Encoding>(data, size, b_crc);
        PACKETIZER_STREAM_WRITE(stream, packet.data.data(), packet.data.size());
    }

    template <typename Encoding = DefaultEncoding>
    inline void send(StreamType& stream, const uint8_t* data, const size_t size)
    {
        const auto& packet = encode<Encoding>(data, size);
        PACKETIZER_STREAM_WRITE(stream, packet.data.data(), packet.data.size());
    }

#endif // PACKETIZER_ENABLE_STREAM

    template <typename Encoding = DefaultEncoding>
    class DecodeManager
    {
        DecodeManager() {}
        DecodeManager(const DecodeManager&) = delete;
        DecodeManager& operator=(const DecodeManager&) = delete;

        DecoderRef<Encoding> decoder;

    public:

        static DecodeManager& getInstance()
        {
            static DecodeManager m;
            return m;
        }

        DecoderRef<Encoding> getDecoderRef()
        {
            if (!decoder) decoder = std::make_shared<Decoder<Encoding>>();
            return decoder;
        }

#ifdef PACKETIZER_ENABLE_STREAM

    private:

        DecoderMap<Encoding> decoders;

    public:

        DecoderRef<Encoding> getDecoderRef(const StreamType& stream)
        {
            StreamType* s = (StreamType*)&stream;
            if (decoders.find(s) == decoders.end())
                decoders.insert(make_pair(s, std::make_shared<Decoder<Encoding>>()));
            return decoders[s];
        }

        void options(const bool b_index, const bool b_crc)
        {
            decoder->indexing(b_index);
            decoder->verifying(b_crc);
            for (auto& d : decoders)
            {
                d.second->indexing(b_index);
                d.second->verifying(b_crc);
            }
        }

        void parse(const bool b_exec_cb = true)
        {
            for (auto& d : decoders)
            {
                while (d.first->available() > 0)
                {
                    const int size = d.first->available();
                    uint8_t* data = new uint8_t[size];
                    d.first->readBytes((char*)data, size);
                    d.second->feed(data, size, b_exec_cb);
                    delete[] data;
                }
            }
        }

        void reset()
        {
            decoder->reset();
            for (auto& d : decoders)
                d.second->reset();
        }

#endif // PACKETIZER_ENABLE_STREAM

    };

    template <typename Encoding = DefaultEncoding>
    inline const Packet& decode(const uint8_t* data, const size_t size)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
        decoder->reset();
        decoder->indexing(PACKETIZER_DEFAULT_INDEX_SETTING);
        decoder->verifying(PACKETIZER_DEFAULT_CRC_SETTING);
        decoder->feed(data, size);
        return decoder->packet();
    }

    template <typename Encoding = DefaultEncoding>
    inline const Packet& decode(const uint8_t* data, const size_t size, const bool b_crc)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
        decoder->reset();
        decoder->indexing(PACKETIZER_DEFAULT_INDEX_SETTING);
        decoder->verifying(b_crc);
        decoder->feed(data, size);
        return decoder->packet();
    }

    template <typename Encoding = DefaultEncoding>
    inline const Packet& decode(const uint8_t* data, const size_t size, const bool b_index, const bool b_crc)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
        decoder->reset();
        decoder->indexing(b_index);
        decoder->verifying(b_crc);
        decoder->feed(data, size);
        return decoder->packet();
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> decode_option(const bool b_index, const bool b_crc)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
        decoder->indexing(b_index);
        decoder->verifying(b_crc);
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> subscribe(const CallbackType& func)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
        decoder->subscribe(func);
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> subscribe(const CallbackAlwaysType& func)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
        decoder->subscribe(func);
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> subscribe(const uint8_t index, const CallbackType& func)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
        decoder->subscribe(index, func);
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> unsubscribe()
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
        decoder->unsubscribe();
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> unsubscribe(const uint8_t index)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
        decoder->unsubscribe(index);
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> feed(const uint8_t* data, const size_t size, bool b_exec_cb = true)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
        decoder->feed(data, size, b_exec_cb);
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> reset()
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef();
        decoder->reset();
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> getDecoderRef()
    {
        return DecodeManager<Encoding>::getInstance().getDecoderRef();
    }

#ifdef PACKETIZER_ENABLE_STREAM

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> options(const StreamType& stream, const bool b_index, const bool b_crc)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
        decoder->indexing(b_index);
        decoder->verifying(b_crc);
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> subscribe(const StreamType& stream, const CallbackType& func)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
        decoder->subscribe(func);
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> subscribe(const StreamType& stream, const CallbackAlwaysType& func)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
        decoder->subscribe(func);
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> subscribe(const StreamType& stream, const uint8_t index, const CallbackType& func)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
        decoder->subscribe(index, func);
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> unsubscribe(const StreamType& stream)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
        decoder->unsubscribe();
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> unsubscribe(const StreamType& stream, const uint8_t index)
    {
        auto decoder = DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
        decoder->unsubscribe(index);
        return decoder;
    }

    template <typename Encoding = DefaultEncoding>
    inline void parse(bool b_exec_cb = true)
    {
        DecodeManager<Encoding>::getInstance().parse(b_exec_cb);
    }

    template <typename Encoding = DefaultEncoding>
    inline DecoderRef<Encoding> getDecoderRef(const StreamType& stream)
    {
        return DecodeManager<Encoding>::getInstance().getDecoderRef(stream);
    }

#endif // PACKETIZER_ENABLE_STREAM

} // packetizer
} // serial
} // ht

namespace Packetizer = ht::serial::packetizer;

#endif // HT_SERIAL_PACKETIZER
