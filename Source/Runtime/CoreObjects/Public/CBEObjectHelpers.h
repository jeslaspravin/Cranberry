/*!
 * \file CBEObjectHelpers.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CBEObject.h"
#include "ICoreObjectsModule.h"
#include "CoreObjectsDB.h"
#include "ObjectPathHelpers.h"
#include "Property/PropertyHelper.h"

namespace CBE
{

//////////////////////////////////////////////////////////////////////////
// Generic object related helpers
//////////////////////////////////////////////////////////////////////////

FORCE_INLINE bool isValid(const Object *obj)
{
    return obj && NO_BITS_SET(obj->getFlags(), EObjectFlagBits::Deleted | EObjectFlagBits::MarkedForDelete);
}

template <ReflectClassOrStructType AsType, ReflectClassOrStructType FromType>
requires StaticCastable<FromType *, AsType *> FORCE_INLINE AsType *cast(FromType *obj)
{
    if (isValid(obj) && PropertyHelper::isChildOf(obj->getType(), AsType::staticType()))
    {
        return static_cast<AsType *>(obj);
    }
    return nullptr;
}
template <ReflectClassOrStructType AsType, ReflectClassOrStructType FromType>
requires(!StaticCastable<FromType *, AsType *>) FORCE_INLINE AsType *cast(FromType *obj) { return nullptr; }

template <typename AsType, typename FromType>
requires StaticCastable<FromType *, AsType *> FORCE_INLINE AsType *cast(FromType *obj) { return static_cast<AsType *>(obj); }

//////////////////////////////////////////////////////////////////////////
// Object Related helpers
//////////////////////////////////////////////////////////////////////////
COREOBJECTS_EXPORT void INTERNAL_destroyCBEObject(CBE::Object *obj);
FORCE_INLINE bool INTERNAL_validateCreatedObject(CBE::Object *obj) { return BIT_NOT_SET(obj->getFlags(), EObjectFlagBits::Default); }

template <typename... CtorArgs>
Object *create(CBEClass clazz, const String &name, Object *outerObj, EObjectFlags flags = 0, CtorArgs &&...ctorArgs)
{
    // If empty string then try create from class name
    String objectName = name.empty() ? clazz->nameString : name;
    // Using valid property name, Change if needed other wise also change in ObjectAllocatorBase::constructDefault
    if (!PropertyHelper::isValidSymbolName(objectName))
    {
        alertIf(false, "Invalid object name! Invalid characters will be replaced with underscore(_)");
        objectName = PropertyHelper::getValidSymbolName(objectName);
    }
    String objectFullPath = ObjectPathHelper::getFullPath(objectName.getChar(), outerObj);

    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
#if DEV_BUILD
    if (objectsDb.hasObject(objectFullPath))
    {
        LOG_WARN(
            "ObjectHelper",
            "Object with path %s already exists, If object path needs to be exactly same use createOrGet() to retrieve existing object",
            objectFullPath
        );
    }
#endif // DEV_BUILD

    /**
     * **NOTICE**
     * If modifying any allocation and construction logic modify same at ObjectAllocatorBase::constructDefault and allocation of default at
     * ObjectAllocator's default constructor
     */

    // void* is first param of ctor since we pass in object on which construction needs to be executed
    const GlobalFunctionWrapper *ctor = PropertyHelper::findMatchingCtor<void *, CtorArgs...>(clazz);
    alertIf(ctor, "Constructor arguments are invalid");
    if (!ctor)
    {
        LOG_ERROR("ObjectHelper", "Cannot construct object with given constructor arguments");
        return nullptr;
    }

    void *objPtr = static_cast<const GlobalFunctionWrapper *>(clazz->allocFunc->funcPtr)->invokeUnsafe<void *>();
    Object *object = reinterpret_cast<Object *>(objPtr);

    // Object's data must be populated even before constructor is called
    if (objectsDb.hasObject(objectFullPath))
    {
        // Appending allocation ID and class name will make it unique
        SizeT uniqueNameId = uint32(clazz->name);
        HashUtility::combineSeeds(uniqueNameId, INTERNAL_ObjectCoreAccessors::getAllocIdx(object));
        objectName = StringFormat::format(TCHAR("%s_%llu"), objectName, uniqueNameId);
    }
    INTERNAL_ObjectCoreAccessors::getFlags(object) |= flags;
    INTERNAL_ObjectCoreAccessors::setOuterAndName(object, objectName, outerObj, clazz);

    object = ctor->invokeUnsafe<Object *, void *, CtorArgs...>(objPtr, std::forward<CtorArgs>(ctorArgs)...);

    if (!INTERNAL_validateCreatedObject(object))
    {
        alertIf(false, "Object validation failed! Destroying %s", object->getFullPath());
        INTERNAL_destroyCBEObject(object);
        object = nullptr;
    }
    return object;
}

template <typename... CtorArgs>
Object *createOrGet(CBEClass clazz, const String &name, Object *outerObj, EObjectFlags flags = 0, CtorArgs &&...ctorArgs)
{
    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
    StringID objectFullPath = ObjectPathHelper::getFullPath(name.getChar(), outerObj);
    if (objectsDb.hasObject(objectFullPath))
    {
        return objectsDb.getObject(objectFullPath);
    }
    return create<CtorArgs...>(clazz, name, outerObj, flags, std::forward<CtorArgs>(ctorArgs)...);
}

template <typename ClassType, typename... CtorArgs>
ClassType *create(const String &name, Object *outerObj, EObjectFlags flags = 0, CtorArgs &&...ctorArgs)
{
    return static_cast<ClassType *>(create(ClassType::staticType(), name, outerObj, flags, std::forward<CtorArgs>(ctorArgs)...));
}

template <typename ClassType, typename... CtorArgs>
ClassType *createOrGet(const String &name, Object *outerObj, EObjectFlags flags = 0, CtorArgs &&...ctorArgs)
{
    return static_cast<ClassType *>(createOrGet(ClassType::staticType(), name, outerObj, flags, std::forward<CtorArgs>(ctorArgs)...));
}

FORCE_INLINE Object *get(const TChar *objectFullPath) { return ICoreObjectsModule::get()->getObjectsDB().getObject(objectFullPath); }

template <typename ClassType>
ClassType *get(const TChar *objectFullPath)
{
    Object *obj = get(objectFullPath);
    return cast<ClassType>(obj);
}

COREOBJECTS_EXPORT Object *load(String objectPath);
COREOBJECTS_EXPORT Object *getOrLoad(String objectPath);
template <typename ClassType>
ClassType *load(const String &objectPath)
{
    Object *obj = load(objectPath);
    return cast<ClassType>(obj);
}
template <typename ClassType>
ClassType *getOrLoad(const String &objectPath)
{
    Object *obj = getOrLoad(objectPath);
    return cast<ClassType>(obj);
}

COREOBJECTS_EXPORT void markDirty(Object *obj);
// Saves object as package if it is subobject of a valid package
COREOBJECTS_EXPORT bool save(Object *obj);

template <typename ClassType>
ClassType *getDefaultObject()
{
    ObjectAllocator<ClassType> &objAllocator = getObjAllocator<ClassType>();
    return reinterpret_cast<ClassType *>(objAllocator.getDefault());
}

FORCE_INLINE CBE::Object *getDefaultObject(CBEClass clazz)
{
    ObjectAllocatorBase *objAllocator = getObjAllocator(clazz);
    if (objAllocator)
    {
        return reinterpret_cast<CBE::Object *>(objAllocator->getDefault());
    }
    return nullptr;
}

} // namespace CBE