/*!
 * \file BuiltinMemAlloc.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Memory/MemAllocator.h"

// Not very good for small allocations, for small allocation below 32bytes do not use this as allocation header data occupies above 50% of each allocation
class PROGRAMCORE_EXPORT CBEBuiltinMemAlloc final : public CBEMemAlloc
{
private:

    struct AllocHeader
    {
        SizeT size;
        uint32 alignment;
#ifndef PLATFORM_ALIGNED_MALLOC
        // Alignment offset from allocated ptr, for freeing
        uint32 offset;
#endif
    };

    FORCE_INLINE SizeT calcHeaderPadding(uint32 alignment) const;
    FORCE_INLINE SizeT calcExtraWidth(uint32 alignment) const;
    FORCE_INLINE void* writeAllocMeta(void* allocatedPtr, SizeT size, uint32 alignment) const;
    FORCE_INLINE void* getAllocationInfo(void* ptr, SizeT& outSize, uint32& outAlignment) const;
public:
    void* tryMalloc(SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) final;
    void* memAlloc(SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) final;
    void* tryRealloc(void* currentPtr, SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) final;
    void* memRealloc(void* currentPtr, SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) final;
    void memFree(void* ptr) final;

    SizeT getAllocationSize(void* ptr) const;
};

template <typename Type>
struct CBEStlMallocAllocator
{
private:
    CBEBuiltinMemAlloc allocator;

public:
    using value_type = Type;
    using size_type = SizeT;
    using difference_type = IntPtr;
    using propagate_on_container_move_assignment = std::true_type;

    CONST_EXPR CBEStlMallocAllocator() noexcept {}

    CONST_EXPR CBEStlMallocAllocator(const CBEStlMallocAllocator&) noexcept = default;
    template <typename OtherType>
    CONST_EXPR CBEStlMallocAllocator(const CBEStlMallocAllocator<OtherType>&) noexcept {}
    CONST_EXPR ~CBEStlMallocAllocator() = default;
    CONST_EXPR CBEStlMallocAllocator& operator=(const CBEStlMallocAllocator&) = default;


    CONST_EXPR void deallocate(Type* const ptr, const SizeT size)
    {
        allocator.memFree(ptr);
    }

    NODISCARD CONST_EXPR Type* allocate(const SizeT size) 
    {
        return static_cast<Type*>(allocator.memAlloc(size));
    }
};

template<typename T1, typename T2>
FORCE_INLINE CONST_EXPR bool operator==(const CBEStlMallocAllocator<T1>& lhs, const CBEStlMallocAllocator<T2>& rhs) noexcept
{
    // All are equal due to malloc usage
    return true;
}