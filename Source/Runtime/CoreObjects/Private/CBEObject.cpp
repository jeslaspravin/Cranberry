/*!
 * \file CBEObject.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
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
    destroy();

    if (BIT_NOT_SET(flags, EObjectFlagBits::ObjFlag_GCPurge))
    {
        String objPath = getFullPath();
        CoreObjectsDB::NodeIdxType objNodeIdx
            = CoreObjectsModule::objectsDB().getObjectNodeIdx({ .objectPath = objPath.getChar(), .objectId = sid });

        // Must have entry in object DB if we constructed the objects properly unless object is default
        debugAssert(CoreObjectsModule::objectsDB().hasObject(objNodeIdx) || BIT_SET(flags, EObjectFlagBits::ObjFlag_Default));
        if (CoreObjectsModule::objectsDB().hasObject(objNodeIdx))
        {
            CoreObjectsModule::objectsDB().removeObject(objNodeIdx);
        }
    }

    objOuter = nullptr;
    sid = StringID();
    SET_BITS(flags, EObjectFlagBits::ObjFlag_Deleted);
}

void Object::beginDestroy()
{
    markReadyForDestroy();

    CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();

    uint64 uniqNameSuffix = 0;
    String newObjName = objectName + TCHAR("_Delete");
    String newObjPath = ObjectPathHelper::getFullPath(newObjName.getChar(), objOuter);
    while (objectsDb.hasObject({ .objectPath = newObjPath.getChar(), .objectId = newObjPath.getChar() }))
    {
        newObjName = objectName + TCHAR("_Delete") + String::toString(uniqNameSuffix);
        newObjPath = ObjectPathHelper::getFullPath(newObjName.getChar(), objOuter);
        uniqNameSuffix++;
    }
    // Rename it Immediately to allow other objects to replace this object with same name
    INTERNAL_ObjectCoreAccessors::setOuterAndName(this, newObjName, objOuter, getType());
}

String Object::getFullPath() const { return ObjectPathHelper::getFullPath(this); }

//////////////////////////////////////////////////////////////////////////
/// PrivateObjectCoreAccessors implementations
//////////////////////////////////////////////////////////////////////////

EObjectFlags &INTERNAL_ObjectCoreAccessors::getFlags(Object *object) { return object->flags; }

ObjectAllocIdx INTERNAL_ObjectCoreAccessors::getAllocIdx(const Object *object) { return object->allocIdx; }

void INTERNAL_ObjectCoreAccessors::setAllocIdx(Object *object, ObjectAllocIdx allocIdx) { object->allocIdx = allocIdx; }

void INTERNAL_ObjectCoreAccessors::setOuterAndName(Object *object, const String &newName, Object *outer, CBEClass clazz /*= nullptr*/)
{
    fatalAssertf(!newName.empty(), "Object name cannot be empty");
    if (outer == object->getOuter() && object->getName() == newName)
    {
        return;
    }

    CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();

    String objPath = object->getFullPath();
    String newObjPath = ObjectPathHelper::getFullPath(newName.getChar(), outer);
    StringID newSid(newObjPath);
    fatalAssertf(
        !objectsDb.hasObject({ .objectPath = newObjPath.getChar(), .objectId = newSid }),
        "Object cannot be renamed to another existing object! [Old name: %s, New name: %s]", object->getName(), newName
    );

    CoreObjectsDB::NodeIdxType existingNodeIdx
        = objectsDb.getObjectNodeIdx({ .objectPath = objPath.getChar(), .objectId = object->getStringID() });
    if (object->getStringID().isValid() && objectsDb.hasObject(existingNodeIdx))
    {
        // Setting object name here so that sub object's new full path can be calculated easily
        object->objectName = newName;

        // If there is child then all of them must be renamed before object can be renamed
        if (objectsDb.hasChild(existingNodeIdx))
        {
            std::vector<CoreObjectsDB::NodeIdxType> subobjNodeIdxs;
            objectsDb.getSubobjects(subobjNodeIdxs, existingNodeIdx);
            for (CoreObjectsDB::NodeIdxType subObjNodeIdx : subobjNodeIdxs)
            {
                debugAssert(objectsDb.hasObject(subObjNodeIdx));
                Object *subObj = objectsDb.getObject(subObjNodeIdx);
                String newSubObjFullPath = subObj->getFullPath();

                subObj->sid = newSubObjFullPath.getChar();
                // Only need to set name, No need to reset parent
                objectsDb.setObject(subObjNodeIdx, subObj->getStringID(), newSubObjFullPath.getChar());
            }
        }

        objectsDb.setObject(existingNodeIdx, newSid, newObjPath.getChar());
        // It is fine to use the existingNodeIdx since the node index do not change when changing object id or parent
        if (outer)
        {
            String outerObjPath = outer->getFullPath();
            objectsDb.setObjectParent(existingNodeIdx, { .objectPath = outerObjPath.getChar(), .objectId = outer->getStringID() });
        }
        else
        {
            objectsDb.setObjectParent(existingNodeIdx, { .objectId = StringID::INVALID });
        }
    }
    else
    {
        // constructing object's name
        new (&object->objectName) String(newName);

        CoreObjectsDB::ObjectData objData{
            .path = newObjPath, .clazz = (clazz != nullptr ? clazz : object->getType()), .allocIdx = object->allocIdx, .sid = newSid
        };

        if (outer)
        {
            String outerObjPath = outer->getFullPath();
            objectsDb.addObject(newSid, objData, { .objectPath = outerObjPath.getChar(), .objectId = outer->getStringID() });
        }
        else
        {
            objectsDb.addRootObject(newSid, objData);
        }
    }
    // Setting object outer
    object->objOuter = outer;
    // Setting object's new sid
    object->sid = newSid;
}

void INTERNAL_ObjectCoreAccessors::setOuter(Object *object, Object *outer) { setOuterAndName(object, object->objectName, outer); }

void INTERNAL_ObjectCoreAccessors::renameObject(Object *object, const String &newName) { setOuterAndName(object, newName, object->objOuter); }
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

String ObjectPathHelper::getObjectPath(const cbe::Object *object, const cbe::Object *stopAt)
{
    debugAssert(stopAt != object);

    if (object->getOuter() == nullptr)
    {
        return object->getName();
    }

    std::vector<const TChar *> outers;
    // Since last path element must be this obj name
    outers.emplace_back(object->getName().getChar());
    const cbe::Object *outer = object->getOuter();
    while (outer != stopAt && outer->getOuter())
    {
        outers.emplace_back(outer->getName().getChar());
        outer = outer->getOuter();
    }
    if (outer != stopAt)
    {
        debugAssertf(stopAt == nullptr, "Object %s is not subobject of %s", object->getFullPath(), stopAt->getFullPath());
        return outer->getName() + ObjectPathHelper::RootObjectSeparator
               + String::join(outers.crbegin(), outers.crend(), ObjectPathHelper::ObjectObjectSeparator);
    }
    return String::join(outers.crbegin(), outers.crend(), ObjectPathHelper::ObjectObjectSeparator);
}

FORCE_INLINE String ObjectPathHelper::getFullPath(const cbe::Object *object)
{
    if (object->getOuter() == nullptr)
    {
        return object->getName();
    }

    std::vector<const TChar *> outers;
    // Since last path element must be this obj name
    outers.emplace_back(object->getName().getChar());
    const cbe::Object *outer = object->getOuter();
    while (outer->getOuter())
    {
        outers.emplace_back(outer->getName().getChar());
        outer = outer->getOuter();
    }
    String objectPath = outer->getName() + ObjectPathHelper::RootObjectSeparator
                        + String::join(outers.crbegin(), outers.crend(), ObjectPathHelper::ObjectObjectSeparator);

    return objectPath;
}

String ObjectPathHelper::getFullPath(StringView objectName, const cbe::Object *outerObj)
{
    if (outerObj)
    {
        String outputStr = outerObj->getFullPath();
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