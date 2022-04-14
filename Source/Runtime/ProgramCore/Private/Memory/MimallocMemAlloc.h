/*!
 * \file MimallocMemAlloc.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#if USE_MIMALLOC

#include "Memory/MemAllocator.h"

class MimallocMemAlloc final : public CBEMemAlloc
{
private:
public:
    void *tryMalloc(SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) final;
    void *memAlloc(SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) final;
    void *tryRealloc(void *currentPtr, SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) final;
    void *memRealloc(void *currentPtr, SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) final;
    void memFree(void *ptr) final;

    SizeT getAllocationSize(void *ptr) const;
};
#endif // USE_MIMALLOC