#pragma once
#include "Memory/SmartPointers.h"
#include "String/StringFormat.h"
#include "ProgramCoreExports.h"

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

    static std::ostringstream& loggerBuffer();
    static GenericFile* getLogFile();
    static std::vector<uint8>& muteFlags();

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
    static void pushMuteSeverities(uint8 muteSeverities);
    static void popMuteSeverities();
};

struct ScopedMuteLogServerity
{
    ScopedMuteLogServerity(uint8 muteSeverities)
    {
        Logger::pushMuteSeverities(muteSeverities);
    }

    ~ScopedMuteLogServerity()
    {
        Logger::popMuteSeverities();
    }
};
#define SCOPED_MUTE_LOG_SEVERITIES(SeverityFlags) ScopedMuteLogServerity COMBINE(__zzzz__muteSeverities_, __LINE__) { SeverityFlags }