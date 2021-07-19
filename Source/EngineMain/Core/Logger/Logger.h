#pragma once
#include "../Memory/SmartPointers.h"
#include "../String/String.h"
#include <cstdio>

class GenericFile;


class Logger {

private:
    Logger() = default;

    static GenericFile* getLogFile();

    template<typename... Args>
    static String fmtString(const AChar* fmt, Args... args)
    {
        int32 size = std::snprintf(nullptr, 0, fmt, args...);
        String fmted;
        fmted.resize(size + 1);
        std::snprintf(fmted.data(), size + 1, fmt, args...);
        return fmted;
    }

    template<typename StrType>
    static std::enable_if_t<std::is_convertible_v<decltype(std::declval<StrType>().c_str()), const AChar*>, const AChar*>
        getChar(const StrType& value)
    {
        return static_cast<const AChar*>(value.c_str());
    }

    template<typename StrType>
    static std::enable_if_t<
                std::conjunction_v<
                    std::is_convertible<decltype(std::declval<StrType>().getChar()), const AChar*>, std::negation<std::is_function<decltype(StrType::getChar)>>>, const AChar*>
        getChar(const StrType& value)
    {
        return static_cast<const AChar*>(value.getChar());
    }

    template<typename StrType>
    static const std::enable_if_t<std::is_convertible_v<StrType, const AChar*>, const AChar*> getChar(const StrType& value)
    {
        return static_cast<const AChar*>(value);
    }

    static void debugInternal(const AChar* category, const String& message);
    static void logInternal(const AChar* category, const String& message);
    static void warnInternal(const AChar* category, const String& message);
    static void errorInternal(const AChar* category, const String& message);
public:
    template<typename CatType, typename FmtType, typename... Args>
    static void debug(const CatType category, const FmtType fmt, Args... args)
    {
        debugInternal(getChar(category), fmtString(getChar(fmt), args...));
    }

    template<typename CatType, typename FmtType, typename... Args>
    static void log(const CatType category, const FmtType fmt, Args... args)
    {
        logInternal(getChar(category), fmtString(getChar(fmt), args...));
    }

    template<typename CatType, typename FmtType, typename... Args>
    static void warn(const CatType category, const FmtType fmt, Args... args)
    {
        warnInternal(getChar(category), fmtString(getChar(fmt), args...));
    }

    template<typename CatType, typename FmtType, typename... Args>
    static void error(const CatType category, const FmtType fmt, Args... args)
    {
        errorInternal(getChar(category), fmtString(getChar(fmt), args...));
    }

    static void flushStream();
};