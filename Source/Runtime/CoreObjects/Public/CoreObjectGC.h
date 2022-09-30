/*!
 * \file CoreObjectGC.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CBEObjectTypes.h"
#include "CoreObjectsExports.h"
#include "Types/Containers/BitArray.h"
#include "Types/Time.h"

#include <unordered_map>
#include <vector>

namespace cbe
{
class Object;
}
class IReferenceCollector;

#define COREOBJCTGC_METRICS DEV_BUILD

// Garbage collection proceeds through the each class's object allocators and collects and clears
class CoreObjectGC
{
private:
    enum class EGCState
    {
        NewGC,      // Fresh GC all data will be gathered from beginning
        Collecting, // Collection is in progress from each objects
        Clearing    // Collection is finished now clearing based on collection results
    };

    // number of objects cleared during last clear
    uint64 lastClearCount = 0;
    // Maps directly to cbe::ObjectAllocatorBase's allocValidity indices and holds true if an object is
    // referenced, if not referenced it will be destroyed If marked for destroy it will be unreferenced
    // and destroyed
    std::unordered_map<CBEClass, BitArray<uint64>> objUsedFlags;
    // In collecting state classes that left to be crawled and collected, In clearing stage classes that
    // are not cleared yet
    std::vector<CBEClass> classesLeft;
    EGCState state = EGCState::NewGC;

    std::vector<IReferenceCollector *> refCollectors;

#if COREOBJCTGC_METRICS
    TickRep gcRefCollectorsTicks = 0;
    TickRep gcMarkNonTransientTicks = 0;
    TickRep gcCollectionTicks = 0;
    TickRep gcClearTicks = 0;
#endif

private:
    uint64 deleteObject(cbe::Object *obj) const;

    void collectFromRefCollectors(TickRep &budgetTicks);
    void markObjectsAsValid(TickRep &budgetTicks);
    void collectObjects(TickRep &budgetTicks);
    void clearUnused(TickRep &budgetTicks);
    void startNewGC(TickRep &budgetTicks);

public:
    /**
     * CoreObjectGC::collect Garbage collects objects and stores a context for next collection if
     * collection exceeds time budget
     *
     * Access: public
     *
     * @param TimeConvType budget - in seconds
     *
     * @return void
     */
    void collect(TimeConvType budget)
    {
        TickRep budgetTicks = std::numeric_limits<TickRep>::max();
        if (budget > 0.0f)
        {
            budgetTicks = Time::fromSeconds(budget);
        }
        collect(budgetTicks);
    }
    COREOBJECTS_EXPORT void collect(TickRep budgetTicks);

    FORCE_INLINE bool isGcComplete() const { return state == EGCState::NewGC; }
    FORCE_INLINE uint64 getLastClearCount() const { return lastClearCount; }

    COREOBJECTS_EXPORT void registerReferenceCollector(IReferenceCollector *collector);
    COREOBJECTS_EXPORT void unregisterReferenceCollector(IReferenceCollector *collector);
};
