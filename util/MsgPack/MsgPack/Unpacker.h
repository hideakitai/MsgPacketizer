
#pragma once
#ifndef HT_SERIAL_MSGPACK_UNPACKER_H
#define HT_SERIAL_MSGPACK_UNPACKER_H

#include "util/ArxTypeTraits/ArxTypeTraits.h"
#ifdef HT_SERIAL_MSGPACK_DISABLE_STL
    #include "util/ArxContainer/ArxContainer.h"
#else
    #include <vector>
    #include <array>
    #include <deque>
    #include <tuple>
    #include <list>
    #include <forward_list>
    #include <set>
    #include <unordered_set>
    #include <map>
    #include <unordered_map>
    #include <limits>
#endif // HT_SERIAL_MSGPACK_DISABLE_STL
#ifdef TEENSYDUINO
    #include "util/TeensyDirtySTLErrorSolution/TeensyDirtySTLErrorSolution.h"
#endif // TEENSYDUINO

#include "Types.h"

namespace ht {
namespace serial {
namespace msgpack {

#ifdef HT_SERIAL_MSGPACK_DISABLE_STL
    using namespace arx;
#else
    using namespace std;
#endif // HT_SERIAL_MSGPACK_DISABLE_STL

    class Unpacker
    {
        uint8_t* raw_data {nullptr};
        idx_t indices;
        size_t curr_index {0};
        bool b_decoded {false};

    public:

        bool feed(const uint8_t* data, size_t size)
        {
            raw_data = (uint8_t*)data;
            for (size_t i = 0; i < size; i += getElementSize(indices.size() - 1))
                indices.emplace_back(i);
            b_decoded = (size == (indices.back() + getElementSize(indices.size() - 1)));
            return b_decoded;
        }

        template <typename First, typename ...Rest>
        void decode(First& first, Rest&&... rest)
        {
            unpack(first);
            decode(std::forward<Rest>(rest)...);
        }
        void decode() {}

        template <typename... Ts>
        void decodeTo(std::tuple<Ts...>& t)
        {
            decodeTo(std::make_index_sequence<sizeof...(Ts)>(), t);
        }
        template <typename... Ts, size_t... Is>
        void decodeTo(std::index_sequence<Is...>&&, std::tuple<Ts...>& t)
        {
            size_t i {0};
            idx_t {(unpack(std::get<Is>(t)), i++)...};
        }
        void decodeTo() {}

        bool available() const { return b_decoded; }
        size_t size() const { return indices.size(); }
        void index(const size_t i) { curr_index = i; }
        size_t index() const { return curr_index; }
        void clear() { indices.clear(); index(0); b_decoded = false; raw_data = nullptr; }


        /////////////////////////////////////////////////
        // ---------- msgpack type adaptors ---------- //
        /////////////////////////////////////////////////

        // adaptation of types to msgpack
        // https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_adaptor


        // ---------- NIL format family ----------
        // - N/A

        template <typename T>
        auto unpack(T& value)
        -> typename std::enable_if<std::is_same<T, object::NIL>::value>::type
        {
            value = unpackNil();
        }


        // ---------- BOOL format family ----------
        // - bool

        template <typename T>
        auto unpack(T& value)
        -> typename std::enable_if<std::is_same<T, bool>::value>::type
        {
            value = unpackBool();
        }


        // ---------- INT format family ----------
        // - char (signed/unsigned)
        // - ints (signed/unsigned)

        template <typename T>
        auto unpack(T& value)
        -> typename std::enable_if <
            std::is_arithmetic<T>::value &&
            !std::is_floating_point<T>::value &&
            !std::is_same<T, bool>::value &&
            !std::is_same<typename std::remove_cv<T>::type, char*>::value &&
            !std::is_signed<T>::value
        >::type
        {
            Type type = getType();
            if      (type == Type::UINT7)  value = unpackIntU7();
            else if (type == Type::UINT8)  value = unpackIntU8();
            else if (type == Type::UINT16) value = unpackIntU16();
            else if (type == Type::UINT32) value = unpackIntU32();
            else if (type == Type::UINT64) value = unpackIntU64();
            else
            {
                // TODO: error handling

                value = 0;
                ++curr_index;
            }
        }

        template <typename T>
        auto unpack(T& value)
        -> typename std::enable_if <
            std::is_arithmetic<T>::value &&
            !std::is_floating_point<T>::value &&
            !std::is_same<T, bool>::value &&
            !std::is_same<typename std::remove_cv<T>::type, char*>::value &&
            std::is_signed<T>::value
        >::type
        {
            Type type = getType();
            if      (type == Type::INT5)  value = unpackInt5();
            else if (type == Type::INT8)  value = unpackInt8();
            else if (type == Type::INT16) value = unpackInt16();
            else if (type == Type::INT32) value = unpackInt32();
            else if (type == Type::INT64) value = unpackInt64();
            else
            {
                // TODO: error handling

                value = 0;
                ++curr_index;
            }
        }


