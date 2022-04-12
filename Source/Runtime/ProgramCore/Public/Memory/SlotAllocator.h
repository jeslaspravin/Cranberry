/*!
 * \file SlotAllocator.h
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
#include "Math/Math.h"
#include "Memory/Memory.h"


// uint32 to allow minimum size of 4bytes, and 4bytes alignment. If alignment or size is below 4bytes they will be aligned to be 4bytes
// uint64 needs 8bytes as min alignment
template <uint32 ElementSize, uint32 ElementAlignment, uint32 SlotsCount>
class SlotAllocator
{
public:
    using SizeType = uint32;

    CONST_EXPR static const SizeType SlotAlignment = (ElementAlignment > sizeof(SizeType) ? ElementAlignment : (SizeType)sizeof(SizeType));
    CONST_EXPR static const SizeType SlotSize = Math::alignByUnsafe(ElementSize, SlotAlignment);
    CONST_EXPR static const SizeType InvalidSize = ~(SizeType)(0);

    static_assert(SlotsCount < InvalidSize, "SlotsCount must less than max of unsigned integer, This is necessary for identifying free slots");
    // Number of slots
    CONST_EXPR static const uint32 Size = SlotsCount;
private:
    void* slots;
    // Each free slot stores index of next free slot
    // If InvalidSize then no slots are available
    SizeType freeHead;
    SizeType freeTail;
    SizeType freeCount;
private:
    // Gives the reference to memory where next free slot after this slot is stored, This reference is also the reference to this slot
    FORCE_INLINE SizeType& nextFree(SizeType slotIdx) const
    {
        return *reinterpret_cast<SizeType*>(reinterpret_cast<uint8*>(slots) + (slotIdx * SlotSize));
    }
    FORCE_INLINE bool isDoubleFreeing(void* ptr) const
    {
        SizeType nextSlot = freeHead;
        while (nextSlot != InvalidSize)
        {
            SizeType& slot = nextFree(nextSlot);
            if (&slot == ptr)
            {
                return true;
            }
            nextSlot = slot;
        }
        return false;
    }
public:
    SlotAllocator()
        : freeHead(0)
        , freeTail(Size - 1)
        , freeCount(Size)
    {
        slots = CBEMemory::memAlloc(SlotSize * Size, SlotAlignment);
        for (SizeType idx = 0; idx < Size; ++idx)
        {
            nextFree(idx) = idx + 1;
        }
        nextFree(freeTail) = InvalidSize;
    }

    ~SlotAllocator()
    {
        CBEMemory::memFree(slots);
    }

    FORCE_INLINE bool isOwningMemory(void* ptr) const
    {
        IntPtr diff = (IntPtr)(ptr)-(IntPtr)(slots);
        return diff >= 0 && diff < (SlotSize* Size);
    }

    FORCE_INLINE SizeType ptrToSlotIdx(void* ptr) const
    {
        return ((UIntPtr)(ptr)-(UIntPtr)(slots)) / SlotSize;
    }

    void* memAlloc(SizeT size, uint32 alignment = SlotAlignment)
    {
        debugAssert(alignment <= SlotAlignment && size <= SlotSize);
        // No free slot
        if (freeHead == InvalidSize)
        {
            return nullptr;
        }

        SizeType& freeSlot = nextFree(freeHead);
        freeHead = freeSlot;
        --freeCount;
        return &freeSlot;
    }
    void memFree(void* ptr)
    {
        if (ptr && isOwningMemory(ptr))
        {
            debugAssert(!isDoubleFreeing(ptr));

            SizeType& currTail = nextFree(freeTail);
            freeTail = ptrToSlotIdx(ptr);
            currTail = freeTail;
            nextFree(freeTail) = InvalidSize;
            ++freeCount;
        }
    }

    void* at(SizeType idx) const
    {
        debugAssert(idx < Size);
        return &nextFree(idx);
    }

    /**
     * Returns true if no objects are allocated/used so all slots as free
     */
    bool empty() const
    {
        return freeCount == Size;
    }
};