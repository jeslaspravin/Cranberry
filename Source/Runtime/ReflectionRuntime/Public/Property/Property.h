#pragma once
#include "String/String.h"
#include "Types/HashTypes.h"
#include "ReflectionRuntimeExports.h"

#include <utility>
#include <map>

class BaseFieldWrapper;
class BaseFunctionWrapper;
class PropertyMetaDataBase;
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
    QualifiedType,// Types that are const, reference or pointer qualified and has inner type of its unqualified type
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

protected:
    const PropertyMetaDataBase* getMetaData(const ReflectTypeInfo* typeInfo) const;
    uint64 getMetaFlags() const;
    void setMetaData(std::vector<const PropertyMetaDataBase*>& propertyMeta, uint64 propertyMetaFlags);
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

    void setPropertyMetaData(std::vector<const PropertyMetaDataBase*>& propertyMeta, uint64 propertyMetaFlags);

    // Get functions
    template <class MetaType> 
    FORCE_INLINE const MetaType* getPropertyMetaData() const
    {
        return static_cast<const MetaType*>(getMetaData(typeInfoFrom<MetaType>()));
    }
    uint64 getPropertyMetaFlags() const;
};


// Property that is a function in either struct or class or just global property
class REFLECTIONRUNTIME_EXPORT FunctionProperty final : public BaseProperty
{
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

    void setPropertyMetaData(std::vector<const PropertyMetaDataBase*>& propertyMeta, uint64 propertyMetaFlags);

    // Get functions
    template <class MetaType>
    FORCE_INLINE const MetaType* getPropertyMetaData() const
    {
        return static_cast<const MetaType*>(getMetaData(typeInfoFrom<MetaType>()));
    }
    uint64 getPropertyMetaFlags() const;
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

    FORCE_INLINE FunctionProperty* addCtorPtr()
    {
        FunctionProperty* funcProp = (new FunctionProperty(name))->setOwnerProperty(this);
        constructors.emplace_back(funcProp);
        return funcProp;
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

    void setPropertyMetaData(std::vector<const PropertyMetaDataBase*>& propertyMeta, uint64 propertyMetaFlags);

    // Get functions
    template <class MetaType>
    FORCE_INLINE const MetaType* getPropertyMetaData() const
    {
        return static_cast<const MetaType*>(getMetaData(typeInfoFrom<MetaType>()));
    }
    uint64 getPropertyMetaFlags() const;
};

class REFLECTIONRUNTIME_EXPORT EnumProperty final : public TypedProperty
{
public:
    // enum field's value and type of meta data class is unique key here
    using EnumFieldMetaKey = std::pair<uint64, const ReflectTypeInfo*>;
    struct EnumField
    {
        String entryName;
        uint64 value;
        uint64 metaFlags;
    };

public:
    std::vector<EnumField> fields;
    std::map<EnumFieldMetaKey, const PropertyMetaDataBase*> fieldsMeta;
    // If this enum is flags
    bool bIsFlags;
public:
    // Complete class name including namespace/classes
    EnumProperty(const String& enumName, const ReflectTypeInfo* enumTypeInfo, bool bCanBeUsedAsFlags);

    EnumProperty* addEnumField(const String& fieldName, uint64 fieldValue, uint64 metaFlags, std::vector<const PropertyMetaDataBase*> fieldMetaData);
    void setPropertyMetaData(std::vector<const PropertyMetaDataBase*>& propertyMeta, uint64 propertyMetaFlags);

    // Get functions
    template <class MetaType>
    FORCE_INLINE const MetaType* getPropertyMetaData() const
    {
        return static_cast<const MetaType*>(getMetaData(typeInfoFrom<MetaType>()));
    }
    uint64 getPropertyMetaFlags() const;
};

// Will be class that has proper reflection
class REFLECTIONRUNTIME_EXPORT QualifiedProperty : public TypedProperty
{
public:
    const BaseProperty* unqualTypeProperty;
public:
    QualifiedProperty(const String& propName, const ReflectTypeInfo* propTypeInfo);

    FORCE_INLINE QualifiedProperty* setUnqualifiedType(const BaseProperty* prop)
    {
        unqualTypeProperty = prop;
        return this;
    }
};