/*!
 * \file CoreObjectGC.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CoreObjectGC.h"
#include "CBEObject.h"
#include "CBEPackage.h"
#include "CBEObjectTypes.h"
#include "CoreObjectsModule.h"
#include "Property/PropertyHelper.h"
#include "Visitors/FieldVisitors.h"

void CoreObjectGC::deleteObject(CBE::Object *obj) const
{
    CoreObjectsDB &objsDb = CoreObjectsModule::objectsDB();
    if (objsDb.hasObject(obj->getStringID()))
    {
        // Deleting obj and its subobjects
        std::vector<CBE::Object *> subObjs;
        objsDb.getSubobjects(subObjs, obj->getStringID());
        subObjs.emplace_back(obj);
        for (CBE::Object *subObj : subObjs)
        {
            CBEClass clazz = subObj->getType();
            obj->destroyObject();
            static_cast<const GlobalFunctionWrapper *>(clazz->destructor->funcPtr)->invokeUnsafe<void>(subObj);
        }
    }
}

void CoreObjectGC::collectFromRefCollectors(TickRep &budgetTicks)
{
    debugAssert(state == EGCState::Collecting);

    StopWatch collectionSW;

    std::vector<CBE::Object *> objects;
    std::vector<CBE::Object *> markedDelete;
    for (IReferenceCollector *refCollector : refCollectors)
    {
        objects.resize(0);
        refCollector->collectReferences(objects);
        markedDelete.reserve(objects.size());

        for (CBE::Object *obj : objects)
        {
            if (BIT_SET(obj->getFlags(), CBE::EObjectFlagBits::MarkedForDelete))
            {
                markedDelete.emplace_back(obj);
            }
            else
            {
                // Assumes that caller ensures that objUsedFlags are populated and available
                // throughout collection
                objUsedFlags[obj->getType()][CBE::PrivateObjectCoreAccessors::getAllocIdx(obj)] = true;
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

    CoreObjectsDB &objsDb = CoreObjectsModule::objectsDB();
    for (CBEClass clazz : classesLeft)
    {
        BitArray<uint64> &classObjsFlag = objUsedFlags[clazz];

        auto allocatorItr = gCBEObjectAllocators->find(classesLeft.back());
        debugAssert(allocatorItr != gCBEObjectAllocators->end());

        for (CBE::Object *obj : allocatorItr->second->getAllObjects<CBE::Object>())
        {
            // Only mark as valid if object not marked for delete already and
            // If object is marked explicitly as root then we must not delete it
            if (BIT_NOT_SET(obj->getFlags(), CBE::EObjectFlagBits::MarkedForDelete)
                && BIT_SET(obj->getFlags(), CBE::EObjectFlagBits::RootObject))
            {
                classObjsFlag[CBE::PrivateObjectCoreAccessors::getAllocIdx(obj)] = true;
            }
        }
    }
    // Mark all packages as valid if it has any subobject
    {
        BitArray<uint64> &packagesFlag = objUsedFlags[CBE::Package::staticType()];
        for (CBE::Package *package : (*gCBEObjectAllocators)[CBE::Package::staticType()]->getAllObjects<CBE::Package>())
        {
            if (BIT_NOT_SET(package->getFlags(), CBE::EObjectFlagBits::MarkedForDelete) && objsDb.hasChild(package->getStringID()))
            {
                packagesFlag[CBE::PrivateObjectCoreAccessors::getAllocIdx(package)] = true;
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
            CBE::ObjectAllocatorBase *allocator = allocatorItr->second;

            CBE::ObjectAllocatorBase::AllocIdx allocIdx = 0;
            for (bool bSet : objFlags)
            {
                // If valid and not used then we delete it
                if (allocator->isValid(allocIdx) && !bSet)
                {
                    deleteObject(allocator->getAt<CBE::Object>(allocIdx));
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
        return;

    objUsedFlags.clear();
    classesLeft.clear();
    objUsedFlags.reserve(gCBEObjectAllocators->size());
    classesLeft.reserve(gCBEObjectAllocators->size());
    for (const std::pair<CBEClass const, CBE::ObjectAllocatorBase *> &classAllocator : *gCBEObjectAllocators)
    {
        objUsedFlags[classAllocator.first].resize(classAllocator.second->size());
        classesLeft.emplace_back(classAllocator.first);
    }

    state = EGCState::Collecting;
    collectFromRefCollectors(budgetTicks);
    collectObjects(budgetTicks);
    // If collecting is done within the given budget start clear as last step for this gc
    if (state == EGCState::Clearing)
    {
        clearUnused(budgetTicks);
    }
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
    // Object we are inside, This is to ignore adding reference to itself
    CBE::Object *thisObj = nullptr;
    void *pNext = nullptr;
};

FORCE_INLINE const TypedProperty *getUnqualified(const BaseProperty *prop)
{
    return static_cast<const TypedProperty *>(
        prop->type == EPropertyType::QualifiedType ? static_cast<const QualifiedProperty *>(prop)->unqualTypeProperty : prop
    );
}

struct GCObjectFieldVisitable
{
    static void visitMap(const MapProperty *mapProp, void *val, const PropertyInfo &propInfo, void *userData);
    static void visitSet(const ContainerProperty *setProp, void *val, const PropertyInfo &propInfo, void *userData);

    // Ignore fundamental and special types, we need none const custom types or pointers
    template <typename Type>
    static void visit(Type *val, const PropertyInfo &propInfo, void *userData)
    {}
    static void visit(void *val, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = getUnqualified(propInfo.thisProperty);
        switch (prop->type)
        {
        case EPropertyType::MapType:
        {
            visitMap(static_cast<const MapProperty *>(prop), val, propInfo, userData);
            break;
        }
        case EPropertyType::SetType:
        {
            visitSet(static_cast<const ContainerProperty *>(prop), val, propInfo, userData);
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
    static void visit(const void *val, const PropertyInfo &propInfo, void *userData) {}
    static void visit(void **ptr, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = getUnqualified(propInfo.thisProperty);
        switch (prop->type)
        {
        case EPropertyType::ClassType:
        {
            debugAssert(PropertyHelper::isChildOf(static_cast<CBEClass>(prop), CBE::Object::staticType()));

            GCObjectVisitableUserData *gcUserData = (GCObjectVisitableUserData *)(userData);
            CBE::Object **objPtrPtr = reinterpret_cast<CBE::Object **>(ptr);
            CBE::Object *objPtr = *objPtrPtr;
            if (objPtr && objPtr != gcUserData->thisObj)
            {
                if (BIT_SET(objPtr->getFlags(), CBE::EObjectFlagBits::MarkedForDelete))
                {
                    (*objPtrPtr) = nullptr;
                }
                else
                {
                    (*gcUserData->objUsedFlags)[objPtr->getType()][CBE::PrivateObjectCoreAccessors::getAllocIdx(objPtr)] = true;
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
            alertIf(false, "Unhandled ptr to ptr Field name %s, type %s", propInfo.fieldProperty->nameString, *propInfo.thisProperty->typeInfo);
            break;
        }
    }
    static void visit(const void **ptr, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = getUnqualified(propInfo.thisProperty);
        switch (propInfo.thisProperty->type)
        {
        case EPropertyType::ClassType:
        {
            debugAssert(PropertyHelper::isChildOf(static_cast<CBEClass>(prop), CBE::Object::staticType()));

            GCObjectVisitableUserData *gcUserData = (GCObjectVisitableUserData *)(userData);
            const CBE::Object **objPtrPtr = reinterpret_cast<const CBE::Object **>(ptr);
            const CBE::Object *objPtr = *objPtrPtr;
            if (objPtr && objPtr != gcUserData->thisObj)
            {
                if (BIT_SET(objPtr->getFlags(), CBE::EObjectFlagBits::MarkedForDelete))
                {
                    (*objPtrPtr) = nullptr;
                }
                else
                {
                    (*gcUserData->objUsedFlags)[objPtr->getType()][CBE::PrivateObjectCoreAccessors::getAllocIdx(objPtr)] = true;
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
            alertIf(
                false, "Unhandled ptr to const ptr Field name %s, type %s", propInfo.fieldProperty->nameString, *propInfo.thisProperty->typeInfo
            );
            break;
        }
    }
};

void GCObjectFieldVisitable::visitMap(const MapProperty *mapProp, void *val, const PropertyInfo &propInfo, void *userData)
{
    const IterateableDataRetriever *dataRetriever = static_cast<const IterateableDataRetriever *>(mapProp->dataRetriever);
    const TypedProperty *keyProp = static_cast<const TypedProperty *>(mapProp->keyProp);
    const TypedProperty *valueProp = static_cast<const TypedProperty *>(mapProp->valueProp);

    // map key can be either fundamental or special or struct or class ptr but it can never be custom
    // types fundamental or special cannot hold pointer to CBE::Object so only struct and pointer is left
    if (getUnqualified(keyProp)->type == EPropertyType::ClassType)
    {
        // 2 times the data is needed to copy current and new value, to be removed and added later
        uint8 *bufferData = (uint8 *)CBEMemory::memAlloc(mapProp->pairSize * (dataRetriever->size(val) * 2 + 1), mapProp->pairAlignment);
        uint8 *tempData = bufferData;
        bufferData += mapProp->pairSize;

        // First - original data, Second - replacement data. both are key value pairs
        std::vector<std::pair<uint8 *, uint8 *>> editedKeys;
        for (auto itrPtr = dataRetriever->createIterator(val); itrPtr->isValid(); itrPtr->iterateFwd())
        {
            void *keyPtr = itrPtr->getElement();
            void *valPtr = static_cast<MapIteratorWrapper *>(itrPtr.get())->value();

            CBEMemory::memZero(tempData, mapProp->pairSize);
            dataRetriever->copyTo(keyPtr, tempData);
            FieldVisitor::visit<GCObjectFieldVisitable>(keyProp, tempData, userData);
            // If both original and temp new keys are equal then we do not have to replace entry
            if (dataRetriever->equals(keyPtr, tempData))
            {
                // Copy back key just in case pointer is not used for hashing or equality
                // checks
                dataRetriever->copyTo(tempData, keyPtr);
                // visit the value
                FieldVisitor::visit<GCObjectFieldVisitable>(valueProp, valPtr, userData);
            }
            else
            {
                std::pair<uint8 *, uint8 *> originalToNewPairs;
                originalToNewPairs.first = bufferData;
                bufferData += mapProp->pairSize;
                originalToNewPairs.second = bufferData;
                bufferData += mapProp->pairSize;
                // Copy original
                dataRetriever->copyTo(keyPtr, originalToNewPairs.first);
                // Copy new key and value
                dataRetriever->copyTo(tempData, originalToNewPairs.second);
                // visit the value
                FieldVisitor::visit<GCObjectFieldVisitable>(valueProp, originalToNewPairs.second + mapProp->secondOffset, userData);

                editedKeys.emplace_back(std::move(originalToNewPairs));
            }
        }

        for (const std::pair<uint8 *, uint8 *> &originalToNewPairs : editedKeys)
        {
            dataRetriever->remove(val, originalToNewPairs.first);
            dataRetriever->add(val, originalToNewPairs.second);
        }

        CBEMemory::memFree(tempData);
    }
    else // Only value can have pointer
    {
        for (auto itrPtr = dataRetriever->createIterator(val); itrPtr->isValid(); itrPtr->iterateFwd())
        {
            FieldVisitor::visit<GCObjectFieldVisitable>(valueProp, static_cast<MapIteratorWrapper *>(itrPtr.get())->value(), userData);
        }
    }
}

void GCObjectFieldVisitable::visitSet(const ContainerProperty *setProp, void *val, const PropertyInfo &propInfo, void *userData)
{
    const IterateableDataRetriever *dataRetriever = static_cast<const IterateableDataRetriever *>(setProp->dataRetriever);
    const TypedProperty *elementProp = static_cast<const TypedProperty *>(setProp->elementProp);

    // set key can be either fundamental or special or struct or class ptr but it can never be custom
    // types fundamental or special cannot hold pointer to CBE::Object so only struct and pointer is left
    if (getUnqualified(elementProp)->type == EPropertyType::ClassType)
    {
        // 2 times the data is needed to copy current and new value, to be removed and added later
        uint8 *bufferData
            = (uint8 *)CBEMemory::memAlloc(elementProp->typeInfo->size * (dataRetriever->size(val) * 2 + 1), elementProp->typeInfo->alignment);
        uint8 *tempData = bufferData;
        bufferData += elementProp->typeInfo->size;

        // First - original data, Second - replacement data.
        std::vector<std::pair<uint8 *, uint8 *>> editedKeys;
        for (auto itrPtr = dataRetriever->createIterator(val); itrPtr->isValid(); itrPtr->iterateFwd())
        {
            void *elemPtr = itrPtr->getElement();

            CBEMemory::memZero(tempData, elementProp->typeInfo->size);
            dataRetriever->copyTo(elemPtr, tempData);
            FieldVisitor::visit<GCObjectFieldVisitable>(elementProp, tempData, userData);
            // If both original and temp new keys are equal then we do not have to replace entry
            if (dataRetriever->equals(elemPtr, tempData))
            {
                // Copy back key just in case pointer is not used for hashing or equality
                // checks
                dataRetriever->copyTo(tempData, elemPtr);
            }
            else
            {
                std::pair<uint8 *, uint8 *> originalToNewPairs;
                originalToNewPairs.first = bufferData;
                bufferData += elementProp->typeInfo->size;
                originalToNewPairs.second = bufferData;
                bufferData += elementProp->typeInfo->size;
                // Copy original
                dataRetriever->copyTo(elemPtr, originalToNewPairs.first);
                // Copy new key and value
                dataRetriever->copyTo(tempData, originalToNewPairs.second);

                editedKeys.emplace_back(std::move(originalToNewPairs));
            }
        }

        for (const std::pair<uint8 *, uint8 *> &originalToNewPairs : editedKeys)
        {
            dataRetriever->remove(val, originalToNewPairs.first);
            dataRetriever->add(val, originalToNewPairs.second);
        }

        CBEMemory::memFree(tempData);
    }
    else // Only value can have pointer
    {
        for (auto itrPtr = dataRetriever->createIterator(val); itrPtr->isValid(); itrPtr->iterateFwd())
        {
            FieldVisitor::visit<GCObjectFieldVisitable>(elementProp, itrPtr->getElement(), userData);
        }
    }
}

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
            CBE::ObjectAllocatorBase *allocator = allocatorItr->second;

            // Right now we are only going through static fields of classes that has object
            // created from it. We are not using static in struct as well Create a separate
            // collection pass to collect from static field of all class properties in
            // CBE::Object hierarchy However storing referenced object in statics is not wise so
            // we do only as below or we could never scan any statics?
            FieldVisitor::visitStaticFields<GCObjectFieldVisitable>(classesLeft.back(), &userData);

            for (CBE::Object *obj : allocator->getAllObjects<CBE::Object>())
            {
                if (BIT_NOT_SET(obj->getFlags(), CBE::EObjectFlagBits::MarkedForDelete))
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