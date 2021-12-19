#pragma once

#include "Modules/IModuleBase.h"
#include "ReflectionRuntimeExports.h"

struct ReflectTypeInfo;
class ClassProperty;
class EnumProperty;
class BaseProperty;
class String;

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

    template <typename StructType>
    const ClassProperty* getStructType()
    {
        return getStructType(typeInfoFrom<CleanType<StructType>>());
    }
    template <typename ClassType>
    const ClassProperty* getClassType()
    {
        return getClassType(typeInfoFrom<CleanType<ClassType>>());
    }
    template <typename EnumType>
    const EnumProperty* getEnumType()
    {
        return getEnumType(typeInfoFrom<CleanType<EnumType>>());
    }
};
