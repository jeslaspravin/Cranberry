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

void *AllocFromBuiltInMalloc::operator new(SizeT size) { return CBEMemory::builtinMalloc(size); }

void *AllocFromBuiltInMalloc::operator new(SizeT size, const std::nothrow_t &)
{
    return CBEMemory::builtinMalloc(size);
}

void *AllocFromBuiltInMalloc::operator new(SizeT size, SizeT) { return CBEMemory::builtinMalloc(size); }

void *AllocFromBuiltInMalloc::operator new(SizeT size, SizeT, const std::nothrow_t &)
{
    return CBEMemory::builtinMalloc(size);
}

void AllocFromBuiltInMalloc::operator delete(void *ptr) noexcept { CBEMemory::builtinFree(ptr); }

void AllocFromBuiltInMalloc::operator delete(void *ptr, const std::nothrow_t &) noexcept
{
    CBEMemory::builtinFree(ptr);
}

void AllocFromBuiltInMalloc::operator delete(void *ptr, SizeT) noexcept { CBEMemory::builtinFree(ptr); }

void AllocFromBuiltInMalloc::operator delete(void *ptr, SizeT, const std::nothrow_t &) noexcept
{
    CBEMemory::builtinFree(ptr);
}

void *AllocFromBuiltInMalloc::operator new[](SizeT size) { return CBEMemory::builtinMalloc(size); }

void *AllocFromBuiltInMalloc::operator new[](SizeT size, const std::nothrow_t &)
{
    return CBEMemory::builtinMalloc(size);
}

void *AllocFromBuiltInMalloc::operator new[](SizeT size, SizeT)
{
    return CBEMemory::builtinMalloc(size);
}

void *AllocFromBuiltInMalloc::operator new[](SizeT size, SizeT, const std::nothrow_t &)
{
    return CBEMemory::builtinMalloc(size);
}

void AllocFromBuiltInMalloc::operator delete[](void *ptr) noexcept { CBEMemory::builtinFree(ptr); }

void AllocFromBuiltInMalloc::operator delete[](void *ptr, const std::nothrow_t &) noexcept
{
    CBEMemory::builtinFree(ptr);
}

void AllocFromBuiltInMalloc::operator delete[](void *ptr, SizeT) noexcept
{
    CBEMemory::builtinFree(ptr);
}

void AllocFromBuiltInMalloc::operator delete[](void *ptr, SizeT, const std::nothrow_t &) noexcept
{
    CBEMemory::builtinFree(ptr);
}

// Create and delete policy for CBEMemAlloc
bool CBEMemAllocCreatePolicy::create(CBEMemAlloc **outAllocator)
{
    *outAllocator = PlatformMemory::createMemAllocator();

    // Clean up is unnecessary here as everything will be cleared when program exits and this memory
    // allocator exists till end Mark for delete at exit
    // struct CBEMemAllocDeleter
    //{
    //    static void deleteCBEMemAlloc()
    //    {
    //        GALLOC.destroy();
    //    }
    //};
    // atexit(CBEMemAllocDeleter::deleteCBEMemAlloc);
    return *outAllocator != nullptr;
}

void CBEMemAllocCreatePolicy::destroy(CBEMemAlloc *allocator) { delete allocator; }

#include "Memory/Memory.inl"
