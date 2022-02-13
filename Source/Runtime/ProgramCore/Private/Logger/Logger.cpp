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
#include "Types/Platform/LFS/PlatformLFS.h"

#if LOG_TO_CONSOLE
#include <iostream>

// If 0 both user provided category will be skipped in console log
// Severity category will be printed only for errors
#define SKIP_CAT_IN_CONSOLE 1
#endif // LOG_TO_CONSOLE

OStringStream& Logger::loggerBuffer()
{
    static OStringStream buffer(std::ios_base::trunc);
    return buffer;
}

GenericFile* Logger::getLogFile()
{
    static UniquePtr<GenericFile> logFile;

    if (!logFile)
    {
        String logFileName;
        String logFilePath = FileSystemFunctions::applicationDirectory(logFileName).append(TCHAR("/Saved/Logs/"));
        logFileName = PathFunctions::stripExtension(logFileName);
        PlatformFile checkFile{ logFilePath.append(logFileName).append(TCHAR(".log")) };

        if (checkFile.exists()) 
        {
            uint64 lastWrite = checkFile.lastWriteTimeStamp();
            logFilePath = checkFile.getFullPath();
            checkFile.renameFile(logFileName.append(TCHAR("-")).append(String::toString(lastWrite)).append(TCHAR(".log")));
        }

        logFile = UniquePtr<GenericFile>(new PlatformFile(logFilePath));
        logFile->setFileFlags(EFileFlags::OpenAlways | EFileFlags::Write);
        logFile->setSharingMode(EFileSharing::ReadOnly);
        logFile->setAttributes(EFileAdditionalFlags::Normal);

        bool bLogFileOpened = logFile->openOrCreate();
        fatalAssert(bLogFileOpened, "Failed to open log file %s", logFile->getFullPath());
    }

    return &(*logFile);
}

std::vector<uint8>& Logger::muteFlags()
{
    static std::vector<uint8> serverityMuteFlags = { 0 };
    return serverityMuteFlags;
}

void Logger::debugInternal(const TChar* category, const String& message)
{
#if _DEBUG
    static const String CATEGORY{ TCHAR("[DEBUG]") };
    if (BIT_SET(muteFlags().back(), ELogServerity::Debug))
    {
        return;
    }

    OStringStream& stream = loggerBuffer();
    stream << TCHAR("[") << category << TCHAR("]") << CATEGORY << message.getChar() << LINE_FEED_CHAR;
#if LOG_TO_CONSOLE
    COUT
#if SKIP_CAT_IN_CONSOLE == 0
        << TCHAR("[") << category << TCHAR("]") << CATEGORY
#endif
        << message.getChar() << std::endl;
#endif // LOG_TO_CONSOLE
#endif // _DEBUG
}

void Logger::logInternal(const TChar* category, const String& message)
{
    static const String CATEGORY{ TCHAR("[LOG]") };
    if (BIT_SET(muteFlags().back(), ELogServerity::Log))
    {
        return;
    }

    OStringStream& stream = loggerBuffer();
    stream << TCHAR("[") << category << TCHAR("]") << CATEGORY << message.getChar() << LINE_FEED_CHAR;
#if LOG_TO_CONSOLE
    COUT
#if SKIP_CAT_IN_CONSOLE == 0
        << TCHAR("[") << category << TCHAR("]") << CATEGORY
#endif
        << message.getChar() << std::endl;
#endif // LOG_TO_CONSOLE
}

void Logger::warnInternal(const TChar* category, const String& message)
{
    static const String CATEGORY{ TCHAR("[WARN]") };
    if (BIT_SET(muteFlags().back(), ELogServerity::Warning))
    {
        return;
    }

    OStringStream& stream = loggerBuffer();
    stream << TCHAR("[") << category << TCHAR("]") << CATEGORY << message.getChar() << LINE_FEED_CHAR;
#if LOG_TO_CONSOLE
    CERR
#if SKIP_CAT_IN_CONSOLE == 0
        << TCHAR("[") << category << TCHAR("]") << CATEGORY
#endif
        << message.getChar() << std::endl;
#endif // LOG_TO_CONSOLE
}

void Logger::errorInternal(const TChar* category, const String& message)
{
    static const String CATEGORY{ TCHAR("[ERROR]") };
    if (BIT_SET(muteFlags().back(), ELogServerity::Error))
    {
        return;
    }

    OStringStream& stream = loggerBuffer();
    stream << TCHAR("[") << category << TCHAR("]") << CATEGORY << message.getChar() << LINE_FEED_CHAR;
#if LOG_TO_CONSOLE
    CERR
#if SKIP_CAT_IN_CONSOLE == 0
        << TCHAR("[") << category << TCHAR("]")
#endif
        << CATEGORY << message.getChar() << std::endl;
#endif // LOG_TO_CONSOLE
}

void Logger::flushStream()
{
    GenericFile* logFile = getLogFile();
    auto str = loggerBuffer().str();
    if (!str.empty() && logFile)
    {
        std::string utf8str{ TCHAR_TO_UTF8(str.c_str()) };
        logFile->write(ArrayView<uint8>(reinterpret_cast<uint8*>(utf8str.data()), uint32(utf8str.length())));
        loggerBuffer().str({});
    }
}

void Logger::pushMuteSeverities(uint8 muteSeverities)
{
    muteFlags().push_back(muteSeverities);
}

void Logger::popMuteSeverities()
{
    if (muteFlags().size() > 1)
    {
        muteFlags().pop_back();
    }
}
