/*!
 * \file MimallocMemAlloc.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#if USE_MIMALLOC

#include "MimallocMemAlloc.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#include <mimalloc.h>

const CBEProfilerChar *MimallocMemAlloc::ALLOC_NAME = CBE_PROFILER_CHAR("Mimalloc");

void *MimallocMemAlloc::tryMalloc(SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/) noexcept
{
    debugAssert(Math::isPowOf2(alignment));
    if (size == 0)
    {
        return nullptr;
    }
    alignment = adjustAlignment(size, alignment);
    void *ptr = mi_malloc_aligned(size, alignment);

    CBE_PROFILER_ALLOC_N(ptr, size, ALLOC_NAME);
    return ptr;
}

void *MimallocMemAlloc::memAlloc(SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/) noexcept
{
    void *ptr = tryMalloc(size, alignment);
    fatalAssertf(size == 0 || ptr, "Allocation failed!");
    return ptr;
}

void *MimallocMemAlloc::tryRealloc(void *currentPtr, SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/) noexcept
{
    debugAssert(Math::isPowOf2(alignment));
    if (size == 0)
    {
        CBE_PROFILER_FREE_N(currentPtr, ALLOC_NAME);
        mi_free(currentPtr);
        return nullptr;
    }
    void *outPtr = nullptr;
    // We try to reallocate with same old alignment if default alignment is send it
    if (alignment == DEFAULT_ALIGNMENT)
    {
        outPtr = mi_realloc(currentPtr, size);
    }
    else
    {
        alignment = adjustAlignment(size, alignment);
        outPtr = mi_realloc_aligned(currentPtr, size, alignment);
    }

    if (outPtr != currentPtr)
    {
        CBE_PROFILER_FREE_N(currentPtr, ALLOC_NAME);
        CBE_PROFILER_ALLOC_N(outPtr, size, ALLOC_NAME);
    }
    return outPtr;
}

void *MimallocMemAlloc::memRealloc(void *currentPtr, SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/) noexcept
{
    void *ptr = tryRealloc(currentPtr, size, alignment);
    fatalAssertf(size == 0 || ptr, "Reallocation failed!");
    return ptr;
}

void MimallocMemAlloc::memFree(void *ptr) noexcept
{
    if (ptr == nullptr)
    {
        return;
    }
    CBE_PROFILER_FREE_N(ptr, ALLOC_NAME);
    mi_free(ptr);
}

SizeT MimallocMemAlloc::getAllocationSize(void *ptr) const noexcept
{
    if (ptr == nullptr)
    {
        return 0;
    }
    return mi_malloc_size(ptr);
}

#endif // USE_MIMALLOC