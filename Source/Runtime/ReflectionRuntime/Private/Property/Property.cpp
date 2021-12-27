#include "Property/Property.h"
#include "Types/TypesInfo.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "IReflectionRuntime.h"
#include "Property/PropertyMetaData.h"
#include "ReflectionRuntimeModule.h"

BaseProperty::BaseProperty(const String& propName, EPropertyType propType)
    : name(propName)
    , type(propType)
{}

const PropertyMetaDataBase* BaseProperty::getMetaData(const ReflectTypeInfo * typeInfo) const
{
    return IReflectionRuntimeModule::get()->getPropertyMetaData(this, typeInfo);
}

FORCE_INLINE uint64 BaseProperty::getMetaFlags() const
{
    return IReflectionRuntimeModule::get()->getPropertyMetaFlags(this);
}

FORCE_INLINE void BaseProperty::setMetaData(std::vector<const PropertyMetaDataBase*>& propertyMeta, uint64 propertyMetaFlags)
{
    static_cast<ReflectionRuntimeModule*>(IReflectionRuntimeModule::get())->setMetaData(this, propertyMeta, propertyMetaFlags);
}

//////////////////////////////////////////////////////////////////////////
/// FieldProperty
//////////////////////////////////////////////////////////////////////////

FieldProperty::FieldProperty(const String& propName)
    : BaseProperty(propName, EPropertyType::FieldType)
    , ownerProperty(nullptr)
    , field(nullptr)
    , fieldPtr(nullptr)
{}

FieldProperty::~FieldProperty()
{
    delete fieldPtr;
    fieldPtr = nullptr;
}

void FieldProperty::setPropertyMetaData(std::vector<const PropertyMetaDataBase*>& propertyMeta, uint64 propertyMetaFlags)
{
    setMetaData(propertyMeta, propertyMetaFlags);
}

uint64 FieldProperty::getPropertyMetaFlags() const
{
    return getMetaFlags();
}

//////////////////////////////////////////////////////////////////////////
/// FunctionProperty
//////////////////////////////////////////////////////////////////////////

FunctionProperty::FunctionProperty(const String& propName)
    : BaseProperty(propName, EPropertyType::Function)
    , ownerProperty(nullptr)
    , funcPtr(nullptr)
{}

FunctionProperty::~FunctionProperty()
{
    delete funcPtr;
    funcPtr = nullptr;
}

void FunctionProperty::setPropertyMetaData(std::vector<const PropertyMetaDataBase*>& propertyMeta, uint64 propertyMetaFlags)
{
    setMetaData(propertyMeta, propertyMetaFlags);
}

uint64 FunctionProperty::getPropertyMetaFlags() const
{
    return getMetaFlags();
}

//////////////////////////////////////////////////////////////////////////
/// ClassProperty
//////////////////////////////////////////////////////////////////////////

ClassProperty::ClassProperty(const String& className, const ReflectTypeInfo* classTypeInfo)
    : TypedProperty(className, EPropertyType::ClassType, classTypeInfo)
{}

ClassProperty::~ClassProperty()
{
    for (const FunctionProperty* ctor : constructors)
    {
        delete ctor;
    }

    for (const FieldProperty* mbrField : memberFields)
    {
        delete mbrField;
    }
    for (const FunctionProperty* mbrFunc : memberFunctions)
    {
        delete mbrFunc;
    }

    for (const FieldProperty* staticField : staticFields)
    {
        delete staticField;
    }
    for (const FunctionProperty* staticFunc : staticFunctions)
    {
        delete staticFunc;
    }

    constructors.clear();
    memberFields.clear();
    memberFunctions.clear();
    staticFields.clear();
    staticFunctions.clear();
}

void ClassProperty::setPropertyMetaData(std::vector<const PropertyMetaDataBase*>& propertyMeta, uint64 propertyMetaFlags)
{
    setMetaData(propertyMeta, propertyMetaFlags);
}

uint64 ClassProperty::getPropertyMetaFlags() const
{
    return getMetaFlags();
}

//////////////////////////////////////////////////////////////////////////
/// EnumProperty
//////////////////////////////////////////////////////////////////////////

EnumProperty::EnumProperty(const String& enumName, const ReflectTypeInfo* enumTypeInfo, bool bCanBeUsedAsFlags)
    : TypedProperty(enumName, EPropertyType::EnumType, enumTypeInfo)
    , bIsFlags(bCanBeUsedAsFlags)
{}

EnumProperty* EnumProperty::addEnumField(const String& fieldName, uint64 fieldValue, uint64 metaFlags, std::vector<const PropertyMetaDataBase*> fieldMetaData)
{
    fields.emplace_back(EnumField{ fieldName, fieldValue, metaFlags });
    for (const PropertyMetaDataBase* metaData : fieldMetaData)
    {
        fieldsMeta.insert({ { fieldValue, metaData->metaType() }, metaData });
    }
    return this;
}

void EnumProperty::setPropertyMetaData(std::vector<const PropertyMetaDataBase*>& propertyMeta, uint64 propertyMetaFlags)
{
    setMetaData(propertyMeta, propertyMetaFlags);
}

uint64 EnumProperty::getPropertyMetaFlags() const
{
    return getMetaFlags();
}

//////////////////////////////////////////////////////////////////////////
/// PointerProperty
//////////////////////////////////////////////////////////////////////////

QualifiedProperty::QualifiedProperty(const String& propName, const ReflectTypeInfo* propTypeInfo)
    : TypedProperty(propName, EPropertyType::QualifiedType, propTypeInfo)
    , unqualTypeProperty(nullptr)
{
    // Only const type can have no inner type as const int can be qualified type. This case is not a useful one, and so we throw error here.
    fatalAssert(typeInfo->innerType, "%s() : Inner type cannot be nullptr for a qualified type type %s", __func__, propName);
}
