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
    using UnpackerRef = arx::shared_ptr<MsgPack::Unpacker>;
    using UnpackerMap = arx::map<StreamType*, UnpackerRef, PACKETIZER_MAX_STREAM_MAP_SIZE>;
    using namespace arx;
#else
    using UnpackerRef = std::shared_ptr<MsgPack::Unpacker>;
    using UnpackerMap = std::map<StreamType*, UnpackerRef>;
    using namespace std;
#endif // HT_SERIAL_MSGPACKETIZER_DISABLE_STL


    class PackerManager
    {
        PackerManager() {}
        PackerManager(const PackerManager&) = delete;
        PackerManager& operator=(const PackerManager&) = delete;

        MsgPack::Packer encoder;

    public:

        static PackerManager& getInstance()
        {
            static PackerManager m;
            return m;
        }

        const MsgPack::Packer& getPacker() const
        {
            return encoder;
        }

        MsgPack::Packer& getPacker()
        {
            return encoder;
        }
    };

    template <typename... Args>
    inline void send(StreamType& stream, const uint8_t index, Args&&... args)
    {
        auto& packer = PackerManager::getInstance().getPacker();
        packer.clear();
        packer.serialize(std::forward<Args>(args)...);
        Packetizer::send(stream, index, packer.data(), packer.size());
    }

    inline void send(StreamType& stream, const uint8_t index, const uint8_t* data, const uint8_t size)
    {
        auto& packer = PackerManager::getInstance().getPacker();
        packer.clear();
        packer.serialize(data, size);
        Packetizer::send(stream, index, packer.data(), packer.size());
    }

    inline void send(StreamType& stream, const uint8_t index)
    {
        auto& packer = PackerManager::getInstance().getPacker();
        Packetizer::send(stream, index, packer.data(), packer.size());
    }

    const MsgPack::Packer& getPacker()
    {
        return PackerManager::getInstance().getPacker();
    }


    class UnpackerManager
    {
        UnpackerManager() {}
        UnpackerManager(const UnpackerManager&) = delete;
        UnpackerManager& operator=(const UnpackerManager&) = delete;

        UnpackerMap decoders;

    public:

        static UnpackerManager& getInstance()
        {
            static UnpackerManager m;
            return m;
        }

        UnpackerRef getUnpackerRef(const StreamType& stream)
        {
            StreamType* s = (StreamType*)&stream;
            if (decoders.find(s) == decoders.end())
                decoders.insert(make_pair(s, make_shared<MsgPack::Unpacker>()));
            return decoders[s];
        }

        const UnpackerMap& getUnpackerMap() const
        {
            return decoders;
        }

        UnpackerMap& getUnpackerMap()
        {
            return decoders;
        }
    };


    template <typename... Args>
    inline void subscribe(StreamType& stream, const uint8_t index, Args&... args)
    {
        Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size)
        {
            auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
            unpacker->clear();
            unpacker->feed(data, size);
            unpacker->deserialize(args...);
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
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                    unpacker->clear();
                    unpacker->feed(data, size);
                    std::tuple<std::remove_cvref_t<Args>...> t;
                    unpacker->deserializeToTuple(t);
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
                    auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
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


    UnpackerRef getUnpackerRef(const StreamType& stream)
    {
        return UnpackerManager::getInstance().getUnpackerRef(stream);
    }

    UnpackerMap& getUnpackerMap()
    {
        return UnpackerManager::getInstance().getUnpackerMap();
    }

} // namespace msgpacketizer
} // namespace serial
} // namespace ht

namespace MsgPacketizer = ht::serial::msgpacketizer;

#endif // HT_SERIAL_MSGPACKETIZER_H
