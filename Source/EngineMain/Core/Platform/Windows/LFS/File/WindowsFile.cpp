#include "WindowsFile.h"
#include "WindowsFileHandle.h"
#include <windows.h>
#include "../../../PlatformFunctions.h"
#include "../../../LFS/PlatformLFS.h"

WindowsFile::WindowsFile(WindowsFile&& otherFile) : GenericFile()
{
	fileHandle = otherFile.fileHandle;
	otherFile.fileHandle = nullptr;
	fileFlags = std::move(otherFile.fileFlags);
	sharingMode = std::move(otherFile.sharingMode);
	attributes = std::move(otherFile.attributes);
	advancedFlags = std::move(otherFile.advancedFlags);
	fileName = std::move(otherFile.fileName);
	fullPath = std::move(otherFile.fullPath);
	directoryPath = std::move(otherFile.directoryPath);
}

WindowsFile::WindowsFile(const WindowsFile& otherFile) : GenericFile()
{
	fileFlags = otherFile.fileFlags;
	sharingMode = otherFile.sharingMode;
	attributes = otherFile.attributes;
	advancedFlags = otherFile.advancedFlags;
	fileName = otherFile.fileName;
	fullPath = otherFile.fullPath;
	directoryPath = otherFile.directoryPath;
}

void WindowsFile::flush()
{
	if (getFileHandleRaw()) {
		FlushFileBuffers(getFileHandleRaw());
	}
}

bool WindowsFile::exists()
{
	if (getFileName().compare(".") == 0 || getFileName().compare("..") == 0)// not valid files or folders
	{
		return false;
	}

	dword fType = GetFileAttributesA(getFullPath().getChar());

	if (fType == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}

	if (isDirectory())
	{
		return fType & FILE_ATTRIBUTE_DIRECTORY;
	}

	return true;
}

uint64 WindowsFile::fileSize()
{
	UInt64 fSize;
	fSize.quadPart = 0;

	if (getFileHandleRaw()) {
		fSize.lowPart = GetFileSize(getFileHandleRaw(), &fSize.highPart);
	}
	else {
		HANDLE fHandle = FindFirstFileA(getFullPath().getChar(), nullptr);
		if (fHandle != INVALID_HANDLE_VALUE) {
			fSize.lowPart = GetFileSize(fHandle, &fSize.highPart);
		}
	}

	if (fSize.lowPart == INVALID_FILE_SIZE) {
		fSize.quadPart = 0;
	}
	return fSize.quadPart;
}

uint64 WindowsFile::filePointer()
{
	uint64 fPointer = 0;
	LARGE_INTEGER largePointer;
	largePointer.QuadPart = 0;
	if (getFileHandleRaw()) {
		largePointer.LowPart = SetFilePointer(getFileHandleRaw(), largePointer.LowPart, &largePointer.HighPart, FILE_CURRENT);
		fPointer = largePointer.LowPart == INVALID_SET_FILE_POINTER ? 0 :
			TypeConversion<LONGLONG, uint64, true>::toUnsigned(largePointer.QuadPart);
	}
	return fPointer;
}

void WindowsFile::seekEnd()
{
	if (getFileHandleRaw())
	{
		SetFilePointer(getFileHandleRaw(), 0, nullptr, FILE_END);
	}
}

void WindowsFile::seekBegin()
{
	if (getFileHandleRaw()) {
		SetFilePointer(getFileHandleRaw(), 0, nullptr, FILE_BEGIN);
	}
}

void WindowsFile::seek(const int64& pointer)
{
	if (getFileHandleRaw()) {
		LARGE_INTEGER newPointer;
		newPointer.QuadPart = pointer;

		SetFilePointer(getFileHandleRaw(), newPointer.LowPart,&newPointer.HighPart, FILE_BEGIN);
	}
}

void WindowsFile::offsetCursor(const int64& offset)
{
	if (getFileHandleRaw()) {
		LARGE_INTEGER newOffset;
		newOffset.QuadPart = offset;

		SetFilePointer(getFileHandleRaw(), newOffset.LowPart, &newOffset.HighPart, FILE_CURRENT);
	}
}

void WindowsFile::read(std::vector<uint8>& readTo, const uint32& bytesToRead)
{
	if (!getFileHandleRaw() && !(fileFlags & EFileFlags::Read)) {
		return;
	}

	dword readBufferSize = 2 * 1024 * 1024;// 2MB

	uint64 filePointerCache = filePointer();
	uint64 availableSizeCanRead = (fileSize() - filePointerCache);
	dword bytesLeftToRead = (dword)(availableSizeCanRead > bytesToRead ? bytesToRead : availableSizeCanRead);
	readTo.clear();
	readTo.resize(bytesLeftToRead);

	uint64 filePointerOffset = 0;
	dword bytesLastRead = 0;
	while (bytesLeftToRead > 0) {
		ReadFile(getFileHandleRaw(), readTo.data(), bytesLeftToRead > readBufferSize ? readBufferSize : bytesLeftToRead
			, &bytesLastRead, nullptr);

		bytesLeftToRead -= bytesLastRead;
		filePointerOffset += bytesLastRead;
		seek(filePointerOffset + filePointerCache);
	}

	seek(filePointerCache);
}

void WindowsFile::write(const std::vector<uint8>& writeBytes)
{
	if (!getFileHandleRaw() && !(fileFlags & EFileFlags::Write)) {
		return;
	}

	std::vector<uint8>::size_type sizeLeft = writeBytes.size();
	const uint8* pData = writeBytes.data();

	uint32 writeBufferSize = 512 * 1024;// 512KB
	dword bytesWritten = 0;
	uint64 writeFrom = 0;

	uint32 writeSize = writeBufferSize;
	while (sizeLeft > 0)
	{
		writeSize = writeBufferSize > sizeLeft ? (uint32)sizeLeft : writeBufferSize;
		WriteFile(getFileHandleRaw(), pData + writeFrom, writeSize, &bytesWritten, nullptr);

		writeFrom += bytesWritten;
		sizeLeft -= bytesWritten;
		bytesWritten = 0;
	}
}

bool WindowsFile::deleteFile()
{
	if (getFileHandle() && getFileHandleRaw()) {
		closeFile();
	}
	return DeleteFileA(getFullPath().getChar());
}

bool WindowsFile::renameFile(String newName)
{
	WindowsFile newFile{ getHostDirectory().append("\\").append(newName) };

	if (newFile.exists())
	{
		return false;
	}

	bool reopenFile = false;
	if (getFileHandle() && getFileHandleRaw()) {
		closeFile();
		reopenFile = true;
	}
	if (FileSystemFunctions::moveFile(this, &newFile))
	{
		setPaths(newFile.getFullPath());
		if (reopenFile) {
			openFile();
		}
		return true;
	}
	return false;
}

bool WindowsFile::createDirectory()
{
	WindowsFile hostDirectoryFile = WindowsFile(getHostDirectory());
	if (!hostDirectoryFile.exists()) {
		hostDirectoryFile.createDirectory();
	}

	return CreateDirectoryA(getFullPath().getChar(),nullptr);
}

uint64 WindowsFile::lastWriteTimeStamp()
{
	UInt64 timeStamp;
	timeStamp.quadPart = 0;
	if (getFileHandleRaw())
	{
		FILETIME writeTime;
		GetFileTime(getFileHandleRaw(), nullptr, nullptr, &writeTime);
		timeStamp.lowPart = writeTime.dwLowDateTime;
		timeStamp.highPart = writeTime.dwHighDateTime;
	}
	else {
		WIN32_FIND_DATAA data;
		HANDLE fHandle = FindFirstFileA(getFullPath().getChar(), &data);
		if (fHandle != INVALID_HANDLE_VALUE) {
			timeStamp.lowPart = data.ftLastWriteTime.dwLowDateTime;
			timeStamp.highPart = data.ftLastWriteTime.dwHighDateTime;
			FindClose(fHandle);
		}
	}

	return timeStamp.quadPart;
}

GenericFileHandle* WindowsFile::openOrCreateImpl()
{
	WindowsFile hostDirectoryFile{ getHostDirectory() };
	if (!hostDirectoryFile.exists())
	{
		hostDirectoryFile.createDirectory();
	}

	WindowsFileHandle* fHandle = new WindowsFileHandle(fileFlags, sharingMode, attributes, advancedFlags);
	bool openSuccess = false;
	if ((fileFlags & (EFileFlags::CreateNew | EFileFlags::OpenExisting | EFileFlags::ClearExisting)) > 0 && exists())
	{
		if (fileFlags & EFileFlags::CreateNew)
		{
			setCreationAction(EFileFlags::CreateAlways);
		}
		else
		{
			openSuccess = fHandle->openFile(getFullPath());
		}
	}
	else {
		openSuccess = fHandle->openFile(getFullPath());
	}

	if (!openSuccess)
	{
		delete fHandle;
		fHandle = nullptr;
	}
	return fHandle;
}

GenericFileHandle* WindowsFile::openImpl()
{
	WindowsFileHandle* fHandle = new WindowsFileHandle(fileFlags, sharingMode, attributes, advancedFlags);
	
	if (!fHandle->openFile(getFullPath()))
	{
		delete fHandle;
		fHandle = nullptr;
	}

	return fHandle;
}

bool WindowsFile::closeImpl()
{
	WindowsFileHandle* fHandle = static_cast<WindowsFileHandle*>(getFileHandle());
	flush();
	return fHandle->closeFile();
}

bool WindowsFile::dirDelete()
{
	return RemoveDirectoryA(getFullPath().getChar());
}

bool WindowsFile::dirClearAndDelete()
{
	WIN32_FIND_DATAA data;
	HANDLE fHandle = FindFirstFileA(getFullPath().append("\\*").c_str(),&data);

	std::vector<String> filesPath;
	if (fHandle != INVALID_HANDLE_VALUE)
	{
		do {
			String path = getFullPath().append("\\").append(&data.cFileName[0]);
			PlatformFile foundFile{ path };
			if (foundFile.exists() && foundFile.isFile())
			{
				filesPath.push_back(path);
			}
		} while (FindNextFileA(fHandle, &data));
		FindClose(fHandle);
	}
	
	for (const String& filePath : filesPath)
	{
		if (!DeleteFileA(filePath.getChar()))
		{
			return false;
		}
	}

	return dirDelete();
}
