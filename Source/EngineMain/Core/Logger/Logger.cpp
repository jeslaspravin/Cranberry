#include "Logger.h"
#include "../Platform/LFS/PlatformLFS.h"
#include <sstream>
#include <cstdarg>


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
		logFile->openOrCreate();
	}

	return &(*logFile);
}

void Logger::writeString(const String& message)
{
	std::vector<uint8> data;
	data.resize(message.length());
	memcpy(data.data(), message.getChar(), message.length());
	if(getLogFile())
		getLogFile()->write(data);
}

void Logger::debugInternal(const String& category, const String& message)
{
#if _DEBUG
	static const String CATEGORY = "[DEBUG]";

	std::stringstream stream;
	stream << "[" << category << "]" << CATEGORY << message.getChar() << "\r\n";

	writeString(stream.str());
#endif
}

void Logger::logInternal(const String& category, const String& message)
{
	static const String CATEGORY = "[LOG]";

	std::stringstream stream;
	stream << "[" << category << "]" << CATEGORY << message.getChar() << "\r\n";

	writeString(stream.str());
}

void Logger::warnInternal(const String& category, const String& message)
{
	static const String CATEGORY = "[WARN]";

	std::stringstream stream;
	stream << "[" << category << "]" << CATEGORY << message.getChar() << "\r\n";

	writeString(stream.str());
}

void Logger::errorInternal(const String& category, const String& message)
{
	static const String CATEGORY = "[ERROR]";

	std::stringstream stream;
	stream << "[" << category << "]" << CATEGORY << message.getChar() << "\r\n";

	writeString(stream.str());
}