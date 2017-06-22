#pragma once

#include "../..//libs/msgpack/include/msgpack.hpp"
#include "Packer.h"
#include "Unpacker.h"
#include <memory>

namespace MsgPacketizer
{
    class Streamer : public Packer, public Unpacker
    {
    public:
        
        Streamer(Checker m = Checker::CRC8) { setCheckMode(m); }
        
        ~Streamer() {}
        
        void setCheckMode(Checker m)
        {
            Unpacker::setCheckMode(m);
            Packer::setCheckMode(m);
        }
        
        bool next()
        {
            if (this->Unpacker::available())
            {
                const msgpack::sbuffer& s = this->Unpacker::_readBuffer.front()->sbuf;
                if (!unpacker) unpacker = std::make_shared<msgpack::unpacker>();
                unpacker->reserve_buffer(s.size());
                memcpy(unpacker->buffer(), s.data(), s.size());
                unpacker->buffer_consumed(s.size());
                return unpacker->next(this->Unpacker::oh);
            }
            return false;
        }
        
        template <typename T>
        void pack(const T& data)
        {
            if (!packer) packer = std::make_shared<msgpack::packer<msgpack::sbuffer>>(&sbuf);
            packer->pack(data);
        }
        
        template <typename T>
        T unpack()
        {
            const msgpack::object& o = this->Unpacker::oh.get();
            return o.as<T>();
        }
        
        void serialize(const uint8_t& index = 0)
        {
            Packer::pack(sbuf, index);
            Packer::data();
            packer.reset();
            sbuf.clear();
        }
        
        size_t size() { return Packer::size(); }
        
        uint8_t* data() { return Packer::data(); }
        
    private:
        
        std::shared_ptr<msgpack::packer<msgpack::sbuffer>> packer;
        std::shared_ptr<msgpack::unpacker> unpacker;
        msgpack::sbuffer sbuf;
        
    };
}
