#pragma once
#include "Logger.hpp"
#include <cstdint>
#include <cstring>
#include <string>
#include <map>
#include <algorithm>
#include <type_traits>

namespace optparse
{


enum class OptionType : uint8_t { Str, Int, UInt, Double, Bool, StrMap };

struct StrArg
{
    const char* const arg;
};
struct UIntArg
{
    const uint64_t arg;
};
struct IntArg
{
    const int64_t arg;
};
struct DoubleArg
{
    const double arg;
};
struct BoolArg
{
    const bool arg;
};
struct StrMapArg
{
    const std::map<std::string, std::string> arg;
};


struct OptionItem
{
    const std::string LongName;
    const std::string Description;
    std::map<std::string, std::string> StrMapValue;
    std::string StrValue;
    double DoubleValue;
    uint64_t UIntValue;
    int64_t IntValue;
    bool BoolValue;
    const char Flag;
    const OptionType Type;
private:
    OptionItem(const OptionType type, const char flag, const std::string& name, const std::string& desc) noexcept
        : LongName(name), Description(desc), Flag(flag), Type(type) 
    { }
public:
    OptionItem(const char flag, const std::string& name, const std::string& desc, const StrArg& arg) noexcept
        : OptionItem(OptionType::Str, flag, name, desc)
    {
        StrValue = arg.arg;
    }
    OptionItem(const char flag, const std::string& name, const std::string& desc, const UIntArg& arg) noexcept
        : OptionItem(OptionType::UInt, flag, name, desc)
    {
        UIntValue = arg.arg;
    }
    OptionItem(const char flag, const std::string& name, const std::string& desc, const IntArg& arg) noexcept
        : OptionItem(OptionType::Int, flag, name, desc)
    {
        IntValue = arg.arg;
    }
    OptionItem(const char flag, const std::string& name, const std::string& desc, const DoubleArg& arg) noexcept
        : OptionItem(OptionType::Double, flag, name, desc)
    {
        DoubleValue = arg.arg;
    }
    OptionItem(const char flag, const std::string& name, const std::string& desc, const BoolArg& arg) noexcept
        : OptionItem(OptionType::Bool, flag, name, desc)
    {
        BoolValue = arg.arg;
    }
    OptionItem(const char flag, const std::string& name, const std::string& desc, const StrMapArg& arg) noexcept
        : OptionItem(OptionType::StrMap, flag, name, desc)
    {
        StrMapValue = arg.arg;
    }
    OptionItem(const OptionItem& item) = default;
    OptionItem(OptionItem&&) = default;
    OptionItem& operator=(const OptionItem&) = delete;
    OptionItem& operator=(OptionItem&&) = delete;
    ~OptionItem() noexcept {}
    const char* TypeStr() const
    {
        switch (Type)
        {
        case OptionType::Bool:     return "{on|off}";
        case OptionType::UInt:     return "<uint>";
        case OptionType::Int:      return "<int>";
        case OptionType::Double:   return "<double>";
        case OptionType::Str:      return "<string>";
        case OptionType::StrMap:   return "<str=str>";
        default:                   return nullptr;
        }
    }
};
inline bool SetArg(OptionItem& item, const char *arg)
{
    switch (item.Type)
    {
    case OptionType::Str:
        item.StrValue = arg; return true;
    case OptionType::Double:
        item.DoubleValue = strtod(arg, nullptr); return true;
    case OptionType::UInt:
        item.UIntValue = strtoull(arg, nullptr, 10); return true;
    case OptionType::Int:
        item.IntValue = strtoll(arg, nullptr, 10); return true;
    case OptionType::Bool:
        if (std::strcmp(arg, "on") == 0)
            item.BoolValue = true;
        else if (std::strcmp(arg, "off") == 0)
            item.BoolValue = false;
        else
        {
            logger::Error("expect {on|off} for [%s]/[%c] (%s)!\n", item.LongName, item.Flag, item.Description);
            return false;
        }
        return true;
    case OptionType::StrMap:
        {
            std::string theArg(arg);
            const auto pos = theArg.find('=');
            if (pos == std::string::npos)
            {
                logger::Error("expect {key=value} for [%s]/[%c] (%s)!\n", item.LongName, item.Flag, item.Description);
                return false;
            }
            item.StrMapValue[theArg.substr(0, pos)] = theArg.substr(pos + 1);
            return true;
        }
    default:
        logger::Error("Internal error with [%s]/[%c] (%s)!\n", item.LongName, item.Flag, item.Description);
        return false;
    }
}
inline bool SetArg(OptionItem& item, uint32_t& idx, const uint32_t argc, char *argv[])
{
    if (++idx >= argc) // need more arg
    {
        logger::Error("need argument for option [%s]/[%c] (%s)!\n", item.LongName, item.Flag, item.Description);
        return false;
    }
    return SetArg(item, argv[idx]);
}
template<typename T>
inline bool PrintHelp(const T& options)
{
    static_assert(std::is_same<typename T::value_type, OptionItem>::value, "Need a collection of OptionItem");
    logger::Info("%s\n", "Commandline options:");
    for (const auto& item : options)
    {
        logger::Info("\t[-%c --%-12s %9s] \t%s\n", item.Flag, item.LongName, item.TypeStr(), item.Description);
    }
    return false;
}
template<typename T>
inline bool ParseCommands(T& options, const uint32_t argc, char *argv[])
{
    static_assert(std::is_same<typename T::value_type, OptionItem>::value, "Need a collection of OptionItem");
    for (uint32_t i = 1; i < argc; ++i)
    {
        const std::string opt(argv[i]);
        if (opt == "-h" || opt == "--help")
            return false;
        if (opt.size() < 2 || opt[0] != '-')
        {
            logger::Error("wrong option format [%s], should start with '-' or '--'!\n", opt);
            return false;
        }
        char val = opt[1];
        if (val == '-') // long name
        {
            if (opt.size() < 3)
            {
                logger::Error("wrong option format [%s], should start with '-' or '--'!\n", opt);
                return false;
            }
            const auto target = opt.substr(2);
            const auto it = std::find_if(options.begin(), options.end(), [=](const OptionItem& item) { return item.LongName == target; });
            if (it == options.end())
            {
                logger::Error("unrecognized long option [%s]\n", target);
                return false;
            }
            if (!SetArg(*it, i, argc, argv))
                return false;
        }
        else // short name
        {
            const auto it = std::find_if(options.begin(), options.end(), [=](const OptionItem& item) { return item.Flag == val; });
            if (it == options.end())
            {
                logger::Error("unrecognized long option [%c]\n", val);
                return false;
            }
            if (!(opt.size() > 2 ? SetArg(*it, &opt[2]) : SetArg(*it, i, argc, argv)))
                return false;
        }
    }
    return true;
}


}