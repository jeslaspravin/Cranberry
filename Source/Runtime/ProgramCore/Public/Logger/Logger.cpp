/*!
 * \file Logger.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
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

NODISCARD constexpr const AChar *filterFileName(const AChar *fileName) noexcept
{
    SizeT len = TCharStr::length(fileName);
    while (len > 0)
    {
        len--;
        if (fileName[len] == '\\' || fileName[len] == '/')
        {
            return fileName + len + 1;
        }
    }
    return fileName;
}

NODISCARD constexpr std::string_view filterFuncName(const AChar *funcName) noexcept
{
    SizeT funcNameEnd = 0;
    bool bFound = TCharStr::rfind<AChar>(funcName, '(', &funcNameEnd);
    if (bFound)
    {
        SizeT funcNameStart = funcNameEnd;
        while (funcNameStart > 0)
        {
            funcNameStart--;
            if (funcName[funcNameStart] == ':' || std::isspace(funcName[funcNameStart]))
            {
                funcNameStart += 1;
                return { funcName + funcNameStart, funcNameEnd - funcNameStart };
            }
        }
        return { funcName, funcNameStart };
    }
    return funcName;
}

// Maps log severity to corresponding log out's string
const StringView SEVERITY_OUT_STR[Logger::ESeverityID::SevID_Max]
    = { TCHAR("[VERBOSE]"), TCHAR("[DEBUG]  "), TCHAR("[LOG]    "), TCHAR("[WARN]   "), TCHAR("[ERROR]  ") };

//////////////////////////////////////////////////////////////////////////
/// Logger impl
//////////////////////////////////////////////////////////////////////////

class LoggerImpl
{
private:
    using LoggerWorkerTask = copat::JobSystemWorkerThreadTask;
    using LogPacketsDelegate = Delegate<const String &, const std::vector<Logger::LogMsgPacket> &>;

    struct LoggerPerThreadData
    {
        // Right now we do per thread mute which is not correct, However this is better than solving below multi threaded scenario
        // Thread 1 Pushes -> Thread 2 Pushes -> Thread 1 Pops incorrect mute flags -> Thread 2 Pops incorrect mute flags
        std::vector<uint8> serverityMuteFlags{ 0 };

        OStringStream bufferStream = OStringStream(std::ios_base::trunc);
        // Packets to log this frame
        std::vector<Logger::LogMsgPacket> packets;
        // Per thread stream locks when flushing all streams
        CBESpinLock streamRWLock;
    };

    uint32 tlsSlot;
    // This will not be contented in fast path, fast path which is log, debug, warn, error has minimum spin lock contention
    CBESpinLock allTlDataLock;
    std::vector<LoggerPerThreadData *> allPerThreadData;

    LoggerWorkerTask lastFlushTask{ nullptr };

    CBESpinLock packetsListenersLock;
    LogPacketsDelegate packetsListeners;

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
    using LogPacketsListener = LogPacketsDelegate::SingleCastDelegateType;

public:
    bool initialize();
    void shutdown();
    void startLoggingTime() { bEnableLogTime.test_and_set(std::memory_order::relaxed); }
    void stopLoggingTime() { bEnableLogTime.clear(std::memory_order::relaxed); }

    std::vector<uint8> &muteFlags() { return getOrCreatePerThreadData().serverityMuteFlags; }
    bool canLogTime() const { return bEnableLogTime.test(std::memory_order::relaxed); }

    DelegateHandle bindPacketListener(LogPacketsListener &&listener)
    {
        std::scoped_lock<CBESpinLock> lockListenersList(packetsListenersLock);
        return packetsListeners.bind(std::forward<LogPacketsDelegate::SingleCastDelegateType>(listener));
    }
    void unbindPacketListener(DelegateHandle handle)
    {
        std::scoped_lock<CBESpinLock> lockListenersList(packetsListenersLock);
        packetsListeners.unbind(handle);
    }

    OStringStream &lockLoggerBuffer()
    {
        LoggerPerThreadData &tlData = getOrCreatePerThreadData();
        tlData.streamRWLock.lock();
        return tlData.bufferStream;
    }
    Logger::LogMsgPacket &getPacketPayload()
    {
        LoggerPerThreadData &tlData = getOrCreatePerThreadData();
        debugAssertf(!tlData.streamRWLock.try_lock(), "Packet payload must be retrieved only after locking the logger stream buffer");
        return tlData.packets.emplace_back();
    }
    void unlockLoggerBuffer() { getOrCreatePerThreadData().streamRWLock.unlock(); }

    void flushStream() noexcept;

private:
    bool openNewLogFile();
    void flushStreamInternal() noexcept;

    LoggerWorkerTask flushInWorkerThread(LoggerWorkerTask execAfter) noexcept
    {
        co_await execAfter;
        // Frees old task
        execAfter = { nullptr };
        flushStreamInternal();
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
    lastFlushTask = { nullptr };
    packetsListeners.clear();
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

void LoggerImpl::flushStream() noexcept
{
    if (copat::JobSystem::get())
    {
        lastFlushTask = flushInWorkerThread(std::move(lastFlushTask));
    }
    else
    {
        flushStreamInternal();
    }
}

void LoggerImpl::flushStreamInternal() noexcept
{
    // The final flush happens after the profiler is shutdown
    CBE_PROFILER_SCOPE_DYN(CBE_PROFILER_CHAR("FlushLogStream"), CBEProfiler::profilerAvailable());

    String bufferStr;
    std::vector<Logger::LogMsgPacket> allPackets;

    String tempOutputBuffer;

    allTlDataLock.lock();
    for (LoggerPerThreadData *tlData : allPerThreadData)
    {
        if (tlData->bufferStream.tellp() != 0)
        {
            SizeT bufferOffset = 0;
            std::vector<Logger::LogMsgPacket> tlPackets;
            // Pull from thread data
            {
                CBE_PROFILER_SCOPE_DYN(CBE_PROFILER_CHAR("PullThreadLocalLogs"), CBEProfiler::profilerAvailable());

                // If not empty then only lock it, Else we can log in next flush!
                std::scoped_lock<CBESpinLock> lockTlStream(tlData->streamRWLock);

                bufferOffset = bufferStr.length();

                // Below code uses null termination to mark end of string, by appending a null char at end, This works and does not clears
                // buffer because c-style string works on null terminated
                // tlData->bufferStream << '\0';
                // bufferStr.append({ &tlData->bufferStream.view().front() });
                // tlData->bufferStream.seekp(0);

                // Below code clears buffer after every data flush
                bufferStr.append(tlData->bufferStream.view());
                tlData->bufferStream.str({});

                tlPackets = std::move(tlData->packets);
            }

            CBE_PROFILER_SCOPE_DYN(CBE_PROFILER_CHAR("WriteThreadLocalLogs"), CBEProfiler::profilerAvailable());
            // Calculate total string size required for this thread logs
            SizeT outputLen = (canLogTime() ? Time::toStringLen() + 2 : 0) /* + 2 because of Time's "[]" */
                              + SEVERITY_OUT_STR[0].length()               /* All severity strings are same size */
                              + 2                                          /* For category's [] */
                              + 3 + 5                                      /* 3 For [Filename:Line], 5 for 5 digit line count */
                              + 5                                          /* For Function name's "() : " */
                              + TCharStr::length(LINE_FEED_TCHAR);
            outputLen = outputLen * tlPackets.size() + (bufferStr.length() - bufferOffset);
            tempOutputBuffer.clear();
            tempOutputBuffer.reserve(outputLen);

            allPackets.reserve(allPackets.size() + tlPackets.size());
            auto processSinglePacket = [&tempOutputBuffer, &allPackets, &bufferStr, bufferOffset](Logger::LogMsgPacket &packet)
            {
                packet.categoryStart += bufferOffset;
                packet.messageStart += bufferOffset;

                allPackets.push_back(packet);

                StringView categoryView{ bufferStr.getChar() + packet.categoryStart, packet.categorySize };
                StringView msgView{ bufferStr.getChar() + packet.messageStart, packet.messageSize };

                tempOutputBuffer.append(SEVERITY_OUT_STR[packet.severity]);

                tempOutputBuffer.append(TCHAR("["));
                tempOutputBuffer.append(categoryView);
                tempOutputBuffer.append(TCHAR("]"));

                tempOutputBuffer.append(TCHAR("["));
                tempOutputBuffer.append(packet.getFileName());
                tempOutputBuffer.append(TCHAR(":"));
                tempOutputBuffer.append(String::toString(packet.srcLoc.line()));
                tempOutputBuffer.append(TCHAR("]"));

                tempOutputBuffer.append(packet.getFuncName());
                tempOutputBuffer.append(TCHAR("() : "));

                tempOutputBuffer.append(msgView);
                tempOutputBuffer.append(LINE_FEED_TCHAR);
            };
            if (canLogTime())
            {
                for (Logger::LogMsgPacket &packet : tlPackets)
                {
                    // Make sure packet also has valid log's timestamp
                    if (packet.timeStamp != 0)
                    {
                        tempOutputBuffer.append(TCHAR("["));
                        tempOutputBuffer.append(Time::toString(packet.timeStamp, false));
                        tempOutputBuffer.append(TCHAR("]"));
                    }

                    processSinglePacket(packet);
                }
            }
            else
            {
                for (Logger::LogMsgPacket &packet : tlPackets)
                {
                    processSinglePacket(packet);
                }
            }

            const std::string utf8str{ TCHAR_TO_UTF8(tempOutputBuffer.getChar()) };
            logFile.write({ reinterpret_cast<const uint8 *>(utf8str.data()), utf8str.length() });
            PlatformFunctions::outputToDebugger(tempOutputBuffer.getChar());
        }
    }
    allTlDataLock.unlock();

    // Broadcast log packets to other systems
    if (packetsListeners.isBound())
    {
        CBE_PROFILER_SCOPE_DYN(CBE_PROFILER_CHAR("BroadcastLogPackets"), CBEProfiler::profilerAvailable());
        std::scoped_lock<CBESpinLock> lockListenersList(packetsListenersLock);
        packetsListeners.invoke(bufferStr, allPackets);
    }
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
        String renameTo = STR_FORMAT(TCHAR("{}-{}.log"), logFileName, lastWrite);
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

