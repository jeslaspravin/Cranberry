#include "Property/Property.h"
#include "Types/TypesInfo.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "IReflectionRuntime.h"

BaseProperty::BaseProperty(const String& propName, EPropertyType propType)
    : name(propName)
    , type(propType)
{}

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

//////////////////////////////////////////////////////////////////////////
/// EnumProperty
//////////////////////////////////////////////////////////////////////////

EnumProperty::EnumProperty(const String& enumName, const ReflectTypeInfo* enumTypeInfo, bool bCanBeUsedAsFlags)
    : TypedProperty(enumName, EPropertyType::EnumType, enumTypeInfo)
    , bIsFlags(bCanBeUsedAsFlags)
{}

//////////////////////////////////////////////////////////////////////////
/// PointerProperty
//////////////////////////////////////////////////////////////////////////

PointerProperty::PointerProperty(const String& propName, const ReflectTypeInfo* propTypeInfo)
    : TypedProperty(propName, EPropertyType::PointerType, propTypeInfo)
    , pointedTypeProperty(nullptr)
{
    fatalAssert(typeInfo->innerType, "%s() : Inner type cannot be nullptr for a pointer type %s", __func__, propName);
}
