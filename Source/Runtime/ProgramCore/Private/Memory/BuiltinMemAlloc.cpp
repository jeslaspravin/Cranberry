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
#include "Math/Math.h"
#include "Memory/Memory.h"

FORCE_INLINE SizeT CBEBuiltinMemAlloc::calcHeaderPadding(uint32 alignment) const noexcept
{
#ifndef PLATFORM_ALIGNED_MALLOC
    // Since we do manual alignment
    return Math::alignByUnsafe(sizeof(AllocHeader), alignof(AllocHeader));
#else  // PLATFORM_ALIGNED_MALLOC
    // In case of aligned alloc we have to make header expand to an entire alignment at least so that
    // actualPtr will be aligned as well
    return Math::alignByUnsafe(sizeof(AllocHeader), (SizeT)(alignment));
#endif // PLATFORM_ALIGNED_MALLOC
}

FORCE_INLINE SizeT CBEBuiltinMemAlloc::calcExtraWidth(uint32 alignment) const noexcept
{
#ifndef PLATFORM_ALIGNED_MALLOC
    // + number of extra byte for worst alignment
    return calcHeaderPadding(alignment) + (alignment - 1);
#else  // PLATFORM_ALIGNED_MALLOC
    return calcHeaderPadding(alignment);
#endif // PLATFORM_ALIGNED_MALLOC
}

FORCE_INLINE void *CBEBuiltinMemAlloc::writeAllocMeta(void *allocatedPtr, SizeT size, uint32 alignment) const noexcept
{
    void *outPtr = ((uint8 *)allocatedPtr) + calcHeaderPadding(alignment);
#ifndef PLATFORM_ALIGNED_MALLOC
    void *alignedPtr = (void *)(Math::alignByUnsafe((UPtrInt)(outPtr), alignment));
    // Header will also be properly aligned for efficient read
    AllocHeader &allocHeader = *(((AllocHeader *)alignedPtr) - 1);
    allocHeader.offset = (uint32)((UPtrInt)(alignedPtr) - (UPtrInt)(allocatedPtr));
    outPtr = alignedPtr;
#else  // PLATFORM_ALIGNED_MALLOC
    AllocHeader &allocHeader = *(((AllocHeader *)outPtr) - 1);
#endif // PLATFORM_ALIGNED_MALLOC
    allocHeader.size = size;
    allocHeader.alignment = alignment;
    return outPtr;
}

FORCE_INLINE void *CBEBuiltinMemAlloc::getAllocationInfo(void *ptr, SizeT &outSize, uint32 &outAlignment) const noexcept
{
    AllocHeader &allocHeader = *(((AllocHeader *)ptr) - 1);
    outSize = allocHeader.size;
    outAlignment = allocHeader.alignment;

#ifndef PLATFORM_ALIGNED_MALLOC
    return ((uint8 *)ptr) - allocHeader.offset;
#else  // PLATFORM_ALIGNED_MALLOC
    return ((uint8 *)ptr) - calcHeaderPadding(allocHeader.alignment);
#endif // PLATFORM_ALIGNED_MALLOC
}

void *CBEBuiltinMemAlloc::tryMalloc(SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/) noexcept
{
    debugAssert(Math::isPowOf2(alignment));
    if (size == 0)
    {
        return nullptr;
    }

    alignment = uint32(Math::max(alignof(AllocHeader), alignBy(size, alignment)));

#ifndef PLATFORM_ALIGNED_MALLOC
    if (void *ptr = CBEMemory::builtinMalloc(size + calcExtraWidth(alignment)))
#else  // PLATFORM_ALIGNED_MALLOC
    if (void *ptr = PLATFORM_ALIGNED_MALLOC(size + calcExtraWidth(alignment), alignment))
#endif // PLATFORM_ALIGNED_MALLOC
    {
        return writeAllocMeta(ptr, size, alignment);
    }
    return nullptr;
}

void *CBEBuiltinMemAlloc::memAlloc(SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/) noexcept
{
    void *ptr = tryMalloc(size, alignment);
    fatalAssertf(size == 0 || ptr, "Allocation failed!");
    return ptr;
}

void *CBEBuiltinMemAlloc::tryRealloc(void *currentPtr, SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/) noexcept
{
    debugAssert(Math::isPowOf2(alignment));

    AllocHeader allocInfo;
    void *actualPtr = getAllocationInfo(currentPtr, allocInfo.size, allocInfo.alignment);

    if (size == 0)
    {
        CBEMemory::builtinFree(actualPtr);
        return nullptr;
    }

    alignment = uint32(Math::max(alignof(AllocHeader), alignBy(size, alignment)));

#ifndef PLATFORM_ALIGNED_MALLOC
    if (void *ptr = CBEMemory::builtinRealloc(actualPtr, size + calcExtraWidth(alignment)))
#else  // PLATFORM_ALIGNED_MALLOC
    if (void *ptr = PLATFORM_ALIGNED_REALLOC(actualPtr, size + calcExtraWidth(alignment), alignment))
#endif // PLATFORM_ALIGNED_MALLOC
    {
        return writeAllocMeta(ptr, size, alignment);
    }
    return nullptr;
}

void *CBEBuiltinMemAlloc::memRealloc(void *currentPtr, SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/) noexcept
{
    void *ptr = tryRealloc(currentPtr, size, alignment);
    fatalAssertf(size == 0 || ptr, "Reallocation failed!");
    return ptr;
}

void CBEBuiltinMemAlloc::memFree(void *ptr) noexcept
{
    if (ptr == nullptr)
    {
        return;
    }

    AllocHeader allocInfo;
    void *actualPtr = getAllocationInfo(ptr, allocInfo.size, allocInfo.alignment);
    fatalAssertf(allocInfo.size > 0 && allocInfo.alignment > 0, "Possible memFree invokation on freed object");

    AllocHeader &allocHeader = *(((AllocHeader *)ptr) - 1);
    // 0 it so that if there is any immediate free calls again with can detect it
    allocHeader.alignment = 0;
    allocHeader.size = 0;

#ifndef PLATFORM_ALIGNED_MALLOC
    allocHeader.offset = 0;
    CBEMemory::builtinFree(actualPtr);
#else  // PLATFORM_ALIGNED_MALLOC
    PLATFORM_ALIGNED_FREE(actualPtr);
#endif // PLATFORM_ALIGNED_MALLOC
}

SizeT CBEBuiltinMemAlloc::getAllocationSize(void *ptr) const noexcept
{
    if (ptr == nullptr)
    {
        return 0;
    }

    AllocHeader allocInfo;
    getAllocationInfo(ptr, allocInfo.size, allocInfo.alignment);
    return allocInfo.size;
}
