#pragma once
#ifndef ARDUINO_MSGPACK_UTILITY_H
#define ARDUINO_MSGPACK_UTILITY_H

#include "Types.h"
#include "Packer.h"
#include "Unpacker.h"

namespace arduino {
namespace msgpack {

    template <typename T>
    inline size_t estimate_size(const T& value) {
        Packer packer;
        packer.serialize(value);
        return packer.size();
    }

#ifdef EEPROM_h

    namespace eeprom {

        template <typename T>
        inline void save(const T& value, const size_t index_offset = 0) {
            Packer packer;
            packer.serialize(value);

#if defined(ESP_PLATFORM) || defined(ESP8266)
            // write size of value
            EEPROM.write(index_offset, packer.size());

            const uint8_t* data = packer.data();
            for (size_t i = 0; i < packer.size(); ++i) {
                EEPROM.write(index_offset + 1 + i, data[i]);  // consider offset of value size
            }
            EEPROM.commit();
#else
            // write size of value
            EEPROM.update(index_offset, packer.size());

            const uint8_t* data = packer.data();
            for (size_t i = 0; i < packer.size(); ++i) {
                EEPROM.update(index_offset + 1 + i, data[i]);  // consider offset of value size
            }
#endif
        }

        template <typename T>
        inline bool load(T& value, const size_t index_offset = 0) {
            // confirm the size of value
            const size_t size = EEPROM.read(index_offset);

            // read data
            bin_t<uint8_t> data;
            for (size_t i = 0; i < size; ++i) {
                data.emplace_back(EEPROM.read(index_offset + 1 + i));  // consider offset of value size
            }

            // deserialize data
            Unpacker unpacker;
            if (unpacker.feed(data.data(), data.size())) {
                Serial.println("size matched!");
                if (unpacker.deserialize(value)) {
                    return true;
                } else {
                    LOG_ERROR("EEPROM data deserialization failed");
                    return false;
                }
            } else {
                LOG_ERROR("EEPROM data size was not correct");
                return false;
            }
        }

        inline void clear_size(const size_t size, const size_t index_offset = 0) {
#if defined(ESP_PLATFORM) || defined(ESP8266)
            for (size_t i = 0; i < size; ++i) {
                EEPROM.write(index_offset + i, 0xFF);
            }
            EEPROM.commit();
#else
            for (size_t i = 0; i < size; ++i) {
                EEPROM.update(index_offset + i, 0xFF);
            }
#endif
        }

        template <typename T>
        inline void clear(const T& value, const size_t index_offset = 0) {
            const size_t size = estimate_size(value);
            clear_size(size, index_offset);
        }

    }  // namespace eeprom

#endif  // ARDUINO

}  // namespace msgpack
}  // namespace arduino

#endif  // ARDUINO_MSGPACK_UTILITY_H
