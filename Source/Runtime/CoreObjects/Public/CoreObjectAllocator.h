/*!
 * \file CoreObjectAllocator.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CBEObjectTypes.h"
#include "CoreObjectsExports.h"
#include "Memory/SlotAllocator.h"
#include "Types/Containers/BitArray.h"

#include <unordered_map>

namespace cbe
{
class ObjectAllocatorBase;
}

extern COREOBJECTS_EXPORT std::unordered_map<CBEClass, cbe::ObjectAllocatorBase *> *gCBEObjectAllocators;

namespace cbe
{
class COREOBJECTS_EXPORT ObjectAllocatorBase
{
protected:
    BitArray<uint64> allocValidity;

public:
    using AllocIdx = ObjectAllocIdx;

protected:
    void constructDefault(void *objPtr, AllocIdx allocIdx, CBEClass clazz) const;

    virtual void *getAllocAt(AllocIdx idx) const = 0;

public:
    virtual ~ObjectAllocatorBase() = default;
    virtual void *getDefault() = 0;
    virtual void *allocate(AllocIdx &outAllocIdx) = 0;
    virtual void free(void *ptr, AllocIdx allocIdx) = 0;
    virtual void free(void *ptr) = 0;

    AllocIdx size() const { return static_cast<AllocIdx>(allocValidity.size()); }
    template <typename AsType>
    AsType *getAt(AllocIdx idx) const
    {
        return (AsType *)(getAllocAt(idx));
    }
    template <typename AsType>
    std::vector<AsType *> getAllObjects() const
    {
        std::vector<AsType *> retVal;
        retVal.reserve(allocValidity.size());

        auto endItr = allocValidity.cend();
        auto beginItr = allocValidity.cbegin();
        for (auto itr = beginItr; itr != endItr; ++itr)
        {
            if (*itr)
            {
                retVal.emplace_back((AsType *)getAllocAt(AllocIdx(itr - beginItr)));
            }
        }
        return retVal;
    }
    FORCE_INLINE bool isValid(AllocIdx idx) const { return allocValidity[idx]; }
};

/**
 * SlotAllocatorTraits based on ClassType to allow overriding slot count at per class level
 * Class is considered to be overriding slot count if it has AllocSlotCount static constexpr value in it and it is greater than 1(default value
 * + at least one instance)
 *
 * Some private members are copied to PoolAllocator.h's PoolAllocator, If any bug fixes were made do the same there as well!
 */
template <typename ClassType>
concept ClassOverridesSlotCount = requires
{
    {
        ClassType::AllocSlotCount
        } -> std::convertible_to<typename SlotAllocator<sizeof(ClassType), alignof(ClassType), 64, true>::SizeType>;
    ClassType::AllocSlotCount > 1;
};
template <typename ClassType>
struct SlotAllocatorTraits
{
    // 64 is good enough value?
    using AllocType = SlotAllocator<sizeof(ClassType), alignof(ClassType), 64, true>;
    using SlotIdxType = typename AllocType::SizeType;
};
template <ClassOverridesSlotCount ClassType>
struct SlotAllocatorTraits<ClassType>
{
    constexpr static const decltype(ClassType::AllocSlotCount) SlotCount = ClassType::AllocSlotCount;
    using AllocType = SlotAllocator<sizeof(ClassType), alignof(ClassType), SlotCount, true>;
    using SlotIdxType = typename AllocType::SizeType;
};

// NOTE : Find a way while freeing to validate proper revision of same allocated ptr free. Now
// we can avoid that by controlling free manually
template <typename ClassType>
class ObjectAllocator final : public ObjectAllocatorBase
{
private:
    using SlotAllocTraits = SlotAllocatorTraits<ClassType>;
    using SlotAllocatorType = typename SlotAllocTraits::AllocType;
    using SlotIdxType = typename SlotAllocTraits::SlotIdxType;

    std::vector<SlotAllocatorType *> allocatorPools;

    // If we get at least 2 empty allocators then we clear them both, This is to avoid frequent delete on
    // frequent add and deletes
    SlotAllocatorType *emptyPool = nullptr;
    SizeT emptyPoolIdx = 0;

    SizeT lastAllocPoolCache = 0;
    AllocIdx defaultAllocIdx = 0;

private:
    bool lastAllocatedCacheValid() const { return lastAllocPoolCache < allocatorPools.size() && allocatorPools[lastAllocPoolCache]; }
    FORCE_INLINE static SizeT allocIdxToSlotIdx(SlotIdxType &slotIdx, AllocIdx allocIdx)
    {
        slotIdx = (allocIdx % SlotAllocatorType::Count);
        return (allocIdx / SlotAllocatorType::Count);
    }
    FORCE_INLINE static AllocIdx slotIdxToAllocIdx(SlotIdxType slotIdx, SizeT poolIdx)
    {
        return (AllocIdx)(poolIdx * SlotAllocatorType::Count + slotIdx);
    }

    FORCE_INLINE SizeT findAllocator();

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
                emptyPool = nullptr;
            }
            else
            {
                emptyPool = allocatorPools[poolIdx];
                emptyPoolIdx = poolIdx;
            }
        }
    }

public:
    ObjectAllocator()
    {
        // Directly calling allocate and object construction routine to skip getting allocator that happens when constructing using
        // CBEObjectConstructionPolicy
        ClassType *objPtr = (ClassType *)allocate(defaultAllocIdx);
        CBEMemory::memZero(objPtr, sizeof(ClassType));
        constructDefault(objPtr, defaultAllocIdx, ClassType::staticType());
    }

    ~ObjectAllocator()
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
    }

    /* ObjectAllocatorBase overrides */
protected:
    void *getAllocAt(AllocIdx idx) const override
    {
        SlotIdxType slotIdx;
        SizeT poolIdx = allocIdxToSlotIdx(slotIdx, idx);

        debugAssert(isValid(idx) && allocatorPools.size() > poolIdx && allocatorPools[poolIdx]);
        return allocatorPools[poolIdx]->at(slotIdx);
    }

public:
    void *getDefault() override { return getAllocAt(defaultAllocIdx); }
    void *allocate(AllocIdx &outAllocIdx) override
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

        outAllocIdx = slotIdxToAllocIdx(slotIdx, allocateFrom);
        allocValidity[outAllocIdx] = true; // Marking this alloc bit as allocated
        lastAllocPoolCache = allocateFrom;
        return ptr;
    }
    void free(void *ptr, AllocIdx allocIdx) override
    {
        // Double freeing?
        debugAssert(isValid(allocIdx));
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
            AllocIdx allocIdx = slotIdxToAllocIdx(ptrAllocator->ptrToSlotIdx(ptr), poolIdx);
            // Double freeing?
            debugAssert(isValid(allocIdx));

            ptrAllocator->memFree(ptr);
            allocValidity[allocIdx] = false;
            onFree(poolIdx);
        }
    }
    /* Overrides ends */
};
template <typename ClassType>
FORCE_INLINE SizeT ObjectAllocator<ClassType>::findAllocator()
{
    // Allocator that is nullptr in allocators vector
    SlotAllocatorType **firstNullAllocator = nullptr;
    SizeT firstNullIdx = allocatorPools.size();

    auto endItr = allocValidity.cend();
    auto beginItr = allocValidity.cbegin();
    for (auto itr = beginItr; itr != endItr; ++itr)
    {
        if (!(*itr))
        {
            SlotIdxType slotIdx;
            SizeT poolIdx = allocIdxToSlotIdx(slotIdx, AllocIdx(itr - beginItr));
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
        return allocatorPools.size() - 1;
    }
}

COREOBJECTS_EXPORT void initializeObjectAllocators();

template <typename ClassType>
ObjectAllocator<ClassType> &INTERNAL_createObjAllocator()
{
    static ObjectAllocator<ClassType> objAllocator;
    if (!gCBEObjectAllocators)
    {
        initializeObjectAllocators();
    }
    (*gCBEObjectAllocators)[ClassType::staticType()] = &objAllocator;
    return objAllocator;
}

template <typename ClassType>
ObjectAllocator<ClassType> &getObjAllocator()
{
    static ObjectAllocator<ClassType> &objAllocator = INTERNAL_createObjAllocator<ClassType>();
    return objAllocator;
}

COREOBJECTS_EXPORT ObjectAllocatorBase *getObjAllocator(CBEClass classType);
} // namespace cbe