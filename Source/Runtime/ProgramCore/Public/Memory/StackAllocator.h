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
 * Alignment must be handled yourself,
 * Can be used as arena as well.
 * For aligned stack use StackAllocator<SharingMode>
 */
template <EThreadSharing SharingMode>
class StackAllocatorUnaligned;

struct StackAllocatorTraits
{
    // We are not going to need more than 4GB stack size, If yes we are doing something unusual
    using SizeType = uint32;
    // In bytes, IDK 2MB maybe?
    constexpr static const SizeType INITIAL_STACK_SIZE = 2 * 1024 * 1024;
    constexpr static const SizeType MAX_STACK_SIZE = ~0u;
};

template <>
class StackAllocatorUnaligned<EThreadSharing::ThreadSharing_Exclusive>
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
    StackAllocatorUnaligned()
        : memoryPtr(CBEMemory::memAlloc(Traits::INITIAL_STACK_SIZE))
        , stackTop(Traits::INITIAL_STACK_SIZE)
        , bsp(0)
    {
        debugAssert(memoryPtr);
    }
    StackAllocatorUnaligned(SizeType stackSize)
        : memoryPtr(CBEMemory::memAlloc(stackSize))
        , stackTop(stackSize)
        , bsp(0)
    {
        debugAssert(memoryPtr && stackSize > 0);
    }
    StackAllocatorUnaligned(StackAllocatorUnaligned &&other) { (*this) = std::forward<StackAllocatorUnaligned>(other); }
    StackAllocatorUnaligned &operator=(StackAllocatorUnaligned &&other)
    {
        freeStack();
        memoryPtr = other.memoryPtr;
        other.memoryPtr = nullptr;

        stackTop = other.stackTop;
        bytesOverAlloc = other.bytesOverAlloc;
        bsp = other.bsp;

        other.freeStack();
    }
    StackAllocatorUnaligned(const StackAllocatorUnaligned &other) = delete;
    StackAllocatorUnaligned &operator=(const StackAllocatorUnaligned &other) = delete;

    ~StackAllocatorUnaligned() { freeStack(); }

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
    FORCE_INLINE void *basePointer() const
    {
        UPtrInt basePtr = (UPtrInt)(memoryPtr);
        return (void *)(basePtr + bsp);
    }
    FORCE_INLINE bool isBspAlignedBy(uint32 alignment) const { return Math::isAligned((UPtrInt)basePointer(), alignment); }

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
        if (!ptr)
        {
            return;
        }
        if (isOwningMemory(ptr))
        {
            const UPtrInt basePtr = (UPtrInt)(memoryPtr);
            const SizeType bspAfterFree = UPtrInt(ptr) - basePtr;
            void *expectedPtr = (void *)(basePtr + (bsp - size));
            debugAssertf(
                ptr == expectedPtr, "Out of order freeing allocated stack memory! Freeing ptr %llu expected ptr %llu", UPtrInt(ptr), expectedPtr
            );
            // If bsp already went below the freeing pointer's stack pointer, Do nothing.
            if (bsp >= (bspAfterFree + size))
            {
                bsp = bspAfterFree;
            }
        }
        else
        {
            CBEMemory::memFree(ptr);
        }
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
class StackAllocatorUnaligned<EThreadSharing::ThreadSharing_Shared>
{
public:
    using Traits = StackAllocatorTraits;
    using SizeType = Traits::SizeType;

private:
    struct alignas(2 * CACHELINE_SIZE) PerThreadData
    {
        StackAllocatorUnaligned<EThreadSharing::ThreadSharing_Exclusive> allocator;
        // For deleting this PerThreadData at whatever ends last of StackAllocator delete or thread exit
        std::atomic_bool bIsActive = true;
    };
    uint32 tlsSlot;

    // For locking everything common to all thread stack allocators
    CBESpinLock allAllocatorsLock;
    std::vector<PerThreadData *> allStackAllocators;
    SizeType byteSize;

public:
    PROGRAMCORE_EXPORT StackAllocatorUnaligned();
    PROGRAMCORE_EXPORT StackAllocatorUnaligned(SizeType stackByteSize);

    MAKE_TYPE_NONCOPY_NONMOVE(StackAllocatorUnaligned)
    PROGRAMCORE_EXPORT ~StackAllocatorUnaligned();

    /**
     * Returns true if nothing is allocated/used so all slots as free
     */
    bool empty() const
    {
        if (PerThreadData *tlData = getThreadData())
        {
            return tlData->allocator.empty();
        }
        return true;
    }
    FORCE_INLINE bool hasEnoughStack(SizeT size) const
    {
        if (PerThreadData *tlData = getThreadData())
        {
            return tlData->allocator.hasEnoughStack(size);
        }
        return true;
    }
    FORCE_INLINE bool isOwningMemory(void *ptr) const
    {
        if (PerThreadData *tlData = getThreadData())
        {
            return tlData->allocator.isOwningMemory(ptr);
        }
        return false;
    }
    FORCE_INLINE void *basePointer() const
    {
        if (PerThreadData *tlData = getThreadData())
        {
            return tlData->allocator.basePointer();
        }
        return 0;
    }
    FORCE_INLINE bool isBspAlignedBy(uint32 alignment) const
    {
        if (PerThreadData *tlData = getThreadData())
        {
            return tlData->allocator.isBspAlignedBy(alignment);
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

private:
    PROGRAMCORE_EXPORT PerThreadData &getThreadData();
    PROGRAMCORE_EXPORT PerThreadData *getThreadData() const;
    PROGRAMCORE_EXPORT PerThreadData *createNewThreadData();
};

template <EThreadSharing SharingMode>
class StackAllocator : private StackAllocatorUnaligned<SharingMode>
{
public:
    using BaseType = StackAllocatorUnaligned<SharingMode>;
    using typename BaseType::SizeType;

    // For now only padding is needed for freeing
    using AlignmentType = uint8;
    using StackAllocHeader = AlignmentType;
    constexpr static const uint32 MIN_ALIGNMENT = CBEMemAlloc::DEFAULT_ALIGNMENT;
    // 128bytes padding is more than large enough right now
    constexpr static const uint32 MAX_SUPPORTED_ALIGNMENT = std::numeric_limits<AlignmentType>::max();

public:
    StackAllocator() = default;
    StackAllocator(SizeType stackByteSize)
        : BaseType(stackByteSize)
    {}
    ~StackAllocator() = default;

    MAKE_TYPE_NONCOPY_NONMOVE(StackAllocator)

    /**
     * Returns true if nothing is allocated/used so all slots as free
     */
    FORCE_INLINE bool empty() const { return BaseType::empty(); }
    FORCE_INLINE bool hasEnoughStack(SizeT size) const { return BaseType::hasEnoughStack(size); }
    FORCE_INLINE bool isOwningMemory(void *ptr) const { return BaseType::isOwningMemory(ptr); }
    FORCE_INLINE void *basePointer() const { return BaseType::basePointer(); }
    FORCE_INLINE bool isBspAlignedBy(uint32 alignment) const { return BaseType::isBspAlignedBy(alignment); }
    FORCE_INLINE void reset(SizeT newByteSize = 0) { BaseType::reset(newByteSize); }

    void *memAlloc(SizeType size, uint32 alignment = MIN_ALIGNMENT)
    {
        alignAllocSize(size, alignment);
        debugAssertf(
            alignment <= MAX_SUPPORTED_ALIGNMENT, "StackAllocator does not support alignment %u greater than %u", alignment,
            MAX_SUPPORTED_ALIGNMENT
        );

        void *ptr = BaseType::memAlloc(size + calcExtraWidth(alignment));
        return writeAllocMeta(ptr, size, alignment);
    }
    void memFree(void *ptr, SizeType size, uint32 alignment = MIN_ALIGNMENT)
    {
        alignAllocSize(size, alignment);
        uint32 padding = 0;
        void *paddedPtr = getAllocationInfo(ptr, padding);
        BaseType::memFree(paddedPtr, size + padding);
    }

private:
    FORCE_INLINE static void alignAllocSize(SizeType &inOutSize, uint32 &inOutAlignment)
    {
        debugAssert(Math::isPowOf2(inOutAlignment));
        inOutAlignment = Math::max(alignof(StackAllocHeader), CBEMemAlloc::alignBy(inOutSize, inOutAlignment), MIN_ALIGNMENT);
        inOutSize = Math::alignByUnsafe(inOutSize, inOutAlignment);
    }

    FORCE_INLINE SizeType calcExtraWidth(uint32 alignment) const
    {
        static_assert(sizeof(StackAllocHeader) == 1, "Assumes that StackAllocHeader is a byte!");
        UPtrInt basePtr = (UPtrInt)basePointer();
        UPtrInt alignedBsp = Math::alignByUnsafe(basePtr, alignment);
        // If bsp is perfectly aligned then we need entire alignment space for adding header
        if (alignedBsp == basePtr)
        {
            return alignment;
        }
        else
        {
            return alignedBsp - basePtr;
        }
    }

    FORCE_INLINE void *writeAllocMeta(void *allocatedPtr, SizeT size, uint32 alignment) const
    {
        static_assert(sizeof(StackAllocHeader) == 1, "Assumes that StackAllocHeader is a byte!");
        UPtrInt alignedOutPtr = Math::alignByUnsafe((UPtrInt)(allocatedPtr), alignment);
        uint32 padding = alignedOutPtr - (UPtrInt)allocatedPtr;
        if (padding == 0)
        {
            alignedOutPtr += alignment;
            padding = alignment;
        }

        StackAllocHeader *allocHeader = (((StackAllocHeader *)alignedOutPtr) - 1);
        *allocHeader = padding;
        return (void *)alignedOutPtr;
    }
    FORCE_INLINE void *getAllocationInfo(void *ptr, uint32 &outPadding) const
    {
        StackAllocHeader *allocHeader = (((StackAllocHeader *)ptr) - 1);
        outPadding = *allocHeader;
        return (void *)(((UPtrInt)ptr) - outPadding);
    }
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

    CONST_EXPR void deallocate(Type *const ptr, const SizeT count)
    {
        if (stackAllocator == nullptr)
            return;

        stackAllocator->memFree(ptr, count * sizeof(Type), alignof(Type));
    }

    NODISCARD CONST_EXPR Type *allocate(const SizeT count)
    {
        if (stackAllocator == nullptr)
            return nullptr;

        return static_cast<Type *>(stackAllocator->memAlloc(count * sizeof(Type), alignof(Type)));
    }

    NODISCARD CONST_EXPR AllocatorType *allocator() const { return stackAllocator; }
};

template <typename T1, typename T2, EThreadSharing SharingMode>
FORCE_INLINE CONST_EXPR bool
    operator==(const CBEStlStackAllocator<T1, SharingMode> &lhs, const CBEStlStackAllocator<T2, SharingMode> &rhs) noexcept
{
    return lhs.allocator() == rhs.allocator();
}

template <typename Type>
using CBEStlStackAllocatorShared = CBEStlStackAllocator<Type, EThreadSharing::ThreadSharing_Shared>;
template <typename Type>
using CBEStlStackAllocatorExclusive = CBEStlStackAllocator<Type, EThreadSharing::ThreadSharing_Exclusive>;