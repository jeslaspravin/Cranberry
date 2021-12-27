#pragma once

#include "IReflectionRuntime.h"
#include "Types/HashTypes.h"
#include "String/String.h"

#include <unordered_map>

class ReflectionRuntimeModule final : public IReflectionRuntimeModule
{
private:
    friend IReflectionRuntimeModule;

    // Property factory registries
    static std::unordered_map<const ReflectTypeInfo*, std::pair<String, ClassPropertyFactoryCell>>& classFactoryFromTypeInfo();
    static std::unordered_map<String, std::pair<const ReflectTypeInfo*, ClassPropertyFactoryCell>>& classFactoryFromTypeName();
    const ClassProperty* createClassProperty(const ReflectTypeInfo* typeInfo);
    const ClassProperty* createClassProperty(const String& typeName);

    static std::unordered_map<const ReflectTypeInfo*, std::pair<String, ClassPropertyFactoryCell>>& structFactoryFromTypeInfo();
    static std::unordered_map<String, std::pair<const ReflectTypeInfo*, ClassPropertyFactoryCell>>& structFactoryFromTypeName();
    const ClassProperty* createStructProperty(const ReflectTypeInfo* typeInfo);
    const ClassProperty* createStructProperty(const String& typeName);

    static std::unordered_map<const ReflectTypeInfo*, std::pair<String, EnumPropertyFactoryCell>>& enumFactoryFromTypeInfo();
    static std::unordered_map<String, std::pair<const ReflectTypeInfo*, EnumPropertyFactoryCell>>& enumFactoryFromTypeName();
    const EnumProperty* createEnumProperty(const ReflectTypeInfo* typeInfo);
    const EnumProperty* createEnumProperty(const String& typeName);

    static std::unordered_map<const ReflectTypeInfo*, TypedPropertyFactoryCell>& otherTypesFactories();
    const BaseProperty* createTypedProperty(const ReflectTypeInfo* typeInfo);

    // Creates common none reflect types to property database
    void initCommonProperties() const;

    // Property database
    std::unordered_map<const ReflectTypeInfo*, const ClassProperty*> dbClassTypes;
    std::unordered_map<String, const ClassProperty*> dbClassTypesFromName;

    std::unordered_map<const ReflectTypeInfo*, const ClassProperty*> dbStructTypes;
    std::unordered_map<String, const ClassProperty*> dbStructTypesFromName;

    std::unordered_map<const ReflectTypeInfo*, const EnumProperty*> dbEnumTypes;
    std::unordered_map<String, const EnumProperty*> dbEnumTypesFromName;

    std::unordered_map<const ReflectTypeInfo*, const BaseProperty*> dbOtherTypes;

    using PropertyMetaDataKey = std::pair<const BaseProperty*, const ReflectTypeInfo*>;
    std::unordered_map<PropertyMetaDataKey, const PropertyMetaDataBase*> propertiesMetaData;
    std::unordered_map<const BaseProperty*, uint64> propertiesMetaFlags;
public:
    void setMetaData(const BaseProperty* forProperty, std::vector<const PropertyMetaDataBase*>& propertyMeta, uint64 propertyMetaFlags);

    /* IReflectionRuntimeModule finals */
    const ClassProperty* getStructType(const ReflectTypeInfo* typeInfo) final;
    const ClassProperty* getStructType(const String& structName) final;

    const ClassProperty* getClassType(const ReflectTypeInfo* typeInfo) final;
    const ClassProperty* getClassType(const String& className) final;

    const EnumProperty* getEnumType(const ReflectTypeInfo* typeInfo) final;
    const EnumProperty* getEnumType(const String& enumName) final;

    const BaseProperty* getType(const ReflectTypeInfo* typeInfo) final;

    std::vector<const PropertyMetaDataBase*> getPropertyMetaData(const BaseProperty* prop) const final;
    const PropertyMetaDataBase* getPropertyMetaData(const BaseProperty* prop, const ReflectTypeInfo* typeInfo) const final;
    uint64 getPropertyMetaFlags(const BaseProperty* prop) const final;

    /* IModuleBase finals */
    void init() final;
    void release() final;
    /* finals end */

};