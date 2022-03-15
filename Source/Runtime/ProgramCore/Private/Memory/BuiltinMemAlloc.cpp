/*!
 * \file BuiltinMemAlloc.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Memory/BuiltinMemAlloc.h"
#include "Memory/Memory.h"
#include "Math/Math.h"

FORCE_INLINE SizeT CBBuiltinMemAlloc::calcExtraWidth(uint32 alignment) const
{
    if (alignment != DEFAULT_ALIGNMENT)
    {
        return Math::alignByUnsafe(sizeof(AllocHeader), Math::max(alignof(AllocHeader), alignment));
    }
    return Math::alignByUnsafe(sizeof(AllocHeader), alignof(AllocHeader));
}

FORCE_INLINE void* CBBuiltinMemAlloc::writeAllocMeta(void* allocatedPtr, SizeT size, uint32 alignment) const
{
    void* outPtr = ((uint8*)allocatedPtr) + calcExtraWidth(alignment);
    AllocHeader& allocHeader = *(((AllocHeader*)outPtr) - 1);
    allocHeader.size = size;
    allocHeader.alignment = alignment;
    return outPtr;
}

FORCE_INLINE void* CBBuiltinMemAlloc::getAllocationInfo(void* ptr, SizeT& outSize, uint32& outAlignment) const
{
    AllocHeader& allocHeader = *(((AllocHeader*)ptr) - 1);
    outSize = allocHeader.size;
    outAlignment = allocHeader.alignment;
    return ((uint8*)ptr) - calcExtraWidth(allocHeader.alignment);
}

void* CBBuiltinMemAlloc::tryMalloc(SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/)
{
    debugAssert(Math::isPowOf2(alignment));
    if (size == 0)
    {
        return nullptr;
    }

    if (void* ptr = CBMemory::builtinMalloc(size + calcExtraWidth(alignment)))
    {
        return writeAllocMeta(ptr, size, alignment);
    }
    return nullptr;
}

void* CBBuiltinMemAlloc::memAlloc(SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/)
{
    void* ptr = tryMalloc(size, alignment);
    fatalAssert(size == 0 || ptr, "Allocation failed!");
    return ptr;
}

void* CBBuiltinMemAlloc::tryRealloc(void* currentPtr, SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/)
{
    debugAssert(Math::isPowOf2(alignment));

    AllocHeader allocInfo;
    void* actualPtr = getAllocationInfo(currentPtr, allocInfo.size, allocInfo.alignment);

    if (size == 0)
    {
        CBMemory::builtinFree(actualPtr);
        return nullptr;
    }
    if (void* ptr = CBMemory::builtinRealloc(actualPtr, size + calcExtraWidth(alignment)))
    {
        return writeAllocMeta(ptr, size, alignment);
    }
    return nullptr;
}

void* CBBuiltinMemAlloc::memRealloc(void* currentPtr, SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/)
{
    void* ptr = tryRealloc(currentPtr, size, alignment);
    fatalAssert(size == 0 || ptr, "Reallocation failed!");
    return ptr;
}

void CBBuiltinMemAlloc::memFree(void* ptr)
{
    if (ptr == nullptr)
    {
        return;
    }

    AllocHeader allocInfo;
    void* actualPtr = getAllocationInfo(ptr, allocInfo.size, allocInfo.alignment);
    fatalAssert(allocInfo.size > 0 && allocInfo.alignment > 0, "Possible memFree invokation on freed object");

    AllocHeader& allocHeader = *(((AllocHeader*)ptr) - 1);
    // 0 it so that if there is any immediate free calls again with can detect it
    allocHeader.alignment = 0;
    allocHeader.size = 0;
    CBMemory::builtinFree(actualPtr);
}

SizeT CBBuiltinMemAlloc::getAllocationSize(void* ptr) const
{
    if (ptr == nullptr)
    {
        return 0;
    }

    AllocHeader allocInfo;
    getAllocationInfo(ptr, allocInfo.size, allocInfo.alignment);
    return allocInfo.size;
}
