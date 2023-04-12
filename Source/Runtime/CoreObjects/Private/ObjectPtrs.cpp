/*!
 * \file ObjectPtrs.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "ObjectPtrs.h"
#include "ObjectPathHelpers.h"

namespace cbe
{

ObjectPath &ObjectPath::operator= (const TChar *fullPath)
{
    allocIdx = 0;

    StringView outerPathView, objectNameView;
    packagePath = ObjectPathHelper::getPathComponents(outerPathView, objectNameView, fullPath);
    outerPath = outerPathView;
    objectName = objectNameView;

    Object *obj = get(fullPath);
    if (cbe::isValidFast(obj))
    {
        allocIdx = INTERNAL_ObjectCoreAccessors::getAllocIdx(obj);
    }
    return *this;
}
ObjectPath &ObjectPath::operator= (Object *obj)
{
    if (!cbe::isValidFast(obj))
    {
        reset();
        return *this;
    }
    allocIdx = INTERNAL_ObjectCoreAccessors::getAllocIdx(obj);

    StringView objFullPath = obj->getObjectData().path;
    StringView outerPathView, objectNameView;
    packagePath = ObjectPathHelper::getPathComponents(outerPathView, objectNameView, objFullPath);
    outerPath = outerPathView;
    objectName = objectNameView;

    return *this;
}

ObjectPath::ObjectPath(Object *outerObj, const TChar *objectName)
{
    if (!cbe::isValidFast(outerObj) && TCharStr::empty(objectName))
    {
        reset();
        return;
    }

    (*this) = ObjectPathHelper::getFullPath(objectName, outerObj).getChar();
}

String ObjectPath::getFullPath() const
{
    return ObjectPathHelper::combinePathComponents(packagePath.getChar(), outerPath.getChar(), objectName.getChar());
}

Object *ObjectPath::getObject() const
{
    const String fullPath = getFullPath();
    Object *obj = get(fullPath.getChar());
    if (!cbe::isValidFast(obj))
    {
        obj = load(fullPath, nullptr);
    }
    if (cbe::isValidFast(obj))
    {
        const ObjectAllocIdx foundObjAllocIdx = INTERNAL_ObjectCoreAccessors::getAllocIdx(obj);
        // 0 is for default mostly and it will always be valid
        if (!(allocIdx == 0 || foundObjAllocIdx == allocIdx))
        {
            LOG_WARN("ObjectPath", "Object %s[allocIdx %llu] does not matches allocation index %llu", fullPath, allocIdx, foundObjAllocIdx);
        }
        return obj;
    }
    return nullptr;
}

} // namespace cbe
