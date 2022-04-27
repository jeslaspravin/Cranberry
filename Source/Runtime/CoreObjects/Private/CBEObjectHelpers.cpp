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
#include "Visitors/FieldVisitors.h"
#include "Property/CustomProperty.h"

namespace CBE
{
void ObjectAllocatorBase::constructDefault(void *objPtr, AllocIdx allocIdx, CBEClass clazz) const
{
    // Direct call to object construction routine to skip getting allocator that happens when constructing using CBEObjectConstructionPolicy
    // Default ctor
    const GlobalFunctionWrapper *ctor = PropertyHelper::findMatchingCtor<void *>(clazz);
    alertIf(ctor, "Default constructor not found to construct defaul object");

    Object *object = reinterpret_cast<Object *>(objPtr);

    // Object's data must be populated even before constructor is called
    INTERNAL_ObjectCoreAccessors::setAllocIdx(object, allocIdx);
    INTERNAL_ObjectCoreAccessors::getFlags(object) |= EObjectFlagBits::Default | EObjectFlagBits::RootObject;
    INTERNAL_ObjectCoreAccessors::setOuterAndName(
        object, PropertyHelper::getValidSymbolName(clazz->nameString) + TCHAR("_Default"), nullptr, clazz
    );

    if (ctor)
    {
        object = ctor->invokeUnsafe<Object *, void *>(objPtr);
    }
}

void INTERNAL_destroyCBEObject(Object *obj)
{
    CBEClass clazz = obj->getType();
    obj->destroyObject();
    static_cast<const GlobalFunctionWrapper *>(clazz->destructor->funcPtr)->invokeUnsafe<void>(obj);
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
        alertIf(false, "Why?! This isn't supposed to be invoked %s", propInfo.thisProperty->nameString);
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
        alertIf(false, "Why?! This isn't supposed to be invoked %s", propInfo.thisProperty->nameString);
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
            debugAssert(PropertyHelper::isChildOf(static_cast<CBEClass>(prop), CBE::Object::staticType()));

            Object **fromDataPtrPtr = reinterpret_cast<Object **>(copyUserData->fromData);
            Object **toDataPtrPtr = reinterpret_cast<Object **>(copyUserData->toData);
            if ((*fromDataPtrPtr)->hasOuter(copyUserData->fromCommonRoot))
            {
                String comRootRelPath = ObjectPathHelper::getObjectPath(*fromDataPtrPtr, copyUserData->fromCommonRoot);
                Object *dupObj
                    = copyUserData->objDb->getObject(ObjectPathHelper::getFullPath(comRootRelPath.getChar(), copyUserData->toCommonRoot));
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
            alertIf(false, "Unhandled ptr to ptr Field name %s, type %s", propInfo.fieldProperty->nameString, *propInfo.thisProperty->typeInfo);
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
    FieldVisitor::visitFields<StartDeepCopyFieldVisitable>(clazz, copyUserData->fromData, userData);
}

bool deepCopy(Object *fromObject, Object *toObject)
{
    debugAssert(fromObject && toObject);

    if (fromObject->getType() != toObject->getType())
    {
        LOG_ERROR(
            "DeepCopy", "Cannot copy %s of type %s to %s of type %s", fromObject->getFullPath(), fromObject->getType()->nameString,
            toObject->getFullPath(), toObject->getType()->nameString
        );
        return false;
    }

    // We need to copy the entire object graph under this objects
    const CoreObjectsDB &objDb = ICoreObjectsModule::get()->getObjectsDB();
    std::vector<Object *> subObjects;
    objDb.getSubobjects(subObjects, fromObject->getStringID());

    // From - To pair
    std::unordered_set<std::pair<Object *, Object *>> duplicatedObjects{
        {fromObject, toObject}
    };
    for (Object *subObj : subObjects)
    {
        // From this subobject at 0 to outer after fromObject at (size - 1)
        std::vector<String> objectNamesChain{ subObj->getName() };
        Object *subObjOuter = subObj->getOuter();
        while (subObjOuter != fromObject)
        {
            objectNamesChain.emplace_back(subObjOuter->getName());
            subObjOuter = subObjOuter->getOuter();
        }
        // Create outer objects from outer most to this sub object
        Object *duplicateSubObjOuter = toObject;
        for (auto outerNameRItr = objectNamesChain.crbegin(); outerNameRItr != objectNamesChain.crend(); ++outerNameRItr)
        {
            String fromObjectFullPath = ObjectPathHelper::getFullPath(outerNameRItr->getChar(), subObjOuter);
            Object *fromOuterObj = objDb.getObject(fromObjectFullPath);
            debugAssert(fromOuterObj);
            Object *outer = createOrGet(fromOuterObj->getType(), *outerNameRItr, duplicateSubObjOuter, fromOuterObj->getFlags());

            duplicatedObjects.insert(std::pair<Object *, Object *>{ fromOuterObj, outer });
            duplicateSubObjOuter = outer;
            subObjOuter = fromOuterObj;
        }
    }

    for (const std::pair<Object *, Object *> fromToPair : duplicatedObjects)
    {
        DeepCopyUserData userData{ .objDb = &objDb,
                                   .fromCommonRoot = fromObject,
                                   .toCommonRoot = toObject,
                                   .fromObject = fromToPair.first,
                                   .toObject = fromToPair.second,
                                   .fromData = fromToPair.first,
                                   .toData = fromToPair.second };
        FieldVisitor::visitFields<StartDeepCopyFieldVisitable>(fromToPair.first->getType(), fromToPair.first, &userData);
    }
    return true;
}

} // namespace CBE