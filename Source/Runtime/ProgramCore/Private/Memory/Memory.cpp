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
#include "Types/Platform/PlatformMemory.h"

void* AllocFromBuiltInMalloc::operator new(SizeT size)
{
    return CBMemory::builtinMalloc(size);
}

void* AllocFromBuiltInMalloc::operator new(SizeT size, const std::nothrow_t&)
{
    return CBMemory::builtinMalloc(size);
}

void* AllocFromBuiltInMalloc::operator new(SizeT size, SizeT)
{
    return CBMemory::builtinMalloc(size);
}

void* AllocFromBuiltInMalloc::operator new(SizeT size, SizeT, const std::nothrow_t&)
{
    return CBMemory::builtinMalloc(size);
}

void AllocFromBuiltInMalloc::operator delete(void* ptr) noexcept
{
    CBMemory::builtinFree(ptr);
}

void AllocFromBuiltInMalloc::operator delete(void* ptr, const std::nothrow_t&) noexcept
{
    CBMemory::builtinFree(ptr);
}

void AllocFromBuiltInMalloc::operator delete(void* ptr, SizeT) noexcept
{
    CBMemory::builtinFree(ptr);
}

void AllocFromBuiltInMalloc::operator delete(void* ptr, SizeT, const std::nothrow_t&) noexcept
{
    CBMemory::builtinFree(ptr);
}

void* AllocFromBuiltInMalloc::operator new[](SizeT size)
{
    return CBMemory::builtinMalloc(size);
}

void* AllocFromBuiltInMalloc::operator new[](SizeT size, const std::nothrow_t&)
{
    return CBMemory::builtinMalloc(size);
}

void* AllocFromBuiltInMalloc::operator new[](SizeT size, SizeT)
{
    return CBMemory::builtinMalloc(size);
}

void* AllocFromBuiltInMalloc::operator new[](SizeT size, SizeT, const std::nothrow_t&)
{
    return CBMemory::builtinMalloc(size);
}

void AllocFromBuiltInMalloc::operator delete[](void* ptr) noexcept
{
    CBMemory::builtinFree(ptr);
}

void AllocFromBuiltInMalloc::operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
    CBMemory::builtinFree(ptr);
}

void AllocFromBuiltInMalloc::operator delete[](void* ptr, SizeT) noexcept
{
    CBMemory::builtinFree(ptr);
}

void AllocFromBuiltInMalloc::operator delete[](void* ptr, SizeT, const std::nothrow_t&) noexcept
{
    CBMemory::builtinFree(ptr);
}


// Create and delete policy for CBMemAlloc
bool CBMemAllocCreatePolicy::create(CBMemAlloc** outAllocator)
{
    *outAllocator = PlatformMemory::createMemAllocator();

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
