#pragma once

#ifndef HT_SERIAL_MSGPACKETIZER_H
#define HT_SERIAL_MSGPACKETIZER_H

#define PACKETIZER_USE_INDEX_AS_DEFAULT
#define PACKETIZER_USE_CRC_AS_DEFAULT
#ifdef MSGPACKETIZER_ENABLE_DEBUG_LOG
    #define MSGPACK_ENABLE_DEBUG_LOG
#endif

#include "util/Packetizer/Packetizer.h"
#include "util/MsgPack/MsgPack.h"

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11
    // use standard c++ libraries
#else
#ifndef MSGPACKETIZER_MAX_PUBLISH_ELEMENT_SIZE
#define MSGPACKETIZER_MAX_PUBLISH_ELEMENT_SIZE 5
#endif
#ifndef MSGPACKETIZER_MAX_PUBLISH_DESTINATION_SIZE
#define MSGPACKETIZER_MAX_PUBLISH_DESTINATION_SIZE 2
#endif
#ifndef MSGPACK_MAX_PACKET_BYTE_SIZE
#define MSGPACK_MAX_PACKET_BYTE_SIZE 96
#endif
#ifndef MSGPACK_MAX_ARRAY_SIZE
#define MSGPACK_MAX_ARRAY_SIZE 3
#endif
#ifndef MSGPACK_MAX_MAP_SIZE
#define MSGPACK_MAX_MAP_SIZE 3
#endif
#ifndef MSGPACK_MAX_OBJECT_SIZE
#define MSGPACK_MAX_OBJECT_SIZE 16
#endif
#ifndef PACKETIZER_MAX_PACKET_QUEUE_SIZE
#define PACKETIZER_MAX_PACKET_QUEUE_SIZE 1
#endif
#ifndef PACKETIZER_MAX_PACKET_BINARY_SIZE
#define PACKETIZER_MAX_PACKET_BINARY_SIZE 96
#endif
#ifndef PACKETIZER_MAX_CALLBACK_QUEUE_SIZE
#define PACKETIZER_MAX_CALLBACK_QUEUE_SIZE 3
#endif
#ifndef PACKETIZER_MAX_STREAM_MAP_SIZE
#define PACKETIZER_MAX_STREAM_MAP_SIZE 1
#endif
#endif // have libstdc++11


namespace ht {
namespace serial {
namespace msgpacketizer {

#ifdef ARDUINO
    using StreamType = Stream;
    #define ELAPSED_MICROS micros
#elif defined (OF_VERSION_MAJOR)
    using StreamType = ofSerial;
    #define ELAPSED_MICROS ofGetElapsedTimeMicros
#endif

    namespace element
    {
        struct Base
        {
            uint32_t last_publish_us {0};
            uint32_t interval_us {33333}; // 30 fps

            bool next() const { return ELAPSED_MICROS() >= (last_publish_us + interval_us); }
            void setFrameRate(float fps) { interval_us = (uint32_t)(1000000.f / fps); }
            void setIntervalUsec(const uint32_t us) { interval_us = us; }
            void setIntervalMsec(const float ms) { interval_us = (uint32_t)(ms * 1000.f); }
            void setIntervalSec(const float sec) { interval_us = (uint32_t)(sec * 1000.f * 1000.f); }

            virtual ~Base() {}
            virtual void encodeTo(MsgPack::Packer& p) = 0;
        };

        using Ref = std::shared_ptr<Base>;
#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11
        using TupleRef = std::vector<Ref>;
#else
        using TupleRef = arx::vector<Ref, MSGPACKETIZER_MAX_PUBLISH_ELEMENT_SIZE>;
#endif

        template <typename T>
        class Value : public Base
        {
            T& t;
        public:
            Value(T& t) : t(t) {}
            virtual ~Value() {}
            virtual void encodeTo(MsgPack::Packer& p) override { p.pack(t); }
        };

        template <typename T>
        class Const : public Base
        {
            const T t;
        public:
            Const(const T& t) : t(t) {}
            virtual ~Const() {}
            virtual void encodeTo(MsgPack::Packer& p) override { p.pack(t); }
        };

        template <typename T>
        class Function : public Base
        {
            std::function<T()> getter;
        public:
            Function(const std::function<T()>& getter) : getter(getter) {}
            virtual ~Function() {}
            virtual void encodeTo(MsgPack::Packer& p) override { p.pack(getter()); }
        };

        class Tuple : public Base
        {
            TupleRef ts;
        public:
            Tuple(TupleRef&& ts) : ts(std::move(ts)) {}
            virtual ~Tuple() {}
            virtual void encodeTo(MsgPack::Packer& p) override { for (auto& t : ts) t->encodeTo(p); }
        };

    } // namespace element

    using PublishElementRef = element::Ref;
    using ElementTupleRef = element::TupleRef;


    template <typename T>
    inline auto make_element_ref(T& value)
    -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef>
    {
        return PublishElementRef(new element::Value<T>(value));
    }

