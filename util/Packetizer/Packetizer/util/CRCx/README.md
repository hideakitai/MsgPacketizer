# CRCx

CRC calculation for Arduino and other C++ programs.
This library is created to use crc calculation as same method for both Arduino and other C++ propgrams (like openFrameworks and so on).
CRCx is based on fast and efficient two great works, and CRCx is just a glue for these two great works.

- [FastCRC](https://github.com/FrankBoesing/FastCRC) by @FrankBoesing for Arduino
- [CRCpp](https://github.com/d-bahr/CRCpp) by @d-bahr for other C++ programs


## Usage

``` C++
#include <CRCx.h>

const uint8_t data[] = { 'H', 'E', 'L', 'L', 'O', ' ', 'W', 'O', 'R', 'L', 'D' };
const size_t size = sizeof(data);

void setup()
{
    Serial.begin(115200);
    delay(2000);

    uint8_t result8 = crcx::crc8(data, size);
    uint16_t result16 = crcx::crc16(data, size);
    uint32_t result32 = crcx::crc32(data, size);

    Serial.print("crc8  = 0x"); Serial.println(result8, HEX);
    Serial.print("crc16 = 0x"); Serial.println(result16, HEX);
    Serial.print("crc32 = 0x"); Serial.println(result32, HEX);
}

// Console Output:
// crc8  = 0x7
// crc16 = 0xB944
// crc32 = 0x87E5865B

```

## CRC Options

You can specify how to calculate by setting 3rd argument.

``` C++
uint32_t result32 = crcx::crc32(data, size, Crc32::POSIX);
```

Available options and default parameters are shown below.

``` C++
enum class Crc8 : uint8_t
{
    SMBUS, // default
    MAXIM
};

enum class Crc16 : uint8_t
{
    CCITT,
    KERMIT,
    MODBUS, // default
    XMODEM,
    X25
};

enum class Crc32 : uint8_t
{
    CRC32, // default
    POSIX
};
```

## Use `FastCRC` or `CRCpp` directly

Of course, you can use `FastCRC` or `CRCpp` directly.

### FastCRC

``` C++
FastCRC32 fastcrc;
uint32_t crc32 = fastcrc.crc32(data, size);
```

Please refer to [FastCRC original page](https://github.com/FrankBoesing/FastCRC) for more information.

### CRCpp

NOTE: Following `CRCpp` options are enabled as default.

```C++
#define CRCPP_USE_CPP11
#define CRCPP_USE_NAMESPACE
#define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
```

and you can use as:

``` C++
uint32_t crc32 = CRCpp::CRC::Calculate(data, size, CRCpp::CRC::CRC_32());
```

Please refer to [CRCpp original page](https://github.com/d-bahr/CRCpp) for more information.

## License

MIT
