/*!
 * \file CBEObject.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CBEObject.h"
#include "CoreObjectsDB.h"
#include "CoreObjectsModule.h"
#include "ObjectPathHelpers.h"

namespace cbe
{

//////////////////////////////////////////////////////////////////////////
/// cbe::Object implementations
//////////////////////////////////////////////////////////////////////////

void Object::destroyObject()
{
    ObjectPrivateDataView objDatV = getObjectData();
    debugAssert(objDatV);

    destroy();
    if (BIT_NOT_SET(objDatV.flags, EObjectFlagBits::ObjFlag_GCPurge))
    {
        CoreObjectsModule::objectsDB().removeObject(getDbIdx());
    }
}

void Object::beginDestroy()
{
    CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();
    ObjectPrivateDataView objectDatV = getObjectData();
    String objectNameBase = objectDatV.name;
    Object *outerObj = objectsDb.getObject(objectDatV.outerIdx);

    uint64 uniqNameSuffix = 0;
    String newObjName = objectNameBase + TCHAR("_Delete");
    String newObjPath = ObjectPathHelper::getFullPath(newObjName.getChar(), outerObj);
    while (objectsDb.hasObject({ .objectPath = newObjPath.getChar(), .objectId = newObjPath.getChar() }))
    {
        newObjName = objectNameBase + TCHAR("_Delete") + String::toString(uniqNameSuffix);
        newObjPath = ObjectPathHelper::getFullPath(newObjName.getChar(), outerObj);
        uniqNameSuffix++;
    }
    // Rename it Immediately to allow other objects to replace this object with same name
    INTERNAL_ObjectCoreAccessors::setOuterAndName(this, newObjName, outerObj, getType());
    SET_BITS(INTERNAL_ObjectCoreAccessors::getFlags(this), EObjectFlagBits::ObjFlag_MarkedForDelete);
}

cbe::ObjectPrivateDataView Object::getObjectData() const { return CoreObjectsModule::objectsDB().getObjectData(getDbIdx()); }

cbe::Object *Object::getOuter() const
{
    const CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();
    ObjectDbIdx outerIdx = objectsDb.getParentIdx(getDbIdx());
    return objectsDb.getObject(outerIdx);
}
cbe::Object *Object::getOuterMost() const
{
    const CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();

    ObjectDbIdx lastValidOuter = CoreObjectsDB::InvalidDbIdx;
    ObjectDbIdx outerIdx = objectsDb.getParentIdx(getDbIdx());
    while (outerIdx != CoreObjectsDB::InvalidDbIdx)
    {
        lastValidOuter = outerIdx;
        outerIdx = objectsDb.getParentIdx(outerIdx);
    }

    return objectsDb.getObject(lastValidOuter);
}
cbe::Object *Object::getOuterOfType(CBEClass clazz) const
{
    const CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();

    ObjectDbIdx outerIdx = objectsDb.getParentIdx(getDbIdx());
    while (ObjectPrivateDataView objectDatV = objectsDb.getObjectData(outerIdx))
    {
        if (objectDatV.clazz == clazz)
        {
            return objectsDb.getObject(outerIdx);
        }
        outerIdx = objectDatV.outerIdx;
    }
    return nullptr;
}

bool Object::hasOuter(Object *checkOuter) const
{
    const CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();
    ObjectDbIdx checkOuterIdx = checkOuter->getDbIdx();

    ObjectDbIdx outerIdx = objectsDb.getParentIdx(getDbIdx());
    bool bHasOuter = false;
    while (outerIdx != CoreObjectsDB::InvalidDbIdx && !bHasOuter)
    {
        bHasOuter = bHasOuter || outerIdx == checkOuterIdx;
        outerIdx = objectsDb.getParentIdx(outerIdx);
    }
    return bHasOuter;
}

EObjectFlags Object::collectAllFlags() const
{
    EObjectFlags retVal = 0;
    const CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();
    for (cbe::ObjectPrivateDataView objectDatV = objectsDb.getObjectData(getDbIdx()); objectDatV;
         objectDatV = objectsDb.getObjectData(objectDatV.outerIdx))
    {
        retVal |= objectDatV.flags;
    }
    return retVal;
}

//////////////////////////////////////////////////////////////////////////
/// PrivateObjectCoreAccessors implementations
//////////////////////////////////////////////////////////////////////////

EObjectFlags &INTERNAL_ObjectCoreAccessors::getFlags(Object *object) { return CoreObjectsModule::objectsDB().objectFlags(object->getDbIdx()); }

ObjectAllocIdx INTERNAL_ObjectCoreAccessors::getAllocIdx(const Object *object)
{
    return CoreObjectsModule::objectsDB().getObjectData(object->getDbIdx()).allocIdx;
}
void INTERNAL_ObjectCoreAccessors::setAllocIdx(Object *object, ObjectAllocIdx allocIdx)
{
    CoreObjectsModule::objectsDB().setAllocIdx(object->getDbIdx(), allocIdx);
}

void INTERNAL_ObjectCoreAccessors::setDbIdx(Object *object, ObjectDbIdx dbIdx) { object->dbIdx = dbIdx; }

void INTERNAL_ObjectCoreAccessors::setOuterAndName(Object *object, StringView newName, Object *outer, CBEClass clazz /*= nullptr*/)
{
    fatalAssertf(!newName.empty(), "Object name cannot be empty");

    CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();
    ObjectPrivateDataView objectDatV = objectsDb.getObjectData(object->getDbIdx());

    String newObjPath = ObjectPathHelper::getFullPath(newName, outer);
    StringID newSid(newObjPath);
    const bool bNewNameIsUnique = !objectsDb.hasObject({ .objectPath = newObjPath.getChar(), .objectId = newSid });
    fatalAssertf(
        bNewNameIsUnique, "Object cannot be renamed to another existing object! [Old name: {}, New name: {}]", objectDatV.name, newName
    );

    if (objectDatV.isValid())
    {
        ObjectDbIdx outerDbIdx = outer ? outer->getDbIdx() : CoreObjectsDB::InvalidDbIdx;
        if (objectDatV.outerIdx == outerDbIdx && objectDatV.name == newName)
        {
            return;
        }

        ObjectDbIdx existingNodeIdx = object->getDbIdx();
        // Setting object name here so that sub object's new full path can be calculated easily
        objectsDb.setObject(existingNodeIdx, newSid, newObjPath, newName);
        // It is fine to use the existingNodeIdx since the node index do not change when changing object id or parent
        objectsDb.setObjectParent(existingNodeIdx, outerDbIdx);

        // If there is child then all of them must be renamed before object can be renamed
        if (objectsDb.hasChild(existingNodeIdx))
        {
            std::vector<CoreObjectsDB::NodeIdxType> subobjNodeIdxs;
            objectsDb.getSubobjects(subobjNodeIdxs, existingNodeIdx);
            for (CoreObjectsDB::NodeIdxType subObjNodeIdx : subobjNodeIdxs)
            {
                debugAssert(objectsDb.hasObject(subObjNodeIdx));
                Object *subObj = objectsDb.getObject(subObjNodeIdx);
                String newSubObjFullPath = ObjectPathHelper::computeFullPath(subObj);
                StringID newSubObjSid{ newSubObjFullPath };

                // Only need to set name, No need to reset parent
                objectsDb.setObject(subObjNodeIdx, newSubObjSid, newSubObjFullPath, objectsDb.getObjectData(subObjNodeIdx).name);
            }
        }
    }
    else
    {
        clazz = clazz != nullptr ? clazz : object->getType();
        ObjectDbIdx dbIdx = CoreObjectsDB::InvalidDbIdx;
        if (outer)
        {
            dbIdx = objectsDb.addObject(newSid, newObjPath, newName, clazz, outer->getDbIdx());
        }
        else
        {
            dbIdx = objectsDb.addRootObject(newSid, newObjPath, newName, clazz);
        }
        setDbIdx(object, dbIdx);
    }
}

void INTERNAL_ObjectCoreAccessors::setOuter(Object *object, Object *outer) { setOuterAndName(object, object->getObjectData().name, outer); }

void INTERNAL_ObjectCoreAccessors::renameObject(Object *object, StringView newName)
{
    setOuterAndName(object, newName, CoreObjectsModule::objectsDB().getObject(object->getObjectData().outerIdx));
}
} // namespace cbe

ObjectArchive &ObjectArchive::serialize(cbe::Object *& /*obj*/)
{
    fatalAssertf(false, "cbe::Object serialization not implemented!");
    return *this;
}
void ObjectArchive::relinkSerializedPtr(void ** /*objPtrPtr*/) const { fatalAssertf(false, "relinkSerializedPtr not implemented!"); }
void ObjectArchive::relinkSerializedPtr(const void ** /*objPtrPtr*/) const { fatalAssertf(false, "relinkSerializedPtr not implemented!"); }

//////////////////////////////////////////////////////////////////////////
/// ObjectPathHelper implementations
//////////////////////////////////////////////////////////////////////////

FORCE_INLINE StringView ObjectPathHelper::getOuterPathAndObjectName(StringView &outObjectName, StringView objectPath)
{
    debugAssert(!TCharStr::find(objectPath, ObjectPathHelper::RootObjectSeparator));

    SizeT outerObjectSepIdx;
    if (TCharStr::rfind(objectPath, ObjectPathHelper::ObjectObjectSeparator, &outerObjectSepIdx))
    {
        outObjectName = objectPath.substr(outerObjectSepIdx + 1);
        return objectPath.substr(0, outerObjectSepIdx);
    }
    outObjectName = objectPath;
    return {};
}

String ObjectPathHelper::computeFullPath(const cbe::Object *object)
{
    const CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();
    std::vector<StringView> outers;
    for (cbe::ObjectPrivateDataView objectDatV = object->getObjectData(); objectDatV; objectDatV = objectsDb.getObjectData(objectDatV.outerIdx))
    {
        outers.emplace_back(objectDatV.name);
    }

    return String(outers.back()) + ObjectPathHelper::RootObjectSeparator
           + String::join(outers.crbegin() + 1, outers.crend(), ObjectPathHelper::ObjectObjectSeparator);
}

String ObjectPathHelper::computeFullPath(StringView objectName, const cbe::Object *outerObj)
{
    if (outerObj)
    {
        String outputStr = computeFullPath(outerObj);
        SizeT outerPathLen = outputStr.length();
        outputStr.resize(
            outerPathLen + objectName.length() + 1,
            outerObj->getOuter() ? ObjectPathHelper::ObjectObjectSeparator : ObjectPathHelper::RootObjectSeparator
        );
        objectName.copy(outputStr.data() + outerPathLen + 1, objectName.length());
        return outputStr;
    }
    return objectName;
}

String ObjectPathHelper::computeObjectPath(const cbe::Object *object, const cbe::Object *stopAt)
{
    debugAssert(stopAt != object);

    cbe::ObjectPrivateDataView objectDatV = object->getObjectData();
    const CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();
    if (objectDatV.outerIdx == CoreObjectsDB::InvalidDbIdx)
    {
        return objectDatV.name;
    }

    std::vector<StringView> outers;
    // Since last path element must be this obj name
    outers.emplace_back(objectDatV.name);

    ObjectDbIdx outerIdx = objectDatV.outerIdx;
    objectDatV = objectsDb.getObjectData(outerIdx);
    // Check if this outer has an outer else it is last outer and must be package
    while (outerIdx != stopAt->getDbIdx() && objectDatV.outerIdx != CoreObjectsDB::InvalidDbIdx)
    {
        outers.emplace_back(objectDatV.name);
        outerIdx = objectDatV.outerIdx;
        objectDatV = objectsDb.getObjectData(outerIdx);
    }
    if (outerIdx != stopAt->getDbIdx())
    {
        debugAssertf(stopAt == nullptr, "Object {} is not subobject of {}", objectDatV.path, stopAt->getObjectData().path);
        return String(objectDatV.name) + ObjectPathHelper::RootObjectSeparator
               + String::join(outers.crbegin(), outers.crend(), ObjectPathHelper::ObjectObjectSeparator);
    }
    return String::join(outers.crbegin(), outers.crend(), ObjectPathHelper::ObjectObjectSeparator);
}

String ObjectPathHelper::getFullPath(StringView objectName, const cbe::Object *outerObj)
{
    if (outerObj)
    {
        String outputStr = outerObj->getObjectData().path;
        SizeT outerPathLen = outputStr.length();
        outputStr.resize(
            outerPathLen + objectName.length() + 1,
            outerObj->getOuter() ? ObjectPathHelper::ObjectObjectSeparator : ObjectPathHelper::RootObjectSeparator
        );
        objectName.copy(outputStr.data() + outerPathLen + 1, objectName.length());
        return outputStr;
    }
    return objectName;
}

StringView ObjectPathHelper::getPackagePath(StringView objFullPath)
{
    SizeT rootObjSepIdx;
    bool bRootSepFound = TCharStr::find(objFullPath, ObjectPathHelper::RootObjectSeparator, &rootObjSepIdx);
    if (bRootSepFound)
    {
        return objFullPath.substr(0, rootObjSepIdx);
    }
    return {};
}

StringView ObjectPathHelper::getPathComponents(StringView &outOuterObjectPath, StringView &outObjectName, StringView objFullPath)
{
    SizeT rootObjSepIdx;
    bool bRootSepFound = TCharStr::find(objFullPath, ObjectPathHelper::RootObjectSeparator, &rootObjSepIdx);

    if (bRootSepFound)
    {
        // Path after RootObjectSeparator is outers... and object name
        outOuterObjectPath = getOuterPathAndObjectName(outObjectName, objFullPath.substr(rootObjSepIdx + 1));
        return objFullPath.substr(0, rootObjSepIdx);
    }
    else
    {
        outOuterObjectPath = getOuterPathAndObjectName(outObjectName, objFullPath);
        return {};
    }
}

String ObjectPathHelper::combinePathComponents(StringView packagePath, StringView outerObjectPath, StringView objectName)
{
    // Ensure that package path is package path without any additional root path and outer object is without any root/package object path
    debugAssert(!packagePath.empty() && !TCharStr::find(packagePath, ObjectPathHelper::RootObjectSeparator));

    String outputStr;
    // + 2 for Root separator and object separator
    outputStr.reserve(packagePath.length() + outerObjectPath.length() + objectName.length() + 2);

    if (outerObjectPath.empty())
    {
        outputStr.resize(packagePath.length() + objectName.length() + 1);

        outputStr[packagePath.length()] = ObjectPathHelper::RootObjectSeparator;

        packagePath.copy(outputStr.data(), packagePath.length());
        objectName.copy(outputStr.data() + packagePath.length() + 1, objectName.length());
    }
    else
    {
        outputStr.resize(outputStr.capacity());
        SizeT outerIdx = packagePath.length() + 1;
        SizeT objNameIdx = outerIdx + outerObjectPath.length() + 1;

        outputStr[outerIdx - 1] = ObjectPathHelper::RootObjectSeparator;
        outputStr[objNameIdx - 1] = ObjectPathHelper::ObjectObjectSeparator;

        packagePath.copy(outputStr.data(), packagePath.length());
        outerObjectPath.copy(outputStr.data() + outerIdx, outerObjectPath.length());
        objectName.copy(outputStr.data() + objNameIdx, objectName.length());
    }
    return outputStr;
}

StringView ObjectPathHelper::getObjectName(StringView objPath)
{
    SizeT outerObjectSepIdx;
    if (TCharStr::rfind(objPath, ObjectPathHelper::ObjectObjectSeparator, &outerObjectSepIdx))
    {
        return objPath.substr(outerObjectSepIdx + 1);
    }
    return {};
}

StringView ObjectPathHelper::splitPackageNameAndPath(StringView &outName, StringView objPath)
{
    SizeT rootObjSepIdx;
    bool bRootSepFound = TCharStr::find(objPath, ObjectPathHelper::RootObjectSeparator, &rootObjSepIdx);

    if (bRootSepFound)
    {
        return getOuterPathAndObjectName(outName, objPath.substr(0, rootObjSepIdx));
    }
    return getOuterPathAndObjectName(outName, objPath);
}