    template <typename T>
    inline auto make_element_ref(const T& value)
    -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef>
    {
        return PublishElementRef(new element::Const<T>(value));
    }

    template <typename T>
    inline auto make_element_ref(const std::function<T()>& func)
    -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef>
    {
        return PublishElementRef(new element::Function<T>(func));
    }

    template <typename Func>
    inline auto make_element_ref(const Func& func)
    -> std::enable_if_t<arx::is_callable<Func>::value, PublishElementRef>
    {
        return make_element_ref(arx::function_traits<Func>::cast(func));
    }

    // multiple parameters helper
    inline PublishElementRef make_element_ref(ElementTupleRef& t)
    {
        return PublishElementRef(new element::Tuple(std::move(t)));
    }


    struct Destination
    {
        StreamType* stream;
        uint8_t index;

        Destination(const Destination& dest)
        : stream(dest.stream), index(dest.index) {}
        Destination(Destination&& dest)
        : stream(std::move(dest.stream)), index(std::move(dest.index)) {}
        Destination(const StreamType& stream, const uint8_t index)
        : stream((StreamType*)&stream), index(index) {}
        Destination() {}

        Destination& operator= (const Destination& dest)
        {
            stream = dest.stream;
            index = dest.index;
            return *this;
        }
        Destination& operator= (Destination&& dest)
        {
            stream = std::move(dest.stream);
            index = std::move(dest.index);
            return *this;
        }
        inline bool operator< (const Destination& rhs) const
        {
            return (stream != rhs.stream) ? (stream < rhs.stream) : (index < rhs.index);
        }
        inline bool operator== (const Destination& rhs) const
        {
            return (stream == rhs.stream) && (index == rhs.index);
        }
        inline bool operator!= (const Destination& rhs) const
        {
            return !(*this == rhs);
        }
    };


#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11
    using PackerMap = std::map<Destination, PublishElementRef>;
    using UnpackerRef = std::shared_ptr<MsgPack::Unpacker>;
    using UnpackerMap = std::map<StreamType*, UnpackerRef>;
    using namespace std;
#else
    using PackerMap = arx::map<Destination, PublishElementRef, MSGPACKETIZER_MAX_PUBLISH_DESTINATION_SIZE>;
    using UnpackerRef = std::shared_ptr<MsgPack::Unpacker>;
    using UnpackerMap = arx::map<StreamType*, UnpackerRef, PACKETIZER_MAX_STREAM_MAP_SIZE>;
    using namespace arx;
#endif


    class PackerManager
    {
        PackerManager() {}
        PackerManager(const PackerManager&) = delete;
        PackerManager& operator=(const PackerManager&) = delete;

        MsgPack::Packer encoder;
        PackerMap addr_map;

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

        void send(const Destination& dest, PublishElementRef elem)
        {
            encoder.clear();
            elem->encodeTo(encoder);
            Packetizer::send(*dest.stream, dest.index, encoder.data(), encoder.size());
        }

        void post()
        {
            for (auto& mp : addr_map)
            {
                if (mp.second->next())
                {
                    mp.second->last_publish_us = ELAPSED_MICROS();
                    send(mp.first, mp.second);
                }
            }
        }

        PublishElementRef publish(const StreamType& stream, const uint8_t index, const char* const value)
        {
            return publish_impl(stream, index, make_element_ref(value));
        }

        template <typename T>
        auto publish(const StreamType& stream, const uint8_t index, T& value)
        -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef>
        {
            return publish_impl(stream, index, make_element_ref(value));
        }

        template <typename T>
        auto publish(const StreamType& stream, const uint8_t index, const T& value)
        -> std::enable_if_t<!arx::is_callable<T>::value, PublishElementRef>
        {
            return publish_impl(stream, index, make_element_ref(value));
        }

        template <typename Func>
        auto publish(const StreamType& stream, const uint8_t index, Func&& func)
        -> std::enable_if_t<arx::is_callable<Func>::value, PublishElementRef>
        {
            return publish(stream, index, arx::function_traits<Func>::cast(func));
        }

        template <typename T>
        PublishElementRef publish(const StreamType& stream, const uint8_t index, std::function<T()>&& getter)
        {
            return publish_impl(stream, index, make_element_ref(getter));
        }

        template <typename... Args>
        PublishElementRef publish(const StreamType& stream, const uint8_t index, Args&&... args)
        {
            ElementTupleRef v { make_element_ref(std::forward<Args>(args))... };
            return publish_impl(stream, index, make_element_ref(v));
        }

        template <typename... Args>
        PublishElementRef publish_arr(const StreamType& stream, const uint8_t index, Args&&... args)
        {
            static MsgPack::arr_size_t s(sizeof...(args));
            return publish(stream, index, s, std::forward<Args>(args)...);
        }

        template <typename... Args>
        PublishElementRef publish_map(const StreamType& stream, const uint8_t index, Args&&... args)
        {
            if ((sizeof...(args) % 2) == 0)
            {
                static MsgPack::map_size_t s(sizeof...(args) / 2);
                return publish(stream, index, s, std::forward<Args>(args)...);
            }
            else
            {
                LOG_WARNING("serialize arg size must be even for map :", sizeof...(args));
                return nullptr;
            }
        }

        PublishElementRef getPublishElementRef(const StreamType& stream, const uint8_t index)
        {
            Destination dest {stream, index};
            return addr_map[dest];
        }

    private:

        PublishElementRef publish_impl(const StreamType& stream, const uint8_t index, PublishElementRef ref)
        {
            Destination dest {stream, index};
            addr_map.insert(make_pair(dest, ref));
            return ref;
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
        packer.pack(data, size);
        Packetizer::send(stream, index, packer.data(), packer.size());
    }

    inline void send(StreamType& stream, const uint8_t index)
    {
        auto& packer = PackerManager::getInstance().getPacker();
        Packetizer::send(stream, index, packer.data(), packer.size());
    }

    template <typename... Args>
    inline void send_arr(StreamType& stream, const uint8_t index, Args&&... args)
    {
        auto& packer = PackerManager::getInstance().getPacker();
        packer.clear();
        packer.serialize(MsgPack::arr_size_t(sizeof...(args)), std::forward<Args>(args)...);
        Packetizer::send(stream, index, packer.data(), packer.size());
    }

    template <typename... Args>
    inline void send_map(StreamType& stream, const uint8_t index, Args&&... args)
    {
        if ((sizeof...(args) % 2) == 0)
        {
            auto& packer = PackerManager::getInstance().getPacker();
            packer.clear();
            packer.serialize(MsgPack::arr_size_t(sizeof...(args) / 2), std::forward<Args>(args)...);
            Packetizer::send(stream, index, packer.data(), packer.size());
        }
        else
        {
            LOG_WARNING("serialize arg size must be even for map :", sizeof...(args));
        }
    }

    template <typename... Args>
    inline PublishElementRef publish(const StreamType& stream, const uint8_t index, Args&&... args)
    {
        return PackerManager::getInstance().publish(stream, index, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline PublishElementRef publish_arr(const StreamType& stream, const uint8_t index, Args&&... args)
    {
        return PackerManager::getInstance().publish_arr(stream, index, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline PublishElementRef publish_map(const StreamType& stream, const uint8_t index, Args&&... args)
    {
        return PackerManager::getInstance().publish_map(stream, index, std::forward<Args>(args)...);
    }

    inline void post()
    {
        PackerManager::getInstance().post();
    }

    inline PublishElementRef getPublishElementRef(const StreamType& stream, const uint8_t index)
    {
        return PackerManager::getInstance().getPublishElementRef(stream, index);
    }

    inline const MsgPack::Packer& getPacker()
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
                decoders.insert(make_pair(s, std::make_shared<MsgPack::Unpacker>()));
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
    inline void subscribe(StreamType& stream, const uint8_t index, Args&&... args)
    {
        Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size)
        {
            auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
            unpacker->clear();
            unpacker->feed(data, size);
            unpacker->deserialize(std::forward<Args>(args)...);
        });
    }

    template <typename... Args>
    inline void subscribe_arr(StreamType& stream, const uint8_t index, Args&&... args)
    {
        static MsgPack::arr_size_t sz;
        Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size)
        {
            auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
            unpacker->clear();
            unpacker->feed(data, size);
            unpacker->deserialize(sz, std::forward<Args>(args)...);
        });
    }

    template <typename... Args>
    inline void subscribe_map(StreamType& stream, const uint8_t index, Args&&... args)
    {
        if ((sizeof...(args) % 2) == 0)
        {
            static MsgPack::map_size_t sz;
            Packetizer::subscribe(stream, index, [&](const uint8_t* data, const uint8_t size)
            {
                auto unpacker = UnpackerManager::getInstance().getUnpackerRef(stream);
                unpacker->clear();
                unpacker->feed(data, size);
                unpacker->deserialize(sz, std::forward<Args>(args)...);
            });
        }
        else
        {
            LOG_WARNING("deserialize arg size must be even for map :", sizeof...(args));
        }
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
                    unpacker->to_tuple(t);
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


    inline UnpackerRef getUnpackerRef(const StreamType& stream)
    {
        return UnpackerManager::getInstance().getUnpackerRef(stream);
    }

    inline UnpackerMap& getUnpackerMap()
    {
        return UnpackerManager::getInstance().getUnpackerMap();
    }


    inline void update(bool b_exec_cb = true)
    {
        Packetizer::parse(b_exec_cb);
        PackerManager::getInstance().post();
    }

} // namespace msgpacketizer
} // namespace serial
} // namespace ht

namespace MsgPacketizer = ht::serial::msgpacketizer;

#endif // HT_SERIAL_MSGPACKETIZER_H
