#include "ReflectionRuntimeModule.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Modules/ModuleManager.h"
#include "Types/TypesInfo.h"
#include "Property/Property.h"

#include <unordered_set>

DECLARE_MODULE(ReflectionRuntime, ReflectionRuntimeModule)

const ReflectTypeInfo* ReflectTypeInfo::createTypeInfo(const std::type_info& cleanTypeInfo, const ReflectTypeInfo* innerTypeInfo, uint32 inQualifiers)
{
    static std::unordered_set<ReflectTypeInfo> dbTypeInfo;

    std::pair<std::unordered_set<ReflectTypeInfo>::iterator, bool> insertResult = dbTypeInfo.insert(ReflectTypeInfo{ cleanTypeInfo, innerTypeInfo, inQualifiers });
    return &(*insertResult.first);
}

IReflectionRuntimeModule* IReflectionRuntimeModule::get()
{
    static WeakModulePtr appModule = ModuleManager::get()->getOrLoadModule("ReflectionRuntime");
    if (appModule.expired())
    {
        return nullptr;
    }
    return static_cast<IReflectionRuntimeModule*>(appModule.lock().get());
}

void IReflectionRuntimeModule::registerClassFactory(const String& className, const ReflectTypeInfo* classTypeInfo, const ClassPropertyFactoryCell& factoryCell)
{
    auto factoryFromTypeInfoItr = ReflectionRuntimeModule::classFactoryFromTypeInfo().find(classTypeInfo);
    auto factoryFromTypeNameItr = ReflectionRuntimeModule::classFactoryFromTypeName().find(className);
    // Make sure that both factory function registries are in sync
    debugAssert((factoryFromTypeInfoItr == ReflectionRuntimeModule::classFactoryFromTypeInfo().end()) == (factoryFromTypeNameItr == ReflectionRuntimeModule::classFactoryFromTypeName().end()));

    if (factoryFromTypeInfoItr == ReflectionRuntimeModule::classFactoryFromTypeInfo().end())
    {
        ReflectionRuntimeModule::classFactoryFromTypeInfo()[classTypeInfo] = { className, factoryCell };
        ReflectionRuntimeModule::classFactoryFromTypeName()[className] = { classTypeInfo, factoryCell };
    }
}

void IReflectionRuntimeModule::registerStructFactory(const String& structName, const ReflectTypeInfo* structTypeInfo, const ClassPropertyFactoryCell& factoryCell)
{
    auto factoryFromTypeInfoItr = ReflectionRuntimeModule::structFactoryFromTypeInfo().find(structTypeInfo);
    auto factoryFromTypeNameItr = ReflectionRuntimeModule::structFactoryFromTypeName().find(structName);
    // Make sure that both factory function registries are in sync
    debugAssert((factoryFromTypeInfoItr == ReflectionRuntimeModule::structFactoryFromTypeInfo().end()) == (factoryFromTypeNameItr == ReflectionRuntimeModule::structFactoryFromTypeName().end()));

    if (factoryFromTypeInfoItr == ReflectionRuntimeModule::structFactoryFromTypeInfo().end())
    {
        ReflectionRuntimeModule::structFactoryFromTypeInfo()[structTypeInfo] = { structName, factoryCell };
        ReflectionRuntimeModule::structFactoryFromTypeName()[structName] = { structTypeInfo, factoryCell };
    }
}

void IReflectionRuntimeModule::registerEnumFactory(const String& enumName, const ReflectTypeInfo* enumTypeInfo, const EnumPropertyFactoryCell& factoryCell)
{
    auto factoryFromTypeInfoItr = ReflectionRuntimeModule::enumFactoryFromTypeInfo().find(enumTypeInfo);
    auto factoryFromTypeNameItr = ReflectionRuntimeModule::enumFactoryFromTypeName().find(enumName);
    // Make sure that both factory function registries are in sync
    debugAssert((factoryFromTypeInfoItr == ReflectionRuntimeModule::enumFactoryFromTypeInfo().end()) == (factoryFromTypeNameItr == ReflectionRuntimeModule::enumFactoryFromTypeName().end()));

    if (factoryFromTypeInfoItr == ReflectionRuntimeModule::enumFactoryFromTypeInfo().end())
    {
        ReflectionRuntimeModule::enumFactoryFromTypeInfo()[enumTypeInfo] = { enumName, factoryCell };
        ReflectionRuntimeModule::enumFactoryFromTypeName()[enumName] = { enumTypeInfo, factoryCell };
    }
}

void IReflectionRuntimeModule::registerTypeFactory(const ReflectTypeInfo* typeInfo, const TypedPropertyFactoryCell& factoryCell)
{
    auto factoryFromTypeInfoItr = ReflectionRuntimeModule::otherTypesFactories().find(typeInfo);

    if (factoryFromTypeInfoItr == ReflectionRuntimeModule::otherTypesFactories().end())
    {
        ReflectionRuntimeModule::otherTypesFactories()[typeInfo] = factoryCell;
    }
}

