/*!
 * \file Property.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Property/Property.h"
#include "IReflectionRuntime.h"
#include "Property/PropertyMetaData.h"
#include "ReflectionRuntimeModule.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/TypesInfo.h"

BaseProperty::BaseProperty(const StringID &propNameID, const TChar *propName, EPropertyType propType)
    : name(propNameID)
    , nameString(propName)
    , type(propType)
{}

const PropertyMetaDataBase *BaseProperty::getMetaData(const ReflectTypeInfo *typeInfo) const
{
    return IReflectionRuntimeModule::get()->getPropertyMetaData(this, typeInfo);
}

FORCE_INLINE uint64 BaseProperty::getMetaFlags() const { return IReflectionRuntimeModule::get()->getPropertyMetaFlags(this); }

void BaseProperty::setMetaData(const std::vector<const PropertyMetaDataBase *> &propertyMeta, uint64 propertyMetaFlags)
{
    static_cast<ReflectionRuntimeModule *>(IReflectionRuntimeModule::get())->setMetaData(this, propertyMeta, propertyMetaFlags);
}

//////////////////////////////////////////////////////////////////////////
/// FieldProperty
//////////////////////////////////////////////////////////////////////////

FieldProperty::FieldProperty(const StringID &propNameID, const TChar *propName)
    : BaseProperty(propNameID, propName, EPropertyType::FieldType)
    , ownerProperty(nullptr)
    , field(nullptr)
    , fieldPtr(nullptr)
{}

FieldProperty::~FieldProperty()
{
    delete fieldPtr;
    fieldPtr = nullptr;
}

FieldProperty *FieldProperty::setPropertyMetaData(const std::vector<const PropertyMetaDataBase *> &propertyMeta, uint64 propertyMetaFlags)
{
    setMetaData(propertyMeta, propertyMetaFlags);
    return this;
}

uint64 FieldProperty::getPropertyMetaFlags() const { return getMetaFlags(); }

//////////////////////////////////////////////////////////////////////////
/// FunctionProperty
//////////////////////////////////////////////////////////////////////////

FunctionProperty::FunctionProperty(const StringID &propNameID, const TChar *propName)
    : BaseProperty(propNameID, propName, EPropertyType::Function)
    , ownerProperty(nullptr)
    , funcPtr(nullptr)
    , funcReturnProp(nullptr)
{}

FunctionProperty::~FunctionProperty()
{
    delete funcPtr;
    funcPtr = nullptr;
}

FunctionProperty *FunctionProperty::setPropertyMetaData(const std::vector<const PropertyMetaDataBase *> &propertyMeta, uint64 propertyMetaFlags)
{
    setMetaData(propertyMeta, propertyMetaFlags);
    return this;
}

uint64 FunctionProperty::getPropertyMetaFlags() const { return getMetaFlags(); }

//////////////////////////////////////////////////////////////////////////
/// ClassProperty
//////////////////////////////////////////////////////////////////////////

ClassProperty::ClassProperty(const StringID &propNameID, const TChar *propName, const ReflectTypeInfo *classTypeInfo)
    : TypedProperty(propNameID, propName, EPropertyType::ClassType, classTypeInfo)
    , allocFunc(nullptr)
    , destructor(nullptr)
    , baseClass(nullptr)
{}

ClassProperty::~ClassProperty()
{
    for (const FunctionProperty *ctor : constructors)
    {
        delete ctor;
    }

    for (const FieldProperty *mbrField : memberFields)
    {
        delete mbrField;
    }
    for (const FunctionProperty *mbrFunc : memberFunctions)
    {
        delete mbrFunc;
    }

    for (const FieldProperty *staticField : staticFields)
    {
        delete staticField;
    }
    for (const FunctionProperty *staticFunc : staticFunctions)
    {
        delete staticFunc;
    }

    constructors.clear();
    memberFields.clear();
    memberFunctions.clear();
    staticFields.clear();
    staticFunctions.clear();
}

ClassProperty *ClassProperty::setPropertyMetaData(const std::vector<const PropertyMetaDataBase *> &propertyMeta, uint64 propertyMetaFlags)
{
    setMetaData(propertyMeta, propertyMetaFlags);
    return this;
}

uint64 ClassProperty::getPropertyMetaFlags() const { return getMetaFlags(); }

//////////////////////////////////////////////////////////////////////////
/// EnumProperty
//////////////////////////////////////////////////////////////////////////

EnumProperty::EnumProperty(const StringID &propNameID, const TChar *propName, const ReflectTypeInfo *enumTypeInfo, bool bCanBeUsedAsFlags)
    : TypedProperty(propNameID, propName, EPropertyType::EnumType, enumTypeInfo)
    , bIsFlags(bCanBeUsedAsFlags)
{}

EnumProperty *EnumProperty::addEnumField(
    const StringID &fieldNameID, const TChar *fieldName, uint64 fieldValue, uint64 metaFlags,
    std::vector<const PropertyMetaDataBase *> fieldMetaData
)
{
    fields.emplace_back(EnumField{ fieldValue, metaFlags, fieldName, fieldNameID });
    for (const PropertyMetaDataBase *metaData : fieldMetaData)
    {
        fieldsMeta.insert({
            {fieldValue, metaData->metaType()},
            metaData
        });
    }
    return this;
}

EnumProperty *EnumProperty::setPropertyMetaData(const std::vector<const PropertyMetaDataBase *> &propertyMeta, uint64 propertyMetaFlags)
{
    setMetaData(propertyMeta, propertyMetaFlags);
    return this;
}

uint64 EnumProperty::getPropertyMetaFlags() const { return getMetaFlags(); }

//////////////////////////////////////////////////////////////////////////
/// PointerProperty
//////////////////////////////////////////////////////////////////////////

QualifiedProperty::QualifiedProperty(const StringID &propNameID, const TChar *propName, const ReflectTypeInfo *propTypeInfo)
    : TypedProperty(propNameID, propName, EPropertyType::QualifiedType, propTypeInfo)
    , unqualTypeProperty(nullptr)
{
    // Only const type can have no inner type as const int can be qualified type. This case is not a
    // useful one, and so we throw error here.
    fatalAssertf(typeInfo->innerType, "Inner type cannot be nullptr for a qualified type type %s", propName);
}
