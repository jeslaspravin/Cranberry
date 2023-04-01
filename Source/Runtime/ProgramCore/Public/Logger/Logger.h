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

#include "ProgramCoreExports.h"
#include "String/StringFormat.h"
#include "Types/CompilerDefines.h"
#include "Reflections/Functions.h"

#if HAS_SOURCE_LOCATION_FEATURE
#include <source_location>
#endif

class GenericFile;
class LoggerImpl;
class CBESpinLock;
struct DelegateHandle;

class PROGRAMCORE_EXPORT Logger
{
public:
#if HAS_SOURCE_LOCATION_FEATURE
    using SourceLocationType = std::source_location;
#else
    struct SourceLocationType
    {
        const AChar *fileName = "";
        const AChar *funcName = "";
        uint32 lineNum = {};

        constexpr static SourceLocationType current(const AChar *file, const AChar *func, uint32 line)
        {
            SourceLocationType retVal;
            retVal.fileName = file;
            retVal.funcName = func;
            retVal.lineNum = line;
            return retVal;
        }

        // Signature and names to match std::source_location
        NODISCARD constexpr uint32 line() const noexcept { return lineNum; }
        NODISCARD constexpr const AChar *file_name() const noexcept { return fileName; }
        NODISCARD constexpr const AChar *function_name() const noexcept { return funcName; }
    };
#endif

    enum ESeverityID : uint8
    {
        SevID_Verbose = 0,
        SevID_Debug,
        SevID_Log,
        SevID_Warning,
        SevID_Error,
        SevID_Max
    };
    enum ELogSeverity : uint8
    {
        Verbose = 1 << SevID_Verbose,
        Debug = 1 << SevID_Debug,
        Log = 1 << SevID_Log,
        Warning = 1 << SevID_Warning,
        Error = 1 << SevID_Error
    };

    enum ELogOutputType : uint8
    {
        File = 1,
        Console = 2,
        Profiler = 4
    };

    struct LogMsgPacket
    {
        SourceLocationType srcLoc;
        // Since file name is constant allocated
        const AChar *fileName = nullptr;

        // In const TChar *
        SizeT categoryStart;
        SizeT messageStart;
        uint32 categorySize;
        uint32 messageSize;

        int64 timeStamp;
        ESeverityID severity = ESeverityID::SevID_Max;

        ELogSeverity getServerityFlag() const { return ELogSeverity(1 << severity); }
    };

    using StaticPacketsFunc = Function<void, const String &, const std::vector<LogMsgPacket> &>;
    using LambdaPacketsFunc = LambdaFunction<void, const String &, const std::vector<LogMsgPacket> &>;

    constexpr static const uint8 AllServerity
        = ELogSeverity::Verbose | ELogSeverity::Debug | ELogSeverity::Log | ELogSeverity::Warning | ELogSeverity::Error;
    constexpr static const uint8 AllOutputType = ELogOutputType::File | ELogOutputType::Console | ELogOutputType::Profiler;

    constexpr static const TChar *CONSOLE_FOREGROUND_RED = TCHAR("\x1b[31m");
    constexpr static const TChar *CONSOLE_FOREGROUND_YELLOW = TCHAR("\x1b[33m");
    constexpr static const TChar *CONSOLE_FOREGROUND_DEFAULT = TCHAR("\x1b[0m");

private:
    Logger() = default;

    struct LoggerAutoShutdown
    {
        ~LoggerAutoShutdown() { shutdown(); }
    };
    static LoggerAutoShutdown autoShutdown;
    static LoggerImpl *loggerImpl;

    // Cannot have move reference in dynamically linked functions
#if ENABLE_VERBOSE_LOG
    static void verboseInternal(const SourceLocationType srcLoc, const TChar *category, const String &message);
#endif
    static void debugInternal(const SourceLocationType srcLoc, const TChar *category, const String &message);
    static void logInternal(const SourceLocationType srcLoc, const TChar *category, const String &message);
    static void warnInternal(const SourceLocationType srcLoc, const TChar *category, const String &message);
    static void errorInternal(const SourceLocationType srcLoc, const TChar *category, const String &message);

