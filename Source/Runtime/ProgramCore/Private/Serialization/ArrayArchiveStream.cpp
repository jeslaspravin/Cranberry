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

void ArrayArchiveStream::read(void* toPtr, SizeT len)
{
    fatalAssert(cursor + len <= buffer.size(), "Cannot read past buffer size %ull", buffer.size());
    CBEMemory::memCopy(toPtr, buffer.data() + cursor, len);
    cursor += len;
}

void ArrayArchiveStream::write(const void* ptr, SizeT len)
{
    SizeT currCursor = cursor;
    moveForward(len);
    CBEMemory::memCopy(buffer.data() + currCursor, ptr, len);
}

void ArrayArchiveStream::moveForward(SizeT count)
{
    cursor += count;
    if (buffer.size() <= cursor)
    {
        buffer.resize(cursor + 1);
    }
}

void ArrayArchiveStream::moveBackward(SizeT count)
{
    cursor = Math::max((int64)cursor - (int64)count, 0);
}

bool ArrayArchiveStream::allocate(SizeT count)
{
    buffer.resize(buffer.size() + count);
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

bool ArrayArchiveStream::isAvailable() const
{
    return true;
}
