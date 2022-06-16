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

//////////////////////////////////////////////////////////////////////////
// Object casts
//////////////////////////////////////////////////////////////////////////

// Object to Object conversions
template <ReflectClassOrStructType AsType, ReflectClassOrStructType FromType>
FORCE_INLINE AsType *cast(FromType *obj)
{
    using CBEObjectType = std::conditional_t<std::is_const_v<FromType>, const CBE::Object, CBE::Object>;
    if (isValid(obj) && PropertyHelper::isChildOf(obj->getType(), AsType::staticType()))
    {
        return static_cast<AsType *>(static_cast<CBEObjectType *>(obj));
    }
    return nullptr;
}

// Object to Interface
template <InterfaceType AsType, ReflectClassOrStructType FromType>
requires StaticCastable<FromType *, AsType *> FORCE_INLINE AsType *cast(FromType *obj) { return static_cast<AsType *>(obj); }

template <InterfaceType AsType, ReflectClassOrStructType FromType>
requires(!StaticCastable<FromType *, AsType *>) FORCE_INLINE AsType *cast(FromType *obj)
{
    using UPtrIntType = std::conditional_t<std::is_const_v<FromType>, const UPtrInt, UPtrInt>;

    if (!isValid(obj))
    {
        return nullptr;
    }
    if (const InterfaceInfo *interfaceInfo
        = PropertyHelper::getMatchingInterfaceInfo(obj->getType(), typeInfoFrom<std::remove_const_t<AsType>>()))
    {
        return (AsType *)(reinterpret_cast<UPtrIntType>(obj) + interfaceInfo->offset);
    }
    return nullptr;
}

// Interface to Object
template <ReflectClassOrStructType AsType, InterfaceType FromType>
requires StaticCastable<FromType *, AsType *> FORCE_INLINE AsType *cast(FromType *obj)
{
    return PropertyHelper::isChildOf(obj->getType(), AsType::staticType()) ? static_cast<AsType *>(obj) : nullptr;
}

template <ReflectClassOrStructType AsType, InterfaceType FromType>
requires(!StaticCastable<FromType *, AsType *>) FORCE_INLINE AsType *cast(FromType *obj)
{
    using UPtrIntType = std::conditional_t<std::is_const_v<FromType>, const UPtrInt, UPtrInt>;

    if (!(obj && obj->getType() && PropertyHelper::isChildOf(obj->getType(), AsType::staticType())))
    {
        return nullptr;
    }
    if (const InterfaceInfo *interfaceInfo
        = PropertyHelper::getMatchingInterfaceInfo(obj->getType(), typeInfoFrom<std::remove_const_t<FromType>>()))
    {
        return (AsType *)(reinterpret_cast<UPtrIntType>(obj) - interfaceInfo->offset);
    }
    return nullptr;
}

// Interface to Interface
template <InterfaceType AsType, InterfaceType FromType>
requires StaticCastable<FromType *, AsType *> FORCE_INLINE AsType *cast(FromType *obj) { return static_cast<AsType *>(obj); }

template <InterfaceType AsType, InterfaceType FromType>
requires(!StaticCastable<FromType *, AsType *>) FORCE_INLINE AsType *cast(FromType *obj)
{
    using UPtrIntType = std::conditional_t<std::is_const_v<FromType>, const UPtrInt, UPtrInt>;

    if (!(obj && obj->getType() && PropertyHelper::implementsInterface<AsType>(obj->getType())))
    {
        return nullptr;
    }
    const InterfaceInfo *fromInterfaceInfo
        = PropertyHelper::getMatchingInterfaceInfo(obj->getType(), typeInfoFrom<std::remove_const_t<FromType>>());
    const InterfaceInfo *toInterfaceInfo
        = PropertyHelper::getMatchingInterfaceInfo(obj->getType(), typeInfoFrom<std::remove_const_t<AsType>>());
    if (fromInterfaceInfo && toInterfaceInfo)
    {
        return (AsType *)((reinterpret_cast<UPtrIntType>(obj) - fromInterfaceInfo->offset) + toInterfaceInfo->offset);
    }
    return nullptr;
}

// None of the above casts
template <typename AsType, typename FromType>
requires(
    StaticCastable<FromType *, AsType *> && !(ReflectClassOrStructOrInterfaceType<AsType> || ReflectClassOrStructOrInterfaceType<FromType>)
) FORCE_INLINE AsType *cast(FromType *obj)
{
    return static_cast<AsType *>(obj);
}

//////////////////////////////////////////////////////////////////////////
// Object Related helpers
//////////////////////////////////////////////////////////////////////////
COREOBJECTS_EXPORT void INTERNAL_destroyCBEObject(Object *obj);
FORCE_INLINE bool INTERNAL_validateCreatedObject(Object *obj) { return BIT_NOT_SET(obj->getFlags(), EObjectFlagBits::Default); }

template <typename... CtorArgs>
Object *create(CBEClass clazz, const String &name, Object *outerObj, EObjectFlags flags = 0, CtorArgs &&...ctorArgs)
{
    // If empty string then try create from class name
    String objectName = name.empty() ? clazz->nameString : name;
    // Using valid property name, Change if needed other wise also change in ObjectAllocatorBase::constructDefault
    if (!PropertyHelper::isValidSymbolName(objectName))
    {
        alertAlwaysf(false, "Invalid object name! Invalid characters will be replaced with underscore(_)");
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
    fatalAssertf(clazz->allocFunc && clazz->destructor, "Abstract class %s cannot be instantiated!", clazz->nameString);

    /**
     * **NOTICE**
     * If modifying any allocation and construction logic modify same at ObjectAllocatorBase::constructDefault and allocation of default at
     * ObjectAllocator's default constructor
     */

    // void* is first param of ctor since we pass in object on which construction needs to be executed
    const GlobalFunctionWrapper *ctor = PropertyHelper::findMatchingCtor<void *, CtorArgs...>(clazz);
    alertAlwaysf(ctor, "Constructor arguments are invalid");
    if (!ctor)
    {
        LOG_ERROR("ObjectHelper", "Cannot construct object with given constructor arguments");
        return nullptr;
    }

    void *objPtr = clazz->allocFunc();
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
        alertAlwaysf(false, "Object validation failed! Destroying %s", object->getFullPath());
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

COREOBJECTS_EXPORT Object *getDefaultObject(CBEClass clazz);

/**
 * CBE::deepCopy - copies all reflected data from a object to another object and creates new object for any referenced subobject while copying
 *
 * Access: public
 *
 * @param Object * fromObject
 * @param Object * toObject
 *
 * @return void
 */
COREOBJECTS_EXPORT bool deepCopy(Object *fromObject, Object *toObject);
COREOBJECTS_EXPORT Object *duplicateObject(Object *fromObject, Object *newOuter, String newName = "");

} // namespace CBE