/*!
 * \file StaclAllocator.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Memory/StackAllocator.h"
#include "Types/Platform/Threading/PlatformThreading.h"

StackAllocatorUnaligned<ThreadSharing_Shared>::StackAllocatorUnaligned()
    : byteSize(Traits::INITIAL_STACK_SIZE)
{
    bool bTlsSlotCreated = PlatformThreadingFunctions::createTlsSlot(tlsSlot);
    fatalAssert(bTlsSlotCreated);
    allStackAllocators.emplace_back(nullptr);
}
StackAllocatorUnaligned<ThreadSharing_Shared>::StackAllocatorUnaligned(SizeType stackByteSize)
    : byteSize(stackByteSize)
{
    debugAssert(stackByteSize > 0);

    bool bTlsSlotCreated = PlatformThreadingFunctions::createTlsSlot(tlsSlot);
    fatalAssert(bTlsSlotCreated);
    allStackAllocators.emplace_back(nullptr);
}

StackAllocatorUnaligned<ThreadSharing_Shared>::~StackAllocatorUnaligned()
{
    std::unique_lock lock(allAllocatorsLock);
    for (PerThreadData *tlData : allStackAllocators)
    {
        bool bActive = true;
        while (bActive)
        {
            // If exchange succeeded it is not deactivated already, thread exit has happened after allocator delete
            if (tlData->bIsActive.compare_exchange_weak(bActive, false, std::memory_order::acq_rel))
            {
                // bActive will be true now since CAS will not load the existing value if succeeded
                break;
            }
        }
        if (!bActive)
        { // Thread exit has happened before allocator delete. So delete thread local data here
            delete tlData;
        }
    }
    allStackAllocators.clear();
    PlatformThreadingFunctions::releaseTlsSlot(tlsSlot);
}

StackAllocatorUnaligned<ThreadSharing_Shared>::PerThreadData &StackAllocatorUnaligned<ThreadSharing_Shared>::getThreadData()
{
    PerThreadData *tlData = (PerThreadData *)PlatformThreadingFunctions::getTlsSlotValue(tlsSlot);
    if (tlData == nullptr)
    {
        tlData = createNewThreadData();
    }
    return *tlData;
}
StackAllocatorUnaligned<ThreadSharing_Shared>::PerThreadData *StackAllocatorUnaligned<ThreadSharing_Shared>::getThreadData() const
{
    return (PerThreadData *)PlatformThreadingFunctions::getTlsSlotValue(tlsSlot);
}

StackAllocatorUnaligned<ThreadSharing_Shared>::PerThreadData *StackAllocatorUnaligned<ThreadSharing_Shared>::createNewThreadData()
{
    PerThreadData *tlData = new PerThreadData{ .allocator = { byteSize } };
    bool bTlsSlotSet = PlatformThreadingFunctions::setTlsSlotValue(tlsSlot, tlData);
    fatalAssert(bTlsSlotSet);
    tlData = (PerThreadData *)PlatformThreadingFunctions::getTlsSlotValue(tlsSlot);
    {
        std::unique_lock lock(allAllocatorsLock);
        allStackAllocators.emplace_back(tlData);
    }
    PlatformThreadingFunctions::atThreadExit(
        [tlData]()
        {
            bool bActive = true;
            while (bActive)
            {
                // If exchange succeeded it is not deactivated already, thread exit has happened before allocator delete
                if (tlData->bIsActive.compare_exchange_weak(bActive, false, std::memory_order::acq_rel))
                {
                    return;
                }
            }
            // Thread exit has happened after allocator delete. So delete thread local data here
            delete tlData;
        }
    );
    return tlData;
}