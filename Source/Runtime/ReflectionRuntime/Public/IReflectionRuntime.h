/*!
 * \file IReflectionRuntime.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Modules/IModuleBase.h"
#include "ReflectionRuntimeExports.h"
#include "Reflections/Functions.h"
#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"
#include "Types/TypesInfo.h"

class ClassProperty;
class EnumProperty;
class BaseProperty;
class StringID;
class TypedProperty;
class PropertyMetaDataBase;

// Has to separate init and create to avoid race condition between creating a property and using the
// created property on property that is created in this property init
using ClassPropertyFactoryFunction = Function<ClassProperty *>;
using ClassPropertyInitFunction = Function<void, ClassProperty *>;
using EnumPropertyFactoryFunction = Function<EnumProperty *>;
using EnumPropertyInitFunction = Function<void, EnumProperty *>;
using TypedPropertyFactoryFunction = Function<BaseProperty *>;
using TypedPropertyInitFunction = Function<void, BaseProperty *>;

struct ClassPropertyFactoryCell
{
    ClassPropertyFactoryFunction factoryFunc;
    ClassPropertyInitFunction initFunc;

    ClassPropertyFactoryCell() = default;

    ClassPropertyFactoryCell(
        ClassPropertyFactoryFunction ::StaticDelegate factoryFuncPtr, ClassPropertyInitFunction::StaticDelegate initFuncPtr
    )
        : factoryFunc(factoryFuncPtr)
        , initFunc(initFuncPtr)
    {}

    ClassPropertyFactoryCell(ClassPropertyFactoryFunction::StaticDelegate factoryFuncPtr)
        : factoryFunc(factoryFuncPtr)
        , initFunc(nullptr)
    {}
};

struct EnumPropertyFactoryCell
{
    EnumPropertyFactoryFunction factoryFunc;
    EnumPropertyInitFunction initFunc;

    EnumPropertyFactoryCell() = default;

    EnumPropertyFactoryCell(EnumPropertyFactoryFunction::StaticDelegate factoryFuncPtr, EnumPropertyInitFunction::StaticDelegate initFuncPtr)
        : factoryFunc(factoryFuncPtr)
        , initFunc(initFuncPtr)
    {}

    EnumPropertyFactoryCell(EnumPropertyFactoryFunction::StaticDelegate factoryFuncPtr)
        : factoryFunc(factoryFuncPtr)
        , initFunc(nullptr)
    {}
};

struct TypedPropertyFactoryCell
{
    TypedPropertyFactoryFunction factoryFunc;
    TypedPropertyInitFunction initFunc;

    TypedPropertyFactoryCell() = default;

    TypedPropertyFactoryCell(TypedPropertyFactoryFunction::StaticDelegate factoryFuncPtr, TypedPropertyInitFunction::StaticDelegate initFuncPtr)
        : factoryFunc(factoryFuncPtr)
        , initFunc(initFuncPtr)
    {}

    TypedPropertyFactoryCell(TypedPropertyFactoryFunction::StaticDelegate factoryFuncPtr)
        : factoryFunc(factoryFuncPtr)
        , initFunc(nullptr)
    {}
};

class REFLECTIONRUNTIME_EXPORT IReflectionRuntimeModule : public IModuleBase
{
public:
    virtual const ClassProperty *getStructType(const ReflectTypeInfo *typeInfo) = 0;
    // Name must be none const plain struct name including namespace and outer class/struct, separated by
    // ::(eg., Namespace1::Class1::structName)
    virtual const ClassProperty *getStructType(const StringID &structName) = 0;

    // Non-const because we might lazy initialize ClassProperty if any present
    virtual void getChildsOf(
        const ClassProperty *clazz, std::vector<const ClassProperty *> &outChilds, bool bRecursively = false, bool bOnlyLeafChilds = false
    ) = 0;
    virtual const ClassProperty *getClassType(const ReflectTypeInfo *typeInfo) = 0;
    // Name must be none const plain struct name including namespace and outer class/struct, separated by
    // ::(eg., Namespace1::Class1::structName)
    virtual const ClassProperty *getClassType(const StringID &className) = 0;

    virtual const EnumProperty *getEnumType(const ReflectTypeInfo *typeInfo) = 0;
    // Name must be none const plain struct name including namespace and outer class/struct, separated by
    // ::(eg., Namespace1::Class1::structName)
    virtual const EnumProperty *getEnumType(const StringID &enumName) = 0;

    // Any types other than Struct, Class, Enum. This also accounts for const, reference and pointer in
    // type so const int32 is different from int32 and each has its own property
    virtual const BaseProperty *getType(const ReflectTypeInfo *typeInfo) = 0;

    // Using this to get list of all meta data for a property is not efficient(Use search by property and
    // meta data type)
    virtual std::vector<const PropertyMetaDataBase *> getPropertyMetaData(const BaseProperty *prop) const = 0;
    virtual const PropertyMetaDataBase *getPropertyMetaData(const BaseProperty *prop, const ReflectTypeInfo *typeInfo) const = 0;
    virtual uint64 getPropertyMetaFlags(const BaseProperty *prop) const = 0;

    static IReflectionRuntimeModule *get();
    static void
        registerClassFactory(const StringID &className, const ReflectTypeInfo *classTypeInfo, const ClassPropertyFactoryCell &factoryCell);
    static void
        registerStructFactory(const StringID &structName, const ReflectTypeInfo *structTypeInfo, const ClassPropertyFactoryCell &factoryCell);
    static void registerEnumFactory(const StringID &enumName, const ReflectTypeInfo *enumTypeInfo, const EnumPropertyFactoryCell &factoryCell);
    static void registerTypeFactory(const ReflectTypeInfo *typeInfo, const TypedPropertyFactoryCell &factoryCell);
    // Just a function to have same register signature as other factor register functions
    FORCE_INLINE static void
        registerTypeFactory(const StringID &, const ReflectTypeInfo *typeInfo, const TypedPropertyFactoryCell &factoryCell)
    {
        registerTypeFactory(typeInfo, factoryCell);
    }

    template <typename StructType>
    static const ClassProperty *getStructType()
    {
        return get()->getStructType(typeInfoFrom<CleanType<StructType>>());
    }
    template <typename ClassType>
    static const ClassProperty *getClassType()
    {
        return get()->getClassType(typeInfoFrom<CleanType<ClassType>>());
    }
    template <typename EnumType>
    static const EnumProperty *getEnumType()
    {
        return get()->getEnumType(typeInfoFrom<CleanType<EnumType>>());
    }

    template <typename Type>
    static const BaseProperty *getType()
    {
        return get()->getType(typeInfoFrom<Type>());
    }

    template <typename MetaType>
    static const MetaType *getPropertyMetaData(const BaseProperty *prop)
    {
        return static_cast<const MetaType *>(get()->getPropertyMetaData(prop, typeInfoFrom<MetaType>()));
    }
};