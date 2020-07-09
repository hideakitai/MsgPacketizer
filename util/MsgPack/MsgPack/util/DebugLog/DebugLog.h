#pragma once
#ifndef ARX_DEBUGLOG_H
#define ARX_DEBUGLOG_H

#ifdef ARDUINO
    #include <Arduino.h>
    #include <SD.h>
    #include <stdarg.h>
#else
    #include <iostream>
    #include <string>
    #include <string.h>
#endif


namespace arx {
namespace debug {

    namespace detail
    {
        template <class T> struct remove_reference      { using type = T; };
        template <class T> struct remove_reference<T&>  { using type = T; };
        template <class T> struct remove_reference<T&&> { using type = T; };
        template <class T> constexpr T&& forward(typename remove_reference<T>::type& t) noexcept { return static_cast<T&&>(t); }
        template <class T> constexpr T&& forward(typename remove_reference<T>::type&& t) noexcept { return static_cast<T&&>(t); }
    }

#ifdef ARDUINO
    using string_t = String;
    Stream* stream {&Serial};
    inline void attach(Stream& s) { stream = &s; }
    bool b_only_sd {false};
    SDClass* sd {nullptr};
    File file;
    String filename;
    inline void attach(SDClass& s, const String& path, bool only_sd)
    {
        sd = &s;
        filename = path;
        b_only_sd = only_sd;
    }
#else
    using string_t = std::string;
#endif

    inline void print() { if (file) file.close(); }

    template<typename Head, typename... Tail>
    inline void print(Head&& head, Tail&&... tail)
    {
#ifdef ARDUINO
        if (!b_only_sd)
        {
            stream->print(head);
            stream->print(" ");
        }
        if (sd)
        {
            if (!file) file = sd->open(filename.c_str(), FILE_WRITE);
            if (file)
            {
                file.print(head);
                file.print(" ");
            }
        }
        print(detail::forward<Tail>(tail)...);
#else
        std::cout << head << " ";
        print(std::forward<Tail>(tail)...);
#endif
    }

    inline void println()
    {
#ifdef ARDUINO
        if (!b_only_sd)
        {
            stream->println();
        }
        if (file)
        {
            file.println();
            file.close();
        }
#else
        std::cout << std::endl;
#endif
    }

    template<typename Head, typename... Tail>
    inline void println(Head&& head, Tail&&... tail)
    {
#ifdef ARDUINO
        if (!b_only_sd)
        {
            stream->print(head);
            stream->print(" ");
        }
        if (sd)
        {
            if (!file) file = sd->open(filename.c_str(), FILE_WRITE);
            if (file)
            {
                file.print(head);
                file.print(" ");
            }
        }
        println(detail::forward<Tail>(tail)...);
#else
        std::cout << head << " ";
        println(std::forward<Tail>(tail)...);
#endif
    }

#ifdef ARDUINO
    inline void assertion(bool b, const char* file, int line, const char* func, const char* expr)
    {
        while (!b)
        {
            println("[ ASSERT ]", file, ":", line, ":", func, "() :", expr);
            delay(1000);
        }
    }
#endif

    enum class LogLevel {NONE, ERRORS, WARNINGS, VERBOSE};
    LogLevel log_level = LogLevel::VERBOSE;

    template <typename... Args>
    inline void log(LogLevel level, const char* file, int line, const char* func, Args&&... args)
    {
        if ((log_level == LogLevel::NONE) || (level == LogLevel::NONE)) return;
        if ((int)level <= (int)log_level)
        {
            string_t lvl_str;
            if      (level == LogLevel::ERRORS)   lvl_str = "ERROR";
            else if (level == LogLevel::WARNINGS) lvl_str = "WARNING";
            else if (level == LogLevel::VERBOSE)  lvl_str = "VERBOSE";
            print("[", lvl_str, "]", file, ":", line, ":", func, ":");
#ifdef ARDUINO
            println(detail::forward<Args>(args)...);
#else
            println(std::forward<Args>(args)...);
#endif
        }
    }

} // namespace debug
} // namespace ard

#ifdef NDEBUG

#define PRINT(...) ((void)0)
#define PRINTLN(...) ((void)0)
#define LOG_ERROR(...) ((void)0)
#define LOG_WARNING(...) ((void)0)
#define LOG_VERBOSE(...) ((void)0)
#define ASSERT(b) ((void)0)
#ifdef ARDUINO
    #define DEBUG_LOG_ATTACH_STREAM(s) ((void)0)
    #define DEBUG_LOG_TO_SD(s, f, b) ((void)0)
#endif

#else // NDEBUG

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define PRINT(...) arx::debug::print(__VA_ARGS__)
#define PRINTLN(...) arx::debug::println(__VA_ARGS__)
#define LOG_ERROR(...) arx::debug::log(arx::debug::LogLevel::ERRORS, __FILENAME__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARNING(...) arx::debug::log(arx::debug::LogLevel::WARNINGS, __FILENAME__, __LINE__, __func__, __VA_ARGS__)
#define LOG_VERBOSE(...) arx::debug::log(arx::debug::LogLevel::VERBOSE, __FILENAME__, __LINE__, __func__, __VA_ARGS__)
#ifdef ARDUINO
    #define DEBUG_LOG_ATTACH_STREAM(s) arx::debug::attach(s)
    #define DEBUG_LOG_TO_SD(s, f, b) arx::debug::attach(s, f, b)
    #define ASSERT(b) arx::debug::assertion((b), __FILENAME__, __LINE__, __func__, #b)
#else
    #define DEBUG_LOG_ATTACH_STREAM(s) ((void)0)
    #define DEBUG_LOG_TO_SD(s, f, b) ((void)0)
    #include <cassert>
    #define ASSERT(b) assert(b)
#endif

#endif // #ifdef NDEBUG


#define LOG_SET_LEVEL(lvl) arx::debug::log_level = lvl
#define LOG_GET_LEVEL() arx::debug::log_level
using DebugLogLevel = arx::debug::LogLevel;


#endif // ARX_DEBUGLOG_H
