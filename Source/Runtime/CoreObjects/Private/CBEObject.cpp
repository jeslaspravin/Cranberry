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

namespace CBE
{

//////////////////////////////////////////////////////////////////////////
/// CBE::Object implementations
//////////////////////////////////////////////////////////////////////////

void Object::destroyObject()
{
    destroy();

    // Must have entry in object DB if we constructed the objects properly unless object is default
    debugAssert(CoreObjectsModule::objectsDB().hasObject(sid) || BIT_SET(flags, EObjectFlagBits::Default));
    if (CoreObjectsModule::objectsDB().hasObject(sid))
    {
        CoreObjectsModule::objectsDB().removeObject(sid);
    }
    objOuter = nullptr;
    sid = StringID();
    SET_BITS(flags, EObjectFlagBits::Deleted);
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
    fatalAssert(!newName.empty(), "Object name cannot be empty");
    if (outer == object->getOuter() && object->getName() == newName)
        return;

    CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();

    // Setting object outer
    object->objOuter = outer;

    StringID newSid(ObjectPathHelper::getFullPath(newName.getChar(), outer));
    if (object->getStringID().isValid() && objectsDb.hasObject(object->getStringID()))
    {
        // Setting object name
        object->objectName = newName;
        objectsDb.setObject(object->getStringID(), newSid);
        if (outer)
        {
            objectsDb.setObjectParent(newSid, object->getOuter()->getStringID());
        }
        else
        {
            objectsDb.setObjectParent(newSid, StringID::INVALID);
        }
    }
    else
    {
        // constructing object's name
        new (&object->objectName) String(newName);

        CoreObjectsDB::ObjectData objData{ .clazz = (clazz != nullptr ? clazz : object->getType()),
                                           .allocIdx = object->allocIdx,
                                           .sid = newSid };

        if (outer)
        {
            objectsDb.addObject(newSid, objData, outer->getStringID());
        }
        else
        {
            objectsDb.addRootObject(newSid, objData);
        }
    }
    // Setting object's new sid
    object->sid = newSid;
}

void INTERNAL_ObjectCoreAccessors::setOuter(Object *object, Object *outer) { setOuterAndName(object, object->objectName, outer); }

void INTERNAL_ObjectCoreAccessors::renameObject(Object *object, const String &newName) { setOuterAndName(object, newName, object->objOuter); }
} // namespace CBE

ObjectArchive &ObjectArchive::serialize(CBE::Object *&obj)
{
    fatalAssert(false, "CBE::Object serialization not implemented!");
    return *this;
}

//////////////////////////////////////////////////////////////////////////
/// ObjectPathHelper implementations
//////////////////////////////////////////////////////////////////////////

FORCE_INLINE String ObjectPathHelper::getOuterPathAndObjectName(String &outObjectName, const TChar *objectPath)
{
    debugAssert(!TCharStr::find(objectPath, ObjectPathHelper::RootObjectSeparator));

    SizeT outerObjectSepIdx;
    if (TCharStr::rfind(objectPath, ObjectPathHelper::ObjectObjectSeparator, &outerObjectSepIdx))
    {
        outObjectName = objectPath + outerObjectSepIdx + 1;
        return { objectPath, outerObjectSepIdx };
    }
    outObjectName = objectPath;
    return TCHAR("");
}

String ObjectPathHelper::getObjectPath(const CBE::Object *object, const CBE::Object *stopAt)
{
    debugAssert(stopAt != object);

    if (object->getOuter() == nullptr)
    {
        return object->getName();
    }

    std::vector<String> outers;
    // Since last path element must be this obj name
    outers.emplace_back(object->getName());
    const CBE::Object *outer = object->getOuter();
    while (outer != stopAt && outer->getOuter())
    {
        outers.emplace_back(outer->getName());
        outer = outer->getOuter();
    }
    if (outer != stopAt)
    {
        return outer->getName() + ObjectPathHelper::RootObjectSeparator
               + String::join(outers.crbegin(), outers.crend(), ObjectPathHelper::ObjectObjectSeparator);
    }
    return String::join(outers.crbegin(), outers.crend(), ObjectPathHelper::ObjectObjectSeparator);
}

FORCE_INLINE String ObjectPathHelper::getFullPath(const CBE::Object *object)
{
    if (object->getOuter() == nullptr)
    {
        return object->getName();
    }

    std::vector<String> outers;
    // Since last path element must be this obj name
    outers.emplace_back(object->getName());
    const CBE::Object *outer = object->getOuter();
    while (outer->getOuter())
    {
        outers.emplace_back(outer->getName());
        outer = outer->getOuter();
    }
    String objectPath = outer->getName() + ObjectPathHelper::RootObjectSeparator
                        + String::join(outers.crbegin(), outers.crend(), ObjectPathHelper::ObjectObjectSeparator);

    return objectPath;
}

String ObjectPathHelper::getFullPath(const TChar *objectName, const CBE::Object *outerObj)
{
    if (outerObj)
    {
        return outerObj->getFullPath()
               + (outerObj->getOuter() ? ObjectPathHelper::ObjectObjectSeparator : ObjectPathHelper::RootObjectSeparator) + objectName;
    }
    return objectName;
}

String ObjectPathHelper::getPackagePath(const TChar *objFullPath)
{
    SizeT rootObjSepIdx;
    bool bRootSepFound = TCharStr::find(objFullPath, ObjectPathHelper::RootObjectSeparator, &rootObjSepIdx);
    if (bRootSepFound)
    {
        return { objFullPath, rootObjSepIdx };
    }
    return TCHAR("");
}

String ObjectPathHelper::getPathComponents(String &outOuterObjectPath, String &outObjectName, const TChar *objFullPath)
{
    SizeT rootObjSepIdx;
    bool bRootSepFound = TCharStr::find(objFullPath, ObjectPathHelper::RootObjectSeparator, &rootObjSepIdx);

    if (bRootSepFound)
    {
        // Path after RootObjectSeparator is outers... and object name
        outOuterObjectPath = getOuterPathAndObjectName(outObjectName, String(objFullPath + rootObjSepIdx + 1).getChar());
        return { objFullPath, rootObjSepIdx };
    }
    else
    {
        outOuterObjectPath = getOuterPathAndObjectName(outObjectName, objFullPath);
        return TCHAR("");
    }
}

String ObjectPathHelper::combinePathComponents(const String &packagePath, const String &outerObjectPath, const String &objectName)
{
    // Ensure that package path is package path without any additional root path and outer object is without any root/package object path
    debugAssert(
        !(TCharStr::find(packagePath.getChar(), ObjectPathHelper::RootObjectSeparator)
          || TCharStr::find(packagePath.getChar(), ObjectPathHelper::RootObjectSeparator))
    );

    return packagePath + ObjectPathHelper::RootObjectSeparator + outerObjectPath + ObjectPathHelper::ObjectObjectSeparator + objectName;
}

String ObjectPathHelper::splitPackageNameAndPath(String &outName, const TChar *path)
{
    SizeT rootObjSepIdx;
    bool bRootSepFound = TCharStr::find(path, ObjectPathHelper::RootObjectSeparator, &rootObjSepIdx);

    if (bRootSepFound)
    {
        return getOuterPathAndObjectName(outName, String(path, rootObjSepIdx).getChar());
    }
    return getOuterPathAndObjectName(outName, path);
}