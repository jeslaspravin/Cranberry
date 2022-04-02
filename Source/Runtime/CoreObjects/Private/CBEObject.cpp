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
#include "ObjectPathHelpers.h"
#include "CoreObjectsDB.h"
#include "CoreObjectsModule.h"

namespace CBE
{
    EObjectFlags& PrivateObjectCoreAccessors::getFlags(Object* object)
    {
        return object->flags;
    }

    ObjectAllocIdx PrivateObjectCoreAccessors::getAllocIdx(Object* object)
    {
        return object->allocIdx;
    }

    void PrivateObjectCoreAccessors::setOuterAndName(Object* object, const String& newName, Object* outer)
    {
        fatalAssert(!newName.empty(), "Object name cannot be empty");
        if (outer == object->getOuter() && object->getName() == newName)
            return;

        CoreObjectsDB& objectsDb = CoreObjectsModule::objectsDB();

        object->objOuter = outer;
        object->objectName = newName;

        StringID newSid(object->getFullPath());
        if (object->getStringID().isValid() && CoreObjectsModule::objectsDB().hasObject(object->sid))
        {
            objectsDb.setObject(object->getStringID(), newSid);
            objectsDb.setObjectParent(newSid, object->getOuter()->getStringID());
        }
        else
        {
            CoreObjectsDB::ObjectData objData
            {
                .clazz = object->getType(),
                .allocIdx = object->allocIdx,
                .sid = newSid
            };

            objectsDb.addObject(newSid, objData, outer->getStringID());
        }
        object->sid = newSid;
    }

    void PrivateObjectCoreAccessors::setOuter(Object* object, Object* outer)
    {
        setOuterAndName(object, object->objectName, outer);
    }

    void PrivateObjectCoreAccessors::renameObject(Object* object, const String& newName)
    {
        setOuterAndName(object, newName, object->objOuter);
    }

    Object::~Object()
    {
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

    //////////////////////////////////////////////////////////////////////////
    /// CBE::Object implementations
    //////////////////////////////////////////////////////////////////////////

    String Object::getFullPath() const
    {
        return ObjectPathHelper::getFullPath(this);
    }
}

//////////////////////////////////////////////////////////////////////////
/// ObjectPathHelper implementations
//////////////////////////////////////////////////////////////////////////

FORCE_INLINE String ObjectPathHelper::getFullPath(const CBE::Object* object)
{
    if (object->getOuter() == nullptr)
    {
        return object->getName();
    }

    std::vector<String> outers;
    // Since last path element must be this obj name
    outers.emplace_back(object->getName());
    const CBE::Object* outer = object->getOuter();
    while (outer->getOuter())
    {
        outers.emplace_back(outer->getName());
        outer = outer->getOuter();
    }
    String objectPath = outer->getName() + ObjectPathHelper::RootObjectSeparator
        + String::join(outers.crbegin(), outers.crend(), ObjectPathHelper::ObjectObjectSeparator);

    return objectPath;
}