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
#include "String/NameString.h"

namespace cbe
{

//////////////////////////////////////////////////////////////////////////
// Generic object related helpers
//////////////////////////////////////////////////////////////////////////

template <typename T>
concept ObjectType = ReflectClassType<T> && std::is_base_of_v<Object, T>;

/**
 * Prefer cbe::get(fullPath, sid) or cbe::get(fullPath) over isValid*(obj) functions
 * If you know the path string or if you are not sure that object is gc'd.
 * Use this if the object is pointed in a reflected field then GC visits it and clears it as null if object pointed is destroyed.
 */

FORCE_INLINE bool isValid(const Object *obj)
{
    if (obj == nullptr)
    {
        return false;
    }

    // TODO(Jeslas) : Find some way to determine validity directly from obj pointer eg. Map obj pointer value to Object node index or have a set
    // of valid objects in objects db
    ObjectDbIdx dbIdx = obj->getDbIdx();

    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
    ObjectPrivateDataView objectDatV = objectsDb.getObjectData(dbIdx);

    if (objectDatV && NO_BITS_SET(objectDatV.flags, EObjectFlagBits::ObjFlag_MarkedForDelete | EObjectFlagBits::ObjFlag_GCPurge))
    {
        ObjectAllocatorBase *objAllocator = getObjAllocator(objectDatV.clazz);
        return objAllocator && objAllocator->isValid(objectDatV.allocIdx) && objAllocator->getAt<Object>(objectDatV.allocIdx) == obj;
    }
    return false;
}

FORCE_INLINE bool isValidFast(const Object *obj)
{
    if (obj == nullptr)
    {
        return false;
    }
    ObjectDbIdx dbIdx = obj->getDbIdx();

    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
    ObjectPrivateDataView objectDatV = objectsDb.getObjectData(dbIdx);

    return objectDatV && NO_BITS_SET(objectDatV.flags, EObjectFlagBits::ObjFlag_MarkedForDelete | EObjectFlagBits::ObjFlag_GCPurge);
}

FORCE_INLINE bool isValidAlloc(const Object *obj)
{
    if (obj == nullptr)
    {
        return false;
    }
    ObjectDbIdx dbIdx = obj->getDbIdx();

    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
    ObjectPrivateDataView objectDatV = objectsDb.getObjectData(dbIdx);

    if (objectDatV && NO_BITS_SET(objectDatV.flags, EObjectFlagBits::ObjFlag_MarkedForDelete | EObjectFlagBits::ObjFlag_GCPurge))
    {
        ObjectAllocatorBase *objAllocator = getObjAllocator(objectDatV.clazz);
        return objAllocator && objAllocator->isValid(objectDatV.allocIdx);
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
COREOBJECTS_EXPORT void INTERNAL_createdCBEObject(Object *obj);
COREOBJECTS_EXPORT bool INTERNAL_isInMainThread();
COREOBJECTS_EXPORT bool INTERNAL_validateObjectName(StringView name, CBEClass clazz);
COREOBJECTS_EXPORT String INTERNAL_getValidObjectName(StringView name, CBEClass clazz);
FORCE_INLINE bool INTERNAL_validateCreatedObject(Object *, EObjectFlags flags) { return BIT_NOT_SET(flags, EObjectFlagBits::ObjFlag_Default); }
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
Object *INTERNAL_create(CBEClass clazz, StringView name, Object *outerObj, EObjectFlags flags = 0, CtorArgs... ctorArgs)
{
    if (clazz == nullptr)
    {
        alertAlwaysf(false, "Invalid class type! when creating object %.*s", name.length(), name.data());
        return nullptr;
    }

    // Validate inside main thread
    fatalAssertf(INTERNAL_isInMainThread(), "Instance of any class %s must be constructed inside main thread!", clazz->nameString);

    // If empty string then try create from class name
    String objectName = name.empty() ? clazz->nameString : name;
    // Using valid property name, Change if needed other wise also change in ObjectAllocatorBase::constructDefault
    if (!INTERNAL_validateObjectName(objectName, clazz))
    {
        alertAlwaysf(false, "Invalid object name! Invalid characters will be replaced with underscore(_)");
        objectName = INTERNAL_getValidObjectName(objectName, clazz);
    }
    NameString objFullPath = NameString(ObjectPathHelper::getFullPath(objectName, outerObj));

    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
#if DEV_BUILD
    if (objectsDb.hasObject({ .objectPath = objFullPath.toString().getChar(), .objectId = StringID(objFullPath) }))
    {
        LOG_WARN(
            "ObjectHelper",
            "Object with path %s already exists, If object path needs to be exactly same use createOrGet() to retrieve existing object",
            objFullPath
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
    ObjectAllocIdx allocIdx = ObjectAllocIdx(object->getDbIdx());
    INTERNAL_ObjectCoreAccessors::setDbIdx(object, CoreObjectsDB::InvalidDbIdx);

    // Object's data must be populated even before constructor is called
    if (objectsDb.hasObject({ .objectPath = objFullPath.toString().getChar(), .objectId = StringID(objFullPath) }))
    {
        // Appending allocation ID and class name will make it unique
        SizeT uniqueNameId = uint32(clazz->name);
        HashUtility::combineSeeds(uniqueNameId, INTERNAL_ObjectCoreAccessors::getAllocIdx(object));
        objectName = StringFormat::printf(TCHAR("%s_%llu"), objectName, uniqueNameId);
    }
    INTERNAL_ObjectCoreAccessors::setOuterAndName(object, objectName, outerObj, clazz);
    INTERNAL_ObjectCoreAccessors::setAllocIdx(object, allocIdx);
    EObjectFlags objFlags = (INTERNAL_ObjectCoreAccessors::getFlags(object) |= flags);

    object = ctor->invokeUnsafe<Object *, void *, CtorArgs...>(objPtr, std::forward<CtorArgs>(ctorArgs)...);

    if (!INTERNAL_validateCreatedObject(object, objFlags))
    {
        alertAlwaysf(false, "Object validation failed! Destroying %s", object->getObjectData().path);
        INTERNAL_destroyCBEObject(object);
        object = nullptr;
    }
    INTERNAL_createdCBEObject(object);
    return object;
}

template <typename... CtorArgs>
Object *create(CBEClass clazz, StringView name, Object *outerObj, EObjectFlags flags = 0, CtorArgs... ctorArgs)
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
Object *createOrGet(CBEClass clazz, StringView name, Object *outerObj, EObjectFlags flags = 0, CtorArgs... ctorArgs)
{
    String objFullPath = ObjectPathHelper::getFullPath(name, outerObj);
    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
    CoreObjectsDB::NodeIdxType objNodeIdx
        = objectsDb.getObjectNodeIdx({ .objectPath = objFullPath.getChar(), .objectId = objFullPath.getChar() });
    if (objectsDb.hasObject(objNodeIdx))
    {
        return objectsDb.getObject(objNodeIdx);
    }
    return create<CtorArgs...>(clazz, name, outerObj, flags, std::forward<CtorArgs>(ctorArgs)...);
}

template <typename ClassType, typename... CtorArgs>
ClassType *create(StringView name, Object *outerObj, EObjectFlags flags = 0, CtorArgs... ctorArgs)
{
    return static_cast<ClassType *>(create<CtorArgs...>(ClassType::staticType(), name, outerObj, flags, std::forward<CtorArgs>(ctorArgs)...));
}

template <typename ClassType, typename... CtorArgs>
ClassType *createOrGet(StringView name, Object *outerObj, EObjectFlags flags = 0, CtorArgs... ctorArgs)
{
    return static_cast<ClassType *>(
        createOrGet<CtorArgs...>(ClassType::staticType(), name, outerObj, flags, std::forward<CtorArgs>(ctorArgs)...)
    );
}

FORCE_INLINE Object *get(const TChar *objectFullPath)
{
    return ICoreObjectsModule::get()->getObjectsDB().getObject({ .objectPath = objectFullPath, .objectId = objectFullPath });
}
FORCE_INLINE Object *get(StringID objectID, const TChar *objectFullPath)
{
    return ICoreObjectsModule::get()->getObjectsDB().getObject({ .objectPath = objectFullPath, .objectId = objectID });
}

template <typename ClassType>
ClassType *get(const TChar *objectFullPath)
{
    Object *obj = get(objectFullPath);
    return cast<ClassType>(obj);
}

COREOBJECTS_EXPORT Object *load(String objectPath, CBEClass clazz);
COREOBJECTS_EXPORT Object *getOrLoad(String objectPath, CBEClass clazz);
template <typename ClassType>
ClassType *load(const String &objectPath)
{
    Object *obj = load(objectPath, ClassType::staticType());
    return cast<ClassType>(obj);
}
template <typename ClassType>
ClassType *getOrLoad(const String &objectPath)
{
    Object *obj = getOrLoad(objectPath, ClassType::staticType());
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
COREOBJECTS_EXPORT bool
deepCopy(Object *fromObject, Object *toObject, EObjectFlags additionalFlags = 0, EObjectFlags clearFlags = 0, bool bConstructToObject = true);
COREOBJECTS_EXPORT Object *duplicateCBEObject(
    Object *fromObject, Object *newOuter, StringView newName = {}, EObjectFlags additionalFlags = 0, EObjectFlags clearFlags = 0
);

template <typename T, typename AsType = T>
AsType *duplicateObject(T *fromObject, Object *newOuter, StringView newName = {}, EObjectFlags additionalFlags = 0, EObjectFlags clearFlags = 0)
{
    return cast<AsType>(duplicateCBEObject(fromObject, newOuter, newName, additionalFlags, clearFlags));
}

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