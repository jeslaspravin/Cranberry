/*!
 * \file ObjectPtrs.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "ObjectPtrs.h"
#include "ObjectPathHelpers.h"
#include "CoreObjectsModule.h"

namespace cbe
{

ObjectPath &ObjectPath::operator= (const TChar *fullPath) noexcept
{
    dbIdx = CoreObjectsDB::InvalidDbIdx;

    StringView outerPathView, objectNameView;
    packagePath = ObjectPathHelper::getPathComponents(outerPathView, objectNameView, fullPath);
    outerPath = outerPathView;
    objectName = objectNameView;

    Object *obj = get(fullPath);
    if (cbe::isValidFast(obj))
    {
        dbIdx = obj->getDbIdx();
    }
    return *this;
}
ObjectPath &ObjectPath::operator= (Object *obj) noexcept
{
    if (!cbe::isValidFast(obj))
    {
        reset();
        return *this;
    }
    dbIdx = obj->getDbIdx();

    StringView objFullPath = obj->getObjectData().path;
    StringView outerPathView, objectNameView;
    packagePath = ObjectPathHelper::getPathComponents(outerPathView, objectNameView, objFullPath);
    outerPath = outerPathView;
    objectName = objectNameView;

    return *this;
}

ObjectPath::ObjectPath(Object *outerObj, const TChar *objectName) noexcept
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

    const CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();
    Object *obj = nullptr;
    if (dbIdx != CoreObjectsDB::InvalidDbIdx)
    {
        obj = objectsDb.getObject(dbIdx);
    }
    else
    {
        obj = objectsDb.getObject(objectsDb.getObjectNodeIdx({ .objectPath = fullPath, .objectId = fullPath.getChar() }));
    }
    if (!cbe::isValidFast(obj))
    {
        obj = load(fullPath, nullptr);
    }

    return obj;
}

bool ObjectPath::isValid() const
{
    const String fullPath = getFullPath();

    const CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();
    ObjectPrivateDataView objDatV;
    if (dbIdx != CoreObjectsDB::InvalidDbIdx)
    {
        objDatV = objectsDb.getObjectData(dbIdx);
    }
    else
    {
        objDatV = objectsDb.getObjectData(objectsDb.getObjectNodeIdx({ .objectPath = fullPath, .objectId = fullPath.getChar() }));
    }
    return objDatV && TCharStr::isEqual(objDatV.path, fullPath.getChar());
}

void ObjectPath::refreshCache()
{
    const String fullPath = getFullPath();

    const CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();
    // If dbIdx is valid value check if the obj is valid and clear dbIdx if not
    if (dbIdx != CoreObjectsDB::InvalidDbIdx)
    {
        Object *obj = objectsDb.getObject(dbIdx);
        if (!cbe::isValidFast(obj) || !TCharStr::isEqual(objectsDb.getObjectData(dbIdx).path, fullPath.getChar()))
        {
            dbIdx = CoreObjectsDB::InvalidDbIdx;
        }
    }
    // Invalid dbIdx check if we can get the object from the db
    if (dbIdx == CoreObjectsDB::InvalidDbIdx)
    {
        dbIdx = objectsDb.getObjectNodeIdx({ .objectPath = fullPath, .objectId = fullPath.getChar() });
    }
    // If still invalid dbIdx try loading as the last step
    if (dbIdx == CoreObjectsDB::InvalidDbIdx)
    {
        Object *obj = load(fullPath, nullptr);
        dbIdx = obj ? obj->getDbIdx() : CoreObjectsDB::InvalidDbIdx;
    }
}

bool ObjectPath::isValidCache() const
{
    const String fullPath = getFullPath();

    const CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();
    ObjectPrivateDataView objDatV = ObjectPrivateDataView::getInvalid();
    if (dbIdx != CoreObjectsDB::InvalidDbIdx)
    {
        objDatV = objectsDb.getObjectData(dbIdx);
    }

    return objDatV && TCharStr::isEqual(objDatV.path, fullPath.getChar());
}

} // namespace cbe
