#pragma once
#ifndef ARX_DEBUGLOG_H
#define ARX_DEBUGLOG_H

#ifdef ARDUINO
    #include <Arduino.h>
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
    Stream* stream {&Serial};
    inline void attach(Stream& s) { stream = &s; }
    using string_t = String;
#else
    using string_t = std::string;
#endif

    inline void print() { }

    template<typename Head, typename... Tail>
    inline void print(Head&& head, Tail&&... tail)
    {
#ifdef ARDUINO
        stream->print(head);
        stream->print(" ");
        print(detail::forward<Tail>(tail)...);
#else
        std::cout << head << " ";
        print(std::forward<Tail>(tail)...);
#endif
    }

    inline void println()
    {
#ifdef ARDUINO
        stream->println();
#else
        std::cout << std::endl;
#endif
    }

    template<typename Head, typename... Tail>
    inline void println(Head&& head, Tail&&... tail)
    {
#ifdef ARDUINO
        stream->print(head);
        stream->print(" ");
        println(detail::forward<Tail>(tail)...);
#else
        std::cout << head << " ";
        println(std::forward<Tail>(tail)...);
#endif
    }

#ifdef ARDUINO
    inline void assert(bool b, const char* file, int line, const char* func, const char* expr)
    {
        while (!b)
        {
            println("[ ASSERT ]", file, ":", line, ":", func, "() :", expr);
            delay(1000);
        }
    }
#endif

    enum class LogLevel {NONE, ERROR, WARNING, VERBOSE};
    LogLevel log_level = LogLevel::VERBOSE;

    template <typename... Args>
    void log(LogLevel level, const char* file, int line, const char* func, Args&&... args)
    {
        if ((log_level == LogLevel::NONE) || (level == LogLevel::NONE)) return;
        if ((int)level <= (int)log_level)
        {
            string_t lvl_str;
            if      (level == LogLevel::ERROR)   lvl_str = "ERROR";
            else if (level == LogLevel::WARNING) lvl_str = "WARNING";
            else if (level == LogLevel::VERBOSE) lvl_str = "VERBOSE";
            print("[", lvl_str, "]", file, ":", line, ":", func, "() :");
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
#endif

#else // NDEBUG

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define PRINT(...) arx::debug::print(__VA_ARGS__)
#define PRINTLN(...) arx::debug::println(__VA_ARGS__)
#define LOG_ERROR(...) arx::debug::log(arx::debug::LogLevel::ERROR, __FILENAME__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARNING(...) arx::debug::log(arx::debug::LogLevel::WARNING, __FILENAME__, __LINE__, __func__, __VA_ARGS__)
#define LOG_VERBOSE(...) arx::debug::log(arx::debug::LogLevel::VERBOSE, __FILENAME__, __LINE__, __func__, __VA_ARGS__)
#ifdef ARDUINO
    #define DEBUG_LOG_ATTACH_STREAM(s) arx::debug::attach(s)
    #define ASSERT(b) arx::debug::assert((b), __FILENAME__, __LINE__, __func__, #b)
#else
    #define DEBUG_LOG_ATTACH_STREAM(s) ((void)0)
    #include <cassert>
    #define ASSERT(b) assert(b)
#endif

#endif // #ifdef NDEBUG


#define LOG_SET_LEVEL(lvl) arx::debug::log_level = lvl
using DebugLogLevel = arx::debug::LogLevel;


#endif // ARX_DEBUGLOG_H
