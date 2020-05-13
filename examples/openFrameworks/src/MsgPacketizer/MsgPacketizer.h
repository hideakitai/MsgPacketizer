#pragma once

#ifndef HT_SERIAL_MSGPACKETIZER_H
#define HT_SERIAL_MSGPACKETIZER_H

#if defined(ARDUINO_ARCH_AVR)\
 || defined(ARDUINO_ARCH_MEGAAVR)\
 || defined(ARDUINO_ARCH_SAMD)\
 || defined(ARDUINO_spresense_ast)
#define HT_SERIAL_MSGPACKETIZER_DISABLE_STL
#endif

#include <util/Packetizer/Packetizer.h>
#include <util/MsgPack/MsgPack.h>

namespace ht {
namespace serial {
namespace msgpacketizer {

#ifdef ARDUINO
    using StreamType = Stream;
#elif defined (OF_VERSION_MAJOR)
    using StreamType = ofSerial;
#endif

#ifdef HT_SERIAL_MSGPACKETIZER_DISABLE_STL
    using DecoderRef = arx::shared_ptr<MsgPack::Unpacker>;
    using DecoderMap = arx::map<StreamType*, DecoderRef, PACKETIZER_MAX_STREAM_MAP_SIZE>;
    using namespace arx;
#else
    using DecoderRef = std::shared_ptr<MsgPack::Unpacker>;
    using DecoderMap = std::map<StreamType*, DecoderRef>;
    using namespace std;
#endif // HT_SERIAL_MSGPACKETIZER_DISABLE_STL


    class EncodeManager
    {
        EncodeManager() {}
        EncodeManager(const EncodeManager&) = delete;
        EncodeManager& operator=(const EncodeManager&) = delete;

        MsgPack::Packer encoder;

    public:

        static EncodeManager& getInstance()
        {
            static EncodeManager m;
            return m;
        }

        MsgPack::Packer& getEncoder()
        {
            return encoder;
        }
    };

    template <typename... Args>
    inline void send(StreamType& stream, const uint8_t index, Args&&... args)
    {
        auto& packer = EncodeManager::getInstance().getEncoder();
        packer.clear();
        packer.encode(std::forward<Args>(args)...);
        Packetizer::send(stream, index, packer.data(), packer.size());
    }

    inline void send(StreamType& stream, const uint8_t index, const uint8_t* data, const uint8_t size)
    {
        auto& packer = EncodeManager::getInstance().getEncoder();
        packer.clear();
        packer.encode(data, size);
        Packetizer::send(stream, index, packer.data(), packer.size());
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

        DecoderRef getDecoderRef(const StreamType& stream)
        {
            StreamType* s = (StreamType*)&stream;
            if (decoders.find(s) == decoders.end())
                decoders.insert(make_pair(s, make_shared<MsgPack::Unpacker>()));
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


    template <typename... Args>
    inline void subscribe(StreamType& stream, const uint8_t index, Args&... args)
    {
        Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size)
        {
            auto unpacker = DecodeManager::getInstance().getDecoderRef(stream);
            unpacker->clear();
            unpacker->feed(data, size);
            unpacker->decode(args...);
        });
    }

    namespace detail
    {
        template <typename R, typename... Args>
        inline void subscribe(StreamType& stream, const uint8_t index, std::function<R(Args...)>&& callback)
        {
            Packetizer::subscribe(stream, index,
                [&, cb {std::move(callback)}](const uint8_t* data, const uint8_t size)
                {
                    auto unpacker = DecodeManager::getInstance().getDecoderRef(stream);
                    unpacker->clear();
                    unpacker->feed(data, size);
                    std::tuple<std::remove_cvref_t<Args>...> t;
                    unpacker->decodeTo(t);
                    std::apply(cb, t);
                }
            );
        }

        template <typename R, typename... Args>
        inline void subscribe(StreamType& stream, std::function<R(Args...)>&& callback)
        {
            Packetizer::subscribe(stream,
                [&, cb {std::move(callback)}](const uint8_t index, const uint8_t* data, const uint8_t size)
                {
                    auto unpacker = DecodeManager::getInstance().getDecoderRef(stream);
                    unpacker->clear();
                    unpacker->feed(data, size);
                    cb(index, *unpacker);
                }
            );
        }
    }

    template <typename F>
    inline auto subscribe(StreamType& stream, const uint8_t index, F&& callback)
    -> std::enable_if_t<arx::is_callable<F>::value>
    {
        detail::subscribe(stream, index, arx::function_traits<F>::cast(std::move(callback)));
    }

    template <typename F>
    inline auto subscribe(StreamType& stream, F&& callback)
    -> std::enable_if_t<arx::is_callable<F>::value>
    {
        detail::subscribe(stream, arx::function_traits<F>::cast(std::move(callback)));
    }


    inline void parse(bool b_exec_cb = true)
    {
        Packetizer::parse(b_exec_cb);
    }

} // namespace msgpacketizer
} // namespace serial
} // namespace ht

namespace MsgPacketizer = ht::serial::msgpacketizer;

#endif // HT_SERIAL_MSGPACKETIZER_H