static_assert(
    std::is_same_v<TickRep, decltype(Logger::LogMsgPacket::timeStamp)>,
    "Type mismatch between Time's TickRep and Timestamp stored in LogMsgPacket"
);

#if ENABLE_VERBOSE_LOG
void Logger::verboseInternal(const SourceLocationType srcLoc, const TChar *category, const String &message)
{
#if DEV_BUILD
    TickRep timeStamp = 0;
    String timeStr;
    if (canLogTime())
    {
        timeStamp = Time::localTimeNow();
        timeStr = Time::toString(timeStamp, false);
    }
    const AChar *fileName = filterFileName(srcLoc.file_name());
    std::string_view funcName = filterFuncName(srcLoc.function_name());

    if (canLog(ELogSeverity::Verbose, ELogOutputType::File))
    {
        OStringStream &stream = loggerImpl->lockLoggerBuffer();

        LogMsgPacket &packet = loggerImpl->getPacketPayload();
        packet.srcLoc = srcLoc;
        packet.fileNameOffset = uint32(fileName - srcLoc.file_name());
        packet.funcNameOffset = uint32(funcName.data() - srcLoc.function_name());
        packet.funcNameSize = uint32(funcName.size());
        packet.timeStamp = timeStamp;
        packet.severity = ESeverityID::SevID_Verbose;
        // Push category
        packet.categoryStart = stream.tellp();
        packet.categorySize = uint32(TCharStr::length(category));
        stream << category;
        // Push message
        packet.messageStart = stream.tellp();
        packet.messageSize = uint32(message.length());
        stream << message;

        loggerImpl->unlockLoggerBuffer();
    }

    if (canLog(ELogSeverity::Verbose, ELogOutputType::Console))
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
        COUT << SEVERITY_OUT_STR[ESeverityID::SevID_Verbose];
        COUT << TCHAR("[") << category << TCHAR("]");
        COUT << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]");
        COUT << funcName << TCHAR("() : ");
        COUT << message.getChar();
        COUT << std::endl;
