/*!
 * \file CoPaTTypes.h
 *
 * \author Jeslas
 * \date May 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CoPaTConfig.h"

#ifndef OVERRIDE_MEMORY_ALLOCATOR
#include <cstdlib>
#include <type_traits>
#endif

COPAT_NS_INLINED
namespace copat
{

/**
 * MainThread must be 0 and any other threads must be sequential values from 1 to WorkerThreads - 1
 */
enum class EJobThreadType
{
    MainThread = 0,
    // User added thread types start
    USER_DEFINED_THREADS()
    // User added thread types end
    WorkerThreads,
    MaxThreads
};

#ifndef OVERRIDE_MEMORY_ALLOCATOR
struct DefaultCoPaTMemAlloc
{
private:
    // Max we align is 2 x Cache line size. So it is 1 or 2 byte based on cache line size
    constexpr static const bool OFFSET_NEED_2BYTES = (2 * CACHE_LINE_SIZE) > 255;
    constexpr static const u32 ADDITIONAL_PAD = (OFFSET_NEED_2BYTES ? 2 : 1);
    using PadOffsetType = std::conditional_t<OFFSET_NEED_2BYTES, uint16_t, uint8_t>;

    static uintptr_t alignByUnsafe(uintptr_t value, uintptr_t alignVal) { return (value + (alignVal - 1)) & ~(alignVal - 1); }

public:
    static void *memAlloc(size_t size, u32 alignment = 1)
    {
        size_t paddedSize = size + (alignment - 1) + ADDITIONAL_PAD;
        void *ptr = ::malloc(paddedSize);
        uintptr_t ptrint = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t dataStart = alignByUnsafe(ptrint + ADDITIONAL_PAD, alignment);

        PadOffsetType offset = static_cast<PadOffsetType>(dataStart - ptrint);
        PadOffsetType *offsetPtr = reinterpret_cast<PadOffsetType *>(dataStart - sizeof(PadOffsetType));
        *offsetPtr = offset;
        return reinterpret_cast<void *>(dataStart);
    }
    static void memFree(void *ptr)
    {
        uintptr_t dataStart = reinterpret_cast<uintptr_t>(ptr);
        PadOffsetType *offsetPtr = reinterpret_cast<PadOffsetType *>(dataStart - sizeof(PadOffsetType));
        ::free(reinterpret_cast<void *>(dataStart - *offsetPtr));
    }
};
using CoPaTMemAlloc = DefaultCoPaTMemAlloc;
#else
using CoPaTMemAlloc = OVERRIDE_MEMORY_ALLOCATOR;
#endif

/**
 * Helper to new and delete a ptr after invoking its destructor
 */

template <typename T, typename... Args>
T *memNew(Args &&...args)
{
    return new (CoPaTMemAlloc::memAlloc(sizeof(T), alignof(T))) T(std::forward<Args>(args)...);
}

template <typename T>
requires (std::is_fundamental_v<T> || std::is_trivially_destructible_v<T>)
void memDelete(T *ptr)
{
    CoPaTMemAlloc::memFree(ptr);
}
template <typename T>
requires (!std::is_trivially_destructible_v<T>)
void memDelete(T *ptr)
{
    ptr->~T();
    CoPaTMemAlloc::memFree(ptr);
}

} // namespace copat