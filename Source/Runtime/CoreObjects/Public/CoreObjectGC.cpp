/*!
 * \file CoreObjectGC.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CoreObjectGC.h"
#include "GCReferenceCollector.h"
#include "CBEObject.h"
#include "CBEPackage.h"
#include "CBEObjectTypes.h"
#include "CoreObjectsModule.h"
#include "PropertyVisitorHelpers.h"
#include "Property/PropertyHelper.h"
#include "Visitors/FieldVisitors.h"

namespace cbe
{
void INTERNAL_destroyCBEObject(cbe::Object *obj);
}

uint64 CoreObjectGC::deleteObject(cbe::Object *obj) const
{
    CoreObjectsDB &objsDb = CoreObjectsModule::objectsDB();
    if (objsDb.hasObject(obj->getDbIdx()))
    {
        // Deleting obj and its sub objects
        std::vector<cbe::Object *> subObjs;
        subObjs.emplace_back(obj);
        objsDb.getSubobjects(subObjs, obj->getDbIdx());
        // Need to reverse so that children will be destroyed before parent
        for (auto rItr = subObjs.crbegin(); rItr != subObjs.crend(); ++rItr)
        {
            cbe::INTERNAL_destroyCBEObject(*rItr);
        }
        return subObjs.size();
    }
    return 0;
}

void CoreObjectGC::collectFromRefCollectors(TickRep &budgetTicks)
{
    debugAssert(state == EGCState::Collecting);

    StopWatch collectionSW;

    CoreObjectsDB &objsDb = CoreObjectsModule::objectsDB();

    std::vector<cbe::Object *> objects;
    std::vector<cbe::Object *> markedDelete;
    for (IReferenceCollector *refCollector : refCollectors)
    {
        objects.resize(0);
        refCollector->collectReferences(objects);
        markedDelete.reserve(objects.size());

        for (cbe::Object *obj : objects)
        {
            cbe::ObjectPrivateDataView objDatV = objsDb.getObjectData(obj->getDbIdx());
            if (BIT_SET(objDatV.flags, cbe::EObjectFlagBits::ObjFlag_MarkedForDelete))
            {
                markedDelete.emplace_back(obj);
            }
            else
            {
                // Assumes that caller ensures that objUsedFlags are populated and available
                // throughout collection
                objUsedFlags[objDatV.clazz][objDatV.allocIdx] = true;
            }
        }

        refCollector->clearReferences(markedDelete);
        markedDelete.resize(0);
    }

    collectionSW.stop();
    budgetTicks -= collectionSW.durationTick();
#if COREOBJCTGC_METRICS
    gcRefCollectorsTicks += collectionSW.durationTick();
#endif
}

void CoreObjectGC::markObjectsAsValid(TickRep &budgetTicks)
{
    debugAssert(state == EGCState::Collecting);
    StopWatch nonTransientMarker;

    const CoreObjectsDB &objsDb = CoreObjectsModule::objectsDB();
    for (CBEClass clazz : classesLeft)
    {
        BitArray<uint64> &classObjsFlag = objUsedFlags[clazz];

        auto allocatorItr = gCBEObjectAllocators->find(clazz);
        debugAssert(allocatorItr != gCBEObjectAllocators->end());

        for (cbe::Object *obj : allocatorItr->second->getAllObjects<cbe::Object>())
        {
            cbe::ObjectPrivateDataView objDatV = objsDb.getObjectData(obj->getDbIdx());

            // Only mark as valid if object not marked for delete already and
            // If object is marked explicitly as root or default then we must not delete it
            if (BIT_NOT_SET(objDatV.flags, cbe::EObjectFlagBits::ObjFlag_MarkedForDelete)
                && ANY_BIT_SET(objDatV.flags, cbe::EObjectFlagBits::ObjFlag_RootObject | cbe::EObjectFlagBits::ObjFlag_Default))
            {
                classObjsFlag[objDatV.allocIdx] = true;
            }
        }
    }
    // Mark all packages as valid if it has any subobject
    {
        alertOnce(gCBEObjectAllocators->contains(cbe::Package::staticType()));
        BitArray<uint64> &packagesFlag = objUsedFlags[cbe::Package::staticType()];
        for (cbe::Package *package : (*gCBEObjectAllocators)[cbe::Package::staticType()]->getAllObjects<cbe::Package>())
        {
            cbe::ObjectPrivateDataView packageDatV = objsDb.getObjectData(package->getDbIdx());
            debugAssertf(packageDatV.path == packageDatV.name, "Package name is not same as Package full path below logic will fail!");
            if (BIT_NOT_SET(packageDatV.flags, cbe::EObjectFlagBits::ObjFlag_MarkedForDelete) && objsDb.hasChild(package->getDbIdx()))
            {
                packagesFlag[packageDatV.allocIdx] = true;
            }
        }
    }

    nonTransientMarker.stop();
    budgetTicks -= nonTransientMarker.durationTick();
#if COREOBJCTGC_METRICS
    gcMarkNonTransientTicks += nonTransientMarker.durationTick();
#endif
}

void CoreObjectGC::clearUnused(TickRep &budgetTicks)
{
    debugAssert(state == EGCState::Clearing);

    if (budgetTicks <= 0)
    {
        return;
    }

    StopWatch clearSW;
    while (!classesLeft.empty())
    {
        const BitArray<uint64> &objFlags = objUsedFlags[classesLeft.back()];
        auto allocatorItr = gCBEObjectAllocators->find(classesLeft.back());
        if (allocatorItr != gCBEObjectAllocators->end())
        {
            cbe::ObjectAllocatorBase *allocator = allocatorItr->second;

            cbe::ObjectAllocatorBase::AllocIdx allocIdx = 0;
            for (bool bSet : objFlags)
            {
                // If valid and not used then we delete it
                if (allocator->isValid(allocIdx) && !bSet)
                {
                    lastClearCount += deleteObject(allocator->getAt<cbe::Object>(allocIdx));
                }
                allocIdx++;
            }
        }
        classesLeft.pop_back();

        budgetTicks -= clearSW.thisLapTick();
        clearSW.lap();
        if (budgetTicks <= 0)
        {
#if COREOBJCTGC_METRICS
            gcClearTicks += clearSW.durationTick();
#endif
            return;
        }
    }

    state = EGCState::NewGC;
#if COREOBJCTGC_METRICS
    gcClearTicks += clearSW.durationTick();
#endif
}

void CoreObjectGC::startNewGC(TickRep &budgetTicks)
{
    if (gCBEObjectAllocators == nullptr)
    {
        return;
    }

    objUsedFlags.clear();
    classesLeft.clear();
    objUsedFlags.reserve(gCBEObjectAllocators->size());
    classesLeft.reserve(gCBEObjectAllocators->size());
    for (const std::pair<CBEClass const, cbe::ObjectAllocatorBase *> &classAllocator : *gCBEObjectAllocators)
    {
        objUsedFlags[classAllocator.first].resize(classAllocator.second->size());
        classesLeft.emplace_back(classAllocator.first);
    }

    state = EGCState::Collecting;
    markObjectsAsValid(budgetTicks);
    collectFromRefCollectors(budgetTicks);
    collectObjects(budgetTicks);
    // If collecting is done within the given budget start clear as last step for this gc
    if (state == EGCState::Clearing)
    {
        clearUnused(budgetTicks);
    }
}

void CoreObjectGC::purgeAll()
{
    std::vector<cbe::Object *> allObjs;
    CoreObjectsDB &objsDb = CoreObjectsModule::objectsDB();
    objsDb.getAllObjects(allObjs);

    for (auto objRItr = allObjs.crbegin(); objRItr != allObjs.crend(); ++objRItr)
    {
        cbe::Object *obj = *objRItr;
        EObjectFlags &flags = cbe::INTERNAL_ObjectCoreAccessors::getFlags(obj);
        SET_BITS(flags, cbe::EObjectFlagBits::ObjFlag_GCPurge | cbe::EObjectFlagBits::ObjFlag_MarkedForDelete);
        if (BIT_NOT_SET(flags, cbe::EObjectFlagBits::ObjFlag_Default))
        {
            cbe::INTERNAL_destroyCBEObject(obj);
        }
    }
    objsDb.clear();
}

void CoreObjectGC::collect(TickRep budgetTicks)
{
    // recursively calling until budget depletes or until new gc is started
    if (budgetTicks <= 0)
    {
        return;
    }

    switch (state)
    {
    case CoreObjectGC::EGCState::NewGC:
        startNewGC(budgetTicks);
        break;
    case CoreObjectGC::EGCState::Collecting:
        collectObjects(budgetTicks);
        collect(budgetTicks);
        break;
    case CoreObjectGC::EGCState::Clearing:
        clearUnused(budgetTicks);
        collect(budgetTicks);
        break;
    default:
        break;
    }
}

void CoreObjectGC::registerReferenceCollector(IReferenceCollector *collector)
{
    auto itr = std::find(refCollectors.cbegin(), refCollectors.cend(), collector);
    if (itr == refCollectors.cend())
    {
        refCollectors.emplace_back(collector);
    }
}

void CoreObjectGC::unregisterReferenceCollector(IReferenceCollector *collector)
{
    auto itr = std::find(refCollectors.begin(), refCollectors.end(), collector);
    if (itr != refCollectors.end())
    {
        std::iter_swap(itr, refCollectors.end() - 1);
        refCollectors.pop_back();
    }
}

//////////////////////////////////////////////////////////////////////////
// Collection code to collect references from reflected fields
//////////////////////////////////////////////////////////////////////////

struct GCObjectVisitableUserData
{
    std::unordered_map<CBEClass, BitArray<uint64>> *objUsedFlags;
    const CoreObjectsDB &objsDb = CoreObjectsModule::objectsDB();
    // Object we are inside, This is to ignore adding reference to itself
    cbe::Object *thisObj = nullptr;
    void *pNext = nullptr;
};

struct GCObjectFieldVisitable
{
    // Ignore fundamental and special types, we need none const custom types or pointers
    template <typename Type>
    static void visit(Type *, const PropertyInfo &, void *)
    {}
    static void visit(void *val, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        switch (prop->type)
        {
        case EPropertyType::MapType:
        {
            PropertyVisitorHelper::visitEditMapEntriesPtrOnly<GCObjectFieldVisitable>(
                static_cast<const MapProperty *>(prop), val, propInfo, userData
            );
            break;
        }
        case EPropertyType::SetType:
        {
            PropertyVisitorHelper::visitEditSetEntries<GCObjectFieldVisitable>(
                static_cast<const ContainerProperty *>(prop), val, propInfo, userData
            );
            break;
        }
        case EPropertyType::ArrayType:
        {
            const IterateableDataRetriever *dataRetriever
                = static_cast<const IterateableDataRetriever *>(static_cast<const ContainerProperty *>(prop)->dataRetriever);
            const TypedProperty *elemProp = static_cast<const TypedProperty *>(static_cast<const ContainerProperty *>(prop)->elementProp);

            for (auto itrPtr = dataRetriever->createIterator(val); itrPtr->isValid(); itrPtr->iterateFwd())
            {
                FieldVisitor::visit<GCObjectFieldVisitable>(elemProp, itrPtr->getElement(), userData);
            }
            break;
        }
        case EPropertyType::PairType:
        {
            const PairDataRetriever *dataRetriever
                = static_cast<const PairDataRetriever *>(static_cast<const PairProperty *>(prop)->dataRetriever);
            const TypedProperty *keyProp = static_cast<const TypedProperty *>(static_cast<const PairProperty *>(prop)->keyProp);
            const TypedProperty *valueProp = static_cast<const TypedProperty *>(static_cast<const PairProperty *>(prop)->valueProp);

            void *keyPtr = dataRetriever->first(val);
            void *valPtr = dataRetriever->second(val);

            FieldVisitor::visit<GCObjectFieldVisitable>(keyProp, keyPtr, userData);
            FieldVisitor::visit<GCObjectFieldVisitable>(valueProp, valPtr, userData);
            break;
        }
        case EPropertyType::ClassType:
        {
            CBEClass clazz = static_cast<CBEClass>(prop);
            debugAssert(PropertyHelper::isStruct(clazz));
            FieldVisitor::visitFields<GCObjectFieldVisitable>(clazz, val, userData);
            break;
        }
        case EPropertyType::EnumType:
            break;
        }
    }
    // Ignoring const types
    static void visit(const void *, const PropertyInfo &, void *) {}
    static void visit(void **ptr, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        switch (prop->type)
        {
        case EPropertyType::ClassType:
        {
            debugAssert(PropertyHelper::isChildOf(static_cast<CBEClass>(prop), cbe::Object::staticType()));

            GCObjectVisitableUserData *gcUserData = (GCObjectVisitableUserData *)(userData);
            cbe::Object **objPtrPtr = reinterpret_cast<cbe::Object **>(ptr);
            cbe::Object *objPtr = *objPtrPtr;
            if (objPtr && objPtr != gcUserData->thisObj)
            {
                cbe::ObjectPrivateDataView objDatV = gcUserData->objsDb.getObjectData(objPtr->getDbIdx());
                // No need to check if Deleted flag as that happens only when no references where found
                if (BIT_SET(objDatV.flags, cbe::EObjectFlagBits::ObjFlag_MarkedForDelete))
                {
                    (*objPtrPtr) = nullptr;
                }
                else
                {
                    (*gcUserData->objUsedFlags)[objDatV.clazz][objDatV.allocIdx] = true;
                }
            }
            break;
        }
        case EPropertyType::EnumType:
        case EPropertyType::MapType:
        case EPropertyType::SetType:
        case EPropertyType::ArrayType:
        case EPropertyType::PairType:
        default:
            alertAlwaysf(
                false, "Unhandled ptr to ptr Field name {}, type {}", propInfo.fieldProperty->nameString, *propInfo.thisProperty->typeInfo
            );
            break;
        }
    }
    static void visit(const void **ptr, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        switch (propInfo.thisProperty->type)
        {
        case EPropertyType::ClassType:
        {
            debugAssert(PropertyHelper::isChildOf(static_cast<CBEClass>(prop), cbe::Object::staticType()));

            GCObjectVisitableUserData *gcUserData = (GCObjectVisitableUserData *)(userData);
            const cbe::Object **objPtrPtr = reinterpret_cast<const cbe::Object **>(ptr);
            const cbe::Object *objPtr = *objPtrPtr;
            if (objPtr && objPtr != gcUserData->thisObj)
            {
                cbe::ObjectPrivateDataView objDatV = gcUserData->objsDb.getObjectData(objPtr->getDbIdx());
                if (BIT_SET(objDatV.flags, cbe::EObjectFlagBits::ObjFlag_MarkedForDelete))
                {
                    (*objPtrPtr) = nullptr;
                }
                else
                {
                    (*gcUserData->objUsedFlags)[objDatV.clazz][objDatV.allocIdx] = true;
                }
            }
            break;
        }
        case EPropertyType::EnumType:
        case EPropertyType::MapType:
        case EPropertyType::SetType:
        case EPropertyType::ArrayType:
        case EPropertyType::PairType:
        default:
            alertAlwaysf(
                false, "Unhandled ptr to const ptr Field name {}, type {}", propInfo.fieldProperty->nameString, *propInfo.thisProperty->typeInfo
            );
            break;
        }
    }
};

void CoreObjectGC::collectObjects(TickRep &budgetTicks)
{
    debugAssert(state == EGCState::Collecting);

    if (budgetTicks <= 0)
    {
        return;
    }

    StopWatch collectionSW;

    GCObjectVisitableUserData userData{ .objUsedFlags = &objUsedFlags };
    while (!classesLeft.empty())
    {
        auto allocatorItr = gCBEObjectAllocators->find(classesLeft.back());
        debugAssert(allocatorItr != gCBEObjectAllocators->end());
        // Collect
        {
            cbe::ObjectAllocatorBase *allocator = allocatorItr->second;

            // Right now we are only going through static fields of classes that has object
            // created from it. We are not using static in struct as well Create a separate
            // collection pass to collect from static field of all class properties in
            // cbe::Object hierarchy However storing referenced object in statics is not wise so
            // we do only as below or we could never scan any statics?
            FieldVisitor::visitStaticFields<GCObjectFieldVisitable>(classesLeft.back(), &userData);

            for (cbe::Object *obj : allocator->getAllObjects<cbe::Object>())
            {
                if (BIT_NOT_SET(userData.objsDb.getObjectData(obj->getDbIdx()).flags, cbe::EObjectFlagBits::ObjFlag_MarkedForDelete))
                {
                    userData.thisObj = obj;
                    FieldVisitor::visitFields<GCObjectFieldVisitable>(classesLeft.back(), obj, &userData);
                }
            }
            userData.thisObj = nullptr;
        }
        classesLeft.pop_back();

        budgetTicks -= collectionSW.thisLapTick();
        collectionSW.lap();
        if (budgetTicks <= 0)
        {
#if COREOBJCTGC_METRICS
            gcCollectionTicks += collectionSW.durationTick();
#endif
            return;
        }
    }

    lastClearCount = 0;
    state = EGCState::Clearing;
    // Setup classes left for clearing
    classesLeft.reserve(objUsedFlags.size());
    for (const auto &keyVal : objUsedFlags)
    {
        classesLeft.emplace_back(keyVal.first);
    }
    budgetTicks -= collectionSW.thisLapTick();

#if COREOBJCTGC_METRICS
    gcCollectionTicks += collectionSW.durationTick();
#endif
}