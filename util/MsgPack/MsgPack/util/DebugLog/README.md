# DebugLog

Serial based assertion and log library for Arduino

## Feature

- print variadic arguments in one line
- log level control
- automatically save to SD card


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

    // you can also log to sd card automatically when you call log function
    if (SD.begin())
    {
        String filename = "test.txt";
        DEBUG_LOG_TO_SD(SD, filename, false); // 3rd arg: if only log to SD or not
    }

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

### Save Log to SD Card

Please see `examples/sdcard` for more details. And please note:

- sd logging is automatically done if you call log functions
- logging to file `open` and `close` file object everty time you call log functions
- one log function call takes 5-10 ms if you log to SD


## Used Inside of

- [MsgPack](https://github.com/hideakitai/MsgPack)
- [ES920](https://github.com/hideakitai/ES920)


## License

MIT

