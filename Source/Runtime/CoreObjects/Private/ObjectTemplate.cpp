/*!
 * \file ObjectTemplate.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "ObjectTemplate.h"
#include "CBEObjectHelpers.h"
#include "Serialization/CommonTypesSerialization.h"
#include "Serialization/ObjectSerializationHelpers.h"

namespace cbe
{
inline constexpr static const uint32 OBJECT_TEMPLATE_SERIALIZER_VERSION = 0;
inline constexpr static const uint32 OBJECT_TEMPLATE_SERIALIZER_CUTOFF_VERSION = 0;
inline STRINGID_CONSTEXPR static const StringID OBJECT_TEMPLATE_CUSTOM_VERSION_ID = STRID("ObjectTemplate");

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, ObjectTemplate::TemplateObjectEntry &value)
{
    return archive << value.modifiedFields << value.cursorStart;
}

ObjectTemplate::ObjectTemplate(StringID className, const String &name)
    : Object()
    , parentTemplate(nullptr)
{
    CBEClass clazz = IReflectionRuntimeModule::get()->getClassType(className);
    debugAssert(clazz);
    createTemplate(clazz, name.getChar());
    debugAssert(templateObj);
    templateObj->constructed();
    markDirty(this);
}

ObjectTemplate::ObjectTemplate(ObjectTemplate *inTemplate, const String &name)
    : Object()
    , parentTemplate(inTemplate)
{
    debugAssert(parentTemplate && parentTemplate->objectClass);
    createTemplate(parentTemplate->objectClass, name.getChar());
    debugAssert(templateObj);
    templateObj->constructed();
    markDirty(this);
}

void ObjectTemplate::destroy()
{
    Object::destroy();
    if (isValid(templateObj))
    {
        templateObj->beginDestroy();
        templateObj = nullptr;
    }
}

ObjectArchive &ObjectTemplate::serialize(ObjectArchive &ar)
{
    if (ar.isLoading())
    {
        uint32 dataVersion = ar.getCustomVersion(uint32(OBJECT_TEMPLATE_CUSTOM_VERSION_ID));
        // This must crash
        fatalAssertf(
            OBJECT_TEMPLATE_SERIALIZER_CUTOFF_VERSION >= dataVersion,
            "Version of ObjectTemplate %u loaded from package %s is outdated, Minimum supported %u!", dataVersion,
            getOuterMost()->getFullPath(), OBJECT_TEMPLATE_SERIALIZER_CUTOFF_VERSION
        );
    }
    else
    {
        ar.setCustomVersion(uint32(OBJECT_TEMPLATE_CUSTOM_VERSION_ID), OBJECT_TEMPLATE_SERIALIZER_VERSION);
    }

    ar << parentTemplate;
    ar << objectName;
    CBEClass clazz = objectClass;
    ar << clazz;
    if (ar.isLoading())
    {
        if (clazz == nullptr)
        {
            LOG_ERROR(
                "ObjectTemplate", "Failed to get class from class ID %u while serializing %s", clazz->nameString, getOuterMost()->getFullPath()
            );
            return ar;
        }

        createTemplate(clazz, objectName.toString().getChar());
        std::unordered_map<NameString, TemplateObjectEntry> loadedEntries;
        uint64 archiveEnd = 0;
        ar << loadedEntries;
        ar << archiveEnd;
        for (std::pair<const NameString, TemplateObjectEntry> &loadedEntry : loadedEntries)
        {
            if (objectEntries.contains(loadedEntry.first))
            {
                TemplateObjectEntry &entry = objectEntries[loadedEntry.first];
                entry.cursorStart = loadedEntry.second.cursorStart;
                entry.modifiedFields = std::move(loadedEntry.second.modifiedFields);
                Object *entryObj = get(ObjectPathHelper::getFullPath(loadedEntry.first.toString().getChar(), this).getChar());
                debugAssert(isValid(entryObj));

                if (ar.stream()->cursorPos() >= entry.cursorStart)
                {
                    ar.stream()->moveBackward(ar.stream()->cursorPos() - entry.cursorStart);
                }
                else
                {
                    ar.stream()->moveForward(entry.cursorStart - ar.stream()->cursorPos());
                }
                debugAssert(ar.stream()->cursorPos() == entry.cursorStart);
                ObjectSerializationHelpers::serializeOnlyFields(entryObj, ar, entry.modifiedFields);
                entryObj->constructed();
            }
        }
        // archiveEnd cannot be less than current cursor pos if everything is alright
        debugAssert(archiveEnd >= ar.stream()->cursorPos());
        ar.stream()->moveForward(archiveEnd - ar.stream()->cursorPos());
    }
    else
    {
        debugAssert(isValid(templateObj));
        uint64 objectEntriesStart = ar.stream()->cursorPos();
        uint64 archiveEnd = 0; // Necessary when loading to reset to end after random reads
        ar << objectEntries;
        ar << archiveEnd;
        for (std::pair<const NameString, TemplateObjectEntry> &entry : objectEntries)
        {
            entry.second.cursorStart = ar.stream()->cursorPos();
            Object *entryObj = get(ObjectPathHelper::getFullPath(entry.first.toString().getChar(), this).getChar());
            debugAssert(isValid(entryObj));
            ObjectSerializationHelpers::serializeOnlyFields(entryObj, ar, entry.second.modifiedFields);
        }
        // Move back and serialize objectEntries to write cursor start of each serialized objects
        archiveEnd = ar.stream()->cursorPos();
        ar.stream()->moveBackward(archiveEnd - objectEntriesStart);
        ar << objectEntries;
        ar << archiveEnd;
        ar.stream()->moveForward(archiveEnd - ar.stream()->cursorPos());
    }
    return ar;
}

void ObjectTemplate::onFieldModified(const FieldProperty *prop, Object *obj)
{
    debugAssert(obj->hasOuter(this));
    NameString objName(ObjectPathHelper::getObjectPath(obj, this));

    TemplateObjectEntry &entry = objectEntries[objName];
    entry.modifiedFields.emplace(prop->name);
    markDirty(this);
}

void ObjectTemplate::onFieldReset(const FieldProperty *prop, Object *obj)
{
    debugAssert(obj->hasOuter(this));
    NameString objName(ObjectPathHelper::getObjectPath(obj, this));

    TemplateObjectEntry &entry = objectEntries[objName];
    entry.modifiedFields.erase(prop->name);
    markDirty(this);
}

void ObjectTemplate::createTemplate(CBEClass clazz, const TChar *name)
{
    if (clazz != objectClass && isValid(templateObj))
    {
        templateObj->beginDestroy();
        templateObj = nullptr;
        objectEntries.clear();
    }
    objectClass = clazz;
    objectName = name;

    EObjectFlags flags = EObjectFlagBits::ObjFlag_Transient | EObjectFlagBits::ObjFlag_TemplateDefault;
    if (parentTemplate)
    {
        templateObj = create(parentTemplate, name, this, flags);
    }
    else
    {
        templateObj = INTERNAL_create(objectClass, name, this, flags);
    }

    const CoreObjectsDB &objectsDb = ICoreObjectsModule::get()->getObjectsDB();
    std::vector<Object *> subObjs;
    objectsDb.getSubobjects(subObjs, templateObj->getStringID());

    objectEntries.emplace(objectName, TemplateObjectEntry{});
    for (Object *subObj : subObjs)
    {
        objectEntries.emplace(ObjectPathHelper::getObjectPath(subObj, this), TemplateObjectEntry{});
    }
}

cbe::Object *create(ObjectTemplate *objTemplate, const String &name, Object *outerObj, EObjectFlags flags /*= 0*/)
{
    if (!isValid(objTemplate))
    {
        return nullptr;
    }

    SET_BITS(flags, EObjectFlagBits::ObjFlag_FromTemplate);
    return duplicateObject(
        objTemplate->getTemplate(), outerObj, name, flags, EObjectFlagBits::ObjFlag_Transient | EObjectFlagBits::ObjFlag_TemplateDefault
    );
}

} // namespace cbe
