# MsgPack

MessagePack implementation for Arduino (compatible with other C++ apps)


## Typical Usage

This library is only for serialize / deserialize.
To send / receive serialized data with `Stream` class, please use [MsgPacketizer](https://github.com/hideakitai/MsgPacketizer).

``` C++
#include <MsgPack.h>

// input to msgpack
int i = 123;
float f = 1.23;
MsgPack::str_t s = "str"; // std::string or String
MsgPack::arr_t<int> v {1, 2, 3}; // std::vector or arx::vector
MsgPack::map_t<String, float> m {{"one", 1.1}, {"two", 2.2}, {"three", 3.3}}; // std::map or arx::map

// output from msgpack
int ri;
float rf;
MsgPack::str_t rs;
MsgPack::arr_t<int> rv;
MsgPack::map_t<String, float> rm;

void setup()
{
    delay(2000);
    Serial.begin(115200);
    Serial.println("msgpack test start");

    // serialize to msgpack
    MsgPack::Packer packer;
    packer.serialize(i, f, s, v, m);

    // deserialize from msgpack
    MsgPack::Unpacker unpacker;
    unpacker.feed(packer.data(), packer.size());
    unpacker.deserialize(ri, rf, rs, rv, rm);

    if (i != ri) Serial.println("failed: int");
    if (f != rf) Serial.println("failed: float");
    if (s != rs) Serial.println("failed: string");
    if (v != rv) Serial.println("failed: vector<int>");
    if (m != rm) Serial.println("failed: map<string, int>");

    Serial.println("msgpack test success");
}

void loop()
{
}
```

## Encode / Decode to Collections without Container

In msgpack, there two collection types: `Array` and `Map`.
C++ containers will be converted to one of them but you can do that from individual parameters.
To `pack` / `unpack` values as such collections in a simple way, please use these functions.

```C++
packer.to_array(i, f, s); // becoms array format [i, f, s];
unpacker.from_array(ii, ff, ss); // unpack from array format to ii, ff, ss

packer.to_map("i", i, "f", f); // becoms {"i":i, "f":f}
unpacker.from_map(ki, ii, kf, ff); // unpack from map to ii, ff, ss
```

The same conversion can be achieved using `serialize` and `deserialize`.

```C++
packer.serialize(MsgPack::arr_size_t(3), i, f, s); // [i, f, s]
unpacker.deserialize(MsgPack::arr_size_t(3), ii, ff, ss);

packer.serialize(MsgPack::map_size_t(2), "i", i, "f", f); // {"i":i, "f":f}
unpacker.deserialize(MsgPack::map_size_t(2), ki, ii, kf, ff);
```

Here, `MsgPack::arr_size_t` and `MsgPack::map_size_t` are used to identify the size of `Array` and `Map` format in `serialize` or `deserialize`.
This way is expandable to `pack` and `unpack` complex data structure because it can be nested.

```C++
// {"i":i, "arr":[ii, iii]}
packer.serialize(MsgPack::map_size_t(2), "i", i, "arr", MsgPack::arr_size_t(2), ii, iii);
unpacker.deserialize(MsgPack::map_size_t(2), ki, i, karr, MsgPack::arr_size_t(2), ii, iii);
```

## Custom Class Adaptation

To serialize / deserialize custom type you defined, please use `MSGPACK_DEFINE()` macro inside of your class. This macro enables you to convert your custom class to `Array` format.

``` C++
struct CustomClass
{
    int i;
    float f;
    MsgPack::str_t s;
    MSGPACK_DEFINE(i, f, s); // -> [i, f, s]
};
```

After that, you can `serialize` your class completely same as other types.

``` C++
int i;
float f;
MsgPack::str_t s;
CustomClass c;

MsgPack::Packer packer;
packer.serialize(i, f, s, c);
// -> packer.serialize(i, f, s, arr_size_t(3), c.i, c.f, c.s)

int ii;
float ff;
MsgPack::str_t ss;
CustomClass cc;

MsgPack::Unpacker unpacker;
unpacker.feed(packer.data(), packer.size());
unpacker.deserialize(ii, ff, ss, cc);
```

You can also wrap your custom class to `Map` format by using `MSGPACK_DEFINE_MAP` macro.
Please note that you need "key" string for `Map` format.

``` C++
struct CustomClass
{
    MsgPack::str_t key_i {"i"}; int i;
    MsgPack::str_t key_f {"f"}; float f;
    MSGPACK_DEFINE_MAP(key_i, i, key_f, f); // -> {"i":i, "f":f}
};

CustomClass c;
MsgPack::Packer packer;
packer.serialize(c);
// -> packer.serialize(map_size_t(2), c.key_i, c.i, c.key_f, c.f)

CustomClass cc;
MsgPack::Unpacker unpacker;
unpacker.feed(packer.data(), packer.size());
unpacker.deserialize(cc);
```


### Custom Class with Inheritance

Also you can use `MSGPACK_BASE()` macro to pack values of base class.

``` C++
struct Base
{
    int i;
    float f;
    MSGPACK_DEFINE(i, f);
};

struct Derived : public Base
{
    MsgPack::str_t s;
    MSGPACK_DEFINE(s, MSGPACK_BASE(Base));
    // -> packer.serialize(arr_size_t(2), s, arr_size_t(2), Base::i, Base::f)
};
```

If you wamt to use `Map` format in derived class, add "key" for your `MSGPACK_BASE`.

```C++
struct Derived : public Base
{
    MsgPack::str_t key_s; MsgPack::str_t s;
    MsgPack::str_t key_b; // key for base class
    MSGPACK_DEFINE_MAP(key_s, s, key_b, MSGPACK_BASE(Base));
    // -> packer.serialize(map_size_t(2), key_s, s, key_b, arr_size_t(2), Base::i, Base::f)
};
```


### Nested Custom Class

You can nest custom classes to express complex data structure.

```C++
// serialize and deserialize nested structure
// {"i":i, "f":f, "a":["str", {"first":1, "second":"two"}]}

// {"first":1, "second":"two"}
struct MyMap
{
    MsgPack::str_t key_first; int i;
    MsgPack::str_t key_second; MsgPack::str_t s;
    MSGPACK_DEFINE_MAP(key_first, i, key_second, s);
};

// ["str", {"first":1, "second":"two"}]
struct MyArr
{
    MsgPack::str_t s;
    MyMap m;
    MSGPACK_DEFINE(s, m):
};

// {"i":i, "f":f, "a":["str", {"first":1, "second":"two"}]}
struct MyNestedClass
{
    MsgPack::str_t key_i; int i;
    MsgPack::str_t key_f; int f;
    MsgPack::str_t key_a;
    MyArr arr;
    MSGPACK_DEFINE_MAP(key_i, i, key_f, f, key_a, arr);
};
```

And you can `serialize` / `deserialize` as same as other types.

```C++
MyNestedClass c;
MsgPack::Packer packer;
packer.serialize(c);

MyNestedClass cc;
MsgPack::Unpacker unpacker;
unpacker.feed(packer.data(), packer.size());
unpacker.deserialize(cc);
```


## JSON and Other language's msgpack compatibility

In other languages like JavaScript, Python and etc. has also library for msgpack.
But some libraries can NOT convert msgpack in "plain" style.
They always wrap them into collections like `Array` or `Map` by default.
For example, you can't convert "plain" format in other languages.

```C++
packer.serialize(i, f, s);                // "plain" format is NOT unpackable
packer.serialize(arr_size_t(3), i, f, s); // unpackable if you wrap that into Array
```

It is because the msgpack is used as based on JSON (I think).
So you need to use `Array` format for JSON array, and `Map` for Json Object.
To achieve that, there are several ways.

- use `to_array` or `to_map` to convert to simple structure
- use `serialize()` or `deserialize()` with `arr_size_t` / `map_size_t` for complex structure
- use custom class as JSON array / object which is wrapped into `Array` / `Map`
- use custom class nest recursively for more complex structure
- use `ArduinoJson` for more flexible handling of JSON (__TBD__)


### Use MsgPack with ArduinoJson

__TBD__


## Supported Type Adaptors

These are the lists of types which can be `serialize` and `deserialize`.
You can also `pack()`  or `unpack()` variable one by one.

### NIL

- `MsgPack::object::nil_t`

### Bool

- `bool`

### Integer

- `char (signed/unsigned)`
- `ints (signed/unsigned)`

### Float

- `float`
- `double`

### Str

- `char*`
- `char[]`
- `std::string` or `String(Arduino)` (`MsgPack::str_t`)

### Bin

- `unsigned char*` (need to `serialize(ptr, size)` or `pack(ptr, size)`)
- `unsigned char[]` (need to `serialize(ptr, size)` or `pack(ptr, size)`)
- `std::vector<char>` (`MsgPack::bin_t<char>`)
- `std::vector<unsigned char>` (`MsgPack::bin_t<unsigned char>`)
- `std::array<char>`
- `std::array<unsigned char>`

### Array

- `T[]` (need to `serialize(ptr, size)` or `pack(ptr, size)`)
- `std::vector` (`MsgPack::arr_t<T>`)
- `std::array`
- `std::deque`
- `std::pair`
- `std::tuple`
- `std::list`
- `std::forward_list`
- `std::set`
- `std::multiset`
- `std::unordered_set`
- `std::unordered_multiset`

### Map

- `std::map` (`MsgPack::map_t<T>`)
- `std::multimap`
- `std::unordered_map`
- `std::unordered_multimap`

### Ext

- `MsgPack::object::ext`

### TimeStamp

- `MsgPack::object::timespec`

### N/A

- `std::queue`
- `std::priority_queue`
- `std::bitset`
- `std::stack`


### Note

- `unordered_xxx` cannot be used in all Arduino
- C-style array and pointers are supported only packing.
- for NO-STL Arduino, following types can be used
  - all types of NIL, Bool, Integer, Float, Str, Bin
  - for Array, only `T[]` and `MsgPack::arr_t<T>` (= `arx::vector<T>`) can be used
  - for Map, only `MsgPack::map_t<T, U>` (= `arx::map<T, U>`) can be used
  - for the detail of `arx::xxx`, see [ArxContainer](https://github.com/hideakitai/ArxContainer)


### Additional Types for MsgPack

There are some additional types are defined to express msgpack formats easily.

#### Type Aliases for Str / Bin / Array / Map

These types have type aliases like this:

- `MsgPack::str_t` = `String` (Arduino only)
- `MsgPack::bin_t<T>` = `std::vector<T>`
- `MsgPack::arr_t<T>` = `std::vector<T>`
- `MsgPack::map_t<T, U>` = `std::map<T, U>`

For general C++ apps (not Arduino), `str_t` is defined as:

- `MsgPack::str_t` = `std::string`


#### MsgPack::obeject::nil_t

`MsgPack::object::nil_t` is used to `pack` and `unpack` Nil type.
This object is just a dummy and do nothing.

#### MsgPack::obeject::ext

`MsgPack::object::ext` holds binary data of Ext type.

``` C++
// create ext type with args: int8_t, const uint8_t*, uint32_t
MsgPack::object::ext e(type, bin_ptr, size);
MsgPack::Packer packer;
packer.serialize(e); // serialize ext type

MsgPack::object::ext r;
msgPack::Unpacker unpacker;
unpacker.feed(packer.data(), packer.size());
unpacker.deserialize(r); // deserialize ext type
```


#### MsgPack::obeject::timespec

`MsgPack::object::timespec` is used to `pack` and `unpack` Timestamp type.

``` C++
MsgPack::object::timespec t = {
    .tv_sec  = 123456789, /* int64_t  */
    .tv_usec = 123456789  /* uint32_t */
};
MsgPack::Packer packer;
packer.serialize(t); // serialize timestamp type

MsgPack::object::timespec r;
msgPack::Unpacker unpacker;
unpacker.feed(packer.data(), packer.size());
unpacker.deserialize(r); // deserialize timestamp type
```

## Other Options

### Enable Error Info

Error information report is disabled by default. You can enable it by defining this macro.

```C++
#define MSGPACK_ENABLE_DEBUG_LOG
```

Also you can change debug info stream by calling this macro (default: `Serial`).

```C++
DEBUG_LOG_ATTACH_STREAM(Serial1);
```

See [DebugLog](https://github.com/hideakitai/DebugLog) for details.


### Packet Data Storage Class Inside

STL is used to handle packet data by default, but for following boards/architectures, [ArxContainer](https://github.com/hideakitai/ArxContainer) is used to store the packet data because STL can not be used for such boards.
The storage size of such boards for max packet binary size and number of msgpack objects are limited.

- AVR
- megaAVR
- SAMD


### Memory Management (for NO-STL Boards)

As mentioned above, for such boards like Arduino Uno, the storage sizes are limited.
And of course you can manage them by defining following macros.
But these default values are optimized for such boards, please be careful not to excess your boards storage/memory.

``` C++
// msgpack serialized binary size
#define MSGPACK_MAX_PACKET_BYTE_SIZE  128
// max size of MsgPack::arr_t
#define MSGPACK_MAX_ARRAY_SIZE          8
// max size of MsgPack::map_t
#define MSGPACK_MAX_MAP_SIZE            8
// msgpack objects size in one packet
#define MSGPACK_MAX_OBJECT_SIZE        24
```
These macros have no effect for STL enabled boards.

In addtion for such boards, type aliases for following types are different from others.

- `MsgPack::str_t` = `String`
- `MsgPack::bin_t<T>` = `arx::vector<T, N = MSGPACK_MAX_PACKET_BYTE_SIZE>`
- `MsgPack::arr_t<T>` = `arx::vector<T, N = MSGPACK_MAX_ARRAY_SIZE>`
- `MsgPack::map_t<T, U>` = `arx::map<T, U, N = MSGPACK_MAX_MAP_SIZE>`

Please see "Memory Management" section and [ArxContainer](https://github.com/hideakitai/ArxContainer) for detail.


### STL library for Arduino Support

For such boards, there are several STL libraries, like [ArduinoSTL](https://github.com/mike-matera/ArduinoSTL), [StandardCPlusPlus](https://github.com/maniacbug/StandardCplusplus), and so on.
But such libraries are mainly based on [uClibc++](https://cxx.uclibc.org/) and it has many lack of function.
I considered to support them but I won't support them unless uClibc++ becomes much better compatibility to standard C++ library.
I reccomend to use low cost but much better performance chip like ESP series.


## Embedded Libraries

- [ArxTypeTraits v0.2.1](https://github.com/hideakitai/ArxTypeTraits)
- [ArxContainer v0.3.10](https://github.com/hideakitai/ArxContainer)
- [DebugLog v0.3.1](https://github.com/hideakitai/DebugLog)
- [TeensyDirtySTLErrorSolution v0.1.0](https://github.com/hideakitai/TeensyDirtySTLErrorSolution)


## Used Inside of

- [MsgPacketizer](https://github.com/hideakitai/MsgPacketizer)


## APIs

### MsgPack::Packer

``` C++
// variable sized serializer for any type
template <typename First, typename ...Rest>
void serialize(const First& first, Rest&&... rest);
template <typename T>
void serialize(const arr_size_t& arr_size, Args&&... args);
template <typename ...Args>
void serialize(const map_size_t& map_size, Args&&... args);

// variable sized serializer to array or map for any type
template <typename ...Args>
void to_array(Args&&... args);
template <typename ...Args>
void to_map(Args&&... args);

// single arg packer for any type
template <typename T>
void pack<T>(const T& t);
template <typename T>
void pack<T>(const T* ptr, const size_t size); // only for pointer types

// accesor and utility for serialized binary data
const bin_t<uint8_t>& packet() const;
const uint8_t* data() const;
size_t size() const;
size_t indices() const;
void clear();

// abstract serializer for msgpack formats
// serialize() and pack() are wrapper for these methods
void packInteger(const T& value); // accept both uint and int
void packFloat(const T& value);
void packString(const T& str);
void packString(const T& str, const size_t len);
void packBinary(const uint8_t* bin, const size_t size);
void packArraySize(const size_t size);
void packMapSize(const size_t size);
void packFixExt(const int8_t type, const T value);
void packFixExt(const int8_t type, const uint64_t value_h, const uint64_t value_l);
void packFixExt(const int8_t type, const uint8_t* ptr, const uint8_t size);
void packFixExt(const int8_t type, const uint16_t* ptr, const uint8_t size);
void packFixExt(const int8_t type, const uint32_t* ptr, const uint8_t size);
void packFixExt(const int8_t type, const uint64_t* ptr, const uint8_t size);
void packExt(const int8_t type, const T* ptr, const U size);
void packExt(const object::ext& e);
void packTimestamp(const object::timespec& time);

// serializer for detailed msgpack format
// serialize() and pack() are wrapper for these methods
void packNil();
void packNil(const object::nil_t& n);
void packBool(const bool b);
void packUInt7(const uint8_t value);
void packUInt8(const uint8_t value);
void packUInt16(const uint16_t value);
void packUInt32(const uint32_t value);
void packUInt64(const uint64_t value);
void packInt5(const int8_t value);
void packInt8(const int8_t value);
void packInt16(const int16_t value);
void packInt32(const int32_t value);
void packInt64(const int64_t value);
void packFloat32(const float value);
void packFloat64(const double value);
void packString5(const str_t& str);
void packString5(const str_t& str, const size_t len);
void packString5(const char* value);
void packString5(const char* value, const size_t len);
void packString8(const str_t& str);
void packString8(const str_t& str, const size_t len);
void packString8(const char* value);
void packString8(const char* value, const size_t len);
void packString16(const str_t& str);
void packString16(const str_t& str, const size_t len);
void packString16(const char* value);
void packString16(const char* value, const size_t len);
void packString32(const str_t& str);
void packString32(const str_t& str, const size_t len);
void packString32(const char* value);
void packString32(const char* value, const size_t len);
void packString5(const __FlashStringHelper* str);
void packString5(const __FlashStringHelper* str, const size_t len);
void packString8(const __FlashStringHelper* str);
void packString8(const __FlashStringHelper* str, const size_t len);
void packString16(const __FlashStringHelper* str);
void packString16(const __FlashStringHelper* str, const size_t len);
void packString32(const __FlashStringHelper* str);
void packString32(const __FlashStringHelper* str, const size_t len);
void packBinary8(const uint8_t* value, const uint8_t size);
void packBinary16(const uint8_t* value, const uint16_t size);
void packBinary32(const uint8_t* value, const uint32_t size);
void packArraySize4(const uint8_t value);
void packArraySize16(const uint16_t value);
void packArraySize32(const uint32_t value);
void packMapSize4(const uint8_t value);
void packMapSize16(const uint16_t value);
void packMapSize32(const uint32_t value);
void packFixExt1(const int8_t type, const uint8_t value);
void packFixExt2(const int8_t type, const uint16_t value);
void packFixExt2(const int8_t type, const uint8_t* ptr);
void packFixExt2(const int8_t type, const uint16_t* ptr);
void packFixExt4(const int8_t type, const uint32_t value);
void packFixExt4(const int8_t type, const uint8_t* ptr);
void packFixExt4(const int8_t type, const uint32_t* ptr);
void packFixExt8(const int8_t type, const uint64_t value);
void packFixExt8(const int8_t type, const uint8_t* ptr);
void packFixExt8(const int8_t type, const uint64_t* ptr);
void packFixExt16(const int8_t type, const uint64_t value_h, const uint64_t value_l);
void packFixExt16(const int8_t type, const uint8_t* ptr);
void packFixExt16(const int8_t type, const uint64_t* ptr);
void packExtSize8(const int8_t type, const uint8_t size);
void packExtSize16(const int8_t type, const uint16_t size);
void packExtSize32(const int8_t type, const uint32_t size);
void packTimestamp32(const uint32_t unix_time_sec);
void packTimestamp64(const uint64_t unix_time);
void packTimestamp64(const uint64_t unix_time_sec, const uint32_t unix_time_nsec);
void packTimestamp96(const int64_t unix_time_sec, const uint32_t unix_time_nsec);
```

### MsgPack::Unpacker

``` C++
// feed data to deserialize
bool feed(const uint8_t* data, size_t size);

// variable sized deserializer
template <typename First, typename ...Rest>
void deserialize(First& first, Rest&&... rest);

// varibale sized desrializer for array and map
template <typename ...Args>
void from_array(Args&&... args);
template <typename ...Args>
void from_map(Args&&... args);

// single arg deserializer
template <typename T>
void unpack(T& value);

// check if next arg can be deserialized to value
template <typename T>
bool unpackable(const T& value) const;

// accesor and utility for deserialized msgpack data
bool available() const;
size_t size() const;
void index(const size_t i);
size_t index() const;
void clear();

// abstract deserializer for msgpack formats
// deserialize() and unpack() are wrapper for these methods
T unpackUInt();
T unpackInt();
T unpackFloat();
str_t unpackString();
bin_t<T> unpackBinary();
bin_t<T> unpackBinary();
size_t unpackArraySize();
size_t unpackMapSize();
object::ext unpackExt();
object::timespec unpackTimestamp();

// deserializer for detailed msgpack format
// these methods check deserialize index overflow and type mismatch
// deserialize() and unpack() are wrapper for these methods
bool unpackNil();
bool unpackBool();
uint8_t unpackUInt7();
uint8_t unpackUInt8();
uint16_t unpackUInt16();
uint32_t unpackUInt32();
uint64_t unpackUInt64();
int8_t unpackInt5();
int8_t unpackInt8();
int16_t unpackInt16();
int32_t unpackInt32();
int64_t unpackInt64();
float unpackFloat32();
double unpackFloat64();
str_t unpackString5();
str_t unpackString8();
str_t unpackString16();
str_t unpackString32();
bin_t<T> unpackBinary8();
bin_t<T> unpackBinary16();
bin_t<T> unpackBinary32();
std::array<T, N> unpackBinary8();
std::array<T, N> unpackBinary16();
std::array<T, N> unpackBinary32();
size_t unpackArraySize4();
size_t unpackArraySize16();
size_t unpackArraySize32();
size_t unpackMapSize4();
size_t unpackMapSize16();
size_t unpackMapSize32();
object::ext unpackFixExt1();
object::ext unpackFixExt2();
object::ext unpackFixExt4();
object::ext unpackFixExt8();
object::ext unpackFixExt16();
object::ext unpackExt8();
object::ext unpackExt16();
object::ext unpackExt32();
object::timespec unpackTimestamp32();
object::timespec unpackTimestamp64();
object::timespec unpackTimestamp96();

// deserializer for detailed msgpack format
// these methods does NOT check index overflow and type mismatch
bool unpackNilUnchecked();
bool unpackBoolUnchecked();
uint8_t unpackUIntUnchecked7();
uint8_t unpackUIntUnchecked8();
uint16_t unpackUIntUnchecked16();
uint32_t unpackUIntUnchecked32();
uint64_t unpackUIntUnchecked64();
int8_t unpackIntUnchecked5();
int8_t unpackIntUnchecked8();
int16_t unpackIntUnchecked16();
int32_t unpackIntUnchecked32();
int64_t unpackIntUnchecked64();
float unpackFloatUnchecked32();
double unpackFloatUnchecked64();
str_t unpackStringUnchecked5();
str_t unpackStringUnchecked8();
str_t unpackStringUnchecked16();
str_t unpackStringUnchecked32();
bin_t<T> unpackBinaryUnchecked8();
bin_t<T> unpackBinaryUnchecked16();
bin_t<T> unpackBinaryUnchecked32();
std::array<T, N> unpackBinaryUnchecked8();
std::array<T, N> unpackBinaryUnchecked16();
std::array<T, N> unpackBinaryUnchecked32();
size_t unpackArraySizeUnchecked4();
size_t unpackArraySizeUnchecked16();
size_t unpackArraySizeUnchecked32();
size_t unpackMapSizeUnchecked4();
size_t unpackMapSizeUnchecked16();
size_t unpackMapSizeUnchecked32();
object::ext unpackFixExtUnchecked1();
object::ext unpackFixExtUnchecked2();
object::ext unpackFixExtUnchecked4();
object::ext unpackFixExtUnchecked8();
object::ext unpackFixExtUnchecked16();
object::ext unpackExtUnchecked8();
object::ext unpackExtUnchecked16();
object::ext unpackExtUnchecked32();
object::timespec unpackTimestampUnchecked32();
object::timespec unpackTimestampUnchecked64();
object::timespec unpackTimestampUnchecked96();

// checks types of next msgpack object
bool isNil() const;
bool isBool() const;
bool isUInt7() const;
bool isUInt8() const;
bool isUInt16() const;
bool isUInt32() const;
bool isUInt64() const;
bool isUInt() const;
bool isInt5() const;
bool isInt8() const;
bool isInt16() const;
bool isInt32() const;
bool isInt64() const;
bool isInt() const;
bool isFloat32() const;
bool isFloat64() const;
bool isFloat() const;
bool isStr5() const;
bool isStr8() const;
bool isStr16() const;
bool isStr32() const;
bool isStr() const;
bool isBin8() const;
bool isBin16() const;
bool isBin32() const;
bool isBin() const;
bool isArray4() const;
bool isArray16() const;
bool isArray32() const;
bool isArray() const;
bool isMap4() const;
bool isMap16() const;
bool isMap32() const;
bool isMap() const;
bool isFixExt1() const;
bool isFixExt2() const;
bool isFixExt4() const;
bool isFixExt8() const;
bool isFixExt16() const;
bool isFixExt() const;
bool isExt8() const;
bool isExt16() const;
bool isExt32() const;
bool isExt() const;
bool isTimestamp32() const;
bool isTimestamp64() const;
bool isTimestamp96() const;
bool isTimestamp() const;
MsgPack::Type getType() const
```


### MsgPack::Type

``` C++
enum class Type : uint8_t
{
    NA          = 0xC1, // never used
    NIL         = 0xC0,
    BOOL        = 0xC2,
    UINT7       = 0x00, // same as POSITIVE_FIXINT
    UINT8       = 0xCC,
    UINT16      = 0xCD,
    UINT32      = 0xCE,
    UINT64      = 0xCF,
    INT5        = 0xE0, // same as NEGATIVE_FIXINT
    INT8        = 0xD0,
    INT16       = 0xD1,
    INT32       = 0xD2,
    INT64       = 0xD3,
    FLOAT32     = 0xCA,
    FLOAT64     = 0xCB,
    STR5        = 0xA0, // same as FIXSTR
    STR8        = 0xD9,
    STR16       = 0xDA,
    STR32       = 0xDB,
    BIN8        = 0xC4,
    BIN16       = 0xC5,
    BIN32       = 0xC6,
    ARRAY4      = 0x90, // same as FIXARRAY
    ARRAY16     = 0xDC,
    ARRAY32     = 0xDD,
    MAP4        = 0x80, // same as FIXMAP
    MAP16       = 0xDE,
    MAP32       = 0xDF,
    FIXEXT1     = 0xD4,
    FIXEXT2     = 0xD5,
    FIXEXT4     = 0xD6,
    FIXEXT8     = 0xD7,
    FIXEXT16    = 0xD8,
    EXT8        = 0xC7,
    EXT16       = 0xC8,
    EXT32       = 0xC9,
    TIMESTAMP32 = 0xD6,
    TIMESTAMP64 = 0xD7,
    TIMESTAMP96 = 0xC7,

    POSITIVE_FIXINT = 0x00,
    NEGATIVE_FIXINT = 0xE0,
    FIXSTR          = 0xA0,
    FIXARRAY        = 0x90,
    FIXMAP          = 0x80,
};
```

## Reference

- [MessagePack Specification](https://github.com/msgpack/msgpack/blob/master/spec.md)
- [msgpack adaptor](https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_adaptor)
- [msgpack object](https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_object)
- [msgpack-c wiki](https://github.com/msgpack/msgpack-c/wiki)


## License

MIT
