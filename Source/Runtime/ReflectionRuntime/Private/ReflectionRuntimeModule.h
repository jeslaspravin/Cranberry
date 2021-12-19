#pragma once

#include "IReflectionRuntime.h"
#include "String/String.h"

#include <unordered_map>

class ReflectionRuntimeModule final : public IReflectionRuntimeModule
{
private:
    std::unordered_map<const ReflectTypeInfo*, const ClassProperty*> dbClassTypes;
    std::unordered_map<String, const ClassProperty*> dbClassTypesFromName;

    std::unordered_map<const ReflectTypeInfo*, const ClassProperty*> dbStructTypes;
    std::unordered_map<String, const ClassProperty*> dbStructTypesFromName;

    std::unordered_map<const ReflectTypeInfo*, const EnumProperty*> dbEnumTypes;
    std::unordered_map<String, const EnumProperty*> dbEnumTypesFromName;

    std::unordered_map<const ReflectTypeInfo*, const BaseProperty*> dbOtherTypes;
public:
    /* IReflectionRuntimeModule finals */
    const ClassProperty* getStructType(const ReflectTypeInfo* typeInfo) final;
    const ClassProperty* getStructType(const String& structName) final;

    const ClassProperty* getClassType(const ReflectTypeInfo* typeInfo) final;
    const ClassProperty* getClassType(const String& className) final;

    const EnumProperty* getEnumType(const ReflectTypeInfo* typeInfo) final;
    const EnumProperty* getEnumType(const String& enumName) final;

    const BaseProperty* getType(const ReflectTypeInfo* typeInfo) final;

    /* IModuleBase finals */
    void init() final;
    void release() final;
    /* finals end */

};