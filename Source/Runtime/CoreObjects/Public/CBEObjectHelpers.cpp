/*!
 * \file CBEObjectHelpers.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CBEObjectHelpers.h"
#include "CBEPackage.h"
#include "PropertyVisitorHelpers.h"
#include "Visitors/FieldVisitors.h"
#include "Property/CustomProperty.h"
#include "CoreObjectDelegates.h"
#include "CoreObjectsModule.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"

#include <memory_resource>

namespace cbe
{
void ObjectAllocatorBase::constructDefault(void *objPtr, AllocIdx allocIdx, CBEClass clazz) const
{
    fatalAssertf(INTERNAL_isInMainThread(), "Defaults of class {} must be constructed inside main thread!", clazz->nameString);
    // Direct call to object construction routine to skip getting allocator that happens when constructing using CBEObjectConstructionPolicy
    // Default ctor
    const GlobalFunctionWrapper *ctor = PropertyHelper::findMatchingCtor<void *>(clazz);
    alertAlwaysf(ctor, "Default constructor not found to construct defaul object");

    Object *object = reinterpret_cast<Object *>(objPtr);

    // Object's data must be populated even before constructor is called
    String defaultName = PropertyHelper::getValidSymbolName(clazz->nameString) + TCHAR("_Default");
    INTERNAL_ObjectCoreAccessors::setDbIdx(object, CoreObjectsDB::InvalidDbIdx);
    INTERNAL_ObjectCoreAccessors::setOuterAndName(object, defaultName, nullptr, clazz);
    INTERNAL_ObjectCoreAccessors::setAllocIdx(object, allocIdx);
    INTERNAL_ObjectCoreAccessors::getFlags(object) |= EObjectFlagBits::ObjFlag_Default | EObjectFlagBits::ObjFlag_RootObject;

    if (ctor)
    {
        object = ctor->invokeUnsafe<Object *, void *>(objPtr);
    }
}
ObjectAllocatorBase &getOrCreateObjAllocator(CBEClass clazz)
{
    ObjectAllocatorBase *objAllocator = getObjAllocator(clazz);
    if (objAllocator)
    {
        return *objAllocator;
    }
    // If clazz if not abstract we could try and create first instance to trigger object allocator creation
    debugAssertf(clazz->allocFunc && clazz->destructor, "Object allocator cannot be created on Abstract class {}", clazz->nameString);
    Object *obj = create(clazz, TCHAR("DummyForObjectAllocator"), nullptr, EObjectFlagBits::ObjFlag_Transient);
    debugAssert(obj);
    INTERNAL_destroyCBEObject(obj);
    return *getObjAllocator(clazz);
}

void INTERNAL_destroyCBEObject(Object *obj)
{
    ObjectPrivateDataView objDatV = obj->getObjectData();
    debugAssert(objDatV);

    fatalAssertf(
        INTERNAL_isInMainThread(), "Object[{}] of class {} must be destroyed inside main thread!", objDatV.name, objDatV.clazz->nameString
    );

    CoreObjectDelegates::broadcastObjectDestroyed(obj);
    obj->destroyObject();
    // Reset the alloc index into the dbIdx for use by CBEObjectConstructionPolicy::deallocate
    INTERNAL_ObjectCoreAccessors::setDbIdx(obj, objDatV.allocIdx);
    objDatV.clazz->destructor(obj);
}

void INTERNAL_createdCBEObject(Object *obj) { CoreObjectDelegates::broadcastObjectCreated(obj); }

bool INTERNAL_isInMainThread() { return copat::JobSystem::get()->isInThread(copat::EJobThreadType::MainThread); }

bool INTERNAL_validateObjectName(StringView name, CBEClass clazz)
{
    if (PropertyHelper::isChildOf<Package>(clazz))
    {
        return ObjectPathHelper::isValidPackageName(name);
    }
    else
    {
        return PropertyHelper::isValidSymbolName(name);
    }
}

String INTERNAL_getValidObjectName(StringView name, CBEClass clazz)
{
    if (PropertyHelper::isChildOf<Package>(clazz))
    {
        return ObjectPathHelper::getValidPackageName(name);
    }
    else
    {
        return PropertyHelper::getValidSymbolName(name);
    }
}

//////////////////////////////////////////////////////////////////////////
// Copy/Duplicate implementations
//////////////////////////////////////////////////////////////////////////

struct DeepCopyUserData
{
    // Will be common root from which the copy actually started
    const CoreObjectsDB *objDb;
    Object *fromCommonRoot;
    Object *toCommonRoot;

    // From object and to object can be struct or class object
    void *fromObject;
    void *toObject;
    void *fromData;
    void *toData;

    // Additional options
    bool bReplaceSubobjects;
};

template <typename Type>
concept DeepCopyUnsupportedFundamentalOrSpecial = (std::is_const_v<Type> || std::is_pointer_v<Type> || std::is_reference_v<Type>())
                                                  && (IsReflectedSpecial<Type> || IsReflectedFundamental<Type>);

struct DeepCopyFieldVisitable
{
    static void visitStruct(const PropertyInfo &propInfo, void *userData);

    // Ignore const type
    template <DeepCopyUnsupportedFundamentalOrSpecial Type>
    static void visit(const PropertyInfo &propInfo, void * /*userData*/)
    {
        alertAlwaysf(false, "Why?! This isn't supposed to be invoked {}", propInfo.thisProperty->nameString);
    }

    // above UnsupportedFundamentalOrSpecial takes precedence over below generic support
    template <typename Type>
    requires IsReflectedSpecial<Type> || IsReflectedFundamental<Type>
    static void visit(const PropertyInfo & /*propInfo*/, void *userData)
    {
        DeepCopyUserData *readUserData = (DeepCopyUserData *)(userData);
        (*static_cast<Type *>(readUserData->toData)) = (*static_cast<Type *>(readUserData->fromData));
    }
    template <std::same_as<void> Type>
    static void visit(const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        DeepCopyUserData *copyUserData = (DeepCopyUserData *)(userData);

        switch (prop->type)
        {
        case EPropertyType::MapType:
        {
            const MapProperty *mapProp = static_cast<const MapProperty *>(prop);
            const IterateableDataRetriever *dataRetriever = static_cast<const IterateableDataRetriever *>(mapProp->dataRetriever);

            // Do not use element property here as it has possibility of being null when pair data type is not generated some where else
            const TypedProperty *keyProp = static_cast<const TypedProperty *>(mapProp->keyProp);
            const TypedProperty *valueProp = static_cast<const TypedProperty *>(mapProp->valueProp);

            // Up to 64+256 bytes in stack, Most data used in map might fit into this
            // NOTE: Avoid deep nested hierarchy if stack overflow happens
            uint8 buffer[320];
            std::pmr::monotonic_buffer_resource memRes(buffer, ARRAY_LENGTH(buffer));
            std::pmr::vector<uint8> perElementData{ mapProp->pairSize, &memRes };

            // Clear existing data
            dataRetriever->clear(copyUserData->toData);
            DeepCopyUserData newUserData{ *copyUserData };
            for (IteratorElementWrapperRef itr = dataRetriever->createIterator(copyUserData->fromData); itr->isValid(); itr->iterateFwd())
            {
                // zero and reconstruct for each element to avoid using previous values
                CBEMemory::memZero(perElementData.data(), perElementData.size());
                dataRetriever->contruct(perElementData.data());

                newUserData.fromData = itr->getElement();
                newUserData.toData = perElementData.data();
                FieldVisitor::visit<DeepCopyFieldVisitable>(keyProp, &newUserData);
                newUserData.fromData = static_cast<MapIteratorWrapper *>(itr.get())->value();
                newUserData.toData = perElementData.data() + mapProp->secondOffset;
                FieldVisitor::visit<DeepCopyFieldVisitable>(valueProp, &newUserData);

                dataRetriever->add(copyUserData->toData, perElementData.data(), true);
            }
            break;
        }
        case EPropertyType::SetType:
        case EPropertyType::ArrayType:
        {
            const IterateableDataRetriever *dataRetriever
                = static_cast<const IterateableDataRetriever *>(static_cast<const ContainerProperty *>(prop)->dataRetriever);
            const TypedProperty *elemProp = static_cast<const TypedProperty *>(static_cast<const ContainerProperty *>(prop)->elementProp);

            // Up to 256 bytes in stack, Most data used in map might fit into this
            // NOTE: Avoid deep nested hierarchy if stack overflow happens
            uint8 buffer[256];
            std::pmr::monotonic_buffer_resource memRes(buffer, ARRAY_LENGTH(buffer));
            std::pmr::vector<uint8> perElementData{ elemProp->typeInfo->size, &memRes };

            // Clear existing data
            dataRetriever->clear(copyUserData->toData);
            DeepCopyUserData newUserData{ *copyUserData };
            newUserData.toData = perElementData.data();
            for (IteratorElementWrapperRef itr = dataRetriever->createIterator(copyUserData->fromData); itr->isValid(); itr->iterateFwd())
            {
                // zero and reconstruct for each element to avoid using previous values
                CBEMemory::memZero(perElementData.data(), perElementData.size());
                dataRetriever->contruct(perElementData.data());

                newUserData.fromData = itr->getElement();
                FieldVisitor::visit<DeepCopyFieldVisitable>(elemProp, &newUserData);
                dataRetriever->add(copyUserData->toData, perElementData.data(), true);
            }
            break;
        }
        case EPropertyType::PairType:
        {
            const PairDataRetriever *dataRetriever
                = static_cast<const PairDataRetriever *>(static_cast<const PairProperty *>(prop)->dataRetriever);
            const TypedProperty *keyProp = static_cast<const TypedProperty *>(static_cast<const PairProperty *>(prop)->keyProp);
            const TypedProperty *valueProp = static_cast<const TypedProperty *>(static_cast<const PairProperty *>(prop)->valueProp);

            DeepCopyUserData newUserData{ *copyUserData };
            newUserData.fromData = dataRetriever->first(copyUserData->fromData);
            newUserData.toData = dataRetriever->first(copyUserData->toData);
            FieldVisitor::visit<DeepCopyFieldVisitable>(keyProp, &newUserData);
            newUserData.fromData = dataRetriever->second(copyUserData->fromData);
            newUserData.toData = dataRetriever->second(copyUserData->toData);
            FieldVisitor::visit<DeepCopyFieldVisitable>(valueProp, &newUserData);
            break;
        }
        case EPropertyType::ClassType:
        {
            visitStruct(propInfo, userData);
            break;
        }
        case EPropertyType::EnumType:
        {
            const EnumProperty *enumProp = static_cast<const EnumProperty *>(propInfo.thisProperty);
            CBEMemory::memCopy(copyUserData->toData, copyUserData->fromData, enumProp->typeInfo->size);
            break;
        }
        }
    }
    // Ignoring const types
    template <std::same_as<const void> Type>
    static void visit(const PropertyInfo &propInfo, void *userData)
    {
        alertAlwaysf(false, "Why?! This isn't supposed to be invoked {}", propInfo.thisProperty->nameString);
    }

    template <std::same_as<void *> Type>
    static void visit(const PropertyInfo &propInfo, void *userData)
    {
        DeepCopyUserData *copyUserData = (DeepCopyUserData *)(userData);

        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        switch (prop->type)
        {
        case EPropertyType::ClassType:
        {
            debugAssert(PropertyHelper::isChildOf(static_cast<CBEClass>(prop), cbe::Object::staticType()));

            Object **fromDataPtrPtr = reinterpret_cast<Object **>(copyUserData->fromData);
            Object **toDataPtrPtr = reinterpret_cast<Object **>(copyUserData->toData);
            // Replace pointer if we are replacing subobject references and the from pointer is valid sub object of fromCommonRoot
            if (copyUserData->bReplaceSubobjects && isValidFast(*fromDataPtrPtr) && (*fromDataPtrPtr)->hasOuter(copyUserData->fromCommonRoot))
            {
                String dupObjFullPath = ObjectPathHelper::computeObjectPath(*fromDataPtrPtr, copyUserData->fromCommonRoot);
                dupObjFullPath = ObjectPathHelper::getFullPath(dupObjFullPath.getChar(), copyUserData->toCommonRoot);
                Object *dupObj
                    = copyUserData->objDb->getObject({ .objectPath = dupObjFullPath.getChar(), .objectId = dupObjFullPath.getChar() });
                debugAssert(dupObj);
                (*toDataPtrPtr) = dupObj;
            }
            else
            {
                (*toDataPtrPtr) = (*fromDataPtrPtr);
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
    // It is okay we are not going to do anything that violates constant
    template <std::same_as<const void *> Type>
    static void visit(const PropertyInfo &propInfo, void *userData)
    {
        visit<void *>(propInfo, userData);
    }
};

struct StartDeepCopyFieldVisitable
{
    // Ignore const type
    template <typename Type>
    requires std::is_const_v<Type>
    static void visit(Type *val, const PropertyInfo &propInfo, void *userData)
    {}
    template <typename Type>
    requires (!std::is_const_v<Type>)
    static void visit(Type *val, const PropertyInfo &propInfo, void *userData)
    {
        DeepCopyUserData *copyUserData = (DeepCopyUserData *)(userData);
        // At this point both object and data must be same
        debugAssert(
            propInfo.fieldProperty && copyUserData->fromData == copyUserData->fromObject && copyUserData->toData == copyUserData->toObject
        );
        DeepCopyUserData newUserData{ *copyUserData };
        newUserData.fromData = static_cast<const MemberFieldWrapper *>(propInfo.fieldProperty->fieldPtr)->get(copyUserData->fromObject);
        newUserData.toData = static_cast<const MemberFieldWrapper *>(propInfo.fieldProperty->fieldPtr)->get(copyUserData->toObject);
        debugAssert(val == newUserData.fromData);

        DeepCopyFieldVisitable::visit<Type>(propInfo, &newUserData);
    }

    // It is okay we are not going to do anything that violates constant
    static void visit(const void **ptr, const PropertyInfo &propInfo, void *userData) { visit(const_cast<void **>(ptr), propInfo, userData); }
};

void DeepCopyFieldVisitable::visitStruct(const PropertyInfo &propInfo, void *userData)
{
    DeepCopyUserData *copyUserData = (DeepCopyUserData *)(userData);

    const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
    CBEClass clazz = static_cast<CBEClass>(prop);
    debugAssert(PropertyHelper::isStruct(clazz));

    DeepCopyUserData structUserData{ *copyUserData };
    structUserData.fromObject = structUserData.fromData;
    structUserData.toObject = structUserData.toData;
    FieldVisitor::visitFields<StartDeepCopyFieldVisitable>(clazz, copyUserData->fromData, &structUserData);
}

bool copyObject(CopyObjectOptions options)
{
    debugAssert(options.fromObject && options.toObject);
    if (!options.fromObject)
    {
        return false;
    }

    if (options.fromObject->getType() != options.toObject->getType())
    {
        LOG_ERROR(
            "DeepCopy", "Cannot copy {} of type {} to {} of type {}", options.fromObject->getObjectData().path,
            options.fromObject->getType()->nameString, options.toObject->getObjectData().path, options.toObject->getType()->nameString
        );
        return false;
    }
    CBE_PROFILER_SCOPE("CopyObjects");

    const CoreObjectsDB &objDb = CoreObjectsModule::objectsDB();
    std::vector<Object *> subObjects;
    switch (options.copyMode)
    {
    case EObjectTraversalMode::EntireObjectTree:
    {
        CBE_PROFILER_SCOPE("GatherObjsToCopy");
        // We need to copy the entire object graph under this objects
        objDb.getSubobjects(subObjects, options.fromObject->getDbIdx());
        break;
    }
    case EObjectTraversalMode::ObjectAndChildren:
    {
        CBE_PROFILER_SCOPE("GatherObjsToCopy");
        // We need to copy the object and its children only
        objDb.getChildren(subObjects, options.fromObject->getDbIdx());
        break;
    }
    case EObjectTraversalMode::OnlyObject:
    default:
        break;
    }

    // From - To pair
    std::unordered_set<std::pair<Object *, Object *>> duplicatedObjects{
        {options.fromObject, options.toObject}
    };
    // Create all sub-objects to be duplicated with appropriate path and name
    for (Object *subObj : subObjects)
    {
        CBE_PROFILER_SCOPE("CreateSubObject");

        // From this subobject at 0 to outer after fromObject at (size - 1)
        std::vector<StringView> objectNamesChain{ subObj->getObjectData().name };
        Object *subObjOuter = subObj->getOuter();
        while (subObjOuter != options.fromObject)
        {
            objectNamesChain.emplace_back(subObjOuter->getObjectData().name);
            subObjOuter = subObjOuter->getOuter();
        }
        // Create outer objects from outer most(direct child of fromObject) to this sub object
        Object *duplicateSubObjOuter = options.toObject;
        for (auto outerNameRItr = objectNamesChain.crbegin(); outerNameRItr != objectNamesChain.crend(); ++outerNameRItr)
        {
            String fromObjFullPath = ObjectPathHelper::getFullPath(*outerNameRItr, subObjOuter);
            Object *fromOuterObj = get(fromObjFullPath.getChar());
            ObjectPrivateDataView fromOuterObjDatV = objDb.getObjectData(fromOuterObj->getDbIdx());
            debugAssert(fromOuterObj);

            String toOuterFullPath = ObjectPathHelper::getFullPath(*outerNameRItr, duplicateSubObjOuter);
            // Just createOrGet()
            Object *toOuter = get(toOuterFullPath.getChar());
            if (!toOuter)
            {
                EObjectFlags flags = fromOuterObjDatV.flags;
                CLEAR_BITS(flags, options.clearFlags);
                SET_BITS(flags, options.additionalFlags);
                toOuter = INTERNAL_create(fromOuterObjDatV.clazz, *outerNameRItr, duplicateSubObjOuter, flags);
            }
            else
            {
                CLEAR_BITS(INTERNAL_ObjectCoreAccessors::getFlags(toOuter), options.clearFlags);
                SET_BITS(INTERNAL_ObjectCoreAccessors::getFlags(toOuter), options.additionalFlags);
            }

            duplicatedObjects.insert(std::pair<Object *, Object *>{ fromOuterObj, toOuter });
            duplicateSubObjOuter = toOuter;
            subObjOuter = fromOuterObj;
        }
    }

    for (const std::pair<Object *, Object *> fromToPair : duplicatedObjects)
    {
        CBE_PROFILER_SCOPE("CopyAnObject");

        DeepCopyUserData userData{ .objDb = &objDb,
                                   .fromCommonRoot = options.fromObject,
                                   .toCommonRoot = options.toObject,
                                   .fromObject = fromToPair.first,
                                   .toObject = fromToPair.second,
                                   .fromData = fromToPair.first,
                                   .toData = fromToPair.second,
                                   .bReplaceSubobjects = options.bReplaceSubobjRefs };
        FieldVisitor::visitFields<StartDeepCopyFieldVisitable>(fromToPair.first->getType(), fromToPair.first, &userData);
        if (options.bConstructSubObjects && options.toObject != fromToPair.second)
        {
            CBE_PROFILER_SCOPE("ConstructCopiedSubobject");

            fromToPair.second->constructed();
        }
    }
    if (options.bConstructToObject)
    {
        CBE_PROFILER_SCOPE("ConstructCopiedObject");

        options.toObject->constructed();
    }
    return true;
}
COREOBJECTS_EXPORT bool deepCopy(
    Object *fromObject, Object *toObject, EObjectFlags additionalFlags /*= 0*/, EObjectFlags clearFlags /*= 0*/,
    bool bConstructToObject /*= true*/
)
{
    CopyObjectOptions options{ .fromObject = fromObject,
                               .toObject = toObject,
                               .additionalFlags = additionalFlags,
                               .clearFlags = clearFlags,
                               .bReplaceSubobjRefs = true,
                               .bConstructToObject = bConstructToObject,
                               .bConstructSubObjects = true,
                               .copyMode = EObjectTraversalMode::EntireObjectTree };
    return copyObject(options);
}

Object *duplicateCBEObject(
    Object *fromObject, Object *newOuter, StringView newName /* = {}*/, EObjectFlags additionalFlags /*= 0*/, EObjectFlags clearFlags /*= 0*/
)
{
    if (!fromObject)
    {
        return nullptr;
    }

    ObjectPrivateDataView fromObjDatV = fromObject->getObjectData();
    if (newName.empty())
    {
        newName = fromObjDatV.name;
    }

    if (!isValidFast(newOuter))
    {
        newOuter = fromObject->getOuter();
    }

    EObjectFlags flags = fromObjDatV.flags;
    CLEAR_BITS(flags, clearFlags);
    SET_BITS(flags, additionalFlags);
    Object *duplicateObj = INTERNAL_create(fromObjDatV.clazz, newName, newOuter, flags);
    if (deepCopy(fromObject, duplicateObj, additionalFlags, clearFlags, true))
    {
        return duplicateObj;
    }
    duplicateObj->beginDestroy();
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
// replace objects implementations
//////////////////////////////////////////////////////////////////////////

struct ReplaceObjRefsVisitableUserData
{
    const std::unordered_map<Object *, Object *> &replacements;
};

struct ReplaceObjRefsVisitable
{
    // Ignore fundamental and special types, we need none const custom types or pointers
    template <typename Type>
    static void visit(Type * /*val*/, const PropertyInfo & /*propInfo*/, void * /*userData*/)
    {}
    static void visit(void *val, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        switch (prop->type)
        {
        case EPropertyType::MapType:
        {
            PropertyVisitorHelper::visitEditMapEntriesPtrOnly<ReplaceObjRefsVisitable>(
                static_cast<const MapProperty *>(prop), val, propInfo, userData
            );
            break;
        }
        case EPropertyType::SetType:
        {
            PropertyVisitorHelper::visitEditSetEntries<ReplaceObjRefsVisitable>(
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
                FieldVisitor::visit<ReplaceObjRefsVisitable>(elemProp, itrPtr->getElement(), userData);
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

            FieldVisitor::visit<ReplaceObjRefsVisitable>(keyProp, keyPtr, userData);
            FieldVisitor::visit<ReplaceObjRefsVisitable>(valueProp, valPtr, userData);
            break;
        }
        case EPropertyType::ClassType:
        {
            CBEClass clazz = static_cast<CBEClass>(prop);
            debugAssert(PropertyHelper::isStruct(clazz));
            FieldVisitor::visitFields<ReplaceObjRefsVisitable>(clazz, val, userData);
            break;
        }
        case EPropertyType::EnumType:
            break;
        }
    }
    // Ignoring const types
    static void visit(const void * /*val*/, const PropertyInfo &propInfo, void * /*userData*/)
    {
        alertAlwaysf(false, "Why?! This isn't supposed to be invoked {}", propInfo.thisProperty->nameString);
    }
    static void visit(void **ptr, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        switch (prop->type)
        {
        case EPropertyType::ClassType:
        {
            debugAssert(PropertyHelper::isChildOf(static_cast<CBEClass>(prop), cbe::Object::staticType()));

            ReplaceObjRefsVisitableUserData *repRefsUserData = (ReplaceObjRefsVisitableUserData *)(userData);
            cbe::Object **objPtrPtr = reinterpret_cast<cbe::Object **>(ptr);
            cbe::Object *objPtr = *objPtrPtr;

            auto replacementItr = repRefsUserData->replacements.find(objPtr);
            if (replacementItr != repRefsUserData->replacements.cend())
            {
                (*objPtrPtr) = replacementItr->second;
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
    // It is okay we are not going to do anything that violates constant
    static void visit(const void **ptr, const PropertyInfo &propInfo, void *userData) { visit(const_cast<void **>(ptr), propInfo, userData); }
};

void replaceObjectReferences(
    Object *object, const std::unordered_map<Object *, Object *> &replacements,
    EObjectTraversalMode replaceMode /*= EObjectTraversalMode::EntireObjectTree */
)
{
    CBE_PROFILER_SCOPE("ReplaceObjectRefs");

    const CoreObjectsDB &objDb = CoreObjectsModule::objectsDB();
    std::vector<Object *> subObjects;
    switch (replaceMode)
    {
    case EObjectTraversalMode::EntireObjectTree:
    {
        CBE_PROFILER_SCOPE("GatherObjsToCopy");
        objDb.getSubobjects(subObjects, object->getDbIdx());
        break;
    }
    case EObjectTraversalMode::ObjectAndChildren:
    {
        CBE_PROFILER_SCOPE("GatherObjsToCopy");
        objDb.getChildren(subObjects, object->getDbIdx());
        break;
    }
    case EObjectTraversalMode::OnlyObject:
    default:
        break;
    }

    ReplaceObjRefsVisitableUserData userData{ .replacements = replacements };
    FieldVisitor::visitFields<ReplaceObjRefsVisitable>(object->getType(), object, &userData);
    for (Object *subObj : subObjects)
    {
        FieldVisitor::visitFields<ReplaceObjRefsVisitable>(subObj->getType(), subObj, &userData);
    }
}

void replaceTreeObjRefs(Object *fromTreeRoot, Object *toTreeRoot, bool bReplaceInRoot)
{
    CBE_PROFILER_SCOPE("ReplaceTreeObjRefs");

    const CoreObjectsDB &objDb = CoreObjectsModule::objectsDB();

    std::unordered_map<Object *, Object *> replacements = {
        {fromTreeRoot, toTreeRoot}
    };
    std::vector<Object *> objectsToReplace;
    if (bReplaceInRoot)
    {
        objectsToReplace.emplace_back(toTreeRoot);
    }
    {
        CBE_PROFILER_SCOPE("PrepObjTreeRefs");

        std::vector<Object *> fromSubObjs;
        objDb.getSubobjects(fromSubObjs, fromTreeRoot->getDbIdx());

        objectsToReplace.reserve(fromSubObjs.size());
        for (Object *fromObj : fromSubObjs)
        {
            String fullPath = ObjectPathHelper::getFullPath(ObjectPathHelper::computeObjectPath(fromObj, fromTreeRoot), toTreeRoot);
            Object *toObj = get(fullPath);
            debugAssert(toObj);
            replacements[fromObj] = toObj;
            objectsToReplace.emplace_back(toObj);
        }
    }

    // Could be parallelized
    for (Object *thisObj : objectsToReplace)
    {
        cbe::replaceObjectReferences(thisObj, replacements, EObjectTraversalMode::OnlyObject);
    }
}

//////////////////////////////////////////////////////////////////////////
// Find object references implementations
//////////////////////////////////////////////////////////////////////////

struct FindObjRefsVisitableUserData
{
    const std::unordered_set<Object *> &objects;
    std::vector<ObjectReferences> &outReferences;

    Object *searchedIn;
    const FieldProperty *fieldProperty;
};

struct FindObjRefsVisitable
{
    // Ignore fundamental and special types, we need none const custom types or pointers
    template <typename Type>
    static void visit(Type * /*val*/, const PropertyInfo & /*propInfo*/, void * /*userData*/)
    {}
    static void visit(void *val, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        switch (prop->type)
        {
        case EPropertyType::MapType:
        {
            PropertyVisitorHelper::visitEditMapEntriesPtrOnly<FindObjRefsVisitable>(
                static_cast<const MapProperty *>(prop), val, propInfo, userData
            );
            break;
        }
        case EPropertyType::SetType:
        {
            PropertyVisitorHelper::visitEditSetEntries<FindObjRefsVisitable>(
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
                FieldVisitor::visit<FindObjRefsVisitable>(elemProp, itrPtr->getElement(), userData);
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

            FieldVisitor::visit<FindObjRefsVisitable>(keyProp, keyPtr, userData);
            FieldVisitor::visit<FindObjRefsVisitable>(valueProp, valPtr, userData);
            break;
        }
        case EPropertyType::ClassType:
        {
            CBEClass clazz = static_cast<CBEClass>(prop);
            debugAssert(PropertyHelper::isStruct(clazz));
            FieldVisitor::visitFields<FindObjRefsVisitable>(clazz, val, userData);
            break;
        }
        case EPropertyType::EnumType:
            break;
        }
    }
    // Ignoring const types
    static void visit(const void * /*val*/, const PropertyInfo &propInfo, void * /*userData*/)
    {
        alertAlwaysf(false, "Why?! This isn't supposed to be invoked {}", propInfo.thisProperty->nameString);
    }
    static void visit(void **ptr, const PropertyInfo &propInfo, void *userData)
    {
        const TypedProperty *prop = PropertyHelper::getUnqualified(propInfo.thisProperty);
        switch (prop->type)
        {
        case EPropertyType::ClassType:
        {
            debugAssert(PropertyHelper::isChildOf(static_cast<CBEClass>(prop), cbe::Object::staticType()));

            FindObjRefsVisitableUserData *findRefsUserData = (FindObjRefsVisitableUserData *)(userData);
            cbe::Object **objPtrPtr = reinterpret_cast<cbe::Object **>(ptr);
            cbe::Object *objPtr = *objPtrPtr;

            auto foundObjItr = findRefsUserData->objects.find(objPtr);
            if (foundObjItr != findRefsUserData->objects.cend())
            {
                findRefsUserData->outReferences.emplace_back(findRefsUserData->searchedIn, findRefsUserData->fieldProperty, objPtr);
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
    // It is okay we are not going to do anything that violates constant
    static void visit(const void **ptr, const PropertyInfo &propInfo, void *userData) { visit(const_cast<void **>(ptr), propInfo, userData); }
};

struct StartFindObjRefsVisitable
{
    template <typename Type>
    static void visit(Type *val, const PropertyInfo &propInfo, void *userData)
    {
        FindObjRefsVisitableUserData *findRefsUserData = (FindObjRefsVisitableUserData *)(userData);
        debugAssert(propInfo.fieldProperty);
        findRefsUserData->fieldProperty = propInfo.fieldProperty;
        FindObjRefsVisitable::visit<Type>(val, propInfo, &findRefsUserData);
    }
};

COREOBJECTS_EXPORT std::vector<ObjectReferences> findObjectReferences(
    Object *object, const std::unordered_set<Object *> &objects, EObjectTraversalMode replaceMode /*= EObjectTraversalMode::EntireObjectTree */
)
{
    const CoreObjectsDB &objDb = CoreObjectsModule::objectsDB();
    std::vector<Object *> subObjects;
    switch (replaceMode)
    {
    case EObjectTraversalMode::EntireObjectTree:
    {
        objDb.getSubobjects(subObjects, object->getDbIdx());
        break;
    }
    case EObjectTraversalMode::ObjectAndChildren:
    {
        objDb.getChildren(subObjects, object->getDbIdx());
        break;
    }
    case EObjectTraversalMode::OnlyObject:
    default:
        break;
    }

    std::vector<ObjectReferences> references;
    FindObjRefsVisitableUserData userData{ .objects = objects, .outReferences = references, .searchedIn = object };
    FieldVisitor::visitFields<StartFindObjRefsVisitable>(object->getType(), object, &userData);
    for (Object *subObj : subObjects)
    {
        userData.searchedIn = subObj;
        FieldVisitor::visitFields<StartFindObjRefsVisitable>(subObj->getType(), subObj, &userData);
    }
    return references;
}

} // namespace cbe