/*!
 * \file SlotAllocator.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/Math.h"
#include "Memory/Memory.h"
#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"

/**
 * uint32 to allow minimum size of 4bytes, and 4bytes alignment. If alignment or size is below 4bytes
 * they will be aligned to be 4bytes uint64 needs 8bytes as min alignment
 *
 */
template <uint32 ElementSize, uint32 ElementAlignment, uint32 SlotsCount>
class SlotAllocatorBase
{
public:
    using SizeType = uint32;

    constexpr static const SizeType SlotAlignment = (ElementAlignment > sizeof(SizeType) ? ElementAlignment : (SizeType)sizeof(SizeType));
    constexpr static const SizeType SlotSize = Math::alignByUnsafe(ElementSize, SlotAlignment);
    constexpr static const SizeType InvalidSize = ~(SizeType)(0);

    static_assert(
        SlotsCount < InvalidSize, "SlotsCount must less than max of unsigned integer, This is "
                                  "necessary for identifying free slots"
    );
    // Number of slots
    constexpr static const uint32 Count = SlotsCount;

private:
    void *slots;
    // Each free slot stores index of next free slot
    // If InvalidSize then no slots are available
    SizeType freeHeadIdx;
    SizeType freeTailIdx;
    SizeType freeCount;

public:
    SlotAllocatorBase(void *slotsPtr)
        : slots(slotsPtr)
        , freeHeadIdx(0)
        , freeTailIdx(Count - 1)
        , freeCount(Count)
    {
        debugAssert(slots);

        // Setup the initial linked list for slots
        for (SizeType idx = 0; idx < Count; ++idx)
        {
            nextFree(idx) = idx + 1;
        }
        nextFree(freeTailIdx) = InvalidSize;
    }

    FORCE_INLINE bool isOwningMemory(void *ptr) const
    {
        PtrInt diff = (PtrInt)(ptr) - (PtrInt)(slots);
        return diff >= 0 && diff < (SlotSize * Count);
    }

    FORCE_INLINE SizeType ptrToSlotIdx(void *ptr) const { return SizeType(((UPtrInt)(ptr) - (UPtrInt)(slots)) / SlotSize); }

    void *memAlloc(SizeT size, uint32 alignment = SlotAlignment)
    {
        debugAssert(alignment <= SlotAlignment && size <= SlotSize);
        // No free slot
        if (freeHeadIdx == InvalidSize)
        {
            debugAssert(freeTailIdx == InvalidSize);
            return nullptr;
        }
        // If we exhausted every slot
        if (freeHeadIdx == freeTailIdx)
        {
            freeTailIdx = InvalidSize;
        }

        SizeType &freeSlot = nextFree(freeHeadIdx);
        // When all slots are alloted freeHeadIdx will get InvalidSize stored in freeSlot
        freeHeadIdx = freeSlot;
        --freeCount;
        return &freeSlot;
    }
    void memFree(void *ptr)
    {
        if (ptr && isOwningMemory(ptr))
        {
            debugAssert(!isDoubleFreeing(ptr));

            SizeType newTailIdx = ptrToSlotIdx(ptr);
            if (freeTailIdx == InvalidSize)
            {
                debugAssert(freeHeadIdx == InvalidSize);
                freeHeadIdx = newTailIdx;
            }
            else
            {
                SizeType &currTail = nextFree(freeTailIdx);
                // Link new tail to old tail's next ptr
                currTail = newTailIdx;
            }
            freeTailIdx = newTailIdx;
            nextFree(freeTailIdx) = InvalidSize;
            ++freeCount;
        }
    }

    void *at(SizeType idx) const
    {
        debugAssert(idx < Count);
        return memAt(idx);
    }

    /**
     * Returns true if no objects are allocated/used so all slots as free
     */
    bool empty() const { return freeCount == Count; }

protected:
    void *getSlots() const { return slots; }

private:
    // Gives the reference to memory where next free slot after this slot is stored, This reference is
    // also the reference to this slot
    FORCE_INLINE SizeType &nextFree(SizeType slotIdx) const { return *reinterpret_cast<SizeType *>(memAt(slotIdx)); }
    FORCE_INLINE void *memAt(SizeType idx) const { return reinterpret_cast<void *>(reinterpret_cast<uint8 *>(slots) + (idx * SlotSize)); }
    FORCE_INLINE bool isDoubleFreeing(void *ptr) const
    {
        SizeType nextSlot = freeHeadIdx;
        while (nextSlot != InvalidSize)
        {
            SizeType &slot = nextFree(nextSlot);
            if (&slot == ptr)
            {
                return true;
            }
            nextSlot = slot;
        }
        return false;
    }
};

/**
 * IsInlineSlots - Can be used if SlotAllocator itself is heap allocated, This is to avoid another heap allocation for slots
 */
template <uint32 ElementSize, uint32 ElementAlignment, uint32 SlotsCount, bool IsInlineSlots>
class SlotAllocator;

template <uint32 ElementSize, uint32 ElementAlignment, uint32 SlotsCount>
class SlotAllocator<ElementSize, ElementAlignment, SlotsCount, true> : public SlotAllocatorBase<ElementSize, ElementAlignment, SlotsCount>
{
public:
    using BaseType = SlotAllocatorBase<ElementSize, ElementAlignment, SlotsCount>;
    using BaseType::Count;
    using BaseType::SlotAlignment;
    using BaseType::SlotSize;

private:
    alignas(SlotAlignment) uint8 inlineSlots[Count * SlotSize];

public:
    SlotAllocator()
        : BaseType(&inlineSlots)
    {}
};

template <uint32 ElementSize, uint32 ElementAlignment, uint32 SlotsCount>
class SlotAllocator<ElementSize, ElementAlignment, SlotsCount, false> : public SlotAllocatorBase<ElementSize, ElementAlignment, SlotsCount>
{
public:
    using BaseType = SlotAllocatorBase<ElementSize, ElementAlignment, SlotsCount>;
    using BaseType::Count;
    using BaseType::getSlots;
    using BaseType::SlotAlignment;
    using BaseType::SlotSize;

public:
    SlotAllocator()
        : BaseType(CBEMemory::memAlloc(SlotSize * Count, SlotAlignment))
    {}

    ~SlotAllocator() { CBEMemory::memFree(getSlots()); }
};