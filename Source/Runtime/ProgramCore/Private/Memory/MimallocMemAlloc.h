/*!
 * \file MimallocMemAlloc.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#if USE_MIMALLOC

#include "Memory/MemAllocator.h"
#include "Profiler/ProgramProfiler.hpp"

class MimallocMemAlloc final : public CBEMemAlloc
{
private:
    static const CBEProfilerChar *ALLOC_NAME;

public:
    void *tryMalloc(SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) noexcept final;
    void *memAlloc(SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) noexcept final;
    void *tryRealloc(void *currentPtr, SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) noexcept final;
    void *memRealloc(void *currentPtr, SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) noexcept final;
    void memFree(void *ptr) noexcept final;

    SizeT getAllocationSize(void *ptr) const noexcept;
};
#endif // USE_MIMALLOC