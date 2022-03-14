/*!
 * \file Memory.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Memory/Memory.h"

void* AllocFromBuiltInMalloc::operator new(SizeT size)
{
    return CBMemory::builtinMalloc(size);
}

void* AllocFromBuiltInMalloc::operator new[](SizeT size)
{
    return CBMemory::builtinMalloc(size);
}

void AllocFromBuiltInMalloc::operator delete(void* ptr)
{
    CBMemory::builtinFree(ptr);
}

void AllocFromBuiltInMalloc::operator delete[](void* ptr)
{
    CBMemory::builtinFree(ptr);
}

// Create and delete policy for CBMemAlloc
bool CBMemAllocCreatePolicy::create(CBMemAlloc** outAllocator)
{
    // #TODO(Jeslas) 
    *outAllocator = nullptr;

    // Mark for delete at exit
    struct CBMemAllocDeleter
    {
        static void deleteCBMemAlloc()
        {
            GALLOC.destroy();
        }
    };
    atexit(CBMemAllocDeleter::deleteCBMemAlloc);
    return *outAllocator != nullptr;
}

void CBMemAllocCreatePolicy::destroy(CBMemAlloc* allocator)
{
    delete allocator;
}

#include "Memory/Memory.inl"
