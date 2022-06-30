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
#include "Logger/Logger.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"
#include "WindowsCommonHeaders.h"

#define FLAG_CHECK_ASSIGN(OutFlags, InFlags, CheckAgainst, Result) OutFlags |= (InFlags & CheckAgainst) > 0 ? Result : 0
PlatformHandle openWindowsFile(const String &filePath, uint8 fileFlags, uint8 fileSharing, uint32 fileExtraFlags, uint64 rawFileFlags)
{
    uint32 desiredAccess = 0;
    uint32 shareMode = 0;
    uint32 creationAction = OPEN_ALWAYS;
    uint32 attributsAndFlags = 0;

    {
        if ((FileFlags::ACCESS_FLAGS & fileFlags) == FileFlags::ACCESS_FLAGS)
        {
            desiredAccess |= GENERIC_ALL;
        }
        else
        {
            FLAG_CHECK_ASSIGN(desiredAccess, fileFlags, EFileFlags::Read, GENERIC_READ);
            FLAG_CHECK_ASSIGN(desiredAccess, fileFlags, EFileFlags::Write, GENERIC_WRITE);
            FLAG_CHECK_ASSIGN(desiredAccess, fileFlags, EFileFlags::Execute, GENERIC_EXECUTE);
        }
    }

    {
        FLAG_CHECK_ASSIGN(shareMode, fileSharing, EFileSharing::DeleteOnly, FILE_SHARE_DELETE);
        FLAG_CHECK_ASSIGN(shareMode, fileSharing, EFileSharing::ReadOnly, FILE_SHARE_READ);
        FLAG_CHECK_ASSIGN(shareMode, fileSharing, EFileSharing::WriteOnly, FILE_SHARE_WRITE);
    }

    {
        uint8 actionsFlags = fileFlags & FileFlags::OPEN_ACTION_FLAGS;

        if (ONE_BIT_SET(actionsFlags)) // If only one bit is set
        {
            uint8 currentFlag = EFileFlags::CreateNew;
            uint32 count = 1;
            while (FileFlags::OPEN_ACTION_FLAGS & currentFlag)
            {
                if (actionsFlags & currentFlag)
                {
                    creationAction = count; // Since the order is same as int value in windows API
                    break;
                }
                currentFlag <<= 1;
                count++;
            }
        }
    }

    {
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Normal, FILE_ATTRIBUTE_NORMAL);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Archive, FILE_ATTRIBUTE_ARCHIVE);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Encrypted, FILE_ATTRIBUTE_ENCRYPTED);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Hidden, FILE_ATTRIBUTE_HIDDEN);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::ReadOnly, FILE_ATTRIBUTE_READONLY);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::System, FILE_ATTRIBUTE_SYSTEM);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Temporary, FILE_ATTRIBUTE_TEMPORARY);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Backup, FILE_FLAG_BACKUP_SEMANTICS);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::TemporaryDelete, FILE_FLAG_DELETE_ON_CLOSE);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::NoBuffering, FILE_FLAG_NO_BUFFERING);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::OpenRemoteOnly, FILE_FLAG_OPEN_NO_RECALL);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::AsyncOverlapped, FILE_FLAG_OVERLAPPED);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::Posix, FILE_FLAG_POSIX_SEMANTICS);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::RandomAccess, FILE_FLAG_RANDOM_ACCESS);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::SequentialAccess, FILE_FLAG_SEQUENTIAL_SCAN);
        FLAG_CHECK_ASSIGN(attributsAndFlags, fileExtraFlags, EFileAdditionalFlags::WriteDirectDisk, FILE_FLAG_WRITE_THROUGH);

        attributsAndFlags |= rawFileFlags;
    }

    HANDLE fileHandle = ::CreateFile(filePath.getChar(), desiredAccess, shareMode, nullptr, creationAction, attributsAndFlags, nullptr);

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR("WindowsFileHandle", "File handle creation/opening failed for %s", filePath.getChar());
        return nullptr;
    }
    return fileHandle;
}
#undef FLAG_CHECK_ASSIGN

bool closeWindowsFile(PlatformHandle fileHandle)
{
    bool success = false;
    if (!fileHandle)
    {
        return success;
    }

    success = ::CloseHandle((HANDLE)fileHandle) != 0;

    if (success)
    {
        fileHandle = nullptr;
    }
    return success;
}

//////////////////////////////////////////////////////////////////////////
/// WindowsFile implementation
//////////////////////////////////////////////////////////////////////////

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
        LOG_WARN("WindowsFile", "File %s is not closed, Please close it before destroying", getFullPath().getChar());
        closeFile();
    }
}

void WindowsFile::flush() const
{
    if (getFileHandle())
    {
        ::FlushFileBuffers(getFileHandle());
    }
}

bool WindowsFile::exists() const
{
    if (getFileName().compare(TCHAR(".")) == 0 || getFileName().compare(TCHAR("..")) == 0) // not valid files or folders
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

    if (getFileHandle())
    {
        fSize.lowPart = ::GetFileSize(getFileHandle(), &fSize.highPart);
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
    if (getFileHandle())
    {
        largePointer.LowPart = ::SetFilePointer(getFileHandle(), largePointer.LowPart, &largePointer.HighPart, FILE_CURRENT);
        fPointer = largePointer.LowPart == INVALID_SET_FILE_POINTER ? 0u : (uint64)(largePointer.QuadPart);
    }
    return fPointer;
}

void WindowsFile::seekEnd() const
{
    if (getFileHandle())
    {
        ::SetFilePointer(getFileHandle(), 0, nullptr, FILE_END);
        debugAssert(filePointer() == fileSize());
    }
}

void WindowsFile::seekBegin() const
{
    if (getFileHandle())
    {
        ::SetFilePointer(getFileHandle(), 0, nullptr, FILE_BEGIN);
        debugAssert(filePointer() == 0);
    }
}

void WindowsFile::seek(const int64 &pointer) const
{
    if (getFileHandle())
    {
        LARGE_INTEGER newPointer;
        newPointer.QuadPart = pointer;

        ::SetFilePointer(getFileHandle(), newPointer.LowPart, &newPointer.HighPart, FILE_BEGIN);
    }
}

void WindowsFile::offsetCursor(const int64 &offset) const
{
    if (getFileHandle())
    {
        LARGE_INTEGER newOffset;
        newOffset.QuadPart = offset;

        ::SetFilePointer(getFileHandle(), newOffset.LowPart, &newOffset.HighPart, FILE_CURRENT);
    }
}

bool WindowsFile::setFileSize(const int64 &newSize) const
{
    if (!getFileHandle() || BIT_NOT_SET(fileFlags, EFileFlags::Write))
    {
        return false;
    }

    uint64 filePointerCache = filePointer();
    seek(newSize);

    bool bResized = false;
    if (::SetEndOfFile((HANDLE)getFileHandle()) == TRUE)
    {
        filePointerCache = Math::min(newSize, filePointerCache);
        bResized = true;
    }
    seek(filePointerCache);
    return bResized;
}

void WindowsFile::read(std::vector<uint8> &readTo, const uint32 &bytesToRead /*= (~0u)*/) const
{
    if (!getFileHandle() || BIT_NOT_SET(fileFlags, EFileFlags::Read))
    {
        return;
    }

    const dword readBufferSize = 10 * 1024 * 1024; // 10MB

    uint64 filePointerCache = filePointer();
    uint64 availableSizeCanRead = (fileSize() - filePointerCache);
    dword bytesLeftToRead = (dword)(availableSizeCanRead > bytesToRead ? bytesToRead : availableSizeCanRead);
    readTo.clear();
    readTo.resize(bytesLeftToRead, 0);

    read(readTo.data(), bytesLeftToRead);
}

void WindowsFile::read(uint8 *readTo, const uint32 &bytesToRead) const
{
    if (!getFileHandle() || BIT_NOT_SET(fileFlags, EFileFlags::Read))
    {
        return;
    }

    const dword readBufferSize = 10 * 1024 * 1024; // 10MB

    uint64 filePointerCache = filePointer();
    uint64 availableSizeCanRead = (fileSize() - filePointerCache);
    dword bytesLeftToRead = (dword)(availableSizeCanRead > bytesToRead ? bytesToRead : availableSizeCanRead);

    uint64 filePointerOffset = 0;
    dword bytesLastRead = 0;
    while (bytesLeftToRead > 0)
    {
        ::ReadFile(
            getFileHandle(), readTo + filePointerOffset, (bytesLeftToRead > readBufferSize) ? readBufferSize : bytesLeftToRead,
            &bytesLastRead, nullptr
        );

        bytesLeftToRead -= bytesLastRead;
        filePointerOffset += bytesLastRead;
        seek(filePointerOffset + filePointerCache);
    }

    seek(filePointerCache);
}

void WindowsFile::write(const ArrayView<const uint8> &writeBytes) const
{
    if (!getFileHandle() || BIT_NOT_SET(fileFlags, EFileFlags::Write))
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
        ::WriteFile(getFileHandle(), pData + writeFrom, writeSize, &bytesWritten, nullptr);

        writeFrom += bytesWritten;
        sizeLeft -= bytesWritten;
        bytesWritten = 0;
    }
}

bool WindowsFile::deleteFile()
{
    if (getFileHandle())
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
    if (getFileHandle())
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
    if (getFileHandle())
    {
        FILETIME writeTime;
        ::GetFileTime(getFileHandle(), nullptr, nullptr, &writeTime);
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
    if (!getFileHandle() || BIT_NOT_SET(fileFlags, EFileFlags::Write))
    {
        return false;
    }

    UInt64 timeStamp;
    timeStamp.quadPart = uint64(Time::toPlatformTime(timeTick));

    FILETIME writeTime;
    writeTime.dwLowDateTime = timeStamp.lowPart;
    writeTime.dwHighDateTime = timeStamp.highPart;

    ::SetFileTime(getFileHandle(), nullptr, nullptr, &writeTime);
    return true;
}

TickRep WindowsFile::createTimeStamp() const
{
    UInt64 timeStamp;
    timeStamp.quadPart = 0;
    if (getFileHandle())
    {
        FILETIME createdTime;
        ::GetFileTime(getFileHandle(), &createdTime, nullptr, nullptr);
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

PlatformHandle WindowsFile::openOrCreateImpl()
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
            LOG_WARN(
                "WindowsFile",
                "EFileFlags::OpenExisting | EFileFlags::ClearExisting is set on non-existing "
                "file %s",
                getFullPath()
            );
        }
    }

    return openImpl();
}

PlatformHandle WindowsFile::openImpl() const
{
    return openWindowsFile(getFullPath(), fileFlags, sharingMode, attributes, advancedFlags);
}

bool WindowsFile::closeImpl() const
{
    flush();
    return closeWindowsFile(getFileHandle());
}

bool WindowsFile::dirDelete() const { return RemoveDirectory(getFullPath().getChar()); }

bool WindowsFile::dirClearAndDelete() const
{
    std::vector<String> filesPath = FileSystemFunctions::listAllFiles(isDirectory() ? getFullPath() : getHostDirectory(), true);
    for (const String &filePath : filesPath)
    {
        if (!::DeleteFile(filePath.getChar()))
        {
            return false;
        }
    }
    return dirDelete();
}
