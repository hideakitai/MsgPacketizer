
#pragma once
#ifndef HT_SERIAL_MSGPACK_UNPACKER_H
#define HT_SERIAL_MSGPACK_UNPACKER_H

#include "util/ArxTypeTraits/ArxTypeTraits.h"
#include "util/ArxContainer/ArxContainer.h"
#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11
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
#else // Do not have libstdc++11
    // containers are disabled
#endif
#ifdef TEENSYDUINO
    #include "util/TeensyDirtySTLErrorSolution/TeensyDirtySTLErrorSolution.h"
#endif // TEENSYDUINO

#include "Types.h"
#include "util/DebugLog/DebugLog.h"

namespace ht {
namespace serial {
namespace msgpack {

    class Unpacker
    {
        uint8_t* raw_data {nullptr};
        idx_t indices;
        size_t curr_index {0};
        bool b_decoded {false};

    public:

        bool feed(const uint8_t* data, const size_t size)
        {
            raw_data = (uint8_t*)data;
            for (size_t i = 0; i < size; i += getElementSize(indices.size() - 1))
                indices.emplace_back(i);
            b_decoded = (size == (indices.back() + getElementSize(indices.size() - 1)));
            return b_decoded;
        }

        template <typename First, typename ...Rest>
        void deserialize(First& first, Rest&&... rest)
        {
            unpack(first);
            deserialize(std::forward<Rest>(rest)...);
        }
        void deserialize() {}

        template <typename ...Args>
        void from_array(Args&&... args)
        {
            static arr_size_t sz;
            deserialize(sz, std::forward<Args>(args)...);
        }

        template <typename ...Args>
        void from_map(Args&&... args)
        {
            if ((sizeof...(args) % 2) == 0)
            {
                static arr_size_t sz;
                deserialize(sz, std::forward<Args>(args)...);
            }
            else
            {
                LOG_WARNING("serialize arg size not matched for map :", sizeof...(args));
            }
        }

        template <typename... Ts>
        void to_tuple(std::tuple<Ts...>& t)
        {
            to_tuple(std::make_index_sequence<sizeof...(Ts)>(), t);
        }
        template <typename... Ts, size_t... Is>
        void to_tuple(std::index_sequence<Is...>&&, std::tuple<Ts...>& t)
        {
            size_t i {0};
            idx_t {(unpack(std::get<Is>(t)), i++)...};
        }
        void to_tuple() {}

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
        -> typename std::enable_if<std::is_same<T, object::nil_t>::value>::type
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
            std::is_integral<T>::value &&
            !std::is_same<T, bool>::value &&
            !std::is_same<typename std::remove_cv<T>::type, char*>::value &&
            !std::is_signed<T>::value
        >::type
        {
            if (unpackable(value))
            {
                switch (getType())
                {
                    case Type::UINT7:  value = (T)unpackUInt7();  break;
                    case Type::UINT8:  value = (T)unpackUInt8();  break;
                    case Type::UINT16: value = (T)unpackUInt16(); break;
                    case Type::UINT32: value = (T)unpackUInt32(); break;
                    case Type::UINT64: value = (T)unpackUInt64(); break;
                    default:                                      break;
                }
            }
            else
            {
                LOG_WARNING("unpack type is not matched :", (int)getType());
                value = 0;
                ++curr_index;
            }
        }

        template <typename T>
        auto unpack(T& value)
        -> typename std::enable_if <
            std::is_arithmetic<T>::value &&
            std::is_integral<T>::value &&
            !std::is_same<T, bool>::value &&
            !std::is_same<typename std::remove_cv<T>::type, char*>::value &&
            std::is_signed<T>::value
        >::type
        {
            if (unpackable(value))
            {
                switch (getType())
                {
                    case Type::INT5:   value = (T)unpackInt5();   break;
                    case Type::INT8:   value = (T)unpackInt8();   break;
                    case Type::INT16:  value = (T)unpackInt16();  break;
                    case Type::INT32:  value = (T)unpackInt32();  break;
                    case Type::INT64:  value = (T)unpackInt64();  break;
                    case Type::UINT7:  value = (T)unpackUInt7();  break;
                    case Type::UINT8:
                    {
                        if (sizeof(T) > sizeof(int8_t))
                            value = (T)unpackUInt8();
                        else
                        {
                            uint8_t v = unpackUInt8();
                            if (v <= (uint8_t)std::numeric_limits<int8_t>::max())
                                value = (T)v;
                            else
                                value = 0;
                        }
                        break;
                    }
                    case Type::UINT16:
                    {
                        if (sizeof(T) > sizeof(int16_t))
                            value = (T)unpackUInt16();
                        else
                        {
                            uint16_t v = unpackUInt16();
                            if (v <= (uint16_t)std::numeric_limits<int16_t>::max())
                                value = (T)v;
                            else
                                value = 0;
                        }
                        break;
                    }
                    case Type::UINT32:
                    {
                        if (sizeof(T) > sizeof(int32_t))
                            value = (T)unpackUInt32();
                        else
                        {
                            uint32_t v = unpackUInt32();
                            if (v <= (uint32_t)std::numeric_limits<int32_t>::max())
                                value = (T)v;
                            else
                                value = 0;
                        }
                        break;
                    }
                    case Type::UINT64:
                    {
                        if (sizeof(T) > sizeof(int64_t))
                            value = (T)unpackUInt64();
                        else
                        {
                            uint64_t v = unpackUInt64();
                            if (v <= (uint64_t)std::numeric_limits<int64_t>::max())
                                value = (T)v;
                            else
                                value = 0;
                        }
                        break;
                    }
                    default:
                    {
                        value = 0;
                        break;
                    }
                }
            }
            else
            {
                LOG_WARNING("unpack type is not matched :", (int)getType());
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
            if (unpackable(value))
            {
                switch (getType())
                {
                    case Type::UINT7:   value = (T)unpackUInt7();   break;
                    case Type::UINT8:   value = (T)unpackUInt8();   break;
                    case Type::UINT16:  value = (T)unpackUInt16();  break;
                    case Type::UINT32:  value = (T)unpackUInt32();  break;
                    case Type::UINT64:  value = (T)unpackUInt64();  break;
                    case Type::INT5:    value = (T)unpackInt5();    break;
                    case Type::INT8:    value = (T)unpackInt8();    break;
                    case Type::INT16:   value = (T)unpackInt16();   break;
                    case Type::INT32:   value = (T)unpackInt32();   break;
                    case Type::INT64:   value = (T)unpackInt64();   break;
                    case Type::FLOAT32: value = (T)unpackFloat32(); break;
                    case Type::FLOAT64: value = (T)unpackFloat64(); break;
                    default:                                        break;
                }
            }
            else
            {
                LOG_WARNING("unpack type is not matched :", (int)getType());
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
            if      (isStr5())  str = unpackString5();
            else if (isStr8())  str = unpackString8();
            else if (isStr16()) str = unpackString16();
            else if (isStr32()) str = unpackString32();
            else
            {
                LOG_WARNING("unpack type is not matched :", (int)getType());
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

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11

        template <typename T, size_t N>
        auto unpack(std::array<T, N>& bin)
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value
        >::type
        {
            bin = unpackBinary<T, N>();
        }

#endif // Do not have libstdc++11


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

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11

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
                for (auto& a : arr) unpack(a);
            }
            else
            {
                LOG_WARNING("unpack array size is not matched :", size, "must be", N);
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
                LOG_WARNING("unpack array size is not matched :", size, "must be", 1);
            }
        }

        template <typename... Args>
        void unpack(std::tuple<Args...>& t)
        {
            const size_t size = unpackArraySize();
            if (sizeof...(Args) == size)
            {
                to_tuple(t);
            }
            else
            {
                LOG_WARNING("unpack array size is not matched :", size, "must be", sizeof...(Args));
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
                for (auto& a : arr) unpack(a);
            }
            else if (size == 0)
            {
                LOG_WARNING("unpack array size is not matched :", size, "must be", arr_size);
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

#endif // Do not have libstdc++11


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

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11

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

#endif // Do not have libstdc++11


        // ---------- EXT format family ----------

        void unpack(object::ext& e)
        {
            e = unpackExt();
        }


        // ---------- TIMESTAMP format family ----------

        void unpack(object::timespec& t)
        {
            t = unpackTimestamp();
        }


        // ---------- CUSTOM format ----------

        template <typename C>
        auto unpack(C& c)
        -> typename std::enable_if<has_from_msgpack<C, Unpacker&>::value>::type
        {
            c.from_msgpack(*this);
        }


        // ---------- Array/Map Size format ----------

        void unpack(arr_size_t& t)
        {
            t = arr_size_t(unpackArraySize());
        }

        void unpack(map_size_t& t)
        {
            t = map_size_t(unpackMapSize());
        }


        /////////////////////////////////////////
        // ---------- msgpack types ---------- //
        /////////////////////////////////////////

        // ---------- NIL format family ----------

        bool unpackNil()
        {
            bool ret = isNil();
            ++curr_index;
            return ret;
        }

        // ---------- BOOL format family ----------

        bool unpackBool()
        {
            bool ret = isBool() ? (bool)(getRawBytes<uint8_t>(curr_index, 0) & (uint8_t)BitMask::BOOL) : 0;
            ++curr_index;
            return ret;
        }

        // ---------- INT format family ----------

        uint8_t unpackUInt7()
        {
            uint8_t ret = isUInt7() ? (uint8_t)(getRawBytes<uint8_t>(curr_index, 0) & (uint8_t)BitMask::UINT7) : 0;
            ++curr_index;
            return ret;
        }

        uint8_t unpackUInt8()
        {
            uint8_t ret = isUInt8() ? getRawBytes<uint8_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        uint16_t unpackUInt16()
        {
            uint16_t ret = isUInt16() ? getRawBytes<uint16_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        uint32_t unpackUInt32()
        {
            uint32_t ret = isUInt32() ? getRawBytes<uint32_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        uint64_t unpackUInt64()
        {
            uint64_t ret = isUInt64() ? getRawBytes<uint64_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        int8_t unpackInt5()
        {
            int8_t ret = isInt5() ? getRawBytes<uint8_t>(curr_index, 0) : 0;
            ++curr_index;
            return ret;
        }

        int8_t unpackInt8()
        {
            int8_t ret = isInt8() ? getRawBytes<uint8_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        int16_t unpackInt16()
        {
            int16_t ret = isInt16() ? getRawBytes<int16_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        int32_t unpackInt32()
        {
            int32_t ret = isInt32() ? getRawBytes<int32_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        int64_t unpackInt64()
        {
            int64_t ret = isInt64() ? getRawBytes<int64_t>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        // ---------- FLOAT format family ----------

        float unpackFloat32()
        {
            float ret = isFloat32() ? getRawBytes<float>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
        }

        double unpackFloat64()
        {
#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11
            double ret = isFloat64() ? getRawBytes<double>(curr_index, 1) : 0;
            ++curr_index;
            return ret;
#else // Do not have libstdc++11
            return (isFloat32() || isFloat64()) ? unpackFloat32() : 0; // Uno, etc. does not support double
#endif // Do not have libstdc++11
        }

        // ---------- STR format family ----------

        str_t unpackString5()
        {
            str_t str("");
            if (isStr5())
            {
                uint8_t size = getRawBytes<uint8_t>(curr_index, 0) & (uint8_t)BitMask::STR5;
                for (uint8_t c = 0; c < size; ++c) str += getRawBytes<char>(curr_index, c + 1);
            }
            ++curr_index;
            return str;
        }

        str_t unpackString8()
        {
            str_t str("");
            if (isStr8())
            {
                uint8_t size = getRawBytes<uint8_t>(curr_index, 1);
                for (uint8_t c = 0; c < size; ++c) str += getRawBytes<char>(curr_index, c + 1 + sizeof(uint8_t));
            }
            ++curr_index;
            return str;
        }

        str_t unpackString16()
        {
            str_t str("");
            if (isStr16())
            {
                uint16_t size = getRawBytes<uint16_t>(curr_index, 1);
                for (uint16_t c = 0; c < size; ++c) str += getRawBytes<char>(curr_index, c + 1 + sizeof(uint16_t));
            }
            ++curr_index;
            return str;
        }

        str_t unpackString32()
        {
            str_t str("");
            if (isStr32())
            {
                uint32_t size = getRawBytes<uint32_t>(curr_index, 1);
                for (uint32_t c = 0; c < size; ++c) str += getRawBytes<char>(curr_index, c + 1 + sizeof(uint32_t));
            }
            ++curr_index;
            return str;
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
            if      (isBin8())  return unpackBinary8<T>();
            else if (isBin16()) return unpackBinary16<T>();
            else if (isBin32()) return unpackBinary32<T>();
            else                ++curr_index;
            return bin_t<T>();
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
            if (isBin8())
            {
                uint8_t size = getRawBytes<uint8_t>(curr_index, 1);
                for (uint8_t v = 0; v < size; ++v) data.emplace_back(getRawBytes<T>(curr_index, v + 1 + sizeof(uint8_t)));
            }
            ++curr_index;
            return data;
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
            if (isBin16())
            {
                uint16_t size = getRawBytes<uint16_t>(curr_index, 1);
                for (uint16_t v = 0; v < size; ++v) data.emplace_back(getRawBytes<T>(curr_index, v + 1 + sizeof(uint16_t)));
            }
            ++curr_index;
            return data;
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
            if (isBin32())
            {
                uint32_t size = getRawBytes<uint32_t>(curr_index, 1);
                for (uint32_t v = 0; v < size; ++v) data.emplace_back(getRawBytes<T>(curr_index, v + 1 + sizeof(uint32_t)));
            }
            ++curr_index;
            return data;
        }

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11

        template <typename T, size_t N>
        auto unpackBinary()
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value,
            std::array<T, N>
        >::type
        {
            if      (isBin8())  return unpackBinary8<T, N>();
            else if (isBin16()) return unpackBinary16<T, N>();
            else if (isBin32()) return unpackBinary32<T, N>();
            else                ++curr_index;
            return std::array<T, N>();
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
            if (isBin8())
            {
                uint8_t size = getRawBytes<uint8_t>(curr_index, 1);
                for (uint8_t v = 0; v < size; ++v) data[v] = getRawBytes<T>(curr_index, v + 1 + sizeof(uint8_t));
            }
            ++curr_index;
            return data;
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
            if (isBin16())
            {
                uint16_t size = getRawBytes<uint16_t>(curr_index, 1);
                for (uint16_t v = 0; v < size; ++v) data[v] = getRawBytes<T>(curr_index, v + 1 + sizeof(uint16_t));
            }
            ++curr_index;
            return data;
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
            if (isBin32())
            {
                uint32_t size = getRawBytes<uint32_t>(curr_index, 1);
                for (uint32_t v = 0; v < size; ++v) data[v] = getRawBytes<T>(curr_index, v + 1 + sizeof(uint32_t));
            }
            ++curr_index;
            return data;
        }

#endif // Do not have libstdc++11


        // ---------- ARRAY format family ----------

        size_t unpackArraySize()
        {
            if (isArray4())
                return (size_t)(getRawBytes<uint8_t>(curr_index++, 0) & (uint8_t)BitMask::ARRAY4);
            else if (isArray16())
                return (size_t)getRawBytes<uint16_t>(curr_index++, 1);
            else if (isArray32())
                return (size_t)getRawBytes<uint32_t>(curr_index++, 1);
            else
                ++curr_index;

            return 0;
        }

        // ---------- MAP format family ----------

        size_t unpackMapSize()
        {
            if (isMap4())
                return (size_t)(getRawBytes<uint8_t>(curr_index++, 0) & (uint8_t)BitMask::MAP4);
            else if (isMap16())
                return (size_t)getRawBytes<uint16_t>(curr_index++, 1);
            else if (isMap32())
                return (size_t)getRawBytes<uint32_t>(curr_index++, 1);
            else
                ++curr_index;

            return 0;
        }


        // ---------- EXT format family ----------

        object::ext unpackExt()
        {
            if (isFixExt1())
            {
                int8_t ext_type = getRawBytes<int8_t>(curr_index, 1);
                uint8_t* ptr = getRawBytePtr(curr_index++, 2);
                if (ptr) return object::ext(ext_type, ptr, 1);
            }
            else if (isFixExt2())
            {
                int8_t ext_type = getRawBytes<int8_t>(curr_index, 1);
                uint8_t* ptr = getRawBytePtr(curr_index++, 2);
                if (ptr) return object::ext(ext_type, ptr, 2);
            }
            else if (isFixExt4())
            {
                int8_t ext_type = getRawBytes<int8_t>(curr_index, 1);
                uint8_t* ptr = getRawBytePtr(curr_index++, 2);
                if (ptr) return object::ext(ext_type, ptr, 4);
            }
            else if (isFixExt8())
            {
                int8_t ext_type = getRawBytes<int8_t>(curr_index, 1);
                uint8_t* ptr = getRawBytePtr(curr_index++, 2);
                if (ptr) return object::ext(ext_type, ptr, 8);
            }
            else if (isFixExt16())
            {
                int8_t ext_type = getRawBytes<int8_t>(curr_index, 1);
                uint8_t* ptr = getRawBytePtr(curr_index++, 2);
                if (ptr) return object::ext(ext_type, ptr, 16);
            }
            else if (isExt8())
            {
                uint8_t size = getRawBytes<uint8_t>(curr_index, 1);
                int8_t ext_type = getRawBytes<int8_t>(curr_index, 2);
                uint8_t* ptr = getRawBytePtr(curr_index++, 3);
                if (ptr) return object::ext(ext_type, ptr, size);
            }
            else if (isExt16())
            {
                uint16_t size = getRawBytes<uint16_t>(curr_index, 1);
                int8_t ext_type = getRawBytes<int8_t>(curr_index, 3);
                uint8_t* ptr = getRawBytePtr(curr_index++, 4);
                if (ptr) return object::ext(ext_type, ptr, size);
            }
            else if (isExt32())
            {
                uint32_t size = getRawBytes<uint32_t>(curr_index, 1);
                int8_t ext_type = getRawBytes<int8_t>(curr_index, 5);
                uint8_t* ptr = getRawBytePtr(curr_index++, 6);
                if (ptr) return object::ext(ext_type, ptr, size);
            }

            return object::ext();
        }


        // ---------- TIMESTAMP format family ----------

        object::timespec unpackTimestamp()
        {
            object::timespec ts;
            if (isTimestamp32())
            {
                int8_t ext_type = getRawBytes<int8_t>(curr_index, 1);
                if (ext_type == -1)
                {
                    ts.tv_nsec = 0;
                    ts.tv_sec = getRawBytes<uint32_t>(curr_index++, 2);
                }
                else
                {
                    LOG_WARNING("unpack timestamp ext-type not matched :", (int)ext_type, "must be -1");
                }
            }
            else if (isTimestamp64())
            {
                int8_t ext_type = getRawBytes<int8_t>(curr_index, 1);
                if (ext_type == -1)
                {
                    uint64_t data64 = getRawBytes<uint64_t>(curr_index++, 2);
                    ts.tv_nsec = data64 >> 34;
                    ts.tv_sec = data64 & 0x00000003ffffffffL;
                }
                else
                {
                    LOG_WARNING("unpack timestamp ext-type not matched :", (int)ext_type, "must be -1");
                }
            }
            else if (isTimestamp96())
            {
                uint8_t size = getRawBytes<uint8_t>(curr_index, 1);
                if (size == 12)
                {
                    int8_t ext_type = getRawBytes<int8_t>(curr_index, 2);
                    if (ext_type == -1)
                    {
                        ts.tv_nsec = getRawBytes<uint32_t>(curr_index, 3);
                        ts.tv_sec = getRawBytes<uint64_t>(curr_index++, 7);
                    }
                    else
                    {
                        LOG_WARNING("unpack timestamp ext-type not matched :", (int)ext_type, "must be -1");
                    }
                }
                else
                {
                    LOG_WARNING("unpack timestamp ext-size not matched :", (int)size, "must be 12");
                }
            }
            else
            {
                LOG_WARNING("unpack timestamp object-type not matched :", (int)getType());
            }
            return ts;
        }


        /////////////////////////////////////////
        // ---------- type checkers ---------- //
        /////////////////////////////////////////

        // ---------- NIL format family ----------
        // - N/A

        template <typename T>
        auto unpackable(const T& value) const
        -> typename std::enable_if<std::is_same<T, object::nil_t>::value, bool>::type
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
        auto unpackable(const T&) const
        -> typename std::enable_if <
            std::is_arithmetic<T>::value &&
            !std::is_floating_point<T>::value &&
            !std::is_same<T, bool>::value &&
            !std::is_same<typename std::remove_cv<T>::type, char*>::value &&
            !std::is_signed<T>::value,
            bool
        >::type
        {
            if      (isUInt7())  return sizeof(T) >= sizeof(uint8_t);
            else if (isUInt8())  return sizeof(T) >= sizeof(uint8_t);
            else if (isUInt16()) return sizeof(T) >= sizeof(uint16_t);
            else if (isUInt32()) return sizeof(T) >= sizeof(uint32_t);
            else if (isUInt64()) return sizeof(T) >= sizeof(uint64_t);
            else
                return false;
        }

        template <typename T>
        auto unpackable(const T&) const
        -> typename std::enable_if <
            std::is_arithmetic<T>::value &&
            !std::is_floating_point<T>::value &&
            !std::is_same<T, bool>::value &&
            !std::is_same<typename std::remove_cv<T>::type, char*>::value &&
            std::is_signed<T>::value,
            bool
        >::type
        {
            if      (isInt5())   return sizeof(T) >= sizeof(int8_t);
            else if (isInt8())   return sizeof(T) >= sizeof(int8_t);
            else if (isInt16())  return sizeof(T) >= sizeof(int16_t);
            else if (isInt32())  return sizeof(T) >= sizeof(int32_t);
            else if (isInt64())  return sizeof(T) >= sizeof(int64_t);
            else if (isUInt7())  return sizeof(T) >= sizeof(int8_t);
            else if (isUInt8())  return sizeof(T) >= sizeof(int8_t);
            else if (isUInt16()) return sizeof(T) >= sizeof(int16_t);
            else if (isUInt32()) return sizeof(T) >= sizeof(int32_t);
            else if (isUInt64()) return sizeof(T) >= sizeof(int64_t);
            else
                return false;
        }


        // ---------- FLOAT format family ----------
        // - float
        // - double

        template <typename T>
        auto unpackable(const T&) const
        -> typename std::enable_if <
            std::is_arithmetic<T>::value &&
            std::is_floating_point<T>::value,
            bool
        >::type
        {
            switch(getType())
            {
                case Type::UINT7:
                case Type::UINT8:
                case Type::UINT16:
                case Type::UINT32:
                case Type::UINT64:
                case Type::INT5:
                case Type::INT8:
                case Type::INT16:
                case Type::INT32:
                case Type::INT64:
                case Type::FLOAT32:
                case Type::FLOAT64:
                    return true;
                default:
                    return false;
            }
        }


        // ---------- STRING format family ----------
        // - char*
        // - char[]
        // - std::string

        bool unpackable(const str_t& str) const
        {
            (void)str;
            return isStr();
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
            return isBin();
        }

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11

        template <typename T, size_t N>
        auto unpackable(const std::array<T, N>& bin) const
        -> typename std::enable_if<
            std::is_same<T, char>::value ||
            std::is_same<T, uint8_t>::value,
            bool
        >::type
        {
            (void)bin;
            return isBin();
        }

#endif // Do not have libstdc++11


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
            return isArray();
        }

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11

        template <typename T, size_t N>
        auto unpackable(const std::array<T, N>& arr) const
        -> typename std::enable_if<
            !std::is_same<T, char>::value &&
            !std::is_same<T, uint8_t>::value,
            bool
        >::type
        {
            (void)arr;
            return isArray();
        }

        template <typename T>
        bool unpackable(const std::deque<T>& arr) const
        {
            (void)arr;
            return isArray();
        }

        template <typename T, typename U>
        bool unpackable(const std::pair<T, U>& arr) const
        {
            (void)arr;
            return isArray();
        }

        template <typename... Args>
        bool unpackable(const std::tuple<Args...>& arr) const
        {
            (void)arr;
            return isArray();
        }

        template <typename T>
        bool unpackable(const std::list<T>& arr) const
        {
            (void)arr;
            return isArray();
        }

        template <typename T>
        bool unpackable(const std::forward_list<T>& arr) const
        {
            (void)arr;
            return isArray();
        }

        template <typename T>
        bool unpackable(const std::set<T>& arr) const
        {
            (void)arr;
            return isArray();
        }

        template <typename T>
        bool unpackable(const std::unordered_set<T>& arr) const
        {
            (void)arr;
            return isArray();
        }

        template <typename T>
        bool unpackable(const std::multiset<T>& arr) const
        {
            (void)arr;
            return isArray();
        }

        template <typename T>
        bool unpackable(std::unordered_multiset<T>& arr) const
        {
            (void)arr;
            return isArray();
        }

#endif // Do not have libstdc++11


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
            return isMap();
        }

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11

        template <typename T, typename U>
        bool unpackable(const std::multimap<T, U>& mp) const
        {
            (void)mp;
            return isMap();
        }

        template <typename T, typename U>
        bool unpackable(const std::unordered_map<T, U>& mp) const
        {
            (void)mp;
            return isMap();
        }

        template <typename T, typename U>
        bool unpackable(const std::unordered_multimap<T, U>& mp) const
        {
            (void)mp;
            return isMap();
        }

#endif // Do not have libstdc++11


        // ---------- EXT format family ----------

        bool unpackable(const object::ext& e) const
        {
            (void)e;
            return isFixExt() || isExt();
        }


        // ---------- TIMESTAMP format family ----------

        bool unpackable(const object::timespec& t) const
        {
            (void)t;
            return isTimestamp();
        }


        /////////////////////////////////////////////////
        // ---------- msgpack type checkers ---------- //
        /////////////////////////////////////////////////

        bool isNil() const { return getType() == Type::NIL; }
        bool isBool() const { return getType() == Type::BOOL; }
        bool isUInt7() const { return getType() == Type::UINT7; }
        bool isUInt8() const { return getType() == Type::UINT8; }
        bool isUInt16() const { return getType() == Type::UINT16; }
        bool isUInt32() const { return getType() == Type::UINT32; }
        bool isUInt64() const { return getType() == Type::UINT64; }
        bool isUInt() const { return isUInt7() || isUInt8() || isUInt16() || isUInt32() || isUInt64(); }
        bool isInt5() const { return getType() == Type::INT5; }
        bool isInt8() const { return getType() == Type::INT8; }
        bool isInt16() const { return getType() == Type::INT16; }
        bool isInt32() const { return getType() == Type::INT32; }
        bool isInt64() const { return getType() == Type::INT64; }
        bool isInt() const { return isInt5() || isInt8() || isInt16() || isInt32() || isInt64(); }
        bool isFloat32() const { return getType() == Type::FLOAT32; }
        bool isFloat64() const { return getType() == Type::FLOAT64; }
        bool isStr5() const { return getType() == Type::STR5; }
        bool isStr8() const { return getType() == Type::STR8; }
        bool isStr16() const { return getType() == Type::STR16; }
        bool isStr32() const { return getType() == Type::STR32; }
        bool isStr() const { return isStr5() || isStr8() || isStr16() || isStr32(); }
        bool isBin8() const { return getType() == Type::BIN8; }
        bool isBin16() const { return getType() == Type::BIN16; }
        bool isBin32() const { return getType() == Type::BIN32; }
        bool isBin() const { return isBin8() || isBin16() || isBin32(); }
        bool isArray4() const { return getType() == Type::ARRAY4; }
        bool isArray16() const { return getType() == Type::ARRAY16; }
        bool isArray32() const { return getType() == Type::ARRAY32; }
        bool isArray() const { return isArray4() || isArray16() || isArray32(); }
        bool isMap4() const { return getType() == Type::MAP4; }
        bool isMap16() const { return getType() == Type::MAP16; }
        bool isMap32() const { return getType() == Type::MAP32; }
        bool isMap() const { return isMap4() || isMap16() || isMap32(); }
        bool isFixExt1() const { return getType() == Type::FIXEXT1; }
        bool isFixExt2() const { return getType() == Type::FIXEXT2; }
        bool isFixExt4() const { return getType() == Type::FIXEXT4; }
        bool isFixExt8() const { return getType() == Type::FIXEXT8; }
        bool isFixExt16() const { return getType() == Type::FIXEXT16; }
        bool isFixExt() const { return isFixExt1() || isFixExt2() || isFixExt4() || isFixExt8() || isFixExt16(); }
        bool isExt8() const { return getType() == Type::EXT8; }
        bool isExt16() const { return getType() == Type::EXT16; }
        bool isExt32() const { return getType() == Type::EXT32; }
        bool isExt() const { return isExt8() || isExt16() || isExt32(); }
        bool isTimestamp32() const { return (getType() == Type::TIMESTAMP32) && (getRawBytes<int8_t>(curr_index, 1) == -1); }
        bool isTimestamp64() const { return (getType() == Type::TIMESTAMP64) && (getRawBytes<int8_t>(curr_index, 1) == -1); }
        bool isTimestamp96() const { return (getType() == Type::TIMESTAMP96) && (getRawBytes<int8_t>(curr_index, 2) == -1); }
        bool isTimestamp() const { return isTimestamp32() || isTimestamp64() || isTimestamp96(); }


private:

        template <typename DataType>
        DataType getRawBytes(const size_t idx, const size_t offset) const
        {
            if (idx >= indices.size())
            {
                LOG_WARNING("index overrun: idx", idx, " must be <", indices.size());
                return DataType();
            }
            DataType data;
            const auto size = sizeof(DataType);
            for (uint8_t b = 0; b < size; ++b)
            {
                uint8_t distance = size - 1 - b;
                auto index = indices[idx] + offset + distance;
                ((uint8_t*)&data)[b] = raw_data[index];
            }
            return data;
        }

        uint8_t* getRawBytePtr(const size_t idx, const size_t offset) const
        {
            if (idx >= indices.size())
            {
                LOG_WARNING("index overrun: idx", idx, " must be <", indices.size());
                return nullptr;
            }
            auto index = indices[idx] + offset;
            return raw_data + index;
        }

        Type getType() const
        {
            return getType(curr_index);
        }

        Type getType(const size_t idx) const
        {
            if (idx >= indices.size())
            {
                LOG_WARNING("index overrun: idx", idx, " must be <", indices.size());
                return Type::NA;
            }

            uint8_t raw = getRawBytes<uint8_t>(idx, 0);
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

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11
        template <template <typename...> class C, class T>
        void unpackArrayContainerArray(C<T>& arr)
#else // Do not have libstdc++11
        template <typename T>
        void unpackArrayContainerArray(arr_t<T>& arr)
#endif // Do not have libstdc++11
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
                LOG_WARNING("unpack array size is not matched :", size, "must be", arr.size());
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

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11
        template <template <typename...> class C, class T>
        void unpackArrayContainerSet(C<T>& arr)
#else // Do not have libstdc++11
        template <typename T>
        void unpackArrayContainerSet(arr_t<T>& arr)
#endif // Do not have libstdc++11
        {
            const size_t size = unpackArraySize();
            if (size == 0)
            {
                LOG_WARNING("unpack array size is not matched :", size, "must be", arr.size());
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

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11
        template <template <typename...> class C, class T, class U>
        void unpackMapContainer(C<T, U>& mp)
#else // Do not have libstdc++11
        template <typename T, typename U>
        void unpackMapContainer(map_t<T, U>& mp)
#endif // Do not have libstdc++11
        {
            const size_t size = unpackMapSize();
            if (size == 0)
            {
                LOG_WARNING("unpack map size is not matched :", size, "must be", mp.size());
            }
            else
            {
                mp.clear();
                for (size_t a = 0; a < size; ++a)
                {
                    T t; U u;
                    unpack(t);
                    unpack(u);
#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L // Have libstdc++11
                    mp.emplace(std::make_pair(t, u));
#else
                    mp.emplace(arx::make_pair(t, u));
#endif
                }
            }
        }

    };

} // msgpack
} // serial
} // ht

#endif // HT_SERIAL_MSGPACK_UNPACKER_H