        // ---------- FLOAT format family ----------
        // - float
        // - double

        template <typename T>
        auto unpack(T& value)
        -> typename std::enable_if <
            std::is_arithmetic<T>::value &&
            std::is_floating_point<T>::value
        >::type
        {
            Type type = getType();
            if      (type == Type::FLOAT32) value = unpackFloat32();
            else if (type == Type::FLOAT64) value = unpackFloat64();
            else
            {
                // TODO: error handling

                value = 0.0;
                ++curr_index;
            }
        }


        // ---------- STRING format family ----------
        // - char*
        // - char[]
        // - std::string

        void unpack(str_t& str)
        {
            Type type = getType();
            if      (type == Type::STR5)  str = unpackString5();
            else if (type == Type::STR8)  str = unpackString8();
            else if (type == Type::STR16) str = unpackString16();
            else if (type == Type::STR32) str = unpackString32();
            else
            {
                // TODO: error handling

                str = "";
                ++curr_index;
            }
        }


        // ---------- BIN format family ----------
        // - unsigned char*
        // - unsigned char[]
        // - std::vector<char>
        // - std::vector<unsigned char>
        // - std::array<char>
        // - std::array<unsigned char>

        template <typename T>
        auto unpack(bin_t<T>& bin)
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value
        >::type
        {
            bin = unpackBinary<T>();
        }

#ifndef HT_SERIAL_MSGPACK_DISABLE_STL

        template <typename T, size_t N>
        auto unpack(std::array<T, N>& bin)
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value
        >::type
        {
            bin = unpackBinary<T, N>();
        }

#endif // HT_SERIAL_MSGPACK_DISABLE_STL


        // ---------- ARRAY format family ----------
        // - T[]
        // - std::vector
        // - std::array
        // - std::deque
        // - std::pair
        // - std::tuple
        // - std::list
        // - std::forward_list
        // - std::set
        // - std::multiset
        // - std::unordered_set *
        // - std::unordered_multiset *
        // * : not supported in arduino

        template <typename T>
        auto unpack(arr_t<T>& arr)
        -> typename std::enable_if<
            !std::is_same<T, char>::value &&
            !std::is_same<T, uint8_t>::value
        >::type
        {
            unpackArrayContainerArray(arr);
            arr.shrink_to_fit();
        }

#ifndef HT_SERIAL_MSGPACK_DISABLE_STL

        template <typename T, size_t N>
        auto unpack(std::array<T, N>& arr)
        -> typename std::enable_if<
            !std::is_same<T, char>::value &&
            !std::is_same<T, uint8_t>::value
        >::type
        {
            const size_t size = unpackArraySize();
            if (N == size)
            {
                for (auto& a : arr)
                {
                    unpack(a);
                }
            }
            else
            {
                // TODO: error handling
            }
        }

        template <typename T>
        void unpack(std::deque<T>& arr)
        {
            unpackArrayContainerArray(arr);
            arr.shrink_to_fit();
        }

        template <typename T, typename U>
        void unpack(std::pair<T, U>& arr)
        {
            const size_t size = unpackArraySize();
            if (size == 1)
            {
                unpack(arr.first);
                unpack(arr.second);
            }
            else
            {
                // TODO: error handling
            }
        }

        template <typename... Args>
        void unpack(std::tuple<Args...>& t)
        {
            const size_t size = unpackArraySize();
            if (sizeof...(Args) == size)
            {
                decodeTo(t);
            }
            else
            {
                // TODO: error handling
            }
        }

        template <typename T>
        void unpack(std::list<T>& arr)
        {
            unpackArrayContainerArray(arr);
        }

        template <typename T>
        void unpack(std::forward_list<T>& arr)
        {
            const size_t arr_size = std::distance(arr.begin(), arr.end());
            const size_t size = unpackArraySize();
            if (arr_size == size)
            {
                for (auto& a : arr)
                {
                    unpack(a);
                }
            }
            else if (size == 0)
            {
                // TODO: error handling
            }
            else
            {
                arr.clear();
                for (size_t a = 0; a < size; ++a)
                {
                    T t;
                    unpack(t);
                    arr.emplace_after(std::next(arr.before_begin(), a), t);
                }
            }
        }

        template <typename T>
        void unpack(std::set<T>& arr)
        {
            unpackArrayContainerSet(arr);
        }

        template <typename T>
        void unpack(std::multiset<T>& arr)
        {
            unpackArrayContainerSet(arr);
        }

        template <typename T>
        void unpack(std::unordered_set<T>& arr)
        {
            unpackArrayContainerSet(arr);
        }

        template <typename T>
        void unpack(std::unordered_multiset<T>& arr)
        {
            unpackArrayContainerSet(arr);
        }

#endif // HT_SERIAL_MSGPACK_DISABLE_STL


        // ---------- MAP format family ----------
        // - std::map
        // - std::multimap
        // - std::unordered_map *
        // - std::unordered_multimap *
        // * : not supported in arduino

        template <typename T, typename U>
        void unpack(map_t<T, U>& mp)
        {
            unpackMapContainer(mp);
        }

#ifndef HT_SERIAL_MSGPACK_DISABLE_STL

        template <typename T, typename U>
        void unpack(std::multimap<T, U>& mp)
        {
            unpackMapContainer(mp);
        }

        template <typename T, typename U>
        void unpack(std::unordered_map<T, U>& mp)
        {
            unpackMapContainer(mp);
        }

        template <typename T, typename U>
        void unpack(std::unordered_multimap<T, U>& mp)
        {
            unpackMapContainer(mp);
        }

#endif // HT_SERIAL_MSGPACK_DISABLE_STL


        // ---------- EXT format family ----------
        // no pre-defined type





        /////////////////////////////////////////
        // ---------- msgpack types ---------- //
        /////////////////////////////////////////

        // ---------- NIL format family ----------

        bool unpackNil()
        {
            bool ret = (getType() == Type::NIL);
            ++curr_index;
            return ret;
        }

        // ---------- BOOL format family ----------

        bool unpackBool()
        {
            bool ret = (getType() == Type::BOOL) ? (bool)(getRawBytes<uint8_t>(curr_index, 0) & (uint8_t)BitMask::BOOL) : 0;
            ++curr_index;
            return ret;
        }

        // ---------- INT format family ----------

        uint8_t unpackIntU7()
        {
            uint8_t ret = (getType() == Type::UINT7) ? (uint8_t)(getRawBytes<uint8_t>(curr_index, 0) & (uint8_t)BitMask::UINT7) : 0;
            ++curr_index;
            return ret;
        }

        uint8_t unpackIntU8()
        {
            uint8_t ret = (getType() == Type::UINT8) ? getRawBytes<uint8_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        uint16_t unpackIntU16()
        {
            uint16_t ret = (getType() == Type::UINT16) ? getRawBytes<uint16_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        uint32_t unpackIntU32()
        {
            uint32_t ret = (getType() == Type::UINT32) ? getRawBytes<uint32_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        uint64_t unpackIntU64()
        {
            uint64_t ret = (getType() == Type::UINT64) ? getRawBytes<uint64_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        int8_t unpackInt5()
        {
            int8_t ret = (getType() == Type::INT5) ? getRawBytes<uint8_t>(curr_index, 0) : 0;
            ++curr_index;
            return ret;
        }

        int8_t unpackInt8()
        {
            int8_t ret = (getType() == Type::INT8) ? getRawBytes<uint8_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        int16_t unpackInt16()
        {
            int16_t ret = (getType() == Type::INT16) ? getRawBytes<int16_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        int32_t unpackInt32()
        {
            int32_t ret = (getType() == Type::INT32) ? getRawBytes<int32_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        int64_t unpackInt64()
        {
            int64_t ret = (getType() == Type::INT64) ? getRawBytes<int64_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        // ---------- FLOAT format family ----------

        float unpackFloat32()
        {
            float ret = (getType() == Type::FLOAT32) ? getRawBytes<float>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        double unpackFloat64()
        {
#ifndef HT_SERIAL_MSGPACK_DISABLE_STL
            double ret = (getType() == Type::FLOAT64) ? getRawBytes<double>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
#else
            return unpackFloat32(); // Uno, etc. does not support double
#endif // HT_SERIAL_MSGPACK_DISABLE_STL
        }

        // ---------- STR format family ----------

        str_t unpackString5()
        {
            str_t str("");
            if (getType() == Type::STR5)
            {
                uint8_t size = getRawBytes<uint8_t>(curr_index, 0) & (uint8_t)BitMask::STR5;
                for (uint8_t c = 0; c < size; ++c) str += getRawBytes<char>(curr_index, c + 1);
            }
            ++curr_index;
            return std::move(str);
        }

        str_t unpackString8()
        {
            str_t str("");
            if (getType() == Type::STR8)
            {
                uint8_t size = getRawBytes<uint8_t>(curr_index, 1);
                for (uint8_t c = 0; c < size; ++c) str += getRawBytes<char>(curr_index, c + 1 + sizeof(uint8_t));
            }
            ++curr_index;
            return std::move(str);
        }

        str_t unpackString16()
        {
            str_t str("");
            if (getType() == Type::STR16)
            {
                uint16_t size = getRawBytes<uint16_t>(curr_index, 1);
                for (uint16_t c = 0; c < size; ++c) str += getRawBytes<char>(curr_index, c + 1 + sizeof(uint16_t));
            }
            ++curr_index;
            return std::move(str);
        }

        str_t unpackString32()
        {
            str_t str("");
            if (getType() == Type::STR32)
            {
                uint32_t size = getRawBytes<uint32_t>(curr_index, 1);
                for (uint32_t c = 0; c < size; ++c) str += getRawBytes<char>(curr_index, c + 1 + sizeof(uint32_t));
            }
            ++curr_index;
            return std::move(str);
        }


        // ---------- BIN format family ----------

        template <typename T = uint8_t>
        auto unpackBinary()
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value,
            bin_t<T>
        >::type
        {
            Type type = getType();
            if      (type == Type::BIN8)  return std::move(unpackBinary8<T>());
            else if (type == Type::BIN16) return std::move(unpackBinary16<T>());
            else if (type == Type::BIN32) return std::move(unpackBinary32<T>());
            else                          ++curr_index;
            return std::move(bin_t<T>());
        }

        template <typename T = uint8_t>
        auto unpackBinary8()
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value,
            bin_t<T>
        >::type
        {
            bin_t<T> data;
            if (getType() == Type::BIN8)
            {
                uint8_t size = getRawBytes<uint8_t>(curr_index, 1);
                for (uint8_t v = 0; v < size; ++v) data.emplace_back(getRawBytes<T>(curr_index, v + 1 + sizeof(uint8_t)));
            }
            ++curr_index;
            return std::move(data);
        }

        template <typename T = uint8_t>
        auto unpackBinary16()
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value,
            bin_t<T>
        >::type
        {
            bin_t<T> data;
            if (getType() == Type::BIN16)
            {
                uint16_t size = getRawBytes<uint16_t>(curr_index, 1);
                for (uint16_t v = 0; v < size; ++v) data.emplace_back(getRawBytes<T>(curr_index, v + 1 + sizeof(uint16_t)));
            }
            ++curr_index;
            return std::move(data);
        }

        template <typename T = uint8_t>
        auto unpackBinary32()
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value,
            bin_t<T>
        >::type
        {
            bin_t<T> data;
            if (getType() == Type::BIN32)
            {
                uint32_t size = getRawBytes<uint32_t>(curr_index, 1);
                for (uint32_t v = 0; v < size; ++v) data.emplace_back(getRawBytes<T>(curr_index, v + 1 + sizeof(uint32_t)));
            }
            ++curr_index;
            return std::move(data);
        }

#ifndef HT_SERIAL_MSGPACK_DISABLE_STL

        template <typename T, size_t N>
        auto unpackBinary()
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value,
            std::array<T, N>
        >::type
        {
            Type type = getType();
            if      (type == Type::BIN8)  return std::move(unpackBinary8<T, N>());
            else if (type == Type::BIN16) return std::move(unpackBinary16<T, N>());
            else if (type == Type::BIN32) return std::move(unpackBinary32<T, N>());
            else                          ++curr_index;
            return std::move(std::array<T, N>());
        }

        template <typename T, size_t N>
        auto unpackBinary8()
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value,
            std::array<T, N>
        >::type
        {
            std::array<T, N> data;
            if (getType() == Type::BIN8)
            {
                uint8_t size = getRawBytes<uint8_t>(curr_index, 1);
                for (uint8_t v = 0; v < size; ++v) data[v] = getRawBytes<T>(curr_index, v + 1 + sizeof(uint8_t));
            }
            ++curr_index;
            return std::move(data);
        }

        template <typename T, size_t N>
        auto unpackBinary16()
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value,
            std::array<T, N>
        >::type
        {
            std::array<T, N> data;
            if (getType() == Type::BIN16)
            {
                uint16_t size = getRawBytes<uint16_t>(curr_index, 1);
                for (uint16_t v = 0; v < size; ++v) data[v] = getRawBytes<T>(curr_index, v + 1 + sizeof(uint16_t));
            }
            ++curr_index;
            return std::move(data);
        }

        template <typename T, size_t N>
        auto unpackBinary32()
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value,
            std::array<T, N>
        >::type
        {
            std::array<T, N> data;
            if (getType() == Type::BIN32)
            {
                uint32_t size = getRawBytes<uint32_t>(curr_index, 1);
                for (uint32_t v = 0; v < size; ++v) data[v] = getRawBytes<T>(curr_index, v + 1 + sizeof(uint32_t));
            }
            ++curr_index;
            return std::move(data);
        }

#endif // HT_SERIAL_MSGPACK_DISABLE_STL


        // ---------- ARRAY format family ----------

        size_t unpackArraySize()
        {
            Type type = getType();
            if (type == Type::ARRAY4)
                return (size_t)(getRawBytes<uint8_t>(curr_index++, 0) & (uint8_t)BitMask::ARRAY4);
            else if (type == Type::ARRAY16)
                return (size_t)getRawBytes<uint16_t>(curr_index++, 1);
            else if (type == Type::ARRAY32)
                return (size_t)getRawBytes<uint32_t>(curr_index++, 1);
            else
                ++curr_index;

            return 0;
        }

        // ---------- MAP format family ----------

        size_t unpackMapSize()
        {
            Type type = getType();
            if (type == Type::MAP4)
                return (size_t)(getRawBytes<uint8_t>(curr_index++, 0) & (uint8_t)BitMask::MAP4);
            else if (type == Type::MAP16)
                return (size_t)getRawBytes<uint16_t>(curr_index++, 1);
            else if (type == Type::MAP32)
                return (size_t)getRawBytes<uint32_t>(curr_index++, 1);
            else
                ++curr_index;

            return 0;
        }


        // ---------- EXT format family ----------

        // ---------- TIMESTAMP format family ----------



        /////////////////////////////////////////
        // ---------- type checkers ---------- //
        /////////////////////////////////////////

        // ---------- NIL format family ----------
        // - N/A

        template <typename T>
        auto unpackable(const T& value) const
        -> typename std::enable_if<std::is_same<T, object::NIL>::value, bool>::type
        {
            (void)value;
            return isNil();
        }


        // ---------- BOOL format family ----------
        // - bool

        template <typename T>
        auto unpackable(const T& value) const
        -> typename std::enable_if<std::is_same<T, bool>::value, bool>::type
        {
            (void)value;
            return isBool();
        }


        // ---------- INT format family ----------
        // - char (signed/unsigned)
        // - ints (signed/unsigned)

        template <typename T>
        auto unpackable(const T& value) const
        -> typename std::enable_if <
            std::is_arithmetic<T>::value &&
            !std::is_floating_point<T>::value &&
            !std::is_same<T, bool>::value &&
            !std::is_same<typename std::remove_cv<T>::type, char*>::value &&
            !std::is_signed<T>::value,
            bool
        >::type
        {
            (void)value;
            Type type = getType();
            if      (type == Type::UINT7)  return sizeof(T) == sizeof(uint8_t);
            else if (type == Type::UINT8)  return sizeof(T) == sizeof(uint8_t);
            else if (type == Type::UINT16) return sizeof(T) == sizeof(uint16_t);
            else if (type == Type::UINT32) return sizeof(T) == sizeof(uint32_t);
            else if (type == Type::UINT64) return sizeof(T) == sizeof(uint64_t);
            else
                return false;
        }

        template <typename T>
        auto unpackable(const T& value) const
        -> typename std::enable_if <
            std::is_arithmetic<T>::value &&
            !std::is_floating_point<T>::value &&
            !std::is_same<T, bool>::value &&
            !std::is_same<typename std::remove_cv<T>::type, char*>::value &&
            std::is_signed<T>::value,
            bool
        >::type
        {
            (void)value;
            Type type = getType();
            if      (type == Type::INT5)  return sizeof(T) == sizeof(int8_t);
            else if (type == Type::INT8)  return sizeof(T) == sizeof(int8_t);
            else if (type == Type::INT16) return sizeof(T) == sizeof(int16_t);
            else if (type == Type::INT32) return sizeof(T) == sizeof(int32_t);
            else if (type == Type::INT64) return sizeof(T) == sizeof(int64_t);
            else
                return false;
        }


        // ---------- FLOAT format family ----------
        // - float
        // - double

        template <typename T>
        auto unpackable(const T& value) const
        -> typename std::enable_if <
            std::is_arithmetic<T>::value &&
            std::is_floating_point<T>::value,
            bool
        >::type
        {
            (void)value;
            Type type = getType();
            if      (type == Type::FLOAT32) return sizeof(T) == sizeof(float);
            else if (type == Type::FLOAT64) return sizeof(T) == sizeof(double);
            else
                return false;
        }


        // ---------- STRING format family ----------
        // - char*
        // - char[]
        // - std::string

        bool unpackable(const str_t& str) const
        {
            (void)str;
            Type type = getType();
            return ((type == Type::STR5) || (type == Type::STR8) || (type == Type::STR16) || (type == Type::STR32));
        }


        // ---------- BIN format family ----------
        // - unsigned char*
        // - unsigned char[]
        // - std::vector<char>
        // - std::vector<unsigned char>
        // - std::array<char>
        // - std::array<unsigned char>

        template <typename T = uint8_t>
        auto unpackable(const bin_t<T>& bin) const
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value,
            bool
        >::type
        {
            (void)bin;
            Type type = getType();
            return ((type == Type::BIN8) || (type == Type::BIN16) || (type == Type::BIN32));
        }

#ifndef HT_SERIAL_MSGPACK_DISABLE_STL

        template <typename T, size_t N>
        auto unpackable(const std::array<T, N>& bin) const
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value,
            bool
        >::type
        {
            (void)bin;
            Type type = getType();
            return ((type == Type::BIN8) || (type == Type::BIN16) || (type == Type::BIN32));
        }

#endif // HT_SERIAL_MSGPACK_DISABLE_STL


        // ---------- ARRAY format family ----------
        // - T[]
        // - std::vector
        // - std::array
        // - std::deque
        // - std::pair
        // - std::tuple
        // - std::list
        // - std::forward_list
        // - std::set
        // - std::unordered_set
        // - std::multiset
        // - std::unordered_multiset

        template <typename T>
        auto unpackable(const arr_t<T>& arr) const
        -> typename std::enable_if<
            !std::is_same<T, char>::value &&
            !std::is_same<T, uint8_t>::value,
            bool
        >::type
        {
            (void)arr;
            Type type = getType();
            return ((type == Type::ARRAY4) || (type == Type::ARRAY16) || (type == Type::ARRAY32));
        }

#ifndef HT_SERIAL_MSGPACK_DISABLE_STL

        template <typename T, size_t N>
        auto unpackable(const std::array<T, N>& arr) const
        -> typename std::enable_if<
            !std::is_same<T, char>::value &&
            !std::is_same<T, uint8_t>::value,
            bool
        >::type
        {
            (void)arr;
            Type type = getType();
            return ((type == Type::ARRAY4) || (type == Type::ARRAY16) || (type == Type::ARRAY32));
        }

        template <typename T>
        bool unpackable(const std::deque<T>& arr) const
        {
            (void)arr;
            Type type = getType();
            return ((type == Type::ARRAY4) || (type == Type::ARRAY16) || (type == Type::ARRAY32));
        }

        template <typename T, typename U>
        bool unpackable(const std::pair<T, U>& arr) const
        {
            (void)arr;
            Type type = getType();
            return ((type == Type::ARRAY4) || (type == Type::ARRAY16) || (type == Type::ARRAY32));
        }

        // std::tuple???

        template <typename T>
        bool unpackable(const std::list<T>& arr) const
        {
            (void)arr;
            Type type = getType();
            return ((type == Type::ARRAY4) || (type == Type::ARRAY16) || (type == Type::ARRAY32));
        }

        template <typename T>
        bool unpackable(const std::forward_list<T>& arr) const
        {
            (void)arr;
            Type type = getType();
            return ((type == Type::ARRAY4) || (type == Type::ARRAY16) || (type == Type::ARRAY32));
        }

        template <typename T>
        bool unpackable(const std::set<T>& arr) const
        {
            (void)arr;
            Type type = getType();
            return ((type == Type::ARRAY4) || (type == Type::ARRAY16) || (type == Type::ARRAY32));
        }

        template <typename T>
        bool unpackable(const std::unordered_set<T>& arr) const
        {
            (void)arr;
            Type type = getType();
            return ((type == Type::ARRAY4) || (type == Type::ARRAY16) || (type == Type::ARRAY32));
        }

        template <typename T>
        bool unpackable(const std::multiset<T>& arr) const
        {
            (void)arr;
            Type type = getType();
            return ((type == Type::ARRAY4) || (type == Type::ARRAY16) || (type == Type::ARRAY32));
        }

        template <typename T>
        bool unpackable(std::unordered_multiset<T>& arr) const
        {
            (void)arr;
            Type type = getType();
            return ((type == Type::ARRAY4) || (type == Type::ARRAY16) || (type == Type::ARRAY32));
        }

#endif // HT_SERIAL_MSGPACK_DISABLE_STL


        // ---------- MAP format family ----------
        // - std::map
        // - std::multimap
        // - std::unordered_map *
        // - std::unordered_multimap *
        // * : not supported in arduino

        template <typename T, typename U>
        bool unpackable(const map_t<T, U>& mp) const
        {
            (void)mp;
            Type type = getType();
            return ((type == Type::MAP4) || (type == Type::MAP16) || (type == Type::MAP32));
        }

#ifndef HT_SERIAL_MSGPACK_DISABLE_STL

        template <typename T, typename U>
        bool unpackable(const std::multimap<T, U>& mp) const
        {
            (void)mp;
            Type type = getType();
            return ((type == Type::MAP4) || (type == Type::MAP16) || (type == Type::MAP32));
        }

        template <typename T, typename U>
        bool unpackable(const std::unordered_map<T, U>& mp) const
        {
            (void)mp;
            Type type = getType();
            return ((type == Type::MAP4) || (type == Type::MAP16) || (type == Type::MAP32));
        }

        template <typename T, typename U>
        bool unpack(const std::unordered_multimap<T, U>& mp) const
        {
            (void)mp;
            Type type = getType();
            return ((type == Type::MAP4) || (type == Type::MAP16) || (type == Type::MAP32));
        }

#endif // HT_SERIAL_MSGPACK_DISABLE_STL


        // ---------- EXT format family ----------
        // no pre-defined type



        bool isNil() const { return getType() == Type::NIL; }
        bool isBool() const { return getType() == Type::BOOL; }
        bool isUInt7() const { return getType() == Type::UINT7; }
        bool isUInt8() const { return getType() == Type::UINT8; }
        bool isUInt16() const { return getType() == Type::UINT16; }
        bool isUInt32() const { return getType() == Type::UINT32; }
        bool isUInt64() const { return getType() == Type::UINT64; }
        bool isInt5() const { return getType() == Type::INT5; }
        bool isInt8() const { return getType() == Type::INT8; }
        bool isInt16() const { return getType() == Type::INT16; }
        bool isInt32() const { return getType() == Type::INT32; }
        bool isInt64() const { return getType() == Type::INT64; }
        bool isFloat32() const { return getType() == Type::FLOAT32; }
        bool isFloat64() const { return getType() == Type::FLOAT64; }
        bool isStr5() const { return getType() == Type::STR5; }
        bool isStr8() const { return getType() == Type::STR8; }
        bool isStr16() const { return getType() == Type::STR16; }
        bool isStr32() const { return getType() == Type::STR32; }
        bool isBin8() const { return getType() == Type::BIN8; }
        bool isBin16() const { return getType() == Type::BIN16; }
        bool isBin32() const { return getType() == Type::BIN32; }
        bool isArray4() const { return getType() == Type::ARRAY4; }
        bool isArray16() const { return getType() == Type::ARRAY16; }
        bool isArray32() const { return getType() == Type::ARRAY32; }
        bool isMap4() const { return getType() == Type::MAP4; }
        bool isMap16() const { return getType() == Type::MAP16; }
        bool isMap32() const { return getType() == Type::MAP32; }
        bool isFixExt1() const { return getType() == Type::FIXEXT1; }
        bool isFixExt2() const { return getType() == Type::FIXEXT2; }
        bool isFixExt4() const { return getType() == Type::FIXEXT4; }
        bool isFixExt8() const { return getType() == Type::FIXEXT8; }
        bool isFixExt16() const { return getType() == Type::FIXEXT16; }
        bool isExt8() const { return getType() == Type::EXT8; }
        bool isExt16() const { return getType() == Type::EXT16; }
        bool isExt32() const { return getType() == Type::EXT32; }
        // bool isTimeStamp32() const { return getType() == Type::TIMESTAMP32; }
        // bool isTimeStamp64() const { return getType() == Type::TIMESTAMP64; }
        // bool isTimeStamp96() const { return getType() == Type::TIMESTAMP96; }


private:

        template <typename DataType>
        DataType getRawBytes(const size_t i, const size_t offset) const
        {
            DataType data;
            const auto size = sizeof(DataType);
            for (uint8_t b = 0; b < size; ++b)
            {
                uint8_t distance = size - 1 - b;
                auto index = indices[i] + offset + distance;
                ((uint8_t*)&data)[b] = raw_data[index];
            }
            return std::move(data);
        }

        Type getType() const
        {
            return getType(curr_index);
        }

        Type getType(const size_t i) const
        {
            uint8_t raw = getRawBytes<uint8_t>(i, 0);
            if      (raw <  (uint8_t)Type::MAP4)   return Type::UINT7;
            else if (raw <  (uint8_t)Type::ARRAY4) return Type::MAP4;
            else if (raw <  (uint8_t)Type::STR5)   return Type::ARRAY4;
            else if (raw <  (uint8_t)Type::NIL)    return Type::STR5;
            else if (raw == (uint8_t)Type::NIL)    return Type::NIL;
            else if (raw == (uint8_t)Type::NA)     return Type::NA;
            else if (raw <  (uint8_t)Type::BIN8)   return Type::BOOL;
            else if (raw <  (uint8_t)Type::INT5)   return (Type)raw;
            else                                   return Type::INT5;
        }

        size_t getElementSize(size_t i) const
        {
            Type type = getType(i);
            switch(type)
            {
                // size of ARRAY & MAP is size of OBJECTs
                // so the size from header to first object is returned
                case Type::NIL:
                case Type::BOOL:
                case Type::UINT7:
                case Type::INT5:
                case Type::ARRAY4:
                case Type::MAP4:
                    return 1;
                case Type::UINT8:
                case Type::INT8:
                    return 2;
                case Type::UINT16:
                case Type::INT16:
                case Type::ARRAY16:
                case Type::MAP16:
                case Type::FIXEXT1:
                    return 3;
                case Type::FIXEXT2:
                    return 4;
                case Type::UINT32:
                case Type::INT32:
                case Type::FLOAT32:
                case Type::ARRAY32:
                case Type::MAP32:
                    return 5;
                case Type::FIXEXT4: // includes case Type::TIMESTAMP32:
                    return 6;
                case Type::UINT64:
                case Type::INT64:
                case Type::FLOAT64:
                    return 9;
                case Type::FIXEXT8: // includes case Type::TIMESTAMP64:
                    return 10;
                case Type::FIXEXT16:
                    return 18;
                case Type::STR5:
                    return (getRawBytes<uint8_t>(i, 0) & (uint8_t)BitMask::STR5) + 1;
                case Type::STR8:
                case Type::BIN8:
                    return getRawBytes<uint8_t>(i, 1) + sizeof(uint8_t) + 1;
                case Type::STR16:
                case Type::BIN16:
                    return (size_t)getRawBytes<uint16_t>(i, 1) + sizeof(uint16_t) + 1;
                case Type::STR32:
                case Type::BIN32:
                    return (size_t)getRawBytes<uint32_t>(i, 1) + sizeof(uint32_t) + 1;
                case Type::EXT8: // includes case Type::TIMESTAMP96:
                    return (size_t)getRawBytes<uint8_t>(i, 1) + sizeof(uint8_t) + 1 + 1;
                case Type::EXT16:
                    return (size_t)getRawBytes<uint16_t>(i, 1) + sizeof(uint16_t) + 1 + 1;
                case Type::EXT32:
                    return (size_t)getRawBytes<uint32_t>(i, 1) + sizeof(uint32_t) + 1 + 1;
                default:
                    return 0;
            }
        }

#ifndef HT_SERIAL_MSGPACK_DISABLE_STL
        template <template <typename...> class C, class T>
        void unpackArrayContainerArray(C<T>& arr)
#else
        template <typename T>
        void unpackArrayContainerArray(arr_t<T>& arr)
#endif
        {
            const size_t size = unpackArraySize();
            if (arr.size() == size)
            {
                for (auto& a : arr)
                {
                    unpack(a);
                }
            }
            else if (size == 0)
            {
                // TODO: error handling
            }
            else
            {
                arr.clear();
                for (size_t a = 0; a < size; ++a)
                {
                    arr.emplace_back(T());
                    unpack(arr.back());
                }
            }
        }

#ifndef HT_SERIAL_MSGPACK_DISABLE_STL
        template <template <typename...> class C, class T>
        void unpackArrayContainerSet(C<T>& arr)
#else
        template <typename T>
        void unpackArrayContainerSet(arr_t<T>& arr)
#endif
        {
            const size_t size = unpackArraySize();
            if (size == 0)
            {
                // TODO: error handling
            }
            else
            {
                arr.clear();
                for (size_t a = 0; a < size; ++a)
                {
                    T t;
                    unpack(t);
                    arr.emplace(t);
                }
            }
        }

#ifndef HT_SERIAL_MSGPACK_DISABLE_STL
        template <template <typename...> class C, class T, class U>
        void unpackMapContainer(C<T, U>& mp)
#else
        template <typename T, typename U>
        void unpackMapContainer(map_t<T, U>& mp)
#endif
        {
            const size_t size = unpackMapSize();
            if (size == 0)
            {
                // TODO: error handling
            }
            else
            {
                mp.clear();
                for (size_t a = 0; a < size; ++a)
                {
                    T t; U u;
                    unpack(t);
                    unpack(u);
                    mp.emplace(make_pair(t, u));
                }
            }
        }

    };

} // msgpack
} // serial
} // ht

#endif // HT_SERIAL_MSGPACK_UNPACKER_H