std::unordered_map<const ReflectTypeInfo*, std::pair<String, ClassPropertyFactoryCell>>& ReflectionRuntimeModule::classFactoryFromTypeInfo()
{
    static std::unordered_map<const ReflectTypeInfo*, std::pair<String, ClassPropertyFactoryCell>> factoriesRegistry;
    return factoriesRegistry;
}
std::unordered_map<String, std::pair<const ReflectTypeInfo*, ClassPropertyFactoryCell>>& ReflectionRuntimeModule::classFactoryFromTypeName()
{
    static std::unordered_map<String, std::pair<const ReflectTypeInfo*, ClassPropertyFactoryCell>> factoriesRegistry;
    return factoriesRegistry;
}
const ClassProperty* ReflectionRuntimeModule::createClassProperty(const ReflectTypeInfo* typeInfo)
{
    auto propertyCreateFactoryItr = classFactoryFromTypeInfo().find(typeInfo);
    if (propertyCreateFactoryItr != classFactoryFromTypeInfo().end() && propertyCreateFactoryItr->second.second.factoryFunc)
    {
        ClassProperty* prop = propertyCreateFactoryItr->second.second.factoryFunc();

        // Set the property in db for other property access immediately
        dbClassTypes[typeInfo] = prop;
        dbClassTypesFromName[propertyCreateFactoryItr->second.first] = prop;
        // Initialize the property now
        if (propertyCreateFactoryItr->second.second.initFunc)
        {
            propertyCreateFactoryItr->second.second.initFunc(prop);
        }
        // Not clear the registered factories
        classFactoryFromTypeName().erase(propertyCreateFactoryItr->second.first);
        classFactoryFromTypeInfo().erase(propertyCreateFactoryItr);
        return prop;
    }
    else
    {
        Logger::error("ReflectionRuntimeModule", "%s() : Creating class property failed for type %s", __func__, *typeInfo);
    }
    return nullptr;
}
const ClassProperty* ReflectionRuntimeModule::createClassProperty(const String& typeName)
{
    auto propertyCreateFactoryItr = classFactoryFromTypeName().find(typeName);
    if (propertyCreateFactoryItr != classFactoryFromTypeName().end() && propertyCreateFactoryItr->second.second.factoryFunc)
    {
        ClassProperty* prop = propertyCreateFactoryItr->second.second.factoryFunc();

        dbClassTypes[propertyCreateFactoryItr->second.first] = prop;
        dbClassTypesFromName[typeName] = prop;

        if (propertyCreateFactoryItr->second.second.initFunc)
        {
            propertyCreateFactoryItr->second.second.initFunc(prop);
        }

        classFactoryFromTypeName().erase(propertyCreateFactoryItr);
        classFactoryFromTypeInfo().erase(propertyCreateFactoryItr->second.first);
        return prop;
    }
    else
    {
        Logger::error("ReflectionRuntimeModule", "%s() : Creating class property failed for type %s", __func__, typeName);
    }
    return nullptr;
}

std::unordered_map<const ReflectTypeInfo*, std::pair<String, ClassPropertyFactoryCell>>& ReflectionRuntimeModule::structFactoryFromTypeInfo()
{
    static std::unordered_map<const ReflectTypeInfo*, std::pair<String, ClassPropertyFactoryCell>> factoriesRegistry;
    return factoriesRegistry;
}
std::unordered_map<String, std::pair<const ReflectTypeInfo*, ClassPropertyFactoryCell>>& ReflectionRuntimeModule::structFactoryFromTypeName()
{
    static std::unordered_map<String, std::pair<const ReflectTypeInfo*, ClassPropertyFactoryCell>> factoriesRegistry;
    return factoriesRegistry;
}
const ClassProperty* ReflectionRuntimeModule::createStructProperty(const ReflectTypeInfo* typeInfo)
{
    auto propertyCreateFactoryItr = structFactoryFromTypeInfo().find(typeInfo);
    if (propertyCreateFactoryItr != structFactoryFromTypeInfo().end() && propertyCreateFactoryItr->second.second.factoryFunc)
    {
        ClassProperty* prop = propertyCreateFactoryItr->second.second.factoryFunc();

        dbClassTypes[typeInfo] = prop;
        dbClassTypesFromName[propertyCreateFactoryItr->second.first] = prop;

        if (propertyCreateFactoryItr->second.second.initFunc)
        {
            propertyCreateFactoryItr->second.second.initFunc(prop);
        }
        
        classFactoryFromTypeName().erase(propertyCreateFactoryItr->second.first);
        classFactoryFromTypeInfo().erase(propertyCreateFactoryItr);
        return prop;
    }
    else
    {
        Logger::error("ReflectionRuntimeModule", "%s() : Creating struct property failed for type %s", __func__, *typeInfo);
    }
    return nullptr;
}
const ClassProperty* ReflectionRuntimeModule::createStructProperty(const String& typeName)
{
    auto propertyCreateFactoryItr = structFactoryFromTypeName().find(typeName);
    if (propertyCreateFactoryItr != structFactoryFromTypeName().end() && propertyCreateFactoryItr->second.second.factoryFunc)
    {
        ClassProperty* prop = propertyCreateFactoryItr->second.second.factoryFunc();

        dbClassTypes[propertyCreateFactoryItr->second.first] = prop;
        dbClassTypesFromName[typeName] = prop;

        if (propertyCreateFactoryItr->second.second.initFunc)
        {
            propertyCreateFactoryItr->second.second.initFunc(prop);
        }

        classFactoryFromTypeName().erase(propertyCreateFactoryItr);
        classFactoryFromTypeInfo().erase(propertyCreateFactoryItr->second.first);
        return prop;
    }
    else
    {
        Logger::error("ReflectionRuntimeModule", "%s() : Creating struct property failed for type %s", __func__, typeName);
    }
    return nullptr;
}

std::unordered_map<const ReflectTypeInfo*, std::pair<String, EnumPropertyFactoryCell>>& ReflectionRuntimeModule::enumFactoryFromTypeInfo()
{
    static std::unordered_map<const ReflectTypeInfo*, std::pair<String, EnumPropertyFactoryCell>> factoriesRegistry;
    return factoriesRegistry;
}
std::unordered_map<String, std::pair<const ReflectTypeInfo*, EnumPropertyFactoryCell>>& ReflectionRuntimeModule::enumFactoryFromTypeName()
{
    static std::unordered_map<String, std::pair<const ReflectTypeInfo*, EnumPropertyFactoryCell>> factoriesRegistry;
    return factoriesRegistry;
}
const EnumProperty* ReflectionRuntimeModule::createEnumProperty(const ReflectTypeInfo* typeInfo)
{
    auto propertyCreateFactoryItr = enumFactoryFromTypeInfo().find(typeInfo);
    if (propertyCreateFactoryItr != enumFactoryFromTypeInfo().end() && propertyCreateFactoryItr->second.second.factoryFunc)
    {
        EnumProperty* prop = propertyCreateFactoryItr->second.second.factoryFunc();

        dbEnumTypes[typeInfo] = prop;
        dbEnumTypesFromName[propertyCreateFactoryItr->second.first] = prop;

        if (propertyCreateFactoryItr->second.second.initFunc)
        {
            propertyCreateFactoryItr->second.second.initFunc(prop);
        }

        enumFactoryFromTypeName().erase(propertyCreateFactoryItr->second.first);
        enumFactoryFromTypeInfo().erase(propertyCreateFactoryItr);
        return prop;
    }
    else
    {
        Logger::error("ReflectionRuntimeModule", "%s() : Creating enum property failed for type %s", __func__, *typeInfo);
    }
    return nullptr;
}
const EnumProperty* ReflectionRuntimeModule::createEnumProperty(const String& typeName)
{
    auto propertyCreateFactoryItr = enumFactoryFromTypeName().find(typeName);
    if (propertyCreateFactoryItr != enumFactoryFromTypeName().end() && propertyCreateFactoryItr->second.second.factoryFunc)
    {
        EnumProperty* prop = propertyCreateFactoryItr->second.second.factoryFunc();

        dbEnumTypes[propertyCreateFactoryItr->second.first] = prop;
        dbEnumTypesFromName[typeName] = prop;

        if (propertyCreateFactoryItr->second.second.initFunc)
        {
            propertyCreateFactoryItr->second.second.initFunc(prop);
        }

        enumFactoryFromTypeName().erase(propertyCreateFactoryItr);
        enumFactoryFromTypeInfo().erase(propertyCreateFactoryItr->second.first);
        return prop;
    }
    else
    {
        Logger::error("ReflectionRuntimeModule", "%s() : Creating enum property failed for type %s", __func__, typeName);
    }
    return nullptr;
}

std::unordered_map<const ReflectTypeInfo*, TypedPropertyFactoryCell>& ReflectionRuntimeModule::otherTypesFactories()
{
    static std::unordered_map<const ReflectTypeInfo*, TypedPropertyFactoryCell> factoriesRegistry;
    return factoriesRegistry;
}
const BaseProperty* ReflectionRuntimeModule::createTypedProperty(const ReflectTypeInfo* typeInfo)
{
    auto propertyCreateFactoryItr = otherTypesFactories().find(typeInfo);
    if (propertyCreateFactoryItr != otherTypesFactories().end() && propertyCreateFactoryItr->second.factoryFunc)
    {
        BaseProperty* typeProperty = propertyCreateFactoryItr->second.factoryFunc();
        dbOtherTypes[typeInfo] = typeProperty;
        if (propertyCreateFactoryItr->second.initFunc)
        {
            propertyCreateFactoryItr->second.initFunc(typeProperty);
        }
        otherTypesFactories().erase(propertyCreateFactoryItr);
        return typeProperty;
    }
    else
    {
        Logger::error("ReflectionRuntimeModule", "%s() : Creating typed property failed for type %s", __func__, *typeInfo);
    }
    return nullptr;
}

const ClassProperty* ReflectionRuntimeModule::getStructType(const ReflectTypeInfo* typeInfo)
{
    const ClassProperty* retVal = nullptr;
    auto typePropertyItr = dbStructTypes.find(typeInfo);
    if (typePropertyItr == dbStructTypes.end())
    {
        retVal = createStructProperty(typeInfo);
    }
    else
    {
        retVal = typePropertyItr->second;
    }
    return retVal;
}
const ClassProperty* ReflectionRuntimeModule::getStructType(const String& structName)
{
    const ClassProperty* retVal = nullptr;
    auto typePropertyItr = dbStructTypesFromName.find(structName);
    if (typePropertyItr == dbStructTypesFromName.end())
    {
        retVal = createStructProperty(structName);
    }
    else
    {
        retVal = typePropertyItr->second;
    }
    return retVal;
}

const ClassProperty* ReflectionRuntimeModule::getClassType(const ReflectTypeInfo* typeInfo)
{
    const ClassProperty* retVal = nullptr;
    auto typePropertyItr = dbClassTypes.find(typeInfo);
    if (typePropertyItr == dbClassTypes.end())
    {
        retVal = createClassProperty(typeInfo);
    }
    else
    {
        retVal = typePropertyItr->second;
    }
    return retVal;
}
const ClassProperty* ReflectionRuntimeModule::getClassType(const String& className)
{
    const ClassProperty* retVal = nullptr;
    auto typePropertyItr = dbClassTypesFromName.find(className);
    if (typePropertyItr == dbClassTypesFromName.end())
    {
        retVal = createClassProperty(className);
    }
    else
    {
        retVal = typePropertyItr->second;
    }
    return retVal;
}

const EnumProperty* ReflectionRuntimeModule::getEnumType(const ReflectTypeInfo* typeInfo)
{
    const EnumProperty* retVal = nullptr;
    auto typePropertyItr = dbEnumTypes.find(typeInfo);
    if (typePropertyItr == dbEnumTypes.end())
    {
        retVal = createEnumProperty(typeInfo);
    }
    else
    {
        retVal = typePropertyItr->second;
    }
    return retVal;
}
const EnumProperty* ReflectionRuntimeModule::getEnumType(const String& enumName)
{
    const EnumProperty* retVal = nullptr;
    auto typePropertyItr = dbEnumTypesFromName.find(enumName);
    if (typePropertyItr == dbEnumTypesFromName.end())
    {
        retVal = createEnumProperty(enumName);
    }
    else
    {
        retVal = typePropertyItr->second;
    }
    return retVal;
}

const BaseProperty* ReflectionRuntimeModule::getType(const ReflectTypeInfo* typeInfo)
{
    SCOPED_MUTE_LOG_SEVERITIES(Logger::AllServerity);
    const BaseProperty* retVal = nullptr;
    auto typePropertyItr = dbOtherTypes.find(typeInfo);
    if (typePropertyItr == dbOtherTypes.end())
    {
        retVal = createTypedProperty(typeInfo);
    }
    else
    {
        retVal = typePropertyItr->second;
    }
    // If we cannot find in other types search for it in class/struct/enum
    if (retVal == nullptr)
    {
        retVal = getClassType(typeInfo);
    }
    if (retVal == nullptr)
    {
        retVal = getStructType(typeInfo);
    }
    if (retVal == nullptr)
    {
        retVal = getEnumType(typeInfo);
    }
    return retVal;
}

void ReflectionRuntimeModule::init()
{
    initCommonProperties();
}

void ReflectionRuntimeModule::release()
{
    for (const auto& typeProperty : dbClassTypes)
    {
        delete typeProperty.second;
    }
    for (const auto& typeProperty : dbStructTypes)
    {
        delete typeProperty.second;
    }
    for (const auto& typeProperty : dbEnumTypes)
    {
        delete typeProperty.second;
    }
    for (const auto& typeProperty : dbOtherTypes)
    {
        delete typeProperty.second;
    }
    dbClassTypes.clear();
    dbClassTypesFromName.clear();
    dbStructTypes.clear();
    dbStructTypesFromName.clear();
    dbEnumTypes.clear();
    dbEnumTypesFromName.clear();
    dbOtherTypes.clear();
}