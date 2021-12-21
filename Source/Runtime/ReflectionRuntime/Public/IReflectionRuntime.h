#pragma once

#include "Modules/IModuleBase.h"
#include "ReflectionRuntimeExports.h"
#include "Reflections/Functions.h"

struct ReflectTypeInfo;
class ClassProperty;
class EnumProperty;
class BaseProperty;
class String;
class TypedProperty;

// Has to separate init and create to avoid race condition between creating a property and using the created property on property that is created in this property init
using ClassPropertyFactoryFunction = Function<ClassProperty*>;
using ClassPropertyInitFunction = Function<void, ClassProperty*>;
using EnumPropertyFactoryFunction = Function<EnumProperty*>;
using EnumPropertyInitFunction = Function<void, EnumProperty*>;
using TypedPropertyFactoryFunction = Function<BaseProperty*>;
using TypedPropertyInitFunction = Function<void, BaseProperty*>;

struct ClassPropertyFactoryCell
{
    ClassPropertyFactoryFunction factoryFunc;
    ClassPropertyInitFunction initFunc;
};

struct EnumPropertyFactoryCell
{
    EnumPropertyFactoryFunction factoryFunc;
    EnumPropertyInitFunction initFunc;
};

struct TypedPropertyFactoryCell
{
    TypedPropertyFactoryFunction factoryFunc;
    TypedPropertyInitFunction initFunc;
};

class REFLECTIONRUNTIME_EXPORT IReflectionRuntimeModule : public IModuleBase
{
public:
    virtual const ClassProperty* getStructType(const ReflectTypeInfo* typeInfo) = 0;
    // Name must be none const plain struct name including namespace and outer class/struct, separated by ::(eg., Namespace1::Class1::structName)
    virtual const ClassProperty* getStructType(const String& structName) = 0;

    virtual const ClassProperty* getClassType(const ReflectTypeInfo* typeInfo) = 0;
    // Name must be none const plain struct name including namespace and outer class/struct, separated by ::(eg., Namespace1::Class1::structName)
    virtual const ClassProperty* getClassType(const String& className) = 0;

    virtual const EnumProperty* getEnumType(const ReflectTypeInfo* typeInfo) = 0;
    // Name must be none const plain struct name including namespace and outer class/struct, separated by ::(eg., Namespace1::Class1::structName)
    virtual const EnumProperty* getEnumType(const String& enumName) = 0;

    // Any types other than Struct, Class, Enum. This also accounts for const, reference and pointer in type so const int32 is different from int32 and each has its own property 
    virtual const BaseProperty* getType(const ReflectTypeInfo* typeInfo) = 0;

    static IReflectionRuntimeModule* get();
    static void registerClassFactory(const String& className, const ReflectTypeInfo* classTypeInfo, const ClassPropertyFactoryCell& factoryCell);
    static void registerStructFactory(const String& structName, const ReflectTypeInfo* structTypeInfo, const ClassPropertyFactoryCell& factoryCell);
    static void registerEnumFactory(const String& enumName, const ReflectTypeInfo* enumTypeInfo, const EnumPropertyFactoryCell& factoryCell);
    static void registerTypeFactory(const ReflectTypeInfo* typeInfo, const TypedPropertyFactoryCell& factoryCell);

    template <typename StructType>
    static const ClassProperty* getStructType()
    {
        return get()->getStructType(typeInfoFrom<CleanType<StructType>>());
    }
    template <typename ClassType>
    static const ClassProperty* getClassType()
    {
        return get()->getClassType(typeInfoFrom<CleanType<ClassType>>());
    }
    template <typename EnumType>
    static const EnumProperty* getEnumType()
    {
        return get()->getEnumType(typeInfoFrom<CleanType<EnumType>>());
    }

    template <typename Type>
    static const BaseProperty* getType()
    {
        return get()->getType(typeInfoFrom<Type>());
    }
};