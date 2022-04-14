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
        file->openOrCreate();
    }
}

FileArchiveStream::~FileArchiveStream()
{
    file->closeFile();
    delete file;
    file = nullptr;
}

void FileArchiveStream::read(void *toPtr, SizeT len)
{
    fatalAssert(
        file->fileSize() >= fileCursor + len, "Cannot read past file size %ull", file->fileSize());

    file->read(reinterpret_cast<uint8 *>(toPtr), len);
    file->offsetCursor((int64)len);
    fileCursor += len;
}

void FileArchiveStream::write(const void *ptr, SizeT len)
{
    if (bIsReadOnly)
        return;

    file->write({ reinterpret_cast<const uint8 *>(ptr), len });
    fileCursor += len;
}

void FileArchiveStream::moveForward(SizeT count)
{
    if (count == 0)
        return;

    fileCursor += count;
    if (file->fileSize() <= fileCursor)
    {
        if (!bIsReadOnly)
            file->setFileSize(fileCursor + 1);
        file->seekEnd();
        fileCursor = file->fileSize();
    }
    else
    {
        file->offsetCursor((int64)count);
    }
}

void FileArchiveStream::moveBackward(SizeT count)
{
    if (count == 0)
        return;

    fileCursor = (SizeT)Math::max(0, (int64)(fileCursor) - (int64)(count));
    file->seek(fileCursor);
}

bool FileArchiveStream::allocate(SizeT count)
{
    if (bIsReadOnly)
        return false;

    return file->setFileSize(file->fileSize() + count);
}

uint8 FileArchiveStream::readForwardAt(SizeT idx) const
{
    if (file->fileSize() <= fileCursor + idx)
        return 0;

    file->offsetCursor(idx);
    uint8 outVal;
    file->read(&outVal, 1);
    file->offsetCursor(-idx);
    return outVal;
}

uint8 FileArchiveStream::readBackwardAt(SizeT idx) const
{
    if (fileCursor < idx)
        return 0;

    file->offsetCursor(-idx);
    uint8 outVal;
    file->read(&outVal, 1);
    file->offsetCursor(idx);
    return outVal;
}

bool FileArchiveStream::isAvailable() const { return file->isFile() && file->exists(); }
