#pragma once
#include "Memory/SmartPointers.h"
#include "String/StringFormat.h"
#include "ProgramCoreExports.h"

#include <sstream>
#include <cstdio>

class GenericFile;

class PROGRAMCORE_EXPORT Logger 
{
private:
    Logger() = default;

    static std::ostringstream& loggerBuffer();
    static GenericFile* getLogFile();

    static void debugInternal(const AChar* category, const String& message);
    static void logInternal(const AChar* category, const String& message);
    static void warnInternal(const AChar* category, const String& message);
    static void errorInternal(const AChar* category, const String& message);
public:
    // && (Not necessary but nice to have this)passes the type as it is from the caller like r-values as well else r-values gets converted to l-values on this call
    template<typename CatType, typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static void debug(CatType&& category, FmtType&& fmt, Args&&... args)
    {
        debugInternal(
            StringFormat::getChar<CatType>(std::forward<CatType>(category))
            , StringFormat::format<FmtType, Args...>(
                std::forward<FmtType>(fmt)
                , std::forward<Args>(args)...
            )
        );
    }

    template<typename CatType, typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static void log(CatType&& category, FmtType&& fmt, Args&&... args)
    {
        logInternal(
            StringFormat::getChar<CatType>(std::forward<CatType>(category))
            , StringFormat::format<FmtType, Args...>(
                std::forward<FmtType>(fmt)
                , std::forward<Args>(args)...
            )
        );
    }

    template<typename CatType, typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static void warn(CatType&& category, FmtType&& fmt, Args&&... args)
    {
        warnInternal(
            StringFormat::getChar<CatType>(std::forward<CatType>(category))
            , StringFormat::format<FmtType, Args...>(
                std::forward<FmtType>(fmt)
                , std::forward<Args>(args)...
            )
        );
    }

    template<typename CatType, typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static void error(CatType&& category, FmtType&& fmt, Args&&... args)
    {
        errorInternal(
            StringFormat::getChar<CatType>(std::forward<CatType>(category))
            , StringFormat::format<FmtType, Args...>(
                std::forward<FmtType>(fmt)
                , std::forward<Args>(args)...
            )
        );
    }

    static void flushStream();
};