    static CBESpinLock &consoleOutputLock();
    static bool canLog(ELogSeverity severity, ELogOutputType output);
    static bool canLogTime();

public:
#if ENABLE_VERBOSE_LOG
    template <typename CatType, typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static void verbose(const SourceLocationType srcLoc, CatType &&category, FmtType &&fmt, Args &&...args)
    {
        verboseInternal(
            srcLoc, StringFormat::getChar<CatType>(std::forward<CatType>(category)),
            StringFormat::format<FmtType, Args...>(std::forward<FmtType>(fmt), std::forward<Args>(args)...)
        );
    }
#endif

    // && (Not necessary but nice to have this)passes the type as it is from the caller like r-values as
    // well else r-values gets converted to l-values on this call
    template <typename CatType, typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static void debug(const SourceLocationType srcLoc, CatType &&category, FmtType &&fmt, Args &&...args)
    {
        debugInternal(
            srcLoc, StringFormat::getChar<CatType>(std::forward<CatType>(category)),
            StringFormat::format<FmtType, Args...>(std::forward<FmtType>(fmt), std::forward<Args>(args)...)
        );
    }

    template <typename CatType, typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static void log(const SourceLocationType srcLoc, CatType &&category, FmtType &&fmt, Args &&...args)
    {
        logInternal(
            srcLoc, StringFormat::getChar<CatType>(std::forward<CatType>(category)),
            StringFormat::format<FmtType, Args...>(std::forward<FmtType>(fmt), std::forward<Args>(args)...)
        );
    }

    template <typename CatType, typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static void warn(const SourceLocationType srcLoc, CatType &&category, FmtType &&fmt, Args &&...args)
    {
        warnInternal(
            srcLoc, StringFormat::getChar<CatType>(std::forward<CatType>(category)),
            StringFormat::format<FmtType, Args...>(std::forward<FmtType>(fmt), std::forward<Args>(args)...)
        );
    }

    template <typename CatType, typename FmtType, typename... Args>
    DEBUG_INLINE CONST_EXPR static void error(const SourceLocationType srcLoc, CatType &&category, FmtType &&fmt, Args &&...args)
    {
        errorInternal(
            srcLoc, StringFormat::getChar<CatType>(std::forward<CatType>(category)),
            StringFormat::format<FmtType, Args...>(std::forward<FmtType>(fmt), std::forward<Args>(args)...)
        );
    }

    static void flushStream();
    static void pushMuteSeverities(uint8 muteSeverities);
    static void popMuteSeverities();

    DelegateHandle bindPacketListener(StaticPacketsFunc listener);
    DelegateHandle bindPacketListener(const LambdaPacketsFunc &listener);
    void unbindPacketListener(DelegateHandle handle);

    static void initialize();
    static void shutdown();

    static void startLoggingTime();
    static void stopLoggingTime();
};

#if HAS_SOURCE_LOCATION_FEATURE
#define CURRENT_SRC_LOC() Logger::SourceLocationType::current()
#else
#define CURRENT_SRC_LOC() Logger::SourceLocationType::current(__FILE__, __func__, __LINE__)
#endif

#if ENABLE_VERBOSE_LOG
#define LOG_VERBOSE(Category, Fmt, ...) Logger::verbose(CURRENT_SRC_LOC(), TCHAR(Category), TCHAR(Fmt), __VA_ARGS__)
#else
#define LOG_VERBOSE(Category, Fmt, ...)
#endif

#define LOG_DEBUG(Category, Fmt, ...) Logger::debug(CURRENT_SRC_LOC(), TCHAR(Category), TCHAR(Fmt), __VA_ARGS__)
#define LOG(Category, Fmt, ...) Logger::log(CURRENT_SRC_LOC(), TCHAR(Category), TCHAR(Fmt), __VA_ARGS__)
#define LOG_WARN(Category, Fmt, ...) Logger::warn(CURRENT_SRC_LOC(), TCHAR(Category), TCHAR(Fmt), __VA_ARGS__)
#define LOG_ERROR(Category, Fmt, ...) Logger::error(CURRENT_SRC_LOC(), TCHAR(Category), TCHAR(Fmt), __VA_ARGS__)

struct ScopedMuteLogServerity
{
    ScopedMuteLogServerity(uint8 muteSeverities) { Logger::pushMuteSeverities(muteSeverities); }

    ~ScopedMuteLogServerity() { Logger::popMuteSeverities(); }
};
#define SCOPED_MUTE_LOG_SEVERITIES(SeverityFlags)                                                                                              \
    ScopedMuteLogServerity COMBINE(__zzzz__muteSeverities_, __LINE__) { SeverityFlags }