#endif // SHORT_MSG_IN_CONSOLE
#endif // LOG_TO_CONSOLE
    }

    // Send to profiler
    if (canLog(ELogSeverity::Verbose, ELogOutputType::Profiler))
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

    TickRep timeStamp = 0;
    String timeStr;
    if (canLogTime())
    {
        timeStamp = Time::localTimeNow();
        timeStr = Time::toString(timeStamp, false);
    }
    const AChar *fileName = filterFileName(srcLoc.file_name());
    std::string_view funcName = filterFuncName(srcLoc.function_name());

    if (canLog(ELogSeverity::Debug, ELogOutputType::File))
    {
        OStringStream &stream = loggerImpl->lockLoggerBuffer();

        LogMsgPacket &packet = loggerImpl->getPacketPayload();
        packet.srcLoc = srcLoc;
        packet.fileNameOffset = uint32(fileName - srcLoc.file_name());
        packet.funcNameOffset = uint32(funcName.data() - srcLoc.function_name());
        packet.funcNameSize = uint32(funcName.size());
        packet.timeStamp = timeStamp;
        packet.severity = ESeverityID::SevID_Debug;
        // Push category
        packet.categoryStart = stream.tellp();
        packet.categorySize = uint32(TCharStr::length(category));
        stream << category;
        // Push message
        packet.messageStart = stream.tellp();
        packet.messageSize = uint32(message.length());
        stream << message;

        loggerImpl->unlockLoggerBuffer();
    }

    if (canLog(ELogSeverity::Debug, ELogOutputType::Console))
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
        COUT << SEVERITY_OUT_STR[ESeverityID::SevID_Debug];
        COUT << TCHAR("[") << category << TCHAR("]");
        COUT << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]");
        COUT << funcName << TCHAR("() : ");
        COUT << message.getChar();
        COUT << std::endl;
#endif // SHORT_MSG_IN_CONSOLE
#endif // LOG_TO_CONSOLE
    }

    // Send to profiler
    if (canLog(ELogSeverity::Debug, ELogOutputType::Profiler))
    {
        CBE_PROFILER_MESSAGE_C(message.getChar(), ColorConst::GRAY);
    }

#else  // DEV_BUILD
    CompilerHacks::ignoreUnused(srcLoc, category, message);
#endif // DEV_BUILD
}

void Logger::logInternal(const SourceLocationType srcLoc, const TChar *category, const String &message)
{
    TickRep timeStamp = 0;
    String timeStr;
    if (canLogTime())
    {
        timeStamp = Time::localTimeNow();
        timeStr = Time::toString(timeStamp, false);
    }
    const AChar *fileName = filterFileName(srcLoc.file_name());
    std::string_view funcName = filterFuncName(srcLoc.function_name());

    if (canLog(ELogSeverity::Log, ELogOutputType::File))
    {
        OStringStream &stream = loggerImpl->lockLoggerBuffer();

        LogMsgPacket &packet = loggerImpl->getPacketPayload();
        packet.srcLoc = srcLoc;
        packet.fileNameOffset = uint32(fileName - srcLoc.file_name());
        packet.funcNameOffset = uint32(funcName.data() - srcLoc.function_name());
        packet.funcNameSize = uint32(funcName.size());
        packet.timeStamp = timeStamp;
        packet.severity = ESeverityID::SevID_Log;
        // Push category
        packet.categoryStart = stream.tellp();
        packet.categorySize = uint32(TCharStr::length(category));
        stream << category;
        // Push message
        packet.messageStart = stream.tellp();
        packet.messageSize = uint32(message.length());
        stream << message;

        loggerImpl->unlockLoggerBuffer();
    }

    if (canLog(ELogSeverity::Log, ELogOutputType::Console))
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
        COUT << SEVERITY_OUT_STR[ESeverityID::SevID_Log];
        COUT << TCHAR("[") << category << TCHAR("]");
        COUT << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]");
        COUT << funcName << TCHAR("() : ");
        COUT << message.getChar();
        COUT << std::endl;
#endif // SHORT_MSG_IN_CONSOLE
#endif // LOG_TO_CONSOLE
    }

    // Send to profiler
    if (canLog(ELogSeverity::Log, ELogOutputType::Profiler))
    {
        CBE_PROFILER_MESSAGE_C(message.getChar(), ColorConst::WHITE);
    }
}

void Logger::warnInternal(const SourceLocationType srcLoc, const TChar *category, const String &message)
{
    TickRep timeStamp = 0;
    String timeStr;
    if (canLogTime())
    {
        timeStamp = Time::localTimeNow();
        timeStr = Time::toString(timeStamp, false);
    }
    const AChar *fileName = filterFileName(srcLoc.file_name());
    std::string_view funcName = filterFuncName(srcLoc.function_name());

    if (canLog(ELogSeverity::Warning, ELogOutputType::File))
    {
        OStringStream &stream = loggerImpl->lockLoggerBuffer();

        LogMsgPacket &packet = loggerImpl->getPacketPayload();
        packet.srcLoc = srcLoc;
        packet.fileNameOffset = uint32(fileName - srcLoc.file_name());
        packet.funcNameOffset = uint32(funcName.data() - srcLoc.function_name());
        packet.funcNameSize = uint32(funcName.size());
        packet.timeStamp = timeStamp;
        packet.severity = ESeverityID::SevID_Warning;
        // Push category
        packet.categoryStart = stream.tellp();
        packet.categorySize = uint32(TCharStr::length(category));
        stream << category;
        // Push message
        packet.messageStart = stream.tellp();
        packet.messageSize = uint32(message.length());
        stream << message;

        loggerImpl->unlockLoggerBuffer();
    }

    if (canLog(ELogSeverity::Warning, ELogOutputType::Console))
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
            CERR << TCHAR("[") << timeStr << TCHAR("]");
        }
        CERR << SEVERITY_OUT_STR[ESeverityID::SevID_Warning];
        CERR << TCHAR("[") << category << TCHAR("]");
        CERR << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]");
        CERR << funcName << TCHAR("() : ");
        CERR << message.getChar();
        CERR << std::endl;
#endif // SHORT_MSG_IN_CONSOLE

#if ENABLE_VIRTUAL_TERMINAL_SEQ
        CERR << CONSOLE_FOREGROUND_DEFAULT;
#else  // ENABLE_VIRTUAL_TERMINAL_SEQ
        PlatformFunctions::setConsoleForegroundColor(255, 255, 255);
#endif // ENABLE_VIRTUAL_TERMINAL_SEQ

#endif // LOG_TO_CONSOLE
    }

    // Send to profiler
    if (canLog(ELogSeverity::Warning, ELogOutputType::Profiler))
    {
        CBE_PROFILER_MESSAGE_C(message.getChar(), ColorConst::YELLOW);
    }
}

void Logger::errorInternal(const SourceLocationType srcLoc, const TChar *category, const String &message)
{
    TickRep timeStamp = 0;
    String timeStr;
    if (canLogTime())
    {
        timeStamp = Time::localTimeNow();
        timeStr = Time::toString(timeStamp, false);
    }
    const AChar *fileName = filterFileName(srcLoc.file_name());
    std::string_view funcName = filterFuncName(srcLoc.function_name());

    if (canLog(ELogSeverity::Error, ELogOutputType::File))
    {
        OStringStream &stream = loggerImpl->lockLoggerBuffer();

        LogMsgPacket &packet = loggerImpl->getPacketPayload();
        packet.srcLoc = srcLoc;
        packet.fileNameOffset = uint32(fileName - srcLoc.file_name());
        packet.funcNameOffset = uint32(funcName.data() - srcLoc.function_name());
        packet.funcNameSize = uint32(funcName.size());
        packet.timeStamp = timeStamp;
        packet.severity = ESeverityID::SevID_Error;
        // Push category
        packet.categoryStart = stream.tellp();
        packet.categorySize = uint32(TCharStr::length(category));
        stream << category;
        // Push message
        packet.messageStart = stream.tellp();
        packet.messageSize = uint32(message.length());
        stream << message;

        loggerImpl->unlockLoggerBuffer();
    }

    if (canLog(ELogSeverity::Error, ELogOutputType::Console))
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
            CERR << TCHAR("[") << timeStr << TCHAR("]");
        }
        CERR << SEVERITY_OUT_STR[ESeverityID::SevID_Error];
        CERR << TCHAR("[") << category << TCHAR("]");
        CERR << TCHAR("[") << fileName << TCHAR(":") << srcLoc.line() << TCHAR("]");
        CERR << funcName << TCHAR("() : ");
        CERR << message.getChar();
        CERR << std::endl;
#endif // SHORT_MSG_IN_CONSOLE

#if ENABLE_VIRTUAL_TERMINAL_SEQ
        CERR << CONSOLE_FOREGROUND_DEFAULT;
#else  // ENABLE_VIRTUAL_TERMINAL_SEQ
        PlatformFunctions::setConsoleForegroundColor(255, 255, 255);
#endif // ENABLE_VIRTUAL_TERMINAL_SEQ

#endif // LOG_TO_CONSOLE
    }

    // Send to profiler
    if (canLog(ELogSeverity::Error, ELogOutputType::Profiler))
    {
        CBE_PROFILER_MESSAGE_C(message.getChar(), ColorConst::RED);
    }
}

CBESpinLock &Logger::consoleOutputLock()
{
    static CBESpinLock lock;
    return lock;
}

bool Logger::canLog(ELogSeverity severity, ELogOutputType output)
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

DelegateHandle Logger::bindPacketListener(const LambdaPacketsFunc &listener)
{
    if (loggerImpl)
    {
        return loggerImpl->bindPacketListener(LoggerImpl::LogPacketsListener::createLambda(listener));
    }
    return {};
}

DelegateHandle Logger::bindPacketListener(StaticPacketsFunc listener)
{
    if (loggerImpl)
    {
        return loggerImpl->bindPacketListener(LoggerImpl::LogPacketsListener::createStatic(listener));
    }
    return {};
}

void Logger::unbindPacketListener(DelegateHandle handle)
{
    if (loggerImpl)
    {
        return loggerImpl->unbindPacketListener(handle);
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
