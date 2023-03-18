/*!
 * \file Logger.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Logger/Logger.h"
#include "CmdLine/CmdLine.h"
#include "String/TCharString.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/LFS/Paths.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/Threading/SyncPrimitives.h"
#include "Types/Platform/Threading/PlatformThreading.h"
#include "Types/Platform/Threading/CoPaT/JobSystemCoroutine.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"
#include "Types/Platform/PlatformFunctions.h"
#include "Profiler/ProgramProfiler.hpp"

#include <mutex>

#if LOG_TO_CONSOLE
#include <iostream>

// If 0 both user provided category and severity will be skipped in console log
// Also Source location informations will be removed
#define SHORT_MSG_IN_CONSOLE 1
#define ENABLE_VIRTUAL_TERMINAL_SEQ !PLATFORM_WINDOWS

#endif // LOG_TO_CONSOLE

//////////////////////////////////////////////////////////////////////////
/// Logger impl
//////////////////////////////////////////////////////////////////////////

class LoggerImpl
{
private:
    struct LoggerPerThreadData
    {
        // Right now we do per thread mute which is not correct, However this is better than solving below multi threaded scenario
        // Thread 1 Pushes -> Thread 2 Pushes -> Thread 1 Pops incorrect mute flags -> Thread 2 Pops incorrect mute flags
        std::vector<uint8> serverityMuteFlags{ 0 };

        OStringStream bufferStream = OStringStream(std::ios_base::trunc);
        // Per thread stream locks when flushing all streams
        CBESpinLock streamRWLock;
    };

    uint32 tlsSlot;
    // This will not be contented in fast path, fast path which is log, debug, warn, error has minimum spin lock contention
    CBESpinLock allTlDataLock;
    std::vector<LoggerPerThreadData *> allPerThreadData;

    // This is only supposed to be enabled when app ticking starts and must be stopped when app begins shutdown
    std::atomic_flag bEnableLogTime;
    PlatformFile logFile;

    LoggerPerThreadData &getOrCreatePerThreadData()
    {
        LoggerPerThreadData *threadLocalData = (LoggerPerThreadData *)PlatformThreadingFunctions::getTlsSlotValue(tlsSlot);
        if (threadLocalData == nullptr)
        {
            LoggerPerThreadData *newTlData = new LoggerPerThreadData();
            bool bTlsValueSet = PlatformThreadingFunctions::setTlsSlotValue(tlsSlot, newTlData);
            if (!bTlsValueSet)
            {
                std::exit(-1);
            }
            threadLocalData = (LoggerPerThreadData *)PlatformThreadingFunctions::getTlsSlotValue(tlsSlot);

            std::scoped_lock<CBESpinLock> lockAllThreadData(allTlDataLock);
            allPerThreadData.emplace_back(threadLocalData);
        }
        return *threadLocalData;
    }

public:
    bool initialize();
    void shutdown();
    void startLoggingTime() { bEnableLogTime.test_and_set(std::memory_order::relaxed); }
    void stopLoggingTime() { bEnableLogTime.clear(std::memory_order::relaxed); }

    std::vector<uint8> &muteFlags() { return getOrCreatePerThreadData().serverityMuteFlags; }
    bool canLogTime() const { return bEnableLogTime.test(std::memory_order::relaxed); }

    OStringStream &lockLoggerBuffer()
    {
        LoggerPerThreadData &tlData = getOrCreatePerThreadData();
        tlData.streamRWLock.lock();
        return tlData.bufferStream;
    }
    void unlockLoggerBuffer() { getOrCreatePerThreadData().streamRWLock.unlock(); }

    void flushStream();

private:
    bool openNewLogFile();
    void flushStreamInternal();

    copat::NormalFuncAwaiter flushStreamAsync() { co_await flushInWorkerThread(); }
    copat::JobSystemEnqTask<copat::EJobThreadType::WorkerThreads> flushInWorkerThread()
    {
        flushStreamInternal();
        co_return;
    }
};

bool LoggerImpl::initialize()
{
    if (!PlatformThreadingFunctions::createTlsSlot(tlsSlot))
    {
        return false;
    }
    PlatformFunctions::setupAvailableConsole();

    return openNewLogFile();
}

void LoggerImpl::shutdown()
{
    logFile.closeFile();

    PlatformFunctions::detachCosole();

    // Thread locals are do not need lock at this point
    for (LoggerPerThreadData *tlData : allPerThreadData)
    {
        delete tlData;
    }
    allPerThreadData.clear();
    PlatformThreadingFunctions::releaseTlsSlot(tlsSlot);
}

void LoggerImpl::flushStream()
{
    if (copat::JobSystem::get())
    {
        flushStreamAsync();
    }
    else
    {
        flushStreamInternal();
    }
}

void LoggerImpl::flushStreamInternal()
{
    allTlDataLock.lock();
    for (LoggerPerThreadData *tlData : allPerThreadData)
    {
        if (tlData->bufferStream.tellp() != 0)
        {
            // If not empty then only lock it, Else we can log in next flush!
            std::scoped_lock<CBESpinLock> lockTlStream(tlData->streamRWLock);

            // Below code uses null termination to mark end of string, by appending a null char at end, This works and does not clears buffer
            // because TCHAR_TO_UTF8 works on null terminated string
            // tlData->bufferStream << '\0';
            // auto str = tlData->bufferStream.str();
            // tlData->bufferStream.seekp(0);

            // Below code clears buffer after every data flush
            auto str = tlData->bufferStream.str();
            tlData->bufferStream.str({});

            const std::string utf8str{ TCHAR_TO_UTF8(str.c_str()) };
            logFile.write(ArrayView<const uint8>(reinterpret_cast<const uint8 *>(utf8str.data()), uint32(utf8str.length())));
        }
    }
    allTlDataLock.unlock();
}

bool LoggerImpl::openNewLogFile()
{
    String logFileName = Paths::applicationName();
    String logFolderPath = PathFunctions::combinePath(Paths::savedDirectory(), TCHAR("Logs"));
    if (ProgramCmdLine::get().hasArg(TCHAR("--logFileName")))
    {
        ProgramCmdLine::get().getArg(logFileName, TCHAR("--logFileName"));
    }

    String logFilePath = PathFunctions::combinePath(logFolderPath, logFileName + TCHAR(".log"));
    PlatformFile checkFile{ logFilePath };

    if (checkFile.exists())
    {
        uint64 lastWrite = checkFile.lastWriteTimeStamp();
        String renameTo = StringFormat::format(TCHAR("%s-%llu.log"), logFileName, lastWrite);
        checkFile.renameFile(renameTo);

        // Remove or clear old logs
        std::vector<String> oldLogFiles = FileSystemFunctions::listFiles(logFolderPath, false, logFileName + TCHAR("-*.log"));
        if (oldLogFiles.size() > 10)
        {
            for (String &oldFile : oldLogFiles)
            {
                oldFile = PathFunctions::fileOrDirectoryName(oldFile);
            }
            std::sort(oldLogFiles.begin(), oldLogFiles.end(), std::greater{});

            for (uint32 i = 10; i < oldLogFiles.size(); ++i)
            {
                PlatformFile(PathFunctions::combinePath(logFolderPath, oldLogFiles[i])).deleteFile();
            }
        }
    }

    logFile = PlatformFile(logFilePath);
    logFile.setFileFlags(EFileFlags::OpenAlways | EFileFlags::Write);
    logFile.setSharingMode(EFileSharing::ReadOnly);
    logFile.setAttributes(EFileAdditionalFlags::Normal);

    return logFile.openOrCreate();
}

//////////////////////////////////////////////////////////////////////////
/// Logger
//////////////////////////////////////////////////////////////////////////

Logger::LoggerAutoShutdown Logger::autoShutdown;
LoggerImpl *Logger::loggerImpl = nullptr;

NODISCARD constexpr const AChar *filterFileName(const AChar *fileName) noexcept
{
    SizeT foundAt = 0;
    TCharStr::rfind(fileName, FS_PATH_SEPARATOR, &foundAt);
    return fileName + foundAt + 1;
}

#if ENABLE_VERBOSE_LOG
void Logger::verboseInternal(const SourceLocationType srcLoc, const TChar *category, const String &message)
{
#if DEV_BUILD
    static const String CATEGORY{ TCHAR("[VERBOSE]") };

    String timeStr = canLogTime() ? Time::toString(Time::localTimeNow(), false) : TCHAR("");
    const AChar *fileName = filterFileName(srcLoc.file_name());

    if (canLog(ELogServerity::Verbose, ELogOutputType::File))
    {
        OStringStream &stream = loggerImpl->lockLoggerBuffer();
        if (canLogTime())
        {
            stream << TCHAR("[") << timeStr << TCHAR("]");
        }
        stream << CATEGORY << TCHAR("[") << category << TCHAR("]")
            << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]") << srcLoc.function_name() << TCHAR("() : ")
            << message.getChar() 
            << LINE_FEED_TCHAR;
        loggerImpl->unlockLoggerBuffer();
    }

    if (canLog(ELogServerity::Verbose, ELogOutputType::Console))
    {
        std::scoped_lock<CBESpinLock> lockConsole(consoleOutputLock());
#if LOG_TO_CONSOLE
#if SHORT_MSG_IN_CONSOLE
        COUT << message.getChar() << std::endl;
#else
        if (canLogTime())
        {
            COUT << TCHAR("[") << timeStr << TCHAR("]");
        }
        COUT << CATEGORY << TCHAR("[") << category << TCHAR("]")
            << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]") << srcLoc.function_name() << TCHAR("() : ")
            << message.getChar()
            << std::endl;
#endif // SHORT_MSG_IN_CONSOLE
#endif // LOG_TO_CONSOLE
    }

    // Send to profiler
    if (canLog(ELogServerity::Verbose, ELogOutputType::Profiler))
    {
        CBE_PROFILER_MESSAGE_C(message.getChar(), ColorConst::DARK_GRAY);
    }

#else  // DEV_BUILD
    CompilerHacks::ignoreUnused(srcLoc, category, message);
#endif // DEV_BUILD
}
#endif // ENABLE_VERBOSE_LOG

void Logger::debugInternal(const SourceLocationType srcLoc, const TChar *category, const String &message)
{
#if DEV_BUILD
    static const String CATEGORY{ TCHAR("[DEBUG]  ") };

    String timeStr = canLogTime() ? Time::toString(Time::localTimeNow(), false) : TCHAR("");
    const AChar *fileName = filterFileName(srcLoc.file_name());

    if (canLog(ELogServerity::Debug, ELogOutputType::File))
    {
        OStringStream &stream = loggerImpl->lockLoggerBuffer();
        if (canLogTime())
        {
            stream << TCHAR("[") << timeStr << TCHAR("]");
        }
        stream  << CATEGORY << TCHAR("[") << category << TCHAR("]")
            << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]") << srcLoc.function_name() << TCHAR("() : ")
            << message.getChar() 
            << LINE_FEED_TCHAR;
        loggerImpl->unlockLoggerBuffer();
    }

    if (canLog(ELogServerity::Debug, ELogOutputType::Console))
    {
        std::scoped_lock<CBESpinLock> lockConsole(consoleOutputLock());
#if LOG_TO_CONSOLE
#if SHORT_MSG_IN_CONSOLE
        COUT << message.getChar() << std::endl;
#else
        if (canLogTime())
        {
            COUT << TCHAR("[") << timeStr << TCHAR("]");
        }
        COUT  << CATEGORY << TCHAR("[") << category << TCHAR("]")
            << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]") << srcLoc.function_name() << TCHAR("() : ")
            << message.getChar()
            << std::endl;
#endif // SHORT_MSG_IN_CONSOLE
#endif // LOG_TO_CONSOLE
    }

    // Send to profiler
    if (canLog(ELogServerity::Debug, ELogOutputType::Profiler))
    {
        CBE_PROFILER_MESSAGE_C(message.getChar(), ColorConst::GRAY);
    }

#else  // DEV_BUILD
    CompilerHacks::ignoreUnused(srcLoc, category, message);
#endif // DEV_BUILD
}

void Logger::logInternal(const SourceLocationType srcLoc, const TChar *category, const String &message)
{
    static const String CATEGORY{ TCHAR("[LOG]    ") };

    String timeStr = canLogTime() ? Time::toString(Time::localTimeNow(), false) : TCHAR("");
    const AChar *fileName = filterFileName(srcLoc.file_name());

    if (canLog(ELogServerity::Log, ELogOutputType::File))
    {
        OStringStream &stream = loggerImpl->lockLoggerBuffer();
        if (canLogTime())
        {
            stream << TCHAR("[") << timeStr << TCHAR("]");
        }
        stream  << CATEGORY << TCHAR("[") << category << TCHAR("]")
            << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]") << srcLoc.function_name() << TCHAR("() : ")
            << message.getChar() 
            << LINE_FEED_TCHAR;
        loggerImpl->unlockLoggerBuffer();
    }

    if (canLog(ELogServerity::Log, ELogOutputType::Console))
    {
        std::scoped_lock<CBESpinLock> lockConsole(consoleOutputLock());
#if LOG_TO_CONSOLE
#if SHORT_MSG_IN_CONSOLE
        COUT << message.getChar() << std::endl;
#else
        if (canLogTime())
        {
            COUT << TCHAR("[") << timeStr << TCHAR("]");
        }
        COUT  << CATEGORY << TCHAR("[") << category << TCHAR("]")
            << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]") << srcLoc.function_name() << TCHAR("() : ")
            << message.getChar()
            << std::endl;
#endif // SHORT_MSG_IN_CONSOLE
#endif // LOG_TO_CONSOLE
    }

    // Send to profiler
    if (canLog(ELogServerity::Log, ELogOutputType::Profiler))
    {
        CBE_PROFILER_MESSAGE_C(message.getChar(), ColorConst::WHITE);
    }
}

void Logger::warnInternal(const SourceLocationType srcLoc, const TChar *category, const String &message)
{
    static const String CATEGORY{ TCHAR("[WARN]   ") };

    String timeStr = canLogTime() ? Time::toString(Time::localTimeNow(), false) : TCHAR("");
    const AChar *fileName = filterFileName(srcLoc.file_name());

    if (canLog(ELogServerity::Warning, ELogOutputType::File))
    {
        OStringStream &stream = loggerImpl->lockLoggerBuffer();
        if (canLogTime())
        {
            stream << TCHAR("[") << timeStr << TCHAR("]");
        }
        stream << CATEGORY << TCHAR("[") << category << TCHAR("]")
            << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]") << srcLoc.function_name() << TCHAR("() : ")
            << message.getChar() 
            << LINE_FEED_TCHAR;
        loggerImpl->unlockLoggerBuffer();
    }

    if (canLog(ELogServerity::Warning, ELogOutputType::Console))
    {
        std::scoped_lock<CBESpinLock> lockConsole(consoleOutputLock());
#if LOG_TO_CONSOLE

#if ENABLE_VIRTUAL_TERMINAL_SEQ
        CERR << CONSOLE_FOREGROUND_YELLOW;
#else  // ENABLE_VIRTUAL_TERMINAL_SEQ
        PlatformFunctions::setConsoleForegroundColor(255, 255, 0);
#endif // ENABLE_VIRTUAL_TERMINAL_SEQ

#if SHORT_MSG_IN_CONSOLE
        CERR << message.getChar() << std::endl;
#else  // SHORT_MSG_IN_CONSOLE
        if (canLogTime())
        {
            COUT << TCHAR("[") << timeStr << TCHAR("]");
        }
        CERR << CATEGORY << TCHAR("[") << category << TCHAR("]")
            << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]") << srcLoc.function_name() << TCHAR("() : ")
            << message.getChar()
            << std::endl;
#endif // SHORT_MSG_IN_CONSOLE

#if ENABLE_VIRTUAL_TERMINAL_SEQ
        CERR << CONSOLE_FOREGROUND_DEFAULT;
#else  // ENABLE_VIRTUAL_TERMINAL_SEQ
        PlatformFunctions::setConsoleForegroundColor(255, 255, 255);
#endif // ENABLE_VIRTUAL_TERMINAL_SEQ

#endif // LOG_TO_CONSOLE
    }

    // Send to profiler
    if (canLog(ELogServerity::Warning, ELogOutputType::Profiler))
    {
        CBE_PROFILER_MESSAGE_C(message.getChar(), ColorConst::YELLOW);
    }
}

void Logger::errorInternal(const SourceLocationType srcLoc, const TChar *category, const String &message)
{
    static const String CATEGORY{ TCHAR("[ERROR]  ") };

    String timeStr = canLogTime() ? Time::toString(Time::localTimeNow(), false) : TCHAR("");
    const AChar *fileName = filterFileName(srcLoc.file_name());

    if (canLog(ELogServerity::Error, ELogOutputType::File))
    {
        OStringStream &stream = loggerImpl->lockLoggerBuffer();
        if (canLogTime())
        {
            stream << TCHAR("[") << timeStr << TCHAR("]");
        }
        stream << CATEGORY << TCHAR("[") << category << TCHAR("]")
            << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]") << srcLoc.function_name() << TCHAR("() : ")
            << message.getChar() 
            << LINE_FEED_TCHAR;
        loggerImpl->unlockLoggerBuffer();
    }

    if (canLog(ELogServerity::Error, ELogOutputType::Console))
    {
        std::scoped_lock<CBESpinLock> lockConsole(consoleOutputLock());
#if LOG_TO_CONSOLE

#if ENABLE_VIRTUAL_TERMINAL_SEQ
        CERR << CONSOLE_FOREGROUND_YELLOW;
#else  // ENABLE_VIRTUAL_TERMINAL_SEQ
        PlatformFunctions::setConsoleForegroundColor(255, 0, 0);
#endif // ENABLE_VIRTUAL_TERMINAL_SEQ

#if SHORT_MSG_IN_CONSOLE
        CERR << message.getChar() << std::endl;
#else  // SHORT_MSG_IN_CONSOLE
        if (canLogTime())
        {
            COUT << TCHAR("[") << timeStr << TCHAR("]");
        }
        CERR << CATEGORY << TCHAR("[") << category << TCHAR("]")
            << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]") << srcLoc.function_name() << TCHAR("() : ")
            << message.getChar()
            << std::endl;
#endif // SHORT_MSG_IN_CONSOLE

#if ENABLE_VIRTUAL_TERMINAL_SEQ
        CERR << CONSOLE_FOREGROUND_DEFAULT;
#else  // ENABLE_VIRTUAL_TERMINAL_SEQ
        PlatformFunctions::setConsoleForegroundColor(255, 255, 255);
#endif // ENABLE_VIRTUAL_TERMINAL_SEQ

#endif // LOG_TO_CONSOLE
    }

    // Send to profiler
    if (canLog(ELogServerity::Error, ELogOutputType::Profiler))
    {
        CBE_PROFILER_MESSAGE_C(message.getChar(), ColorConst::RED);
    }
}

CBESpinLock &Logger::consoleOutputLock()
{
    static CBESpinLock lock;
    return lock;
}

bool Logger::canLog(ELogServerity severity, ELogOutputType output)
{
    if (loggerImpl == nullptr)
    {
        return false;
    }

    switch (output)
    {
    case Logger::File:
        return BIT_NOT_SET(loggerImpl->muteFlags().back(), severity);
        break;
    case Logger::Console:
        return PlatformFunctions::hasAttachedConsole() && BIT_NOT_SET(loggerImpl->muteFlags().back(), severity);
        break;
    case Logger::Profiler:
        return ENABLE_PROFILING && CBEProfiler::profilerAvailable() && BIT_NOT_SET(loggerImpl->muteFlags().back(), severity);
        break;
    default:
        break;
    }
    return false;
}

bool Logger::canLogTime()
{
    if (loggerImpl)
    {
        return loggerImpl->canLogTime();
    }
    return false;
}

void Logger::flushStream()
{
    if (loggerImpl)
    {
        loggerImpl->flushStream();
    }
}

void Logger::pushMuteSeverities(uint8 muteSeverities)
{
    if (loggerImpl)
    {
        loggerImpl->muteFlags().push_back(muteSeverities);
    }
}

void Logger::popMuteSeverities()
{
    if (loggerImpl && loggerImpl->muteFlags().size() > 1)
    {
        loggerImpl->muteFlags().pop_back();
    }
}

void Logger::initialize()
{
    if (!loggerImpl)
    {
        loggerImpl = new LoggerImpl();
        loggerImpl->initialize();
    }
}

void Logger::shutdown()
{
    flushStream();

    if (loggerImpl)
    {
        LoggerImpl *loggerTemp = loggerImpl;
        loggerImpl = nullptr;
        loggerTemp->shutdown();
        delete loggerTemp;
    }
}

void Logger::startLoggingTime()
{
    if (loggerImpl)
    {
        loggerImpl->startLoggingTime();
    }
}

void Logger::stopLoggingTime()
{
    if (loggerImpl)
    {
        loggerImpl->stopLoggingTime();
    }
}
