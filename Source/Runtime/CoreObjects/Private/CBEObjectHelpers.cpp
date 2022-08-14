/*!
 * \file CBEObjectHelpers.cpp
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CBEObjectHelpers.h"
#include "CBEPackage.h"
#include "PropertyVisitorHelpers.h"
#include "Visitors/FieldVisitors.h"
#include "Property/CustomProperty.h"
#include "CoreObjectDelegates.h"

#include <memory_resource>

namespace cbe
{
void ObjectAllocatorBase::constructDefault(void *objPtr, AllocIdx allocIdx, CBEClass clazz) const
{
    // Direct call to object construction routine to skip getting allocator that happens when constructing using CBEObjectConstructionPolicy
    // Default ctor
    const GlobalFunctionWrapper *ctor = PropertyHelper::findMatchingCtor<void *>(clazz);
    alertAlwaysf(ctor, "Default constructor not found to construct defaul object");

    Object *object = reinterpret_cast<Object *>(objPtr);

    // Object's data must be populated even before constructor is called
    INTERNAL_ObjectCoreAccessors::setAllocIdx(object, allocIdx);
    INTERNAL_ObjectCoreAccessors::getFlags(object) |= EObjectFlagBits::ObjFlag_Default | EObjectFlagBits::ObjFlag_RootObject;
    INTERNAL_ObjectCoreAccessors::setOuterAndName(
        object, PropertyHelper::getValidSymbolName(clazz->nameString)
                    + TCHAR("_Default"), nullptr, clazz
                );

    if (ctor)
    {
        object = ctor->invokeUnsafe<Object *, void *>(objPtr);
    }
}

void INTERNAL_destroyCBEObject(Object *obj)
{
    CBEClass clazz = obj->getType();

    CoreObjectDelegates::broadcastObjectDestroyed(obj);
    obj->destroyObject();
    clazz->destructor(obj);
}

void INTERNAL_createdCBEObject(Object *obj) { CoreObjectDelegates::broadcastObjectCreated(obj); }

bool INTERNAL_validateObjectName(const String &name, CBEClass clazz)
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

String INTERNAL_getValidObjectName(const String &name, CBEClass clazz)
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

Object *getDefaultObject(CBEClass clazz)
{
    ObjectAllocatorBase *objAllocator = getObjAllocator(clazz);
    if (objAllocator)
    {
        return reinterpret_cast<Object *>(objAllocator->getDefault());
    }
    // If clazz if not abstract we could try and create first instance to trigger object allocator creation
    if (clazz->allocFunc && clazz->destructor)
    {
        Object *obj = create(clazz, TCHAR("DummyForDefault"), nullptr, EObjectFlagBits::ObjFlag_Transient);
        debugAssert(obj);
        INTERNAL_destroyCBEObject(obj);
        return getDefaultObject(clazz);
    }
    return nullptr;
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
    static void visit(const PropertyInfo &propInfo, void *userData)
    {
        alertAlwaysf(false, "Why?! This isn't supposed to be invoked %s", propInfo.thisProperty->nameString);
    }

    // above UnsupportedFundamentalOrSpecial takes precedence over below generic support
    template <typename Type>
    requires IsReflectedSpecial<Type> || IsReflectedFundamental<Type>
    static void visit(const PropertyInfo &propInfo, void *userData)
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
        alertAlwaysf(false, "Why?! This isn't supposed to be invoked %s", propInfo.thisProperty->nameString);
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
            if (copyUserData->bReplaceSubobjects && isValid(*fromDataPtrPtr) && (*fromDataPtrPtr)->hasOuter(copyUserData->fromCommonRoot))
            {
                String comRootRelPath = ObjectPathHelper::getObjectPath(*fromDataPtrPtr, copyUserData->fromCommonRoot);
                Object *dupObj = copyUserData->objDb->getObject(
                    ObjectPathHelper::getFullPath(comRootRelPath.getChar(), copyUserData->toCommonRoot).getChar()
                );
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
                false, "Unhandled ptr to ptr Field name %s, type %s", propInfo.fieldProperty->nameString, *propInfo.thisProperty->typeInfo
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
    static void visit(Type *val, const PropertyInfo &propInfo, void *userData) {}
    template <typename Type>
    requires(!std::is_const_v<Type>) static void visit(Type *val, const PropertyInfo &propInfo, void *userData)
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
            "DeepCopy", "Cannot copy %s of type %s to %s of type %s", options.fromObject->getFullPath(),
            options.fromObject->getType()->nameString, options.toObject->getFullPath(), options.toObject->getType()->nameString
        );
        return false;
    }

    const CoreObjectsDB &objDb = ICoreObjectsModule::get()->getObjectsDB();
    std::vector<Object *> subObjects;
    switch (options.copyMode)
    {
    case EObjectTraversalMode::EntireObjectTree:
        // We need to copy the entire object graph under this objects
        objDb.getSubobjects(subObjects, options.fromObject->getStringID());
        break;
    case EObjectTraversalMode::ObjectAndChildren:
        // We need to copy the object and its children only
        objDb.getChildren(subObjects, options.fromObject->getStringID());
        break;
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
        // From this subobject at 0 to outer after fromObject at (size - 1)
        std::vector<String> objectNamesChain{ subObj->getName() };
        Object *subObjOuter = subObj->getOuter();
        while (subObjOuter != options.fromObject)
        {
            objectNamesChain.emplace_back(subObjOuter->getName());
            subObjOuter = subObjOuter->getOuter();
        }
        // Create outer objects from outer most(direct child of fromObject) to this sub object
        Object *duplicateSubObjOuter = options.toObject;
        for (auto outerNameRItr = objectNamesChain.crbegin(); outerNameRItr != objectNamesChain.crend(); ++outerNameRItr)
        {
            StringID fromObjectFullPath = StringID(ObjectPathHelper::getFullPath(outerNameRItr->getChar(), subObjOuter));
            Object *fromOuterObj = objDb.getObject(fromObjectFullPath);
            debugAssert(fromOuterObj);
            String toOuterFullPath = ObjectPathHelper::getFullPath(outerNameRItr->getChar(), duplicateSubObjOuter);
            // Just createOrGet()
            Object *toOuter = get(toOuterFullPath.getChar());
            if (!toOuter)
            {
                EObjectFlags flags = fromOuterObj->getFlags();
                CLEAR_BITS(flags, options.clearFlags);
                SET_BITS(flags, options.additionalFlags);
                toOuter = INTERNAL_create(fromOuterObj->getType(), *outerNameRItr, duplicateSubObjOuter, flags);
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
            fromToPair.second->constructed();
        }
    }
    if (options.bConstructToObject)
    {
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

Object *duplicateObject(
    Object *fromObject, Object *newOuter, String newName /*= ""*/, EObjectFlags additionalFlags /*= 0*/, EObjectFlags clearFlags /*= 0*/
)
{
    if (!fromObject)
    {
        return nullptr;
    }

    if (newName.empty())
    {
        newName = fromObject->getName();
    }

    if (!isValid(newOuter))
    {
        newOuter = fromObject->getOuter();
    }

    EObjectFlags flags = fromObject->getFlags();
    CLEAR_BITS(flags, clearFlags);
    SET_BITS(flags, additionalFlags);
    Object *duplicateObj = INTERNAL_create(fromObject->getType(), newName, newOuter, flags);
    if (deepCopy(fromObject, duplicateObj, additionalFlags, clearFlags, false))
    {
        duplicateObj->constructed();
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
    static void visit(Type *val, const PropertyInfo &propInfo, void *userData)
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
    static void visit(const void *val, const PropertyInfo &propInfo, void *userData)
    {
        alertAlwaysf(false, "Why?! This isn't supposed to be invoked %s", propInfo.thisProperty->nameString);
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
                false, "Unhandled ptr to ptr Field name %s, type %s", propInfo.fieldProperty->nameString, *propInfo.thisProperty->typeInfo
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
    const CoreObjectsDB &objDb = ICoreObjectsModule::get()->getObjectsDB();
    std::vector<Object *> subObjects;
    switch (replaceMode)
    {
    case EObjectTraversalMode::EntireObjectTree:
        objDb.getSubobjects(subObjects, object->getStringID());
        break;
    case EObjectTraversalMode::ObjectAndChildren:
        objDb.getChildren(subObjects, object->getStringID());
        break;
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
    static void visit(Type *val, const PropertyInfo &propInfo, void *userData)
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
    static void visit(const void *val, const PropertyInfo &propInfo, void *userData)
    {
        alertAlwaysf(false, "Why?! This isn't supposed to be invoked %s", propInfo.thisProperty->nameString);
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
                false, "Unhandled ptr to ptr Field name %s, type %s", propInfo.fieldProperty->nameString, *propInfo.thisProperty->typeInfo
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
    const CoreObjectsDB &objDb = ICoreObjectsModule::get()->getObjectsDB();
    std::vector<Object *> subObjects;
    switch (replaceMode)
    {
    case EObjectTraversalMode::EntireObjectTree:
        objDb.getSubobjects(subObjects, object->getStringID());
        break;
    case EObjectTraversalMode::ObjectAndChildren:
        objDb.getChildren(subObjects, object->getStringID());
        break;
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