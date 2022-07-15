/*!
 * \file StackAllocator.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreTypes.h"
#include "Types/CoreDefines.h"
#include "Math/Math.h"
#include "Memory/Memory.h"
#include "Types/Platform/Threading/SyncPrimitives.h"

#include <shared_mutex>

/**
 * Alignment must be handled yourself
 */
template <EThreadSharing SharingMode>
class StackAllocator;

struct StackAllocatorTraits
{
    // We are not going to need more than 4GB stack size, If yes we are doing something wrong
    using SizeType = uint32;
    // In bytes, IDK 2MB maybe?
    constexpr static const SizeType INITIAL_STACK_SIZE = 2 * 1024 * 1024;
    constexpr static const SizeType MAX_STACK_SIZE = ~0u;
};

template <>
class StackAllocator<EThreadSharing::ThreadSharing_Exclusive>
{
public:
    using Traits = StackAllocatorTraits;
    using SizeType = Traits::SizeType;

private:
    void *memoryPtr;
    SizeType stackTop;
    // Num of bytes allocated out of stack, This happens when stack runs out of memory
    SizeType bytesOverAlloc = 0;
    SizeType bsp;

public:
    StackAllocator()
        : memoryPtr(CBEMemory::memAlloc(Traits::INITIAL_STACK_SIZE))
        , stackTop(Traits::INITIAL_STACK_SIZE)
        , bsp(0)
    {
        debugAssert(memoryPtr);
    }
    StackAllocator(SizeType stackSize)
        : memoryPtr(CBEMemory::memAlloc(stackSize))
        , stackTop(stackSize)
        , bsp(0)
    {
        debugAssert(memoryPtr && stackSize > 0);
    }
    StackAllocator(StackAllocator &&other) { (*this) = std::forward<StackAllocator>(other); }
    StackAllocator &operator=(StackAllocator &&other)
    {
        freeStack();
        memoryPtr = other.memoryPtr;
        other.memoryPtr = nullptr;

        stackTop = other.stackTop;
        bytesOverAlloc = other.bytesOverAlloc;
        bsp = other.bsp;

        other.freeStack();
    }
    StackAllocator(const StackAllocator &other) = delete;
    StackAllocator &operator=(const StackAllocator &other) = delete;

    ~StackAllocator() { freeStack(); }

    /**
     * Returns true if nothing is allocated/used so all slots as free
     */
    bool empty() const { return bsp == 0; }
    FORCE_INLINE bool hasEnoughStack(SizeT size) const { return (stackTop - bsp) >= size; }
    FORCE_INLINE bool isOwningMemory(void *ptr) const
    {
        PtrInt diff = (PtrInt)(ptr) - (PtrInt)(memoryPtr);
        return diff >= 0 && diff < stackTop;
    }
    FORCE_INLINE bool isBspAlignedBy(uint32 alignment) const
    {
        UPtrInt basePtr = (UPtrInt)(memoryPtr);
        basePtr += bsp;
        return Math::isAligned(basePtr, alignment);
    }

    void reset(SizeT newByteSize = 0)
    {
        debugAssert(bsp == 0);
        newByteSize = newByteSize == 0 ? stackTop + bytesOverAlloc : newByteSize;
        if (newByteSize != stackTop)
        {
            CBEMemory::memFree(memoryPtr);
            memoryPtr = CBEMemory::memAlloc(newByteSize);
            stackTop = newByteSize;
        }
        bytesOverAlloc = 0;
        bsp = 0;
    }

    void *memAlloc(SizeT size)
    {
        if (hasEnoughStack(size))
        {
            UPtrInt basePtr = (UPtrInt)(memoryPtr);
            basePtr += bsp;
            bsp += size;
            return (void *)basePtr;
        }
        // Out of stack, Use heap
        void *outPtr = CBEMemory::memAlloc(size);
        bytesOverAlloc += size;
        return outPtr;
    }
    void memFree(void *ptr, SizeType size)
    {
        if (ptr)
        {
            return;
        }
        if (isOwningMemory(ptr))
        {
            alertAlwaysf(UPtrInt(ptr) == (UPtrInt(memoryPtr) + (bsp - size)), "Out of order freeing allocated stack memory!");
        }
        else
        {
            CBEMemory::memFree(ptr);
        }
    }
    void memFreeChecked(void *ptr, SizeType size)
    {
        if (ptr)
        {
            return;
        }
        if (isOwningMemory(ptr))
        {
            alertAlwaysf(UPtrInt(ptr) == (UPtrInt(memoryPtr) + (bsp - size)), "Out of order freeing allocated stack memory!");
        }
        memFree(ptr, size);
    }

private:
    void freeStack()
    {
        if (memoryPtr)
        {
            CBEMemory::memFree(memoryPtr);
        }
        stackTop = 0;
        bsp = 0;
        bytesOverAlloc = 0;
    }
};

/**
 * Just having per thread stack allocated, All threads that used this stack must exit before StackAllocator is destroyed
 */
template <>
class StackAllocator<EThreadSharing::ThreadSharing_Shared>
{
public:
    using Traits = StackAllocatorTraits;
    using SizeType = Traits::SizeType;

private:
    struct alignas(2 * CACHELINE_SIZE) PerThreadData
    {
        StackAllocator<EThreadSharing::ThreadSharing_Exclusive> allocator;
        // For deleting this PerThreadData at whatever ends last of StackAllocator delete or thread exit
        std::atomic_bool bIsActive = true;
    };
    uint32 tlsSlot;

    // For locking everything common to all thread stack allocators
    CBESpinLock allAllocatorsLock;
    std::vector<PerThreadData *> allStackAllocators;
    SizeType byteSize;

public:
    PROGRAMCORE_EXPORT StackAllocator();
    PROGRAMCORE_EXPORT StackAllocator(SizeType stackByteSize);

    MAKE_TYPE_NONCOPY_NONMOVE(StackAllocator)
    PROGRAMCORE_EXPORT ~StackAllocator();

    /**
     * Returns true if nothing is allocated/used so all slots as free
     */
    bool empty() const
    {
        if (PerThreadData *tlData = getThreadData())
        {
            tlData->allocator.empty();
        }
        return true;
    }
    FORCE_INLINE bool hasEnoughStack(SizeT size) const
    {
        if (PerThreadData *tlData = getThreadData())
        {
            tlData->allocator.hasEnoughStack(size);
        }
        return true;
    }
    FORCE_INLINE bool isOwningMemory(void *ptr) const
    {
        if (PerThreadData *tlData = getThreadData())
        {
            tlData->allocator.isOwningMemory(ptr);
        }
        return false;
    }
    FORCE_INLINE bool isBspAlignedBy(uint32 alignment) const
    {
        if (PerThreadData *tlData = getThreadData())
        {
            tlData->allocator.isBspAlignedBy(alignment);
        }
        return false;
    }

    FORCE_INLINE void reset(SizeT newByteSize = 0)
    {
        newByteSize = newByteSize == 0 ? byteSize : newByteSize;
        PerThreadData &tlData = getThreadData();
        tlData.allocator.reset(newByteSize);
    }

    void *memAlloc(SizeT size)
    {
        PerThreadData &tlData = getThreadData();
        return tlData.allocator.memAlloc(size);
    }
    void memFree(void *ptr, SizeType size)
    {
        PerThreadData &tlData = getThreadData();
        tlData.allocator.memFree(ptr, size);
    }
    void memFreeChecked(void *ptr, SizeType size)
    {
        PerThreadData &tlData = getThreadData();
        tlData.allocator.memFreeChecked(ptr, size);
    }

private:
    PROGRAMCORE_EXPORT PerThreadData &getThreadData();
    PROGRAMCORE_EXPORT PerThreadData *getThreadData() const;
    PROGRAMCORE_EXPORT PerThreadData *createNewThreadData();
};

template <typename Type, EThreadSharing SharingMode>
struct CBEStlStackAllocator
{
public:
    using value_type = Type;
    using size_type = SizeT;
    using difference_type = PtrInt;
    using reference = value_type &;
    using const_reference = value_type const &;
    using pointer = value_type *;
    using const_pointer = value_type const *;
    using propagate_on_container_move_assignment = std::true_type;
    template <class U>
    struct rebind
    {
        using other = CBEStlStackAllocator<U, SharingMode>;
    };

    using AllocatorType = StackAllocator<SharingMode>;

private:
    AllocatorType *stackAllocator = nullptr;

public:
    CONST_EXPR CBEStlStackAllocator() noexcept = default;
    template <typename OtherType>
    CONST_EXPR CBEStlStackAllocator(const CBEStlStackAllocator<OtherType, SharingMode> &other) noexcept
        : stackAllocator(other.allocator())
    {}
    CONST_EXPR CBEStlStackAllocator(StackAllocator<SharingMode> &inAllocator) noexcept
        : stackAllocator(&inAllocator)
    {}
    CONST_EXPR CBEStlStackAllocator(const CBEStlStackAllocator &) noexcept = default;
    CONST_EXPR CBEStlStackAllocator &operator=(const CBEStlStackAllocator &) = default;

    CONST_EXPR ~CBEStlStackAllocator() = default;

    CONST_EXPR void deallocate(Type *const ptr, const SizeT size)
    {
        if (stackAllocator == nullptr)
            return;

        stackAllocator->memFree(ptr, size);
    }

    NODISCARD CONST_EXPR Type *allocate(const SizeT size)
    {
        if (stackAllocator == nullptr)
            return nullptr;

        debugAssert(stackAllocator->isBspAlignedBy(alignof(Type)));
        return static_cast<Type *>(stackAllocator->memAlloc(size));
    }

    NODISCARD CONST_EXPR AllocatorType *allocator() const { return stackAllocator; }
};

template <typename T1, typename T2, EThreadSharing SharingMode>
FORCE_INLINE CONST_EXPR bool
    operator==(const CBEStlStackAllocator<T1, SharingMode> &lhs, const CBEStlStackAllocator<T2, SharingMode> &rhs) noexcept
{
    return lhs.allocator() == rhs.allocator();
}