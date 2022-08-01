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
#include "String/NameString.h"

namespace cbe
{

//////////////////////////////////////////////////////////////////////////
// Generic object related helpers
//////////////////////////////////////////////////////////////////////////

template <typename T>
concept ObjectType = ReflectClassType<T> && std::is_base_of_v<Object, T>;

FORCE_INLINE bool isValid(const Object *obj)
{
    if (obj && NO_BITS_SET(obj->getFlags(), EObjectFlagBits::ObjFlag_Deleted | EObjectFlagBits::ObjFlag_MarkedForDelete))
    {
        const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
        return objectsDb.hasObject(obj->getStringID());
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
// Object casts
//////////////////////////////////////////////////////////////////////////

template <typename AsType, typename FromType>
FORCE_INLINE AsType *cast(FromType *obj)
{
    return PropertyHelper::cast<AsType, FromType>(obj);
}

//////////////////////////////////////////////////////////////////////////
// Object Related helpers
//////////////////////////////////////////////////////////////////////////
COREOBJECTS_EXPORT void INTERNAL_destroyCBEObject(Object *obj);
FORCE_INLINE bool INTERNAL_validateCreatedObject(Object *obj) { return BIT_NOT_SET(obj->getFlags(), EObjectFlagBits::ObjFlag_Default); }
/**
 * cbe::INTERNAL_create - Only difference between regular create and this is constructed never gets called under any condition
 * This must be used if constructed() must be delayed without setting any neccessary flags
 *
 * @param CBEClass clazz
 * @param const String & name
 * @param Object * outerObj
 * @param EObjectFlags flags
 * @param CtorArgs && ...ctorArgs
 *
 * @return cbe::Object *
 */
template <typename... CtorArgs>
Object *INTERNAL_create(CBEClass clazz, const String &name, Object *outerObj, EObjectFlags flags = 0, CtorArgs... ctorArgs)
{
    if (clazz == nullptr)
    {
        alertAlwaysf(false, "Invalid class type! when creating object %s", name);
        return nullptr;
    }

    // If empty string then try create from class name
    String objectName = name.empty() ? clazz->nameString : name;
    // Using valid property name, Change if needed other wise also change in ObjectAllocatorBase::constructDefault
    if (!PropertyHelper::isValidSymbolName(objectName))
    {
        alertAlwaysf(false, "Invalid object name! Invalid characters will be replaced with underscore(_)");
        objectName = PropertyHelper::getValidSymbolName(objectName);
    }
    NameString objectFullPath = NameString(ObjectPathHelper::getFullPath(objectName.getChar(), outerObj));

    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
#if DEV_BUILD
    if (objectsDb.hasObject(StringID(objectFullPath)))
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
    if (objectsDb.hasObject(StringID(objectFullPath)))
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
Object *create(CBEClass clazz, const String &name, Object *outerObj, EObjectFlags flags = 0, CtorArgs... ctorArgs)
{
    Object *obj = INTERNAL_create<CtorArgs...>(clazz, name, outerObj, flags, std::forward<CtorArgs>(ctorArgs)...);
    // Also change cbe::Object::constructed(), Always construct for Transients
    if (obj && NO_BITS_SET(obj->collectAllFlags(), EObjectFlagBits::ObjFlag_PackageLoadPending)
        || BIT_SET(flags, EObjectFlagBits::ObjFlag_Transient))
    {
        obj->constructed();
    }
    return obj;
}

template <typename... CtorArgs>
Object *createOrGet(CBEClass clazz, const String &name, Object *outerObj, EObjectFlags flags = 0, CtorArgs... ctorArgs)
{
    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
    StringID objectFullPath = ObjectPathHelper::getFullPath(name.getChar(), outerObj).getChar();
    if (objectsDb.hasObject(objectFullPath))
    {
        return objectsDb.getObject(objectFullPath);
    }
    return create<CtorArgs...>(clazz, name, outerObj, flags, std::forward<CtorArgs>(ctorArgs)...);
}

template <typename ClassType, typename... CtorArgs>
ClassType *create(const String &name, Object *outerObj, EObjectFlags flags = 0, CtorArgs... ctorArgs)
{
    return static_cast<ClassType *>(create<CtorArgs...>(ClassType::staticType(), name, outerObj, flags, std::forward<CtorArgs>(ctorArgs)...));
}

template <typename ClassType, typename... CtorArgs>
ClassType *createOrGet(const String &name, Object *outerObj, EObjectFlags flags = 0, CtorArgs... ctorArgs)
{
    return static_cast<ClassType *>(
        createOrGet<CtorArgs...>(ClassType::staticType(), name, outerObj, flags, std::forward<CtorArgs>(ctorArgs)...)
    );
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

//////////////////////////////////////////////////////////////////////////
// Object modification helpers
//////////////////////////////////////////////////////////////////////////

struct CopyObjectOptions
{
    Object *fromObject;
    Object *toObject;
    EObjectFlags additionalFlags = 0;
    EObjectFlags clearFlags = 0;
    // If each sub object of fromObject references has to be replaced with corresponding sub object of toObject
    bool bReplaceSubobjRefs;
    // If call constructed after copy on ToObject after copy
    bool bConstructToObject;
    // If call constructed after copy on all sub objects
    bool bConstructSubObjects;
    EObjectTraversalMode copyMode;
};
COREOBJECTS_EXPORT bool copyObject(CopyObjectOptions options);
/**
 * cbe::deepCopy - copies all reflected data from a object to another object and creates new object for any referenced subobject while copying
 *
 * @param Object * fromObject
 * @param Object * toObject
 *
 * @return void
 */
COREOBJECTS_EXPORT bool deepCopy(
    Object *fromObject, Object *toObject, EObjectFlags additionalFlags = 0, EObjectFlags clearFlags = 0, bool bConstructToObject = true
);
COREOBJECTS_EXPORT Object *
    duplicateObject(Object *fromObject, Object *newOuter, String newName = "", EObjectFlags additionalFlags = 0, EObjectFlags clearFlags = 0);

/**
 * cbe::replaceObjectReferences - Replaces
 *
 * @param Object * object
 * @param const std::unordered_map<Object *, Object * > &replacements
 *
 */
COREOBJECTS_EXPORT void replaceObjectReferences(
    Object *object, const std::unordered_map<Object *, Object *> &replacements,
    EObjectTraversalMode replaceMode = EObjectTraversalMode::EntireObjectTree
);

struct ObjectReferences
{
    Object *foundInObject;
    const FieldProperty *fieldProperty;
    Object *foundObject;
};
COREOBJECTS_EXPORT std::vector<ObjectReferences> findObjectReferences(
    Object *object, const std::unordered_set<Object *> &objects, EObjectTraversalMode replaceMode = EObjectTraversalMode::EntireObjectTree
);

} // namespace cbe