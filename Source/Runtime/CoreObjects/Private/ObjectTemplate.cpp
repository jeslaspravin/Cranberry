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
#include "CoreObjectsModule.h"
#include "Serialization/CommonTypesSerialization.h"
#include "Serialization/ObjectSerializationHelpers.h"

namespace cbe
{
inline constexpr static const uint32 OBJECT_TEMPLATE_SERIALIZER_VERSION = 0;
inline constexpr static const uint32 OBJECT_TEMPLATE_SERIALIZER_CUTOFF_VERSION = 0;
inline STRINGID_CONSTEXPR static const StringID OBJECT_TEMPLATE_CUSTOM_VERSION_ID = STRID("ObjectTemplate");

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, ObjectTemplate::TemplateObjectEntry &value)
{
    return archive << value.modifiedFields << value.cursorStart;
}

ObjectTemplate::ObjectTemplate(StringID className, String name)
    : Object()
    , parentTemplate(nullptr)
{
    CBEClass clazz = IReflectionRuntimeModule::get()->getClassType(className);
    debugAssert(clazz);
    createTemplate(clazz, name);
    debugAssert(templateObj);
    templateObj->constructed();
    markDirty(this);
}

ObjectTemplate::ObjectTemplate(ObjectTemplate *inTemplate, String name)
    : Object()
    , parentTemplate(inTemplate)
{
    debugAssert(parentTemplate && parentTemplate->templateClass);
    createTemplate(parentTemplate->templateClass, name);
    debugAssert(templateObj);
    templateObj->constructed();
    markDirty(this);
}

void ObjectTemplate::destroy()
{
    // Cannot use cbe::isValid(templateObj) here as there is chance that Allocation might have been deleted
    if (cbe::get(ObjectPathHelper::getFullPath(templateObjName.getChar(), this).getChar()))
    {
        templateObj->beginDestroy();
        templateObj = nullptr;
    }

    Object::destroy();
}

ObjectArchive &ObjectTemplate::serialize(ObjectArchive &ar)
{
    if (ar.isLoading())
    {
        uint32 dataVersion = ar.getCustomVersion(uint32(OBJECT_TEMPLATE_CUSTOM_VERSION_ID));
        // This must crash
        fatalAssertf(
            OBJECT_TEMPLATE_SERIALIZER_CUTOFF_VERSION >= dataVersion,
            "Version of ObjectTemplate {} loaded from package {} is outdated, Minimum supported {}!", dataVersion,
            getOuterMost()->getObjectData().path, OBJECT_TEMPLATE_SERIALIZER_CUTOFF_VERSION
        );
    }
    else
    {
        ar.setCustomVersion(uint32(OBJECT_TEMPLATE_CUSTOM_VERSION_ID), OBJECT_TEMPLATE_SERIALIZER_VERSION);
    }

    ar << parentTemplate;
    // objectClass will be set inside createTemplate when loading
    CBEClass clazz = templateClass;
    ar << clazz;
    ar << templateObjName;
    if (ar.isLoading())
    {
        if (clazz == nullptr)
        {
            LOG_ERROR("ObjectTemplate", "Failed to get class while serializing {}", getOuterMost()->getObjectData().path);
            return ar;
        }

        createTemplate(clazz, templateObjName.getChar());
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
                debugAssert(isValidFast(entryObj));

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
        debugAssert(isValidFast(templateObj));
        uint64 objectEntriesStart = ar.stream()->cursorPos();
        uint64 archiveEnd = 0; // Necessary when loading to reset to end after random reads
        ar << objectEntries;
        ar << archiveEnd;
        for (std::pair<const NameString, TemplateObjectEntry> &entry : objectEntries)
        {
            entry.second.cursorStart = ar.stream()->cursorPos();
            Object *entryObj = get(ObjectPathHelper::getFullPath(entry.first.toString().getChar(), this).getChar());
            debugAssert(isValidFast(entryObj));
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
    NameString objName(ObjectPathHelper::computeObjectPath(obj, this));

    TemplateObjectEntry &entry = objectEntries[objName];
    entry.modifiedFields.emplace(prop->name);
    markDirty(this);
}

void ObjectTemplate::onFieldReset(const FieldProperty *prop, Object *obj)
{
    debugAssert(obj->hasOuter(this));
    NameString objName(ObjectPathHelper::computeObjectPath(obj, this));

    TemplateObjectEntry &entry = objectEntries[objName];
    entry.modifiedFields.erase(prop->name);
    markDirty(this);
}

bool ObjectTemplate::copyFrom(ObjectTemplate *otherTemplate)
{
    if (templateClass != otherTemplate->templateClass || parentTemplate != otherTemplate->parentTemplate
        || templateObjName != otherTemplate->templateObjName)
    {
        return false;
    }
    CBE_PROFILER_SCOPE("CopyObjectTemplate");

    // Copy all values first
    cbe::deepCopy(otherTemplate->templateObj, templateObj, 0, 0, false);
    objectEntries.clear();
    objectEntries.reserve(otherTemplate->objectEntries.size());
    // Check if each object entry names matches this
    for (std::pair<const NameString, TemplateObjectEntry> &entry : otherTemplate->objectEntries)
    {
        Object *thisEntryObj = get(ObjectPathHelper::getFullPath(entry.first.toString().getChar(), this).getChar());
        if (thisEntryObj == nullptr)
        {
            LOG_WARN("ObjectTemplate", "ObjectTemplate {} does not have sub-object named {}", getObjectData().path, entry.first);
        }
        else
        {
            objectEntries.emplace(entry);
        }
    }
    markDirty(this);
    return true;
}

void ObjectTemplate::createTemplate(CBEClass clazz, StringView name)
{
    if (clazz != templateClass && isValidFast(templateObj))
    {
        templateObj->beginDestroy();
        templateObj = nullptr;
        objectEntries.clear();
    }
    templateClass = clazz;
    templateObjName = name;

    EObjectFlags templateObjflags = EObjectFlagBits::ObjFlag_Transient | EObjectFlagBits::ObjFlag_TemplateDefault;
    if (parentTemplate)
    {
        templateObj = create(parentTemplate, templateObjName, this, templateObjflags);
    }
    else
    {
        templateObj = INTERNAL_create(templateClass, templateObjName, this, templateObjflags);
    }
    debugAssert(templateObj->getOuter() == this);

    const CoreObjectsDB &objectsDb = CoreObjectsModule::objectsDB();
    std::vector<Object *> subObjs;
    objectsDb.getSubobjects(subObjs, templateObj->getDbIdx());

    objectEntries.emplace(templateObjName, TemplateObjectEntry{});
    for (Object *subObj : subObjs)
    {
        objectEntries.emplace(ObjectPathHelper::computeObjectPath(subObj, this), TemplateObjectEntry{});
    }
}

cbe::Object *create(ObjectTemplate *objTemplate, StringView name, Object *outerObj, EObjectFlags flags /*= 0*/)
{
    if (!isValidFast(objTemplate))
    {
        return nullptr;
    }

    SET_BITS(flags, EObjectFlagBits::ObjFlag_FromTemplate);
    return duplicateObject(
        objTemplate->getTemplate(), outerObj, name, flags, EObjectFlagBits::ObjFlag_Transient | EObjectFlagBits::ObjFlag_TemplateDefault
    );
}

} // namespace cbe
