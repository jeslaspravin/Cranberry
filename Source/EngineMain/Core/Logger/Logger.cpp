#include "Logger.h"
#include "../Platform/LFS/PlatformLFS.h"
#include <sstream>
#include <cstdarg>

std::ostringstream& loggerBuffer()
{
    static std::ostringstream buffer;
    return buffer;
}

GenericFile* Logger::getLogFile()
{
    static UniquePtr<GenericFile> logFile;

    if (!logFile)
    {
        String logFileName;
        String logFilePath = FileSystemFunctions::applicationDirectory(logFileName).append("\\Saved\\");
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
        logFile->setFileFlags(EFileFlags::CreateAlways | EFileFlags::Write);
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
    stream << "[" << category << "]" << CATEGORY << message.getChar() << "\r\n";
#endif
}

void Logger::logInternal(const AChar* category, const String& message)
{
    static const String CATEGORY = "[LOG]";

    std::ostringstream& stream = loggerBuffer();
    stream << "[" << category << "]" << CATEGORY << message.getChar() << "\r\n";
}

void Logger::warnInternal(const AChar* category, const String& message)
{
    static const String CATEGORY = "[WARN]";

    std::ostringstream& stream = loggerBuffer();
    stream << "[" << category << "]" << CATEGORY << message.getChar() << "\r\n";
}

void Logger::errorInternal(const AChar* category, const String& message)
{
    static const String CATEGORY = "[ERROR]";

    std::ostringstream& stream = loggerBuffer();
    stream << "[" << category << "]" << CATEGORY << message.getChar() << "\r\n";
}

void Logger::flushStream()
{
    GenericFile* logFile = getLogFile();
    auto str = loggerBuffer().str();
    if (!str.empty() && logFile && logFile->openOrCreate())
    {
        logFile->seekEnd();
        logFile->write(reinterpret_cast<const uint8*>(str.data()), str.length());
        loggerBuffer().flush();
        logFile->closeFile();
    }
}
