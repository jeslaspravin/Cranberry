/*!
 * \file MemAllocator.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/Math.h"
#include "ProgramCoreExports.h"
#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"

#include <new>

// Since we cannot create new memory allocator from out memory allocator itself we rely on in built
// system allocators to allocate for this object
class PROGRAMCORE_EXPORT AllocFromBuiltInMalloc
{
public:
    NODISCARD void *operator new(SizeT size);
    NODISCARD void *operator new(SizeT size, const std::nothrow_t &);
    NODISCARD void *operator new(SizeT size, SizeT);
    NODISCARD void *operator new(SizeT size, SizeT, const std::nothrow_t &);
    NODISCARD void *operator new(std::size_t count, void *allocatedPtr) { return allocatedPtr; }
    NODISCARD void *operator new(std::size_t count, std::align_val_t al, void *allocatedPtr) { return allocatedPtr; }
    void operator delete(void *ptr) noexcept;
    void operator delete(void *ptr, const std::nothrow_t &) noexcept;
    void operator delete(void *ptr, SizeT) noexcept;
    void operator delete(void *ptr, SizeT, const std::nothrow_t &) noexcept;

    NODISCARD void *operator new[](SizeT size);
    NODISCARD void *operator new[](SizeT size, const std::nothrow_t &);
    NODISCARD void *operator new[](SizeT size, SizeT);
    NODISCARD void *operator new[](SizeT size, SizeT, const std::nothrow_t &);
    NODISCARD void *operator new[](std::size_t count, void *allocatedPtr) { return allocatedPtr; }
    NODISCARD void *operator new[](std::size_t count, std::align_val_t al, void *allocatedPtr) { return allocatedPtr; }
    void operator delete[](void *ptr) noexcept;
    void operator delete[](void *ptr, const std::nothrow_t &) noexcept;
    void operator delete[](void *ptr, SizeT) noexcept;
    void operator delete[](void *ptr, SizeT, const std::nothrow_t &) noexcept;
};

class PROGRAMCORE_EXPORT CBEMemAlloc : public AllocFromBuiltInMalloc
{
public:
    CONST_EXPR static const uint32 DEFAULT_ALIGNMENT = 0;
    FORCE_INLINE static uint32 alignBy(SizeT size, uint32 alignment) { return Math::max((size > 8) ? 16 : 8, alignment); }

public:
    virtual ~CBEMemAlloc() = default;

    // Why no unaligned allocation? Along with drawbacks of unaligned data Adding unaligned also means
    // remembering whether allocated aligned when freeing. That is unnecessary overhead for
    // programmer/code. delete does not call aligned delete operator unless alignment exceeds
    // __STDCPP_DEFAULT_NEW_ALIGNMENT__(16 in MSVC16 when last checked)
    virtual void *tryMalloc(SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) = 0;
    virtual void *memAlloc(SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) = 0;
    virtual void *tryRealloc(void *currentPtr, SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) = 0;
    virtual void *memRealloc(void *currentPtr, SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) = 0;
    virtual void memFree(void *ptr) = 0;

    // Allocation size of this allocator
    virtual SizeT getAllocationSize(void *ptr) const = 0;
};