/*!
 * \file WindowsFile.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "LFS/File/WindowsFile.h"
#include "LFS/File/WindowsFileHandle.h"
#include "Logger/Logger.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"
#include "WindowsCommonHeaders.h"

WindowsFile::WindowsFile(WindowsFile &&otherFile)
    : GenericFile()
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

WindowsFile::WindowsFile(const WindowsFile &otherFile)
    : GenericFile()
{
    fileFlags = otherFile.fileFlags;
    sharingMode = otherFile.sharingMode;
    attributes = otherFile.attributes;
    advancedFlags = otherFile.advancedFlags;
    fileName = otherFile.fileName;
    fullPath = otherFile.fullPath;
    directoryPath = otherFile.directoryPath;
}

void WindowsFile::operator=(const WindowsFile &otherFile)
{
    fileFlags = otherFile.fileFlags;
    sharingMode = otherFile.sharingMode;
    attributes = otherFile.attributes;
    advancedFlags = otherFile.advancedFlags;
    fileName = otherFile.fileName;
    fullPath = otherFile.fullPath;
    directoryPath = otherFile.directoryPath;
}

void WindowsFile::operator=(WindowsFile &&otherFile)
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

WindowsFile::~WindowsFile()
{
    if (getFileHandle() != nullptr)
    {
        LOG_WARN("WindowsFile", "File %s is not closed, Please close it before destroying",
            getFullPath().getChar());
        closeFile();
    }
}

void WindowsFile::flush() const
{
    if (getFileHandleRaw())
    {
        ::FlushFileBuffers(getFileHandleRaw());
    }
}

bool WindowsFile::exists() const
{
    if (getFileName().compare(TCHAR(".")) == 0
        || getFileName().compare(TCHAR("..")) == 0) // not valid files or folders
    {
        return false;
    }

    dword fType = ::GetFileAttributes(getFullPath().getChar());

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

uint64 WindowsFile::fileSize() const
{
    UInt64 fSize;
    fSize.quadPart = 0;

    if (getFileHandleRaw())
    {
        fSize.lowPart = ::GetFileSize(getFileHandleRaw(), &fSize.highPart);
    }
    else
    {

        WIN32_FIND_DATA data;
        HANDLE fHandle = ::FindFirstFile(getFullPath().getChar(), &data);
        if (fHandle != INVALID_HANDLE_VALUE)
        {
            fSize.lowPart = data.nFileSizeLow;
            fSize.highPart = data.nFileSizeHigh;
        }
    }

    if (fSize.lowPart == INVALID_FILE_SIZE)
    {
        fSize.quadPart = 0;
    }
    return fSize.quadPart;
}

uint64 WindowsFile::filePointer() const
{
    uint64 fPointer = 0;
    LARGE_INTEGER largePointer;
    largePointer.QuadPart = 0;
    if (getFileHandleRaw())
    {
        largePointer.LowPart = ::SetFilePointer(
            getFileHandleRaw(), largePointer.LowPart, &largePointer.HighPart, FILE_CURRENT);
        fPointer
            = largePointer.LowPart == INVALID_SET_FILE_POINTER ? 0u : (uint64)(largePointer.QuadPart);
    }
    return fPointer;
}

void WindowsFile::seekEnd() const
{
    if (getFileHandleRaw())
    {
        ::SetFilePointer(getFileHandleRaw(), 0, nullptr, FILE_END);
        debugAssert(filePointer() == fileSize());
    }
}

void WindowsFile::seekBegin() const
{
    if (getFileHandleRaw())
    {
        ::SetFilePointer(getFileHandleRaw(), 0, nullptr, FILE_BEGIN);
        debugAssert(filePointer() == 0);
    }
}

void WindowsFile::seek(const int64 &pointer) const
{
    if (getFileHandleRaw())
    {
        LARGE_INTEGER newPointer;
        newPointer.QuadPart = pointer;

        ::SetFilePointer(getFileHandleRaw(), newPointer.LowPart, &newPointer.HighPart, FILE_BEGIN);
    }
}

void WindowsFile::offsetCursor(const int64 &offset) const
{
    if (getFileHandleRaw())
    {
        LARGE_INTEGER newOffset;
        newOffset.QuadPart = offset;

        ::SetFilePointer(getFileHandleRaw(), newOffset.LowPart, &newOffset.HighPart, FILE_CURRENT);
    }
}

bool WindowsFile::setFileSize(const int64 &newSize) const
{
    if (!getFileHandleRaw() || BIT_NOT_SET(fileFlags, EFileFlags::Write))
    {
        return false;
    }

    uint64 filePointerCache = filePointer();
    seek(newSize);

    bool bResized = false;
    if (::SetEndOfFile((HANDLE)getFileHandleRaw()) == TRUE)
    {
        filePointerCache = Math::min(newSize, filePointerCache);
        bResized = true;
    }
    seek(filePointerCache);
    return bResized;
}

void WindowsFile::read(std::vector<uint8> &readTo, const uint32 &bytesToRead /*= (~0u)*/) const
{
    if (!getFileHandleRaw() || BIT_NOT_SET(fileFlags, EFileFlags::Read))
    {
        return;
    }

    const dword readBufferSize = 10 * 1024 * 1024; // 10MB

    uint64 filePointerCache = filePointer();
    uint64 availableSizeCanRead = (fileSize() - filePointerCache);
    dword bytesLeftToRead
        = (dword)(availableSizeCanRead > bytesToRead ? bytesToRead : availableSizeCanRead);
    readTo.clear();
    readTo.resize(bytesLeftToRead, 0);

    read(readTo.data(), bytesLeftToRead);
}

void WindowsFile::read(uint8 *readTo, const uint32 &bytesToRead) const
{
    if (!getFileHandleRaw() || BIT_NOT_SET(fileFlags, EFileFlags::Read))
    {
        return;
    }

    const dword readBufferSize = 10 * 1024 * 1024; // 10MB

    uint64 filePointerCache = filePointer();
    uint64 availableSizeCanRead = (fileSize() - filePointerCache);
    dword bytesLeftToRead
        = (dword)(availableSizeCanRead > bytesToRead ? bytesToRead : availableSizeCanRead);

    uint64 filePointerOffset = 0;
    dword bytesLastRead = 0;
    while (bytesLeftToRead > 0)
    {
        ::ReadFile(getFileHandleRaw(), readTo + filePointerOffset,
            (bytesLeftToRead > readBufferSize) ? readBufferSize : bytesLeftToRead, &bytesLastRead,
            nullptr);

        bytesLeftToRead -= bytesLastRead;
        filePointerOffset += bytesLastRead;
        seek(filePointerOffset + filePointerCache);
    }

    seek(filePointerCache);
}

void WindowsFile::write(const ArrayView<const uint8> &writeBytes) const
{
    if (!getFileHandleRaw() || BIT_NOT_SET(fileFlags, EFileFlags::Write))
    {
        return;
    }

    std::vector<uint8>::size_type sizeLeft = writeBytes.size();
    const uint8 *pData = writeBytes.data();

    const uint32 writeBufferSize = 5 * 1024 * 1024; // 5MB
    dword bytesWritten = 0;
    uint64 writeFrom = 0;

    uint32 writeSize = writeBufferSize;
    while (sizeLeft > 0)
    {
        writeSize = writeBufferSize > sizeLeft ? (uint32)sizeLeft : writeBufferSize;
        ::WriteFile(getFileHandleRaw(), pData + writeFrom, writeSize, &bytesWritten, nullptr);

        writeFrom += bytesWritten;
        sizeLeft -= bytesWritten;
        bytesWritten = 0;
    }
}

bool WindowsFile::deleteFile()
{
    if (getFileHandle() && getFileHandleRaw())
    {
        closeFile();
    }
    return ::DeleteFile(getFullPath().getChar());
}

bool WindowsFile::renameFile(String newName)
{
    WindowsFile newFile{ PathFunctions::combinePath(getHostDirectory(), newName) };

    if (newFile.exists())
    {
        return false;
    }

    bool reopenFile = false;
    if (getFileHandle() && getFileHandleRaw())
    {
        closeFile();
        reopenFile = true;
    }
    if (FileSystemFunctions::moveFile(this, &newFile))
    {
        setPath(newFile.getFullPath());
        if (reopenFile)
        {
            openFile();
        }
        return true;
    }
    return false;
}

bool WindowsFile::createDirectory() const
{
    WindowsFile hostDirectoryFile = WindowsFile(getHostDirectory());
    if (!hostDirectoryFile.exists())
    {
        hostDirectoryFile.createDirectory();
    }

    return ::CreateDirectory(getFullPath().getChar(), nullptr);
}

TickRep WindowsFile::lastWriteTimeStamp() const
{
    UInt64 timeStamp;
    timeStamp.quadPart = 0;
    if (getFileHandleRaw())
    {
        FILETIME writeTime;
        ::GetFileTime(getFileHandleRaw(), nullptr, nullptr, &writeTime);
        timeStamp.lowPart = writeTime.dwLowDateTime;
        timeStamp.highPart = writeTime.dwHighDateTime;
    }
    else
    {
        WIN32_FIND_DATA data;
        HANDLE fHandle = ::FindFirstFile(getFullPath().getChar(), &data);
        if (fHandle != INVALID_HANDLE_VALUE)
        {
            timeStamp.lowPart = data.ftLastWriteTime.dwLowDateTime;
            timeStamp.highPart = data.ftLastWriteTime.dwHighDateTime;
            ::FindClose(fHandle);
        }
    }

    // It is okay to convert to signed as time stamp would not cross signed max
    return Time::fromPlatformTime(int64(timeStamp.quadPart));
}

bool WindowsFile::setLastWriteTimeStamp(TickRep timeTick) const
{
    if (!getFileHandleRaw() || BIT_NOT_SET(fileFlags, EFileFlags::Write))
    {
        return false;
    }

    UInt64 timeStamp;
    timeStamp.quadPart = uint64(Time::toPlatformTime(timeTick));

    FILETIME writeTime;
    writeTime.dwLowDateTime = timeStamp.lowPart;
    writeTime.dwHighDateTime = timeStamp.highPart;

    ::SetFileTime(getFileHandleRaw(), nullptr, nullptr, &writeTime);
    return true;
}

TickRep WindowsFile::createTimeStamp() const
{
    UInt64 timeStamp;
    timeStamp.quadPart = 0;
    if (getFileHandleRaw())
    {
        FILETIME createdTime;
        ::GetFileTime(getFileHandleRaw(), &createdTime, nullptr, nullptr);
        timeStamp.lowPart = createdTime.dwLowDateTime;
        timeStamp.highPart = createdTime.dwHighDateTime;
    }
    else
    {
        WIN32_FIND_DATA data;
        HANDLE fHandle = ::FindFirstFile(getFullPath().getChar(), &data);
        if (fHandle != INVALID_HANDLE_VALUE)
        {
            timeStamp.lowPart = data.ftCreationTime.dwLowDateTime;
            timeStamp.highPart = data.ftCreationTime.dwHighDateTime;
            ::FindClose(fHandle);
        }
    }
    // It is okay to convert to signed as time stamp would not cross signed max
    return Time::fromPlatformTime(int64(timeStamp.quadPart));
}

GenericFileHandle *WindowsFile::openOrCreateImpl()
{
    WindowsFile hostDirectoryFile{ getHostDirectory() };
    if (!hostDirectoryFile.exists())
    {
        hostDirectoryFile.createDirectory();
    }
    bool bExists = exists();
    // If exists CreateNew is the only flag that fails
    if (bExists)
    {
        if (BIT_SET(fileFlags, EFileFlags::CreateNew))
        {
            setCreationAction(EFileFlags::OpenExisting);
            LOG_WARN("WindowsFile", "EFileFlags::CreateNew is set on existing file %s", getFullPath());
        }
    }
    else // In this case OpenExisting and ClearExisting fails check and replace them
    {
        if (ANY_BIT_SET(fileFlags, (EFileFlags::OpenExisting | EFileFlags::ClearExisting)))
        {
            setCreationAction(EFileFlags::CreateNew);
            LOG_WARN("WindowsFile",
                "EFileFlags::OpenExisting | EFileFlags::ClearExisting is set on non-existing "
                "file %s",
                getFullPath());
        }
    }

    return openImpl();
}

GenericFileHandle *WindowsFile::openImpl() const
{
    WindowsFileHandle *fHandle
        = new WindowsFileHandle(fileFlags, sharingMode, attributes, advancedFlags);

    if (!fHandle->openFile(getFullPath()))
    {
        delete fHandle;
        fHandle = nullptr;
    }

    return fHandle;
}

bool WindowsFile::closeImpl() const
{
    WindowsFileHandle *fHandle = static_cast<WindowsFileHandle *>(getFileHandle());
    flush();
    return fHandle->closeFile();
}

bool WindowsFile::dirDelete() const { return RemoveDirectory(getFullPath().getChar()); }

bool WindowsFile::dirClearAndDelete() const
{
    std::vector<String> filesPath
        = FileSystemFunctions::listAllFiles(isDirectory() ? getFullPath() : getHostDirectory(), true);
    for (const String &filePath : filesPath)
    {
        if (!::DeleteFile(filePath.getChar()))
        {
            return false;
        }
    }
    return dirDelete();
}
