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

void *MimallocMemAlloc::tryMalloc(SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/) noexcept
{
    debugAssert(Math::isPowOf2(alignment));
    if (size == 0)
    {
        return nullptr;
    }
    alignment = alignBy(size, alignment);
    return mi_malloc_aligned(size, alignment);
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
        mi_free(currentPtr);
        return nullptr;
    }
    // We try to reallocate with same old alignment if default alignment is send it
    if (alignment == DEFAULT_ALIGNMENT)
    {
        return mi_realloc(currentPtr, size);
    }
    alignment = alignBy(size, alignment);
    return mi_realloc_aligned(currentPtr, size, alignment);
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