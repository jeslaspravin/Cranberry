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

void* CBBuiltinMemAlloc::tryMalloc(SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/)
{    
    if (void* ptr = CBMemory::builtinMalloc(size + sizeof(SizeT)))
    {
        // Put size to the front of allocation to get size while freeing
        totalAllocation += size;
        *(SizeT*)(ptr) = size;
        return ((uint8*)(ptr)) + sizeof(SizeT);
    }
    return nullptr;
}

void* CBBuiltinMemAlloc::memAlloc(SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/)
{
    void* ptr = tryMalloc(size, alignment);
    fatalAssert(ptr, "Allocation failed!");
    return ptr;
}

void* CBBuiltinMemAlloc::tryRealloc(void* currentPtr, SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/)
{
    SizeT* actualPtr = ((SizeT*)currentPtr) - 1;
    totalAllocation -= *actualPtr;
    if (void* ptr = CBMemory::builtinRealloc(actualPtr, size + sizeof(SizeT)))
    {
        totalAllocation += size;
        *(SizeT*)(ptr) = size;
        return ((uint8*)(ptr)) + sizeof(SizeT);
    }
    return nullptr;
}

void* CBBuiltinMemAlloc::memRealloc(void* currentPtr, SizeT size, uint32 alignment /*= DEFAULT_ALIGNMENT*/)
{
    void* ptr = tryRealloc(currentPtr, size, alignment);
    fatalAssert(ptr, "Reallocation failed!");
    return ptr;
}

void CBBuiltinMemAlloc::memFree(void* ptr)
{
    SizeT* actualPtr = ((SizeT*)ptr) - 1;
    totalAllocation -= *actualPtr;
    CBMemory::builtinFree(actualPtr);
}

SizeT CBBuiltinMemAlloc::getAllocationSize() const
{
    return totalAllocation;
}
