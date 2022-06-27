/*!
 * \file ArrayArchiveStream.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Serialization/ArrayArchiveStream.h"
#include "Math/Math.h"
#include "Memory/Memory.h"

void ArrayArchiveStream::read(void *toPtr, SizeT byteLen)
{
    if (hasMoreData(byteLen))
    {
        CBEMemory::memCopy(toPtr, buffer.data() + cursor, byteLen);
        cursor += byteLen;
    }
    else
    {
        cursor += byteLen;
        cursor = Math::min(cursor, buffer.size());
    }
}

void ArrayArchiveStream::write(const void *ptr, SizeT byteLen)
{
    SizeT currCursor = cursor;
    moveForward(byteLen);
    CBEMemory::memCopy(buffer.data() + currCursor, ptr, byteLen);
}

void ArrayArchiveStream::moveForward(SizeT byteCount)
{
    cursor += byteCount;
    if (buffer.size() <= cursor)
    {
        buffer.resize(cursor + 1);
    }
}

void ArrayArchiveStream::moveBackward(SizeT byteCount) { cursor = Math::max((int64)cursor - (int64)byteCount, 0); }

bool ArrayArchiveStream::allocate(SizeT byteCount)
{
    buffer.resize(buffer.size() + byteCount);
    return true;
}

uint8 ArrayArchiveStream::readForwardAt(SizeT idx) const
{
    if (buffer.size() <= (cursor + idx))
        return 0;

    return buffer[cursor + idx];
}

uint8 ArrayArchiveStream::readBackwardAt(SizeT idx) const
{
    if (cursor < idx)
        return 0;

    return buffer[cursor - idx];
}

bool ArrayArchiveStream::isAvailable() const { return true; }

bool ArrayArchiveStream::hasMoreData(SizeT requiredByteCount) const { return isAvailable() && (cursor + requiredByteCount) <= buffer.size(); }

uint64 ArrayArchiveStream::cursorPos() const { return cursor; }
