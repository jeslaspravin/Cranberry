/*!
 * \file ReflectionRuntimeModule.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "IReflectionRuntime.h"
#include "String/StringID.h"
#include "Types/HashTypes.h"
#include "Types/Containers/FlatTree.h"
#include "Types/Containers/ArrayView.h"

#include <unordered_map>

using ClassTreeType = FlatTree<const ClassProperty *, uint32>;

class ReflectionRuntimeModule final : public IReflectionRuntimeModule
{
private:
    friend IReflectionRuntimeModule;

    // Property factory registries
    static std::unordered_map<const ReflectTypeInfo *, std::pair<StringID, ClassPropertyFactoryCell>> &classFactoryFromTypeInfo();
    static std::unordered_map<StringID, std::pair<const ReflectTypeInfo *, ClassPropertyFactoryCell>> &classFactoryFromTypeName();
    const ClassProperty *createClassProperty(const ReflectTypeInfo *typeInfo);
    const ClassProperty *createClassProperty(const StringID &typeName);
    void createAllPendingClasses();

    static std::unordered_map<const ReflectTypeInfo *, std::pair<StringID, ClassPropertyFactoryCell>> &structFactoryFromTypeInfo();
    static std::unordered_map<StringID, std::pair<const ReflectTypeInfo *, ClassPropertyFactoryCell>> &structFactoryFromTypeName();
    const ClassProperty *createStructProperty(const ReflectTypeInfo *typeInfo);
    const ClassProperty *createStructProperty(const StringID &typeName);

    static std::unordered_map<const ReflectTypeInfo *, std::pair<StringID, EnumPropertyFactoryCell>> &enumFactoryFromTypeInfo();
    static std::unordered_map<StringID, std::pair<const ReflectTypeInfo *, EnumPropertyFactoryCell>> &enumFactoryFromTypeName();
    const EnumProperty *createEnumProperty(const ReflectTypeInfo *typeInfo);
    const EnumProperty *createEnumProperty(const StringID &typeName);

    static std::unordered_map<const ReflectTypeInfo *, TypedPropertyFactoryCell> &otherTypesFactories();
    const BaseProperty *createTypedProperty(const ReflectTypeInfo *typeInfo);

    // Creates common none reflect types to property database
    void initCommonProperties() const;

    // Property database
    ClassTreeType dbClasses;
    std::unordered_map<const ReflectTypeInfo *, ClassTreeType::NodeIdx> dbClassTypes;
    std::unordered_map<StringID, ClassTreeType::NodeIdx> dbClassTypesFromName;

    std::unordered_map<const ReflectTypeInfo *, const ClassProperty *> dbStructTypes;
    std::unordered_map<StringID, const ClassProperty *> dbStructTypesFromName;

    std::unordered_map<const ReflectTypeInfo *, const EnumProperty *> dbEnumTypes;
    std::unordered_map<StringID, const EnumProperty *> dbEnumTypesFromName;

    std::unordered_map<const ReflectTypeInfo *, const BaseProperty *> dbOtherTypes;

    using PropertyMetaDataKey = std::pair<const BaseProperty *, const ReflectTypeInfo *>;
    std::unordered_map<PropertyMetaDataKey, const PropertyMetaDataBase *> propertiesMetaData;
    std::unordered_map<const BaseProperty *, uint64> propertiesMetaFlags;

public:
    void setMetaData(const BaseProperty *forProperty, ArrayView<const PropertyMetaDataBase *> propertyMeta, uint64 propertyMetaFlags);

    /* IReflectionRuntimeModule finals */
    const ClassProperty *getStructType(const ReflectTypeInfo *typeInfo) final;
    const ClassProperty *getStructType(const StringID &structName) final;

    void getChildsOf(
        const ClassProperty *clazz, std::vector<const ClassProperty *> &outChilds, bool bRecursively = false, bool bOnlyLeafChilds = false
    ) final;
    const ClassProperty *getClassType(const ReflectTypeInfo *typeInfo) final;
    const ClassProperty *getClassType(const StringID &className) final;

    const EnumProperty *getEnumType(const ReflectTypeInfo *typeInfo) final;
    const EnumProperty *getEnumType(const StringID &enumName) final;

    const BaseProperty *getType(const ReflectTypeInfo *typeInfo) final;

    std::vector<const PropertyMetaDataBase *> getPropertyMetaData(const BaseProperty *prop) const final;
    const PropertyMetaDataBase *getPropertyMetaData(const BaseProperty *prop, const ReflectTypeInfo *typeInfo) const final;
    uint64 getPropertyMetaFlags(const BaseProperty *prop) const final;

    /* IModuleBase finals */
    void init() final;
    void release() final;
    /* finals end */
};