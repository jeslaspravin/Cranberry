#include "Logger/Logger.h"
#include "Types/Platform/LFS/PlatformLFS.h"

#include <cstdarg>
#if LOG_TO_CONSOLE
#include <iostream>
#endif // LOG_TO_CONSOLE

std::ostringstream& Logger::loggerBuffer()
{
    static std::ostringstream buffer(std::ios_base::trunc);
    return buffer;
}

GenericFile* Logger::getLogFile()
{
    static UniquePtr<GenericFile> logFile;

    if (!logFile)
    {
        String logFileName;
        String logFilePath = FileSystemFunctions::applicationDirectory(logFileName).append("/Saved/Logs/");
        String extension;
        logFileName = FileSystemFunctions::stripExtension(logFileName, extension);
        PlatformFile checkFile{ logFilePath.append(logFileName).append(".log") };

        if (checkFile.exists()) 
        {
            uint64 lastWrite = checkFile.lastWriteTimeStamp();
            logFilePath = checkFile.getFullPath();
            checkFile.renameFile(logFileName.append("-").append(std::to_string(lastWrite)).append(".log"));
        }

        logFile = UniquePtr<GenericFile>(new PlatformFile(logFilePath));
        logFile->setFileFlags(EFileFlags::OpenAlways | EFileFlags::Write);
        logFile->setSharingMode(EFileSharing::ReadOnly);
        logFile->setAttributes(EFileAdditionalFlags::Normal);
    }

    return &(*logFile);
}

void Logger::debugInternal(const AChar* category, const String& message)
{
#if _DEBUG
    static const String CATEGORY = "[DEBUG]";

    std::ostringstream& stream = loggerBuffer();
    stream << "[" << category << "]" << CATEGORY << message.getChar() << LINE_FEED_CHAR;
#if LOG_TO_CONSOLE
    std::cout << "[" << category << "]" << CATEGORY << message.getChar() << LINE_FEED_CHAR;
#endif // LOG_TO_CONSOLE
#endif // _DEBUG
}

void Logger::logInternal(const AChar* category, const String& message)
{
    static const String CATEGORY = "[LOG]";

    std::ostringstream& stream = loggerBuffer();
    stream << "[" << category << "]" << CATEGORY << message.getChar() << LINE_FEED_CHAR;
#if LOG_TO_CONSOLE
    std::cout << "[" << category << "]" << CATEGORY << message.getChar() << LINE_FEED_CHAR;
#endif // LOG_TO_CONSOLE
}

void Logger::warnInternal(const AChar* category, const String& message)
{
    static const String CATEGORY = "[WARN]";

    std::ostringstream& stream = loggerBuffer();
    stream << "[" << category << "]" << CATEGORY << message.getChar() << LINE_FEED_CHAR;
#if LOG_TO_CONSOLE
    std::cerr << "[" << category << "]" << CATEGORY << message.getChar() << LINE_FEED_CHAR;
#endif // LOG_TO_CONSOLE
}

void Logger::errorInternal(const AChar* category, const String& message)
{
    static const String CATEGORY = "[ERROR]";

    std::ostringstream& stream = loggerBuffer();
    stream << "[" << category << "]" << CATEGORY << message.getChar() << LINE_FEED_CHAR;
#if LOG_TO_CONSOLE
    std::cerr << "[" << category << "]" << CATEGORY << message.getChar() << LINE_FEED_CHAR;
#endif // LOG_TO_CONSOLE
}

void Logger::flushStream()
{
    GenericFile* logFile = getLogFile();
    auto str = loggerBuffer().str();
    if (!str.empty() && logFile && logFile->openOrCreate())
    {
        logFile->seekEnd();
        logFile->write(ArrayView<uint8>(reinterpret_cast<uint8*>(str.data()), uint32(str.length())));
        loggerBuffer().str({});
        logFile->closeFile();
    }
}
