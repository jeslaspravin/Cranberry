/*!
 * \file PoolAllocator.h
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Memory/SlotAllocator.h"

/**
 * Some private members are copied straight from CoreObjectAllocator.h's cbe::ObjectAllocator, If any bug fixes were made do the same there as
 * well!
 */
template <typename ElementType, SizeT PoolSlotsCount>
class PoolAllocator
{
private:
    using SlotAllocatorType = SlotAllocator<sizeof(ElementType), alignof(ElementType), PoolSlotsCount, true>;
    using SlotIdxType = typename SlotAllocatorType::SizeType;

public:
    using SlotGenerationIdxType = SlotIdxType;
    using SlotAllocIdxType = SlotIdxType;

    struct AllocHandle
    {
        SlotGenerationIdxType genIdx;
        SlotAllocIdxType allocIdx;

        FORCE_INLINE bool operator==(const AllocHandle &rhs) const { return genIdx == rhs.genIdx && allocIdx == rhs.allocIdx; }
    };

private:
    BitArray<SlotIdxType> allocValidity;
    std::vector<SlotGenerationIdxType> slotGeneration;
    std::vector<SlotAllocatorType *> allocatorPools;

    // If we get at least 2 empty allocators then we clear them both, This is to avoid frequent delete on
    // frequent add and deletes
    SlotAllocatorType *emptyPool = nullptr;
    SizeT emptyPoolIdx = 0;

    SizeT lastAllocPoolCache = 0;

public:
    PoolAllocator() = default;
    ~PoolAllocator()
    {
        emptyPool = nullptr;
        for (SlotAllocatorType *slotAlloc : allocatorPools)
        {
            if (slotAlloc)
            {
                delete slotAlloc;
            }
        }
        allocatorPools.clear();
        allocValidity.clear();
        slotGeneration.clear();
    }

    FORCE_INLINE bool isValid(const AllocHandle &handle) const
    {
        return allocValidity[handle.allocIdx] && slotGeneration[handle.allocIdx] == handle.genIdx;
    }

    ElementType *getAllocAt(AllocHandle handle) const
    {
        SlotIdxType slotIdx;
        SizeT poolIdx = allocIdxToSlotIdx(slotIdx, handle.allocIdx);
        debugAssert(isValid(handle) && allocatorPools.size() > poolIdx && allocatorPools[poolIdx]);
        return allocatorPools[poolIdx]->at(slotIdx);
    }

    AllocHandle allocate()
    {
        SizeT allocateFrom = lastAllocatedCacheValid() ? lastAllocPoolCache : findAllocator();
        void *ptr = allocatorPools[allocateFrom]->memAlloc(SlotAllocatorType::SlotSize);
        if (ptr == nullptr)
        {
            allocateFrom = findAllocator();
            ptr = allocatorPools[allocateFrom]->memAlloc(SlotAllocatorType::SlotSize);
            fatalAssertf(ptr, "Allocating object failed!");
        }
        SlotIdxType slotIdx = allocatorPools[allocateFrom]->ptrToSlotIdx(ptr);

        SlotAllocIdxType allocIdx = slotIdxToAllocIdx(slotIdx, allocateFrom);
        allocValidity[allocIdx] = true;                            // Marking this alloc bit as allocated
        SlotGenerationIdxType genIdx = ++slotGeneration[allocIdx]; // Increment generation
        lastAllocPoolCache = allocateFrom;
        return AllocHandle{ .genIdx = genIdx, .allocIdx = allocIdx };
    }
    void free(AllocHandle handle)
    {
        if (!isValid(handle))
        {
            return;
        }
        SlotIdxType slotIdx;
        SizeT poolIdx = allocIdxToSlotIdx(slotIdx, handle.allocIdx);
        allocatorPools[poolIdx]->memFree(ptr);
        allocValidity[allocIdx] = false;

        onFree(poolIdx);
    }
    void free(ElementType *ptr, AllocIdx allocIdx)
    {
        // Double freeing?
        debugAssert(allocValidity[allocIdx]);
        if (ptr != getAllocAt(allocIdx))
            return;

        SlotIdxType slotIdx;
        SizeT poolIdx = allocIdxToSlotIdx(slotIdx, allocIdx);
        allocatorPools[poolIdx]->memFree(ptr);
        allocValidity[allocIdx] = false;

        onFree(poolIdx);
    }
    void free(void *ptr) override
    {
        SizeT poolIdx;
        SlotAllocatorType *ptrAllocator = nullptr;
        if (lastAllocatedCacheValid() && allocatorPools[lastAllocPoolCache]->isOwningMemory(ptr))
        {
            ptrAllocator = allocatorPools[lastAllocPoolCache];
            poolIdx = lastAllocPoolCache;
        }
        else
        {
            SizeT idx = 0;
            for (SlotAllocatorType *allocator : allocatorPools)
            {
                if (allocator && allocator->isOwningMemory(ptr))
                {
                    ptrAllocator = allocator;
                    poolIdx = idx;
                    break;
                }
                ++idx;
            }
        }
        if (ptrAllocator)
        {
            SlotAllocIdxType allocIdx = slotIdxToAllocIdx(ptrAllocator->ptrToSlotIdx(ptr), poolIdx);
            // Double freeing?
            debugAssert(allocValidity[allocIdx]);

            ptrAllocator->memFree(ptr);
            allocValidity[allocIdx] = false;
            onFree(poolIdx);
        }
    }

private:
    bool lastAllocatedCacheValid() const { return lastAllocPoolCache < allocatorPools.size() && allocatorPools[lastAllocPoolCache]; }
    FORCE_INLINE static SizeT allocIdxToSlotIdx(SlotIdxType &slotIdx, SlotAllocIdxType allocIdx)
    {
        slotIdx = (allocIdx % SlotAllocatorType::Count);
        return (allocIdx / SlotAllocatorType::Count);
    }
    FORCE_INLINE static AllocIdx slotIdxToAllocIdx(SlotIdxType slotIdx, SizeT poolIdx)
    {
        return (AllocIdx)(poolIdx * SlotAllocatorType::Count + slotIdx);
    }

    ElementType *getAllocAt(SlotAllocIdxType allocIdx) const
    {
        SlotIdxType slotIdx;
        SizeT poolIdx = allocIdxToSlotIdx(slotIdx, allocIdx);
        debugAssert(allocValidity[allocIdx] && allocatorPools.size() > poolIdx && allocatorPools[poolIdx]);
        return allocatorPools[poolIdx]->at(slotIdx);
    }

    FORCE_INLINE SizeT findAllocator()
    {
        // Allocator that is nullptr in allocators vector, This is to allocate new allocator and
        SlotAllocatorType **firstNullAllocator = nullptr;
        SizeT firstNullIdx = allocatorPools.size();

        auto endItr = allocValidity.cend();
        auto beginItr = allocValidity.cbegin();
        for (auto itr = beginItr; itr != endItr; ++itr)
        {
            if (!(*itr))
            {
                SlotIdxType slotIdx;
                SizeT poolIdx = allocIdxToSlotIdx(slotIdx, (itr - beginItr));
                if (allocatorPools[poolIdx])
                {
                    return poolIdx;
                }
                else
                {
                    firstNullIdx = poolIdx;
                    firstNullAllocator = allocatorPools.data() + poolIdx;
                }
            }
        }
        // No free slot found, create new
        if (firstNullAllocator)
        {
            allocatorPools[firstNullIdx] = new SlotAllocatorType();
            return firstNullIdx;
        }
        else
        {
            allocatorPools.emplace_back(new SlotAllocatorType());
            allocValidity.add(SlotAllocatorType::Count);
            slotGeneration.resize(slotGeneration.size() + SlotAllocatorType::Count, 0);
            return allocatorPools.size() - 1;
        }
    }

    FORCE_INLINE void onFree(SizeT poolIdx)
    {
        // Only if allocator has nothing allocated
        if (allocatorPools[poolIdx]->empty())
        {
            // If there is already an empty allocator and it is empty?
            if (emptyPool && emptyPool->empty())
            {
                delete emptyPool;
                delete allocatorPools[poolIdx];
                allocatorPools[emptyPoolIdx] = nullptr;
                allocatorPools[poolIdx] = nullptr;
            }
            else
            {
                emptyPool = allocatorPools[poolIdx];
                emptyPoolIdx = poolIdx;
            }
            // Since we are deleting these pools we should be fine reseting it's generations as well
            resetGenerations(poolIdx);
        }
    }

    FORCE_INLINE void resetGenerations(SizeT poolIdx)
    {
        SizeT firstAllocIdx = slotIdxToAllocIdx(0, poolIdx);
        debugAssert(slotGeneration.size() > firstAllocIdx && slotGeneration.size() > (firstAllocIdx + SlotAllocatorType::Count));
        CBEMemory::memZero(&slotGeneration[firstAllocIdx], sizeof(SlotGenerationIdxType) * SlotAllocatorType::Count);
    }
};
