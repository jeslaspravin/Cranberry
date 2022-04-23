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

class ClassProperty;
namespace CBE
{
class ObjectAllocatorBase;
}

extern COREOBJECTS_EXPORT std::unordered_map<CBEClass, CBE::ObjectAllocatorBase *> *gCBEObjectAllocators;

namespace CBE
{
class COREOBJECTS_EXPORT ObjectAllocatorBase
{
protected:
    // We are not going to have same object type more than max of uint32
    BitArray<uint32> allocValidity;

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
                retVal.emplace_back((AsType *)getAllocAt(itr - beginItr));
            }
        }
        return retVal;
    }
    FORCE_INLINE bool isValid(AllocIdx idx) const { return allocValidity[idx]; }
};

// #TODO(Jeslas) : Find a way while freeing to validate proper revision of same allocated ptr free. Now
// we can avoid that by controlling free manually
template <typename ClassType>
class ObjectAllocator final : public ObjectAllocatorBase
{
private:
    // 64 is good enough value?
    using SlotAllocatorType = SlotAllocator<sizeof(ClassType), alignof(ClassType), 64>;
    using SlotIdxType = typename SlotAllocatorType::SizeType;

    std::vector<SlotAllocatorType *> allocators;

    // If we get at least 2 empty allocators then we clear them both, This is to avoid frequent delete on
    // frequent add and deletes
    SlotAllocatorType *emptyAllocator = nullptr;
    SizeT emptyIdx = 0;

    SizeT lastAllocatedCache = 0;
    AllocIdx defaultAllocIdx = 0;

private:
    bool lastAllocatedCacheValid() const { return lastAllocatedCache < allocators.size() && allocators[lastAllocatedCache]; }
    FORCE_INLINE static SizeT allocIdxToSlotIdx(SlotIdxType &slotIdx, AllocIdx allocIdx)
    {
        slotIdx = (allocIdx % SlotAllocatorType::Size);
        return (allocIdx / SlotAllocatorType::Size);
    }
    FORCE_INLINE static AllocIdx slotIdxToAllocIdx(SlotIdxType slotIdx, SizeT arrayIdx)
    {
        return (AllocIdx)(arrayIdx * SlotAllocatorType::Size + slotIdx);
    }

    FORCE_INLINE SizeT findAllocator();

    FORCE_INLINE void onFree(SizeT arrayIdx)
    {
        // Only if allocator has nothing allocated
        if (allocators[arrayIdx]->empty())
        {
            // If there is already an empty allocator and it is empty?
            if (emptyAllocator && emptyAllocator->empty())
            {
                delete emptyAllocator;
                delete allocators[arrayIdx];
                allocators[emptyIdx] = nullptr;
                allocators[arrayIdx] = nullptr;
            }
            else
            {
                emptyAllocator = allocators[arrayIdx];
                emptyIdx = arrayIdx;
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
        emptyAllocator = nullptr;
        for (SlotAllocatorType *slotAlloc : allocators)
        {
            if (slotAlloc)
            {
                delete slotAlloc;
            }
        }
        allocators.clear();
    }

    /* ObjectAllocatorBase overrides */
protected:
    void *getAllocAt(AllocIdx idx) const override
    {
        SlotIdxType slotIdx;
        SizeT arrayIdx = allocIdxToSlotIdx(slotIdx, idx);

        debugAssert(isValid(idx) && allocators.size() > arrayIdx && allocators[arrayIdx]);
        return allocators[arrayIdx]->at(slotIdx);
    }

public:
    void *getDefault() override { return getAllocAt(defaultAllocIdx); }
    void *allocate(AllocIdx &outAllocIdx) override
    {
        SizeT allocateFrom = lastAllocatedCacheValid() ? lastAllocatedCache : findAllocator();
        void *ptr = allocators[allocateFrom]->memAlloc(SlotAllocatorType::SlotSize);
        SlotIdxType slotIdx = allocators[allocateFrom]->ptrToSlotIdx(ptr);

        outAllocIdx = slotIdxToAllocIdx(slotIdx, allocateFrom);
        allocValidity[outAllocIdx] = true; // Marking this alloc bit as allocated
        lastAllocatedCache = allocateFrom;
        return ptr;
    }
    void free(void *ptr, AllocIdx allocIdx) override
    {
        if (ptr != getAllocAt(allocIdx))
            return;
        // Double freeing?
        debugAssert(isValid(allocIdx));

        SlotIdxType slotIdx;
        SizeT arrayIdx = allocIdxToSlotIdx(slotIdx, allocIdx);
        allocators[arrayIdx]->memFree(ptr);
        allocValidity[allocIdx] = false;

        onFree(arrayIdx);
    }
    void free(void *ptr) override
    {
        SizeT arrayIdx;
        SlotAllocatorType *ptrAllocator = nullptr;
        if (lastAllocatedCacheValid() && allocators[lastAllocatedCache]->isOwningMemory(ptr))
        {
            ptrAllocator = allocators[lastAllocatedCache];
            arrayIdx = lastAllocatedCache;
        }
        else
        {
            SizeT idx = 0;
            for (SlotAllocatorType *allocator : allocators)
            {
                if (allocator && allocator->isOwningMemory(ptr))
                {
                    ptrAllocator = allocator;
                    arrayIdx = idx;
                    break;
                }
                ++idx;
            }
        }
        if (ptrAllocator)
        {
            AllocIdx allocIdx = slotIdxToAllocIdx(ptrAllocator->ptrToSlotIdx(ptr), arrayIdx);
            // Double freeing?
            debugAssert(isValid(allocIdx));

            ptrAllocator->memFree(ptr);
            allocValidity[allocIdx] = false;
            onFree(arrayIdx);
        }
    }
    /* Overrides ends */
};
template <typename ClassType>
FORCE_INLINE SizeT ObjectAllocator<ClassType>::findAllocator()
{
    // Allocator that is nullptr in allocators vector
    SlotAllocatorType **firstNullAllocator = nullptr;
    SizeT firstNullIdx = allocators.size();

    auto endItr = allocValidity.cend();
    auto beginItr = allocValidity.cbegin();
    for (auto itr = beginItr; itr != endItr; ++itr)
    {
        if (!(*itr))
        {
            SlotIdxType slotIdx;
            SizeT arrayIdx = allocIdxToSlotIdx(slotIdx, (itr - beginItr));
            if (allocators[arrayIdx])
            {
                return arrayIdx;
            }
            else
            {
                firstNullIdx = arrayIdx;
                firstNullAllocator = allocators.data() + arrayIdx;
            }
        }
    }
    // No free slot found, create new
    if (firstNullAllocator)
    {
        allocators[firstNullIdx] = new SlotAllocatorType();
        return firstNullIdx;
    }
    else
    {
        allocators.emplace_back(new SlotAllocatorType());
        allocValidity.add(SlotAllocatorType::Size);
        return allocators.size() - 1;
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
} // namespace CBE