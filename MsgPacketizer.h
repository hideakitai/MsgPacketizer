#pragma once

#ifndef HT_SERIAL_MSGPACKETIZER_H
#define HT_SERIAL_MSGPACKETIZER_H


#include <util/ArxTypeTraits/ArxTypeTraits.h>
#include <Packetizer.h>
#include <MsgPack.h>


namespace ht {
namespace serial {
namespace msgpacketizer {

#ifdef ARDUINO
    using StreamType = Stream;
#elif defined (OF_VERSION_MAJOR)
    using StreamType = ofSerial;
#endif

    template <typename... Args>
    inline void subscribe(StreamType& stream, const uint8_t index, Args&... args)
    {
        Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size)
        {
            MsgPack::Unpacker unpacker;
            unpacker.feed(data, size);
            unpacker.decode(args...);
        });
    }

    namespace detail
    {
        template <typename R, typename... Args>
        inline void subscribe(StreamType& stream, const uint8_t index, const std::function<R(Args...)>& callback)
        {
            Packetizer::subscribe(stream, index, [&, callback](const uint8_t* data, const uint8_t size)
            {
                MsgPack::Unpacker unpacker;
                unpacker.feed(data, size);
                std::tuple<std::remove_cvref_t<Args>...> t;
                unpacker.decodeTo(t);
                std::apply(callback, t);
            });
        }
    }

    template <typename F>
    inline auto subscribe(StreamType& stream, const uint8_t index, const F& callback)
    -> std::enable_if_t<arx::is_callable<F>::value>
    {
        detail::subscribe(stream, index, arx::function_traits<F>::cast(callback));
    }

    inline void subscribe(StreamType& stream, std::function<void(const uint8_t, MsgPack::Unpacker&)>& callback)
    {
        Packetizer::subscribe(stream, [callback](const uint8_t index, const uint8_t* data, const uint8_t size)
        {
            MsgPack::Unpacker unpacker;
            unpacker.feed(data, size);
            callback(index, unpacker);
        });
    }

    template <typename... Args>
    inline void send(StreamType& stream, const uint8_t index, Args&&... args)
    {
        MsgPack::Packer packer;
        packer.encode(std::forward<Args>(args)...);
        Packetizer::send(stream, index, packer.data(), packer.size());
    }

    inline void send(StreamType& stream, const uint8_t index, const uint8_t* data, const uint8_t size)
    {
        MsgPack::Packer packer;
        packer.encode(data, size);
        Packetizer::send(stream, index, packer.data(), packer.size());
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
