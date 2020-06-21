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
    static String fmtString(const char* fmt, Args... args)
    {
        int32 size = std::snprintf(nullptr, 0, fmt, args...);
        String fmted;
        fmted.resize(size + 1);
        std::snprintf(fmted.data(), size + 1, fmt, args...);
        return fmted;
    }

    static void writeString(const String& message);

    static void debugInternal(const String& category, const String& message);
    static void logInternal(const String& category, const String& message);
    static void warnInternal(const String& category, const String& message);
    static void errorInternal(const String& category, const String& message);
public:
    
    template<typename... Args>
    static void debug(const String& category, const String& fmt, Args... args)
    {
        debugInternal(category, fmtString(fmt.getChar(), args...));
    }

    template<typename... Args>
    static void log(const String& category, const String& fmt, Args... args)
    {
        logInternal(category, fmtString(fmt.getChar(), args...));
    }

    template<typename... Args>
    static void warn(const String& category, const String& fmt, Args... args)
    {
        warnInternal(category, fmtString(fmt.getChar(), args...));
    }

    template<typename... Args>
    static void error(const String& category, const String& fmt, Args... args)
    {
        errorInternal(category, fmtString(fmt.getChar(), args...));
    }
};