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

ObjectPath &ObjectPath::operator=(const TChar *fullPath)
{
    allocIdx = 0;

    packagePath = ObjectPathHelper::getPathComponents(outerPath, objectName, fullPath);
    Object *obj = getObject();
    if (isValid(obj))
    {
        allocIdx = INTERNAL_ObjectCoreAccessors::getAllocIdx(obj);
    }
    return *this;
}
ObjectPath &ObjectPath::operator=(Object *obj)
{
    allocIdx = INTERNAL_ObjectCoreAccessors::getAllocIdx(obj);
    packagePath = ObjectPathHelper::getPathComponents(outerPath, objectName, obj->getFullPath().getChar());
    return *this;
}

ObjectPath::ObjectPath(Object *outerObj, const TChar *objectName) { (*this) = ObjectPathHelper::getFullPath(objectName, outerObj).getChar(); }

String ObjectPath::getFullPath() const { return ObjectPathHelper::combinePathComponents(packagePath, outerPath, objectName); }

Object *ObjectPath::getObject() const
{
    String fullPath = getFullPath();
    Object *obj = get(fullPath.getChar());
    if (!isValid(obj))
    {
        obj = load(fullPath, nullptr);
    }
    if (isValid(obj))
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
