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

namespace CBE
{
class Object;
}

#define COREOBJCTGC_METRICS DEV_BUILD

class IReferenceCollector
{
public:
    /**
     * IReferenceCollector::clearReferences - Must clear holding references to passed in objects as they
     * will be deleted
     *
     * Access: virtual public
     *
     * @param const std::vector<CBE::Object * > & deletedObjects
     *
     * @return void
     */
    virtual void clearReferences(const std::vector<CBE::Object *> &deletedObjects) = 0;
    virtual void collectReferences(std::vector<CBE::Object *> &outObjects) const = 0;
};

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

    // Maps directly to CBE::ObjectAllocatorBase's allocValidity indices and holds true if an object is
    // referenced, if not referenced it will be detroyed If marked for destroy it will be unreferenced
    // and destroyed
    std::unordered_map<CBEClass, BitArray<uint64>> objUsedFlags;
    // In collecting state classes that left to be crawled and collected, In clearing stage classes that
    // are not cleared yet
    std::vector<CBEClass> classesLeft;
    EGCState state = EGCState::NewGC;

    std::vector<IReferenceCollector *> refCollectors;

#if COREOBJCTGC_METRICS
    TickRep gcRefCollectorsTicks = 0;
    TickRep gcCollectionTicks = 0;
    TickRep gcClearTicks = 0;
#endif

private:
    void deleteObject(CBE::Object *obj) const;

    void collectFromRefCollectors(TickRep &budgetTicks);
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
        TickRep budgetTicks = Time::fromSeconds(budget);
        collect(budgetTicks);
    }
    COREOBJECTS_EXPORT void collect(TickRep budgetTicks);

    COREOBJECTS_EXPORT void registerReferenceCollector(IReferenceCollector *collector);
    COREOBJECTS_EXPORT void unregisterReferenceCollector(IReferenceCollector *collector);
};
