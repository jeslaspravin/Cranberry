/*!
 * \file GenericPlatformMemory.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/Platform/GenericPlatformMemory.h"
#include "Memory/BuiltinMemAlloc.h"
#include "Memory/MimallocMemAlloc.h"
#include "Modules/ModuleManager.h"
#include "Types/Platform/LFS/PathFunctions.h"

CBEMemAlloc *GenericPlatformMemory::createMemAllocator()
{
#if USE_MIMALLOC
    return new MimallocMemAlloc();
#endif

    // fallback mem allocator
    return new CBEBuiltinMemAlloc();
}
