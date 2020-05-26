#pragma once

#ifndef HT_UTIL_CRC_WRAPPER_H
#define HT_UTIL_CRC_WRAPPER_H

#ifdef ARDUINO
#include <Arduino.h>
#include "FastCRC/FastCRC.h" // x2 to x10 faster
#else
#define CRCPP_USE_CPP11
#define CRCPP_USE_NAMESPACE
#define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
#include "CRCpp/CRCpp.h"
#endif

namespace ht {
namespace util {
namespace crc {

    enum class Crc8 : uint8_t
    {
        SMBUS,
        MAXIM
    };

    enum class Crc16 : uint8_t
    {
        CCITT,
        KERMIT,
        MODBUS,
        XMODEM,
        X25
    };

    enum class Crc32 : uint8_t
    {
        CRC32,
        POSIX
    };

#ifdef ARDUINO

    namespace fastcrc
    {
        inline uint8_t crc8(const uint8_t* data, const size_t size, const Crc8 type = Crc8::SMBUS)
        {
            static FastCRC8 fastcrc;
            switch (type)
            {
                case Crc8::SMBUS: return fastcrc.smbus(data, size);
                case Crc8::MAXIM: return fastcrc.maxim(data, size);
            }
            return 0xFF;
        }

        inline uint16_t crc16(const uint8_t* data, const size_t size, const Crc16 type = Crc16::MODBUS)
        {
            static FastCRC16 fastcrc;
            switch (type)
            {
                case Crc16::CCITT:  return fastcrc.ccitt(data, size);
                case Crc16::KERMIT: return fastcrc.kermit(data, size);
                case Crc16::MODBUS: return fastcrc.modbus(data, size);
                case Crc16::XMODEM: return fastcrc.xmodem(data, size);
                case Crc16::X25:    return fastcrc.x25(data, size);
            }
            return 0xFFFF;
        }

        inline uint32_t crc32(const uint8_t* data, const size_t size, const Crc32 type = Crc32::CRC32)
        {
            static FastCRC32 fastcrc;
            switch (type)
            {
                case Crc32::CRC32: return fastcrc.crc32(data, size);
                case Crc32::POSIX: return fastcrc.cksum(data, size);
            }
            return 0xFFFFFFFF;
        }

    } // namespace fastcrc

#else

    namespace crcpp
    {
        using crc8_t = CRCPP::CRC::Parameters<crcpp_uint8, 8>;
        using crc16_t = CRCPP::CRC::Parameters<crcpp_uint16, 16>;
        using crc32_t = CRCPP::CRC::Parameters<crcpp_uint32, 32>;
        using crc64_t = CRCPP::CRC::Parameters<crcpp_uint64, 64>;

        inline crc8_t get_crc8_param(const Crc8 type)
        {
            switch (type)
            {
                case Crc8::SMBUS: return std::move(CRCPP::CRC::CRC_8());
                case Crc8::MAXIM: return std::move(CRCPP::CRC::CRC_8_MAXIM());
            }
            return std::move(CRCPP::CRC::CRC_8());
        }

        inline crc16_t get_crc16_param(const Crc16 type)
        {
            switch (type)
            {
                case Crc16::CCITT:  return std::move(CRCPP::CRC::CRC_16_CCITTFALSE());
                case Crc16::KERMIT: return std::move(CRCPP::CRC::CRC_16_KERMIT());
                case Crc16::MODBUS: return std::move(CRCPP::CRC::CRC_16_MODBUS());
                case Crc16::XMODEM: return std::move(CRCPP::CRC::CRC_16_XMODEM());
                case Crc16::X25:    return std::move(CRCPP::CRC::CRC_16_X25());
            }
            return std::move(CRCPP::CRC::CRC_16_MODBUS());
        }

        inline crc32_t get_crc32_param(const Crc32 type)
        {
            switch (type)
            {
                case Crc32::CRC32: return std::move(CRCPP::CRC::CRC_32());
                case Crc32::POSIX: return std::move(CRCPP::CRC::CRC_32_POSIX());
            }
            return std::move(CRCPP::CRC::CRC_32());
        }

        inline uint8_t crc8(const uint8_t* data, const size_t size, const Crc8 type = Crc8::SMBUS)
        {
            auto param = get_crc8_param(type);
            return CRCPP::CRC::Calculate(data, size, param);
        }

        inline uint16_t crc16(const uint8_t* data, const size_t size, const Crc16 type = Crc16::MODBUS)
        {
            auto param = get_crc16_param(type);
            return CRCPP::CRC::Calculate(data, size, param);
        }

        inline uint32_t crc32(const uint8_t* data, const size_t size, const Crc32 type = Crc32::CRC32)
        {
            auto param = get_crc32_param(type);
            return CRCPP::CRC::Calculate(data, size, param);
        }

        inline uint64_t crc64(const uint8_t* data, const size_t size)
        {
            return CRCPP::CRC::Calculate(data, size, CRCPP::CRC::CRC_64());
        }

    } // namespace crcpp

#endif

} // namespace crc
} // namespace util
} // namespace ht


#ifdef ARDUINO
namespace crcx = ht::util::crc::fastcrc;
#else
namespace crcx = ht::util::crc::crcpp;
#endif

#endif // HT_UTIL_CRC_WRAPPER_H
