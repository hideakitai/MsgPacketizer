#pragma once

#include "../../libs/msgpack/include/msgpack.hpp"
#include "CRC.h"
#include <queue>
#include <limits>

namespace MsgPacketizer
{
    class Packer
    {
    public:

        Packer(Checker m = Checker::CRC8) : mode(m) {}
        ~Packer() {}

        void setCheckMode(Checker m) { mode = m; }

        template <typename T>
        const uint8_t* pack(const T& object, const uint8_t& index = 0)
        {
            msgpack::sbuffer sbuf;
            msgpack::pack(sbuf, object);
            return pack(sbuf, index);
        }

		template <typename T>
		Packer& operator<< (T & object)
		{
			pack(object);
			return *this;
		}

		uint8_t* data() { return pack_buffer.data(); }

        size_t size() { return count; }


    protected:

        const uint8_t* pack(const msgpack::sbuffer& sbuf, const uint8_t& index = 0)
        {
            assert(sbuf.size() <= std::numeric_limits<uint8_t>::max());

            pack_buffer.fill(0);
            count = 0;

            append((uint8_t)START_BYTE, false);
            append((uint8_t)index);
            append((uint8_t)sbuf.size());
            append((uint8_t*)sbuf.data(), sbuf.size());

            if (mode == Checker::Sum)
            {
                uint8_t sum = (uint8_t)START_BYTE + index + (uint8_t)sbuf.size();
                for (size_t i = 0; i < sbuf.size(); ++i) sum += (uint8_t)sbuf.data()[i];
                append(sum);
            }
            else if (mode == Checker::CRC8)
            {
                append(CRC::getCRC8((uint8_t*)sbuf.data(), sbuf.size()));
            }

            return data();
        }

        void append(const uint8_t* const data, const size_t& size, bool isEscape = true)
        {
            if (isEscape)
            {
                std::queue<size_t> escapes;
                for (size_t i = 0; i < size; ++i) if (data[i] == ESCAPE_BYTE) escapes.push(i);

                if (escapes.empty())
                {
                    for (size_t i = 0; i < size; ++i)
                    {
                        pack_buffer[count++] = data[i];
                    }
                }
                else
                {
                    size_t start = 0;
                    while (!escapes.empty())
                    {
                        const size_t& idx = escapes.front();
                        append(data + start, idx - start);
                        append(data[idx], true);
                        start = idx + 1;
                        escapes.pop();
                    }
                    if (start < size) append(data + start, size - start);
                }
            }
            else
            {
                for (size_t i = 0; i < size; ++i)
                {
                    pack_buffer[count++] = data[i];
                }
            }
        }


        void append(const uint8_t& data, bool isEscape = true)
        {
            if (isEscape && (data == ESCAPE_BYTE))
            {
                pack_buffer[count++] = ESCAPE_BYTE;
                pack_buffer[count++] = (uint8_t)(data ^ ESCAPE_MASK);
            }
            else
            {
                pack_buffer[count++] = data;
            }
        }

    private:

        Checker mode;

        std::array<uint8_t, MSGPACKPRTCL_READ_BUFFER_SIZE> pack_buffer;
        size_t count;
    };

}
