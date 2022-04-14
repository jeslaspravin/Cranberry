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
#include "CoreObjectsDB.h"
#include "ObjectPathHelpers.h"
#include "Property/PropertyHelper.h"

namespace CBE
{
template <typename... CtorArgs>
Object *create(
    CBEClass clazz, const String &name, Object *outerObj, EObjectFlags flags = 0, CtorArgs &&...ctorArgs)
{
#if DEV_BUILD
    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
    String objectFullPath = ObjectPathHelper::getFullPath(name.getChar(), outerObj);
    fatalAssert(!objectsDb.hasObject(objectFullPath),
        "Use createOrGet() object interface in existing object %s", objectFullPath);
#endif // DEV_BUILD

    // void* is first param of ctor since we pass in object on which construction needs to be executed
    const GlobalFunctionWrapper *ctor = PropertyHelper::findMatchingCtor<void *, CtorArgs...>(clazz);
    alertIf(ctor, "Constructor arguments are invalid");
    if (!ctor)
    {
        LOG_ERROR("CBEObject", "Cannot construct object with given constructor arguments");
        return nullptr;
    }

    void *objPtr
        = static_cast<const GlobalFunctionWrapper *>(clazz->allocFunc->funcPtr)->invokeUnsafe<void *>();
    Object *object = reinterpret_cast<Object *>(objPtr);

    // Object's data must be populated even before constructor is called
    PrivateObjectCoreAccessors::getFlags(object) |= flags;
    PrivateObjectCoreAccessors::setOuterAndName(object, name, outerObj, clazz);

    object
        = ctor->invokeUnsafe<Object *, void *, CtorArgs...>(objPtr, std::forward<CtorArgs>(ctorArgs)...);

    return object;
}

template <typename... CtorArgs>
Object *createOrGet(
    CBEClass clazz, const String &name, Object *outerObj, EObjectFlags flags = 0, CtorArgs &&...ctorArgs)
{
    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
    StringID objectFullPath = ObjectPathHelper::getFullPath(name.getChar(), outerObj);
    if (objectsDb.hasObject(objectFullPath))
    {
        return objectsDb.getObject(objectFullPath);
    }
    return create<CtorArgs...>(clazz, name, outerObj, flags, std::forward<CtorArgs>(ctorArgs)...);
}

FORCE_INLINE bool isValid(const Object *obj)
{
    return obj
           && NO_BITS_SET(obj->getFlags(), EObjectFlagBits::Deleted | EObjectFlagBits::MarkedForDelete);
}
} // namespace CBE