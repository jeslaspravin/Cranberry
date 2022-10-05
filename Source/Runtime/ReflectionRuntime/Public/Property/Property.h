/*!
 * \file Property.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "ReflectionRuntimeExports.h"
#include "String/StringID.h"
#include "Types/HashTypes.h"

#include <map>
#include <utility>

class BaseFieldWrapper;
class BaseFunctionWrapper;
class PropertyMetaDataBase;
struct ReflectTypeInfo;
class ClassProperty;
class IReflectionRuntimeModule;

enum class EPropertyType
{
    ClassType, // For both struct and class
    EnumType,
    FieldType,
    Function,
    FundamentalType,
    SpecialType,   // Types that are not fundamental but can be serialized as binary stream like
                   // Vector(nD), Matrix, Transforms, Colors, Rotations etc,.
    QualifiedType, // Types that are const, reference or pointer qualified and has inner type of its
                   // unqualified type
    MapType,
    SetType,
    PairType,
    ArrayType,
    StartType = ClassType,
    EndType = ArrayType
};

// TODO(Jeslas) : Property system needs overhaul especially the CustomProperty like map, set, vector.
// Current impl works and glued to work

// Property that defines or holds the information necessary to access data. This class will not work with
// the data itself
class BaseProperty
{
public:
    const TChar *nameString;
    StringID name;
    EPropertyType type;

protected:
    const PropertyMetaDataBase *getMetaData(const ReflectTypeInfo *typeInfo) const;
    uint64 getMetaFlags() const;
    void setMetaData(const std::vector<const PropertyMetaDataBase *> &propertyMeta, uint64 propertyMetaFlags);

public:
    BaseProperty(const StringID &propNameID, const TChar *propName, EPropertyType propType);
    BaseProperty() = delete;
    MAKE_TYPE_NONCOPY_NONMOVE(BaseProperty)

    virtual ~BaseProperty() = default;
};

// Can be used for both special types and fundamental types
class TypedProperty : public BaseProperty
{
public:
    // representing class's type info
    const ReflectTypeInfo *typeInfo;

public:
    TypedProperty(const StringID &propNameID, const TChar *propName, EPropertyType propType, const ReflectTypeInfo *propTypeInfo)
        : BaseProperty(propNameID, propName, propType)
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
    const ClassProperty *ownerProperty;
    EPropertyAccessSpecifier accessor;

    // Field's type property
    const BaseProperty *field;

    const BaseFieldWrapper *fieldPtr;

public:
    FieldProperty(const StringID &propNameID, const TChar *propName);
    ~FieldProperty();

    FORCE_INLINE FieldProperty *setOwnerProperty(const ClassProperty *inOwnerClass)
    {
        ownerProperty = inOwnerClass;
        return this;
    }

    FORCE_INLINE FieldProperty *setField(const BaseProperty *inField)
    {
        field = inField;
        return this;
    }

    FORCE_INLINE FieldProperty *setFieldAccessor(EPropertyAccessSpecifier inAccessor)
    {
        accessor = inAccessor;
        return this;
    }

    template <typename ConstructType, typename... CTorArgs>
    FORCE_INLINE FieldProperty *constructFieldPtr(CTorArgs &&...args)
    {
        fieldPtr = new ConstructType(std::forward<CTorArgs>(args)...);
        return this;
    }

    FieldProperty *setPropertyMetaData(const std::vector<const PropertyMetaDataBase *> &propertyMeta, uint64 propertyMetaFlags);

    // Get functions
    template <class MetaType>
    FORCE_INLINE const MetaType *getPropertyMetaData() const
    {
        return static_cast<const MetaType *>(getMetaData(typeInfoFrom<MetaType>()));
    }
    uint64 getPropertyMetaFlags() const;
};

// Property that is a function in either struct or class or just global property
class REFLECTIONRUNTIME_EXPORT FunctionProperty final : public BaseProperty
{
public:
    struct FunctionParamProperty
    {
        const BaseProperty *typeProperty;
        String nameString;
        StringID name;
    };

    // Class/Struct that owns this field and nullptr for global property
    const ClassProperty *ownerProperty;
    EPropertyAccessSpecifier accessor;

    const BaseFunctionWrapper *funcPtr;
    std::vector<FunctionParamProperty> funcParamsProp;
    const BaseProperty *funcReturnProp;

public:
    FunctionProperty(const StringID &propNameID, const TChar *propName);
    ~FunctionProperty();

    FORCE_INLINE FunctionProperty *setOwnerProperty(const ClassProperty *inOwnerClass)
    {
        ownerProperty = inOwnerClass;
        return this;
    }

    FORCE_INLINE FunctionProperty *setFieldAccessor(EPropertyAccessSpecifier inAccessor)
    {
        accessor = inAccessor;
        return this;
    }

    FORCE_INLINE FunctionProperty *
        addFunctionParamProperty(const StringID &paramNameID, const String &paramName, const BaseProperty *funcParamProperty)
    {
        funcParamsProp.emplace_back(decltype(funcParamsProp)::value_type{ funcParamProperty, paramName, paramNameID });
        return this;
    }

    FORCE_INLINE FunctionProperty *setFunctionReturnProperty(const BaseProperty *funcReturnProperty)
    {
        funcReturnProp = funcReturnProperty;
        return this;
    }

    template <typename ConstructType, typename... CTorArgs>
    FORCE_INLINE FunctionProperty *constructFuncPointer(CTorArgs &&...args)
    {
        funcPtr = new ConstructType(std::forward<CTorArgs>(args)...);
        return this;
    }

    FunctionProperty *setPropertyMetaData(const std::vector<const PropertyMetaDataBase *> &propertyMeta, uint64 propertyMetaFlags);

    // Get functions
    template <class MetaType>
    FORCE_INLINE const MetaType *getPropertyMetaData() const
    {
        return static_cast<const MetaType *>(getMetaData(typeInfoFrom<MetaType>()));
    }
    uint64 getPropertyMetaFlags() const;
};

struct REFLECTIONRUNTIME_EXPORT InterfaceInfo
{
    const TChar *typeName;
    PtrInt offset = 0;
    const ReflectTypeInfo *interfaceTypeInfo = nullptr;
};

class REFLECTIONRUNTIME_EXPORT ClassProperty final : public TypedProperty
{
public:
    using AllocFuncType = Function<void *>;
    using DestructorFuncType = Function<void, void *>;

    AllocFuncType allocFunc;
    DestructorFuncType destructor;
    const ClassProperty *baseClass;

    std::vector<const FunctionProperty *> constructors;

    std::vector<const FieldProperty *> memberFields;
    // We do not support function overload for reflect functions(We can but we don't)
    std::vector<const FunctionProperty *> memberFunctions;

    std::vector<const FieldProperty *> staticFields;
    // We do not support function overload for reflect functions(We can but we don't)
    std::vector<const FunctionProperty *> staticFunctions;
    // List of implemented interfaces
    std::vector<InterfaceInfo> interfaces;

public:
    // Complete class name including namespace/classes
    ClassProperty(const StringID &propNameID, const TChar *propName, const ReflectTypeInfo *classTypeInfo);
    ~ClassProperty();

    FORCE_INLINE FunctionProperty *addCtorPtr()
    {
        FunctionProperty *funcProp = (new FunctionProperty(name, nameString))->setOwnerProperty(this);
        constructors.emplace_back(funcProp);
        return funcProp;
    }
    FORCE_INLINE ClassProperty *setDtorPtr(DestructorFuncType &&inFunc)
    {
        destructor = std::forward<DestructorFuncType>(inFunc);
        return this;
    }
    FORCE_INLINE ClassProperty *setAllocFuncPtr(AllocFuncType &&inFunc)
    {
        allocFunc = std::forward<AllocFuncType>(inFunc);
        return this;
    }

    FORCE_INLINE FieldProperty *addMemberField(const StringID &fieldNameID, const TChar *fieldName)
    {
        FieldProperty *fieldProp = (new FieldProperty(fieldNameID, fieldName))->setOwnerProperty(this);
        memberFields.emplace_back(fieldProp);
        return fieldProp;
    }

    FORCE_INLINE FunctionProperty *addMemberFunc(const StringID &funcNameID, const TChar *funcName)
    {
        FunctionProperty *funcProp = (new FunctionProperty(funcNameID, funcName))->setOwnerProperty(this);
        memberFunctions.emplace_back(funcProp);
        return funcProp;
    }

    // Use field.ownerClass to get back to ClassProperty
    FORCE_INLINE FieldProperty *addStaticField(const StringID &fieldNameID, const TChar *fieldName)
    {
        FieldProperty *fieldProp = (new FieldProperty(fieldNameID, fieldName))->setOwnerProperty(this);
        staticFields.emplace_back(fieldProp);
        return fieldProp;
    }

    FORCE_INLINE FunctionProperty *addStaticFunc(const StringID &funcNameID, const TChar *funcName)
    {
        FunctionProperty *funcProp = (new FunctionProperty(funcNameID, funcName))->setOwnerProperty(this);
        staticFunctions.emplace_back(funcProp);
        return funcProp;
    }

    FORCE_INLINE ClassProperty *setBaseClass(const BaseProperty *baseClassProp)
    {
        baseClass = static_cast<const ClassProperty *>(baseClassProp);
        return this;
    }
    FORCE_INLINE ClassProperty *addInterface(const TChar *interfaceName, PtrInt offset, const ReflectTypeInfo *interfaceTypeInfo)
    {
        interfaces.emplace_back(interfaceName, offset, interfaceTypeInfo);
        return this;
    }

    ClassProperty *setPropertyMetaData(const std::vector<const PropertyMetaDataBase *> &propertyMeta, uint64 propertyMetaFlags);

    // Get functions
    template <class MetaType>
    FORCE_INLINE const MetaType *getPropertyMetaData() const
    {
        return static_cast<const MetaType *>(getMetaData(typeInfoFrom<MetaType>()));
    }
    uint64 getPropertyMetaFlags() const;
};

class REFLECTIONRUNTIME_EXPORT EnumProperty final : public TypedProperty
{
public:
    // enum field's value and type of meta data class is unique key here
    using EnumFieldMetaKey = std::pair<uint64, const ReflectTypeInfo *>;
    struct EnumField
    {
        uint64 value;
        uint64 metaFlags;
        const TChar *entryNameString;
        StringID entryName;
    };

public:
    std::vector<EnumField> fields;
    std::unordered_map<EnumFieldMetaKey, const PropertyMetaDataBase *> fieldsMeta;
    // If this enum is flags
    bool bIsFlags;

public:
    // Complete class name including namespace/classes
    EnumProperty(const StringID &propNameID, const TChar *propName, const ReflectTypeInfo *enumTypeInfo, bool bCanBeUsedAsFlags);

    EnumProperty *addEnumField(
        const StringID &fieldNameID, const TChar *fieldName, uint64 fieldValue, uint64 metaFlags,
        std::vector<const PropertyMetaDataBase *> fieldMetaData
    );
    EnumProperty *setPropertyMetaData(const std::vector<const PropertyMetaDataBase *> &propertyMeta, uint64 propertyMetaFlags);

    // Get functions
    template <class MetaType>
    FORCE_INLINE const MetaType *getPropertyMetaData() const
    {
        return static_cast<const MetaType *>(getMetaData(typeInfoFrom<MetaType>()));
    }
    uint64 getPropertyMetaFlags() const;
};

// Will be class that has proper reflection
class REFLECTIONRUNTIME_EXPORT QualifiedProperty : public TypedProperty
{
public:
    const BaseProperty *unqualTypeProperty;

public:
    QualifiedProperty(const StringID &propNameID, const TChar *propName, const ReflectTypeInfo *propTypeInfo);

    FORCE_INLINE QualifiedProperty *setUnqualifiedType(const BaseProperty *prop)
    {
        unqualTypeProperty = prop;
        return this;
    }
};

//////////////////////////////////////////////////////////////////////////
/// Serialization functions
//////////////////////////////////////////////////////////////////////////

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, ClassProperty const *&value)
{
    if (archive.isLoading())
    {
        StringID className;
        archive << className;
        value = IReflectionRuntimeModule::get()->getClassType(className);
        if (value == nullptr)
        {
            value = IReflectionRuntimeModule::get()->getStructType(className);
        }
    }
    else
    {
        StringID className = value ? value->name : StringID::INVALID;
        archive << className;
    }
    return archive;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, EnumProperty const *&value)
{
    if (archive.isLoading())
    {
        StringID enumName;
        archive << enumName;
        value = IReflectionRuntimeModule::get()->getEnumType(enumName);
    }
    else
    {
        StringID enumName = value ? value->name : StringID::INVALID;
        archive << enumName;
    }
    return archive;
}