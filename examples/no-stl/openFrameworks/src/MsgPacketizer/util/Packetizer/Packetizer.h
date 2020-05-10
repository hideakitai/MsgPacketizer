#pragma once
#ifndef HT_SERIAL_PACKETIZER
#define HT_SERIAL_PACKETIZER

#if defined(ARDUINO_ARCH_AVR)\
 || defined(ARDUINO_ARCH_MEGAAVR)\
 || defined(ARDUINO_ARCH_SAMD)\
 || defined(ARDUINO_spresense_ast)
    #define PACKETIZER_DISABLE_STL
#endif

#if defined(ARDUINO)\
 || defined(OF_VERSION_MAJOR)
    #define PACKETIZER_ENABLE_STREAM
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

#include "util/CRCx/CRCx.h"

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

#ifndef PACKETIZER_START_BYTE
#define PACKETIZER_START_BYTE 0xC1
#endif // PACKETIZER_START_BYTE

class Decoder;
#ifdef PACKETIZER_DISABLE_STL

    #ifndef PACKETIZER_MAX_PACKET_QUEUE_SIZE
    #define PACKETIZER_MAX_PACKET_QUEUE_SIZE 2
    #endif // PACKETIZER_MAX_PACKET_QUEUE_SIZE

    #ifndef PACKETIZER_MAX_PACKET_BINARY_SIZE
    #define PACKETIZER_MAX_PACKET_BINARY_SIZE 128
    #endif // PACKETIZER_MAX_PACKET_BINARY_SIZE

    #ifndef PACKETIZER_MAX_CALLBACK_QUEUE_SIZE
    #define PACKETIZER_MAX_CALLBACK_QUEUE_SIZE 8
    #endif // PACKETIZER_MAX_CALLBACK_QUEUE_SIZE

    #ifndef PACKETIZER_MAX_STREAM_MAP_SIZE
    #define PACKETIZER_MAX_STREAM_MAP_SIZE 2
    #endif // PACKETIZER_MAX_STREAM_MAP_SIZE

    using BinaryBuffer = arx::vector<uint8_t, PACKETIZER_MAX_PACKET_BINARY_SIZE>;
    using EscapeBuffer = arx::deque<uint8_t, PACKETIZER_MAX_PACKET_BINARY_SIZE>;
    using PacketQueue = arx::deque<BinaryBuffer, PACKETIZER_MAX_PACKET_QUEUE_SIZE>;

    using CallbackType = std::function<void(const uint8_t* data, const uint8_t size)>;
    using CallbackAlwaysType = std::function<void(const uint8_t index, const uint8_t* data, const uint8_t size)>;
    using CallbackMap = arx::map<uint8_t, CallbackType, PACKETIZER_MAX_CALLBACK_QUEUE_SIZE>;

    using DecoderRef = arx::shared_ptr<Decoder>;
    using Packet = BinaryBuffer;
    #ifdef PACKETIZER_ENABLE_STREAM
        using DecoderMap = arx::map<StreamType*, DecoderRef, PACKETIZER_MAX_STREAM_MAP_SIZE>;
    #endif

    using namespace arx;

#else

    #ifndef PACKETIZER_MAX_PACKET_QUEUE_SIZE
    #define PACKETIZER_MAX_PACKET_QUEUE_SIZE 0
    #endif // PACKETIZER_MAX_PACKET_QUEUE_SIZE

    using BinaryBuffer = std::vector<uint8_t>;
    using EscapeBuffer = std::deque<uint8_t>;
    using PacketQueue = std::deque<BinaryBuffer>;

    using CallbackType = std::function<void(const uint8_t* data, const uint8_t size)>;
    using CallbackAlwaysType = std::function<void(const uint8_t index, const uint8_t* data, const uint8_t size)>;
    using CallbackMap = std::map<uint8_t, CallbackType>;

    using DecoderRef = std::shared_ptr<Decoder>;
    using Packet = BinaryBuffer;
    #ifdef PACKETIZER_ENABLE_STREAM
        using DecoderMap = std::map<StreamType*, DecoderRef>;
    #endif

    using namespace std;

#endif

    static constexpr uint8_t START_BYTE  {PACKETIZER_START_BYTE};
    static constexpr uint8_t FINISH_BYTE {START_BYTE + 1};
    static constexpr uint8_t ESCAPE_BYTE {START_BYTE + 2};
    static constexpr uint8_t ESCAPE_MASK {0x20};

    static constexpr uint8_t INDEX_OFFSET_INDEX = 1;
    static constexpr uint8_t INDEX_OFFSET_DATA = 2;
    static constexpr uint8_t INDEX_OFFSET_CRC_ESCAPE_FROM_END = 2;
    static constexpr uint8_t N_HEADER_SIZE = 2;
    static constexpr uint8_t N_FOOTER_SIZE = 1; // footer is not bufferred
    static constexpr uint8_t N_HEADER_FOOTER_SIZE = N_HEADER_SIZE + N_FOOTER_SIZE;

    struct endp {}; // for end of packet sign


    class Encoder
    {
        BinaryBuffer buffer;

    public:

        explicit Encoder(const uint8_t idx = 0) { init(idx); }

        void init(const uint8_t index = 0)
        {
            buffer.clear();
            append((uint8_t)START_BYTE, false);
            append((uint8_t)index);
        }

        // ---------- pack w/ variadic arguments ----------

        template <typename ...Rest>
        void pack(const uint8_t first, Rest&& ...args)
        {
            append((uint8_t)first);
            pack(std::forward<Rest>(args)...);
        }
        void pack()
        {
            footer();
        }

        // ---------- pack w/ data pointer and size ----------

        void pack(const uint8_t* const sbuf, const uint8_t size)
        {
            append((uint8_t*)sbuf, size);
            footer();
        }

        // ---------- pack w/ insertion operator ----------

        const endp& operator<< (const endp& e)
        {
            footer();
            return e; // dummy
        }
        Encoder& operator<< (const uint8_t arg)
        {
            append(arg);
            return *this;
        }

        // get packing info
        const BinaryBuffer& packet() const { return buffer; }
        size_t size() const { return buffer.size(); }
        const uint8_t* data() const { return buffer.data(); }

    private:

        void append(const uint8_t* const data, const uint8_t size, const bool b_escape = true)
        {
            if (b_escape)
            {
                EscapeBuffer escapes;
                for (uint8_t i = 0; i < size; ++i)
                    if (is_escape_byte(data[i]))
                        escapes.push_back(i);

                if (escapes.empty())
                    for (uint8_t i = 0; i < size; ++i) buffer.push_back(data[i]);
                else
                {
                    size_t start = 0;
                    while (!escapes.empty())
                    {
                        const uint8_t& idx = escapes.front();
                        append(data + start, idx - start);
                        append(data[idx], true);
                        start = idx + 1;
                        escapes.pop_front();
                    }
                    if (start < size) append(data + start, size - start);
                }
            }
            else
                for (uint8_t i = 0; i < size; ++i) buffer.push_back(data[i]);
        }

        void append(const uint8_t data, const bool b_escape = true)
        {
            if (b_escape && is_escape_byte(data))
            {
                buffer.push_back((uint8_t)ESCAPE_BYTE);
                buffer.push_back((uint8_t)(data ^ ESCAPE_MASK));
            }
            else
                buffer.push_back(data);
        }

        void footer()
        {
            append(crcx::crc8(buffer.data(), buffer.size()));
            append(FINISH_BYTE, false);
        }

        bool is_escape_byte(const uint8_t d) const
        {
            return ((d == START_BYTE) || (d == ESCAPE_BYTE) || (d == FINISH_BYTE));
        }

    };


    class Decoder
    {
        BinaryBuffer buffer;
        PacketQueue packets;
        CallbackMap callbacks;
        CallbackAlwaysType callback_always;

        bool b_parsing {false};
        bool b_escape {false};

        uint32_t err_count {0};

    public:

        void subscribe(const uint8_t index, const CallbackType& func)
        {
            callbacks.emplace(make_pair(index, func));
        }

        void subscribe(const CallbackAlwaysType& func)
        {
            callback_always = func;
        }

        void unsubscribe()
        {
            callback_always = nullptr;
        }

        void unsubscribe(uint8_t index)
        {
            callbacks.erase(index);
        }

        void feed(const uint8_t* const data, const size_t size, bool b_exec_cb = true)
        {
            for (size_t i = 0; i < size; ++i) feed(data[i], b_exec_cb);
        }

        void feed(const uint8_t d, bool b_exec_cb = true)
        {
            if (d == START_BYTE)
            {
                reset();
                buffer.push_back(d);
                b_parsing = true;
            }
            else if (b_parsing)
            {
                if (d == FINISH_BYTE)
                    decode();
                else if (b_parsing)
                    buffer.push_back(d);
            }

            if (available() && b_exec_cb) callback();
        }

        void callback()
        {
            if (!callback_always && callbacks.empty()) return;

            while(available())
            {
                if (callback_always) callback_always(index(), data(), size());
                auto it = callbacks.find(index());
                if (it != callbacks.end()) it->second(data(), size());
                pop();
            }
        }

        bool isParsing() const { return b_parsing; }
        size_t available() const { return packets.size(); }
        void pop() { packets.pop_front(); }
        void pop_back() { packets.pop_back(); }

        uint8_t index() const { return packets.front()[INDEX_OFFSET_INDEX]; }
        uint8_t size() const { return packets.front().size() - N_HEADER_FOOTER_SIZE; }
        uint8_t data(const uint8_t i) const { return data()[i]; }
        const uint8_t* data() const { return packets.front().data() + INDEX_OFFSET_DATA; }

        uint8_t index_back() const { return packets.back()[INDEX_OFFSET_INDEX]; }
        uint8_t size_back() const { return packets.back().size() - N_HEADER_FOOTER_SIZE; }
        uint8_t data_back(const uint8_t i) const { return data_back()[i]; }
        const uint8_t* data_back() const { return packets.back().data() + INDEX_OFFSET_DATA; }

        uint32_t errors() const { return err_count; }

    private:

        void decode()
        {
            if (isCrcMatched())
            {
                for (auto it = buffer.begin(); it != buffer.end(); ++it)
                {
                    if (*it == ESCAPE_BYTE)
                    {
                        it = buffer.erase(it);
                        *it = *it ^ ESCAPE_MASK;
                    }
                }
                packets.push_back(buffer);
            }
            else
                ++err_count;

            reset();

            if (PACKETIZER_MAX_PACKET_QUEUE_SIZE != 0)
                if (available() > PACKETIZER_MAX_PACKET_QUEUE_SIZE)
                    pop();
        }

        bool isCrcMatched()
        {
            uint8_t crc_received = buffer.back();
            uint8_t crc_offset_size = 1;
            if (*(buffer.end() - INDEX_OFFSET_CRC_ESCAPE_FROM_END) == ESCAPE_BYTE) // before CRC byte can be ESCAPE_BYTE only if CRC is escaped
            {
                crc_received ^= ESCAPE_MASK;
                crc_offset_size = 2;
            }

            uint8_t crc = crcx::crc8(buffer.data(), buffer.size() - crc_offset_size);
            return (crc == crc_received);
        }

        void reset()
        {
            buffer.clear();
            b_parsing = false;
        }
    };


    class EncodeManager
    {
        EncodeManager() {}
        EncodeManager(const EncodeManager&) = delete;
        EncodeManager& operator=(const EncodeManager&) = delete;

        Encoder encoder;

    public:

        static EncodeManager& getInstance()
        {
            static EncodeManager m;
            return m;
        }

        Encoder& getEncoder()
        {
            return encoder;
        }
    };

    inline const Packet& encode(const uint8_t index, const uint8_t* data, const uint8_t size)
    {
        auto& e = EncodeManager::getInstance().getEncoder();
        e.init(index);
        e.pack(data, size);
        return e.packet();
    }

    template <typename ...Rest>
    inline const Packet& encode(const uint8_t index, const uint8_t first, Rest&& ...args)
    {
        auto& e = EncodeManager::getInstance().getEncoder();
        e.init(index);
        return encode(e, first, std::forward<Rest>(args)...);
    }

    template <typename ...Rest>
    inline const Packet& encode(Encoder& p, const uint8_t first, Rest&& ...args)
    {
        p << first;
        return encode(p, std::forward<Rest>(args)...);
    }

    inline const Packet& encode(Encoder& p)
    {
        p << endp();
        return p.packet();
    }


#ifdef PACKETIZER_ENABLE_STREAM

    inline void send(StreamType& stream, const uint8_t index, const uint8_t* data, const uint8_t size)
    {
        const auto& packet = encode(index, data, size);
        PACKETIZER_STREAM_WRITE(stream, packet.data(), packet.size());
    }

    template <typename ...Args>
    inline void send(StreamType& stream, const uint8_t index, const uint8_t first, Args&& ...args)
    {
        const auto& packet = encode(index, first, std::forward<Args>(args)...);
        PACKETIZER_STREAM_WRITE(stream, packet.data(), packet.size());
    }


    class DecodeManager
    {
        DecodeManager() {}
        DecodeManager(const DecodeManager&) = delete;
        DecodeManager& operator=(const DecodeManager&) = delete;

        DecoderMap decoders;

    public:

        static DecodeManager& getInstance()
        {
            static DecodeManager m;
            return m;
        }

        DecoderRef subscribe(const StreamType& stream, const uint8_t index, const CallbackType& func)
        {
            auto decoder = getDecoderRef(stream);
            decoder->subscribe(index, func);
            return decoder;
        }

        DecoderRef subscribe(const StreamType& stream, const CallbackAlwaysType& func)
        {
            auto decoder = getDecoderRef(stream);
            decoder->subscribe(func);
            return decoder;
        }

        void parse(bool b_exec_cb = true)
        {
            for (auto& d : decoders)
            {
                while (const int size = d.first->available())
                {
                    uint8_t data[size];
                    d.first->readBytes((char*)data, size);
                    d.second->feed(data, size, b_exec_cb);
                }
            }
        }

        DecoderRef getDecoderRef(const StreamType& stream)
        {
            StreamType* s = (StreamType*)&stream;
            if (decoders.find(s) == decoders.end())
                decoders.insert(make_pair(s, make_shared<Decoder>()));
            return decoders[s];
        }

        const DecoderMap& getDecoderMap() const
        {
            return decoders;
        }

        DecoderMap& getDecoderMap()
        {
            return decoders;
        }

    };


    template <typename StreamType>
    DecoderRef subscribe(const StreamType& stream, uint8_t index, const CallbackType& func)
    {
        auto decoder = DecodeManager::getInstance().getDecoderRef(stream);
        decoder->subscribe(index, func);
        return decoder;
    }

    template <typename StreamType>
    DecoderRef subscribe(const StreamType& stream, const CallbackAlwaysType& func)
    {
        auto decoder = DecodeManager::getInstance().getDecoderRef(stream);
        decoder->subscribe(func);
        return decoder;
    }

    void parse(bool b_exec_cb = true)
    {
        DecodeManager::getInstance().parse(b_exec_cb);
    }

#endif // PACKETIZER_ENABLE_STREAM

} // packetizer
} // serial
} // ht

namespace Packetizer = ht::serial::packetizer;

#endif // HT_SERIAL_PACKETIZER
