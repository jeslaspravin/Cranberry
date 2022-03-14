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

#include "Types/CoreTypes.h"
#include "Types/CoreDefines.h"
#include "ProgramCoreExports.h"

// Since we cannot create new memory allocator from out memory allocator itself we rely on in built system allocators to allocate for this object
class PROGRAMCORE_EXPORT AllocFromBuiltInMalloc
{
public:
    void* operator new(SizeT size);
    void* operator new[](SizeT size);
    void operator delete(void* ptr);
    void operator delete[](void* ptr);
};

class PROGRAMCORE_EXPORT CBMemAlloc : public AllocFromBuiltInMalloc
{
public:
    CONST_EXPR static const uint32 DEFAULT_ALIGNMENT = 0;
public:
    virtual ~CBMemAlloc() = default;

    // Why no unaligned allocation? Along with drawbacks of unaligned data Adding unaligned also means remembering whether allocated aligned when freeing.
    // That is unnecessary overhead for programmer/code.
    // delete does not call aligned delete operator unless alignment exceeds __STDCPP_DEFAULT_NEW_ALIGNMENT__(16 in MSVC16 when last checked)
    virtual void* tryMalloc(SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) = 0;
    virtual void* memAlloc(SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) = 0;
    virtual void* tryRealloc(void* currentPtr, SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) = 0;
    virtual void* memRealloc(void* currentPtr, SizeT size, uint32 alignment = DEFAULT_ALIGNMENT) = 0;
    virtual void memFree(void* ptr) = 0;

    // Allocation size of this allocator
    virtual SizeT getAllocationSize() const = 0;
};