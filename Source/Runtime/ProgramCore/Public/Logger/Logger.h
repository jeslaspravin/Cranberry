/*!
 * \file Logger.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Memory/SmartPointers.h"
#include "ProgramCoreExports.h"
#include "String/StringFormat.h"

class GenericFile;

class PROGRAMCORE_EXPORT Logger
{
public:
    enum ELogServerity : uint8
    {
        Debug = 1,
        Log = 2,
        Warning = 4,
        Error = 8
    };

    static const uint8 AllServerity = ELogServerity::Debug | ELogServerity::Log | ELogServerity::Warning | ELogServerity::Error;

private:
    Logger() = default;

    static OStringStream &loggerBuffer();
    static GenericFile *getLogFile();
    static std::vector<uint8> &muteFlags();

    static void debugInternal(const TChar *category, const String &message);
    static void logInternal(const TChar *category, const String &message);
    static void warnInternal(const TChar *category, const String &message);
    static void errorInternal(const TChar *category, const String &message);

public:
    // && (Not necessary but nice to have this)passes the type as it is from the caller like r-values as
    // well else r-values gets converted to l-values on this call
    template <typename CatType, typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static void debug(CatType &&category, FmtType &&fmt, Args &&...args)
    {
        debugInternal(
            StringFormat::getChar<CatType>(std::forward<CatType>(category)),
            StringFormat::format<FmtType, Args...>(std::forward<FmtType>(fmt), std::forward<Args>(args)...)
        );
    }

    template <typename CatType, typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static void log(CatType &&category, FmtType &&fmt, Args &&...args)
    {
        logInternal(
            StringFormat::getChar<CatType>(std::forward<CatType>(category)),
            StringFormat::format<FmtType, Args...>(std::forward<FmtType>(fmt), std::forward<Args>(args)...)
        );
    }

    template <typename CatType, typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static void warn(CatType &&category, FmtType &&fmt, Args &&...args)
    {
        warnInternal(
            StringFormat::getChar<CatType>(std::forward<CatType>(category)),
            StringFormat::format<FmtType, Args...>(std::forward<FmtType>(fmt), std::forward<Args>(args)...)
        );
    }

    template <typename CatType, typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static void error(CatType &&category, FmtType &&fmt, Args &&...args)
    {
        errorInternal(
            StringFormat::getChar<CatType>(std::forward<CatType>(category)),
            StringFormat::format<FmtType, Args...>(std::forward<FmtType>(fmt), std::forward<Args>(args)...)
        );
    }

    static void flushStream();
    static void pushMuteSeverities(uint8 muteSeverities);
    static void popMuteSeverities();
};

#define LOG_DEBUG(Category, Fmt, ...) Logger::debug(TCHAR(Category), TCHAR(Fmt), __VA_ARGS__)
#define LOG(Category, Fmt, ...) Logger::log(TCHAR(Category), TCHAR(Fmt), __VA_ARGS__)
#define LOG_WARN(Category, Fmt, ...) Logger::warn(TCHAR(Category), TCHAR(Fmt), __VA_ARGS__)
#define LOG_ERROR(Category, Fmt, ...) Logger::error(TCHAR(Category), TCHAR(Fmt), __VA_ARGS__)

struct ScopedMuteLogServerity
{
    ScopedMuteLogServerity(uint8 muteSeverities) { Logger::pushMuteSeverities(muteSeverities); }

    ~ScopedMuteLogServerity() { Logger::popMuteSeverities(); }
};
#define SCOPED_MUTE_LOG_SEVERITIES(SeverityFlags)                                                                                              \
    ScopedMuteLogServerity COMBINE(__zzzz__muteSeverities_, __LINE__) { SeverityFlags }