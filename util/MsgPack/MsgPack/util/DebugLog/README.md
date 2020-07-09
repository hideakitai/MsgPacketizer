# DebugLog

Serial based assertion and log library for Arduino

## Usage

These macros can be used in standard C++ apps.

```C++
// uncommend NDEBUG disables ASSERT and all debug serial (Release Mode)
//#define NDEBUG

#include <DebugLog.h>

void setup()
{
    Serial.begin(115200);

    // you can change target stream (default: Serial, only for Arduino)
    // DEBUG_LOG_ATTACH_STREAM(Serial2);

    PRINT("this is for debug");
    PRINTLN(1, 2.2, "you can", "print variable args")

    // check log level 0: NONE, 1: ERRORS, 2: WARNINGS, 3: VERBOSE
    PRINTLN("current log level is", (int)LOG_GET_LEVEL());

    // set log level (default: DebugLogLevel::VERBOSE)
    LOG_SET_LEVEL(DebugLogLevel::ERRORS); // only ERROR log is printed
    LOG_SET_LEVEL(DebugLogLevel::WARNINGS); // ERROR and WARNING is printed
    LOG_SET_LEVEL(DebugLogLevel::VERBOSE); // all log is printed

    LOG_ERROR("this is error log");
    LOG_WARNING("this is warning log");
    LOG_VERBOSE("this is verbose log");

    int x = 1;
    ASSERT(x != 1); // if assertion failed, Serial endlessly prints message
}
```

### Log Level

```C++
enum class LogLevel {
    NONE     = 0,
    ERRORS   = 1,
    WARNINGS = 2,
    VERBOSE  = 3
};
```

## Used Inside of

- [MsgPack](https://github.com/hideakitai/MsgPack)
- [ES920](https://github.com/hideakitai/ES920)


## License

MIT

