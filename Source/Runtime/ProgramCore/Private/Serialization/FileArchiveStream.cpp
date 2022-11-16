/*!
 * \file FileArchiveStream.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Serialization/FileArchiveStream.h"
#include "Types/Platform/LFS/PlatformLFS.h"

FileArchiveStream::FileArchiveStream(const String &filePath, bool bReading)
    : file(new PlatformFile(filePath))
    , fileCursor(0)
    , bIsReadOnly(bReading)
{
    file->setFileFlags(EFileFlags::Read | (bReading ? 0 : EFileFlags::Write));
    file->setCreationAction(bReading ? EFileFlags::OpenExisting : EFileFlags::CreateAlways);
    file->setSharingMode(EFileSharing::ReadOnly);
    if (file->isFile())
    {
        bIsOpened = file->openOrCreate();
    }
}

FileArchiveStream::~FileArchiveStream()
{
    file->closeFile();
    delete file;
    file = nullptr;
}

void FileArchiveStream::read(void *toPtr, SizeT byteLen)
{
    if (hasMoreData(byteLen))
    {
        file->read(reinterpret_cast<uint8 *>(toPtr), uint32(byteLen));
        file->offsetCursor((int64)byteLen);
        fileCursor += byteLen;
    }
    else
    {
        byteLen = Math::min(byteLen, file->fileSize() - fileCursor);
        file->offsetCursor((int64)byteLen);
        fileCursor += byteLen;
    }
}

void FileArchiveStream::write(const void *ptr, SizeT byteLen)
{
    if (bIsReadOnly)
        return;

    file->write({ reinterpret_cast<const uint8 *>(ptr), byteLen });
    fileCursor += byteLen;
}

void FileArchiveStream::moveForward(SizeT byteCount)
{
    if (byteCount == 0)
        return;

    fileCursor += byteCount;
    if (file->fileSize() <= fileCursor)
    {
        if (!bIsReadOnly)
            file->setFileSize(fileCursor);
        file->seekEnd();
        fileCursor = file->fileSize();
    }
    else
    {
        file->offsetCursor((int64)byteCount);
    }
}

void FileArchiveStream::moveBackward(SizeT byteCount)
{
    if (byteCount == 0)
        return;

    fileCursor = (SizeT)Math::max(0, (int64)(fileCursor) - (int64)(byteCount));
    file->seek(fileCursor);
}

bool FileArchiveStream::allocate(SizeT byteCount)
{
    if (bIsReadOnly)
        return false;

    return file->setFileSize(file->fileSize() + byteCount);
}

uint8 FileArchiveStream::readForwardAt(SizeT idx) const
{
    if (file->fileSize() <= fileCursor + idx)
        return 0;

    file->offsetCursor((int64)idx);
    uint8 outVal;
    file->read(&outVal, 1);
    file->offsetCursor(-(int64)idx);
    return outVal;
}

uint8 FileArchiveStream::readBackwardAt(SizeT idx) const
{
    if (fileCursor < idx)
        return 0;

    file->offsetCursor(-(int64)idx);
    uint8 outVal;
    file->read(&outVal, 1);
    file->offsetCursor((int64)idx);
    return outVal;
}

uint64 FileArchiveStream::cursorPos() const { return fileCursor; }

bool FileArchiveStream::isAvailable() const { return bIsOpened; }

bool FileArchiveStream::hasMoreData(SizeT requiredByteCount) const
{
    return isAvailable() && (fileCursor + requiredByteCount) <= file->fileSize();
}
