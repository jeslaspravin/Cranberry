/*!
 * \file ObjectTemplate.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CoreObjectsExports.h"
#include "String/NameString.h"
#include "CBEObject.h"

#include "ObjectTemplate.gen.h"

class FieldProperty;

namespace cbe
{

class COREOBJECTS_EXPORT ObjectTemplate : public Object
{
    GENERATED_CODES()
public:
    struct TemplateObjectEntry
    {
    public:
        std::unordered_set<StringID> modifiedFields;
        uint64 cursorStart;
    };

private:
    // Will be temporary template object. Will be subobject of this template
    META_ANNOTATE(Transient)
    Object *templateObj;

    META_ANNOTATE()
    ObjectTemplate *parentTemplate;

    CBEClass templateClass;
    // Object names are relative to outer, ie) templateObj will be subobject of ObjectTemplate
    String templateObjName;
    // using map as we might have several sub objects that are created for template object
    std::unordered_map<NameString, TemplateObjectEntry> objectEntries;

public:
    ObjectTemplate()
        : templateObj(nullptr)
        , parentTemplate(nullptr)
        , templateClass(nullptr)
    {}
    ObjectTemplate(StringID className, const String &name);
    ObjectTemplate(ObjectTemplate *inTemplate, const String &name);

    /* cbe::Object overrides */
    void destroy() override;
    ObjectArchive &serialize(ObjectArchive &ar) override;
    /* Overrides ends */

    FORCE_INLINE Object *getTemplate() const { return templateObj; }
    template <typename T>
    FORCE_INLINE T *getTemplateAs() const
    {
        return cast<T>(templateObj);
    }
    FORCE_INLINE CBEClass getTemplateClass() const { return templateClass; }
    FORCE_INLINE ObjectTemplate *getParentTemplate() const { return parentTemplate; }

    void onFieldModified(const FieldProperty *prop, Object *obj);
    void onFieldReset(const FieldProperty *prop, Object *obj);

    // Copies templateObj and data necessary to serialize this ObjectTemplate with same values as otherTemplate
    bool copyFrom(ObjectTemplate *otherTemplate);

private:
    void createTemplate(CBEClass clazz, const TChar *name);
} META_ANNOTATE(NoExport);

/**
 * Creates an object from template
 */
COREOBJECTS_EXPORT Object *create(ObjectTemplate *objTemplate, StringView name, Object *outerObj, EObjectFlags flags = 0);

} // namespace cbe