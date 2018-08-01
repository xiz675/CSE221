#pragma once
#include <cstdint>
#include <cstdio>
#include <string>

//logging utility
//for convinient, frontend and backend is not seperated
//and the logger is stateless (functions rather than classes)
//output console may still be stateful and keeps last color setting
namespace logger
{
namespace detail
{

template<typename T>
const T* Argument(const std::basic_string<T>& value) noexcept
{
    return value.c_str();
}
template<typename T>
T Argument(T value) noexcept
{
    return value;
}
template<typename... Args>
inline void Log(const std::string& format, const Args&... args) noexcept
{
    printf(format.c_str(), Argument(args)...);
}

}


template<typename Arg, typename... Args>
inline void Debug(const Arg& format, const Args&... args) noexcept
{
    detail::Log(std::string("\x1b[95m") + format, args...);
}
template<typename Arg, typename... Args>
inline void Info(const Arg& format, const Args&... args) noexcept
{
    detail::Log(std::string("\x1b[97m") + format, args...);
}
template<typename Arg, typename... Args>
inline void Success(const Arg& format, const Args&... args) noexcept
{
    detail::Log(std::string("\x1b[92m") + format, args...);
}
template<typename Arg, typename... Args>
inline void Warn(const Arg& format, const Args&... args) noexcept
{
    detail::Log(std::string("\x1b[93m") + format, args...);
}
template<typename Arg, typename... Args>
inline void Error(const Arg& format, const Args&... args) noexcept
{
    detail::Log(std::string("\x1b[91m") + format, args...);
}

enum class LogLevel : uint8_t { Debug, Info, Success, Warn, Error };
template<typename Arg, typename... Args>
inline void Log(const LogLevel level, const Arg& format, const Args&... args) noexcept
{
    switch (level)
    {
    case LogLevel::Debug: 
        Debug(format, args...); break;
    case LogLevel::Info:
        Info(format, args...); break;
    case LogLevel::Success:
        Success(format, args...); break;
    case LogLevel::Warn:
        Warn(format, args...); break;
    case LogLevel::Error:
        Error(format, args...); break;
    }
}

}