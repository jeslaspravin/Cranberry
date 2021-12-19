#pragma once
#include "String/String.h"
#include "ReflectionRuntimeExports.h"

#include <utility>

class BaseFieldWrapper;
class BaseFunctionWrapper;
struct ReflectTypeInfo;
class ClassProperty;

enum class EPropertyType
{
    ClassType,// For both struct and class
    EnumType,
    FieldType,
    Function,
    FundamentalType,
    SpecialType,// Types that are not fundamental but can be serialized as binary stream like Vector(nD), Matrix, Transforms, Colors, Rotations etc,.
    PointerType,
    MapType,
    SetType,
    PairType,
    ArrayType
};

// Property that defines or holds the information necessary to access data. This class will not work with the data itself
class BaseProperty
{
public:
    String name;
    EPropertyType type;

public:
    BaseProperty(const String& propName, EPropertyType propType);
    BaseProperty() = delete;
    BaseProperty(BaseProperty&&) = delete;
    BaseProperty(const BaseProperty&) = delete;

    virtual ~BaseProperty() = default;
};

// Can be used for both special types and fundamental types
class TypedProperty : public BaseProperty
{
public:
    // representing class's type info
    const ReflectTypeInfo* typeInfo;
public:
    TypedProperty(const String& propName, EPropertyType propType, const ReflectTypeInfo* propTypeInfo)
        : BaseProperty(propName, propType)
        , typeInfo(propTypeInfo)
    {}
};

enum class EPropertyAccessSpecifier
{
    Private,
    Protected,
    Public
};

// Property that is a field in either struct or class or just global property
class REFLECTIONRUNTIME_EXPORT FieldProperty final : public BaseProperty
{
public:
    // Class/Struct that owns this field and nullptr for global property
    const ClassProperty* ownerProperty;
    EPropertyAccessSpecifier accessor;

    // Field's type property
    const BaseProperty* field;

    const BaseFieldWrapper* fieldPtr;
public:
    FieldProperty(const String& propName);
    ~FieldProperty();

    FORCE_INLINE FieldProperty* setOwnerProperty(const ClassProperty* inOwnerClass)
    {
        ownerProperty = inOwnerClass;
        return this;
    }
    
    FORCE_INLINE FieldProperty* setField(const BaseProperty* inField)
    {
        field = inField;
        return this;
    }

    FORCE_INLINE FieldProperty* setFieldAccessor(EPropertyAccessSpecifier inAccessor)
    {
        accessor = inAccessor;
        return this;
    }

    template <typename ConstructType, typename... CTorArgs>
    FORCE_INLINE FieldProperty* constructFieldPtr(CTorArgs&&... args)
    {
        fieldPtr = new ConstructType(std::forward<CTorArgs>(args)...);
        return this;
    }
};


// Property that is a function in either struct or class or just global property
class REFLECTIONRUNTIME_EXPORT FunctionProperty final : public BaseProperty
{
public:
public:
    // Class/Struct that owns this field and nullptr for global property
    const ClassProperty* ownerProperty;
    EPropertyAccessSpecifier accessor;

    const BaseFunctionWrapper* funcPtr;
    std::vector<std::pair<String, const BaseProperty*>> funcParamsProp;
    const BaseProperty* funcReturnProp;
public:
    FunctionProperty(const String& propName);
    ~FunctionProperty();

    FORCE_INLINE FunctionProperty* setOwnerProperty(const ClassProperty* inOwnerClass)
    {
        ownerProperty = inOwnerClass;
        return this;
    }

    FORCE_INLINE FunctionProperty* setFieldAccessor(EPropertyAccessSpecifier inAccessor)
    {
        accessor = inAccessor;
        return this;
    }

    FORCE_INLINE FunctionProperty* addFunctionParamProperty(const String& paramName, const BaseProperty* funcParamProperty)
    {
        funcParamsProp.emplace_back(decltype(funcParamsProp)::value_type{ paramName, funcParamProperty });
        return this;
    }

    FORCE_INLINE FunctionProperty* setFunctionReturnProperty(const BaseProperty* funcReturnProperty)
    {
        funcReturnProp = funcReturnProperty;
        return this;
    }

    template <typename ConstructType, typename... CTorArgs>
    FORCE_INLINE FunctionProperty* constructFuncPointer(CTorArgs&&... args)
    {
        funcPtr = new ConstructType(std::forward<CTorArgs>(args)...);
        return this;
    }
};

class REFLECTIONRUNTIME_EXPORT ClassProperty final : public TypedProperty
{
public:
    std::vector<const FunctionProperty*> constructors;

    std::vector<const FieldProperty*> memberFields;
    // We do not support function overload for reflect functions(We can but we don't)
    std::vector<const FunctionProperty*> memberFunctions;

    std::vector<const FieldProperty*> staticFields;
    // We do not support function overload for reflect functions(We can but we don't)
    std::vector<const FunctionProperty*> staticFunctions;
public:
    // Complete class name including namespace/classes
    ClassProperty(const String& className, const ReflectTypeInfo* classTypeInfo);
    ~ClassProperty();

    template <typename ConstructType, typename... CTorArgs>
    FORCE_INLINE ClassProperty* addCtorPtr(CTorArgs&&... args)
    {
        constructors.emplace_back(new ConstructType(std::forward<CTorArgs>(args)...));
        return this;
    }

    FORCE_INLINE FieldProperty* addMemberField(const String& fieldName)
    {
        FieldProperty* fieldProp = (new FieldProperty(fieldName))->setOwnerProperty(this);
        memberFields.emplace_back(fieldProp);
        return fieldProp;
    }

    FORCE_INLINE FunctionProperty* addMemberFunc(const String& funcName)
    {
        FunctionProperty* funcProp = (new FunctionProperty(funcName))->setOwnerProperty(this);
        memberFunctions.emplace_back(funcProp);
        return funcProp;
    }

    // Use field.ownerClass to get back to ClassProperty
    FORCE_INLINE FieldProperty* addStaticField(const String& fieldName)
    {
        FieldProperty* fieldProp = (new FieldProperty(fieldName))->setOwnerProperty(this);
        staticFields.emplace_back(fieldProp);
        return fieldProp;
    }

    FORCE_INLINE FunctionProperty* addStaticFunc(const String& funcName)
    {
        FunctionProperty* funcProp = (new FunctionProperty(funcName))->setOwnerProperty(this);
        staticFunctions.emplace_back(funcProp);
        return funcProp;
    }
};

class REFLECTIONRUNTIME_EXPORT EnumProperty final : public TypedProperty
{
public:
    struct EnumField
    {
        String entryName;
        uint64 value;
    };

public:
    std::vector<EnumField> fields;
    // If this enum is flags
    bool bIsFlags;
public:
    // Complete class name including namespace/classes
    EnumProperty(const String& enumName, const ReflectTypeInfo* enumTypeInfo, bool bCanBeUsedAsFlags);

    FORCE_INLINE EnumProperty* addEnumField(const String& fieldName, uint64 fieldValue)
    {
        fields.emplace_back(EnumField{ fieldName, fieldValue });
        return this;
    }
};

// Will be class that has proper reflection
class REFLECTIONRUNTIME_EXPORT PointerProperty : public TypedProperty
{
public:
    // representing class's type info
    const ReflectTypeInfo* pointedTypeInfo;
    const ClassProperty* pointedTypeProperty;
public:
    PointerProperty(const String& propName, const ReflectTypeInfo* propTypeInfo);
};