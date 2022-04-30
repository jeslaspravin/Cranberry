/*!
 * \file ReflectionRuntimeModule.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "ReflectionRuntimeModule.h"
#include "Modules/ModuleManager.h"
#include "Property/Property.h"
#include "Property/PropertyMetaData.h"
#include "String/String.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/TypesInfo.h"

#include <unordered_set>

DECLARE_MODULE(ReflectionRuntime, ReflectionRuntimeModule)

const ReflectTypeInfo *ReflectTypeInfo::createTypeInfo(
    const std::type_info &cleanTypeInfo, const ReflectTypeInfo *innerTypeInfo, SizeT size, uint32 alignment, uint32 inQualifiers
)
{
    static std::unordered_set<ReflectTypeInfo> dbTypeInfo;

    std::pair<std::unordered_set<ReflectTypeInfo>::iterator, bool> insertResult
        = dbTypeInfo.insert(ReflectTypeInfo{ cleanTypeInfo, innerTypeInfo, size, alignment, inQualifiers });
    return &(*insertResult.first);
}

IReflectionRuntimeModule *IReflectionRuntimeModule::get()
{
    static WeakModulePtr appModule = ModuleManager::get()->getOrLoadModule(TCHAR("ReflectionRuntime"));
    if (appModule.expired())
    {
        return nullptr;
    }
    return static_cast<IReflectionRuntimeModule *>(appModule.lock().get());
}

void IReflectionRuntimeModule::registerClassFactory(
    const StringID &className, const ReflectTypeInfo *classTypeInfo, const ClassPropertyFactoryCell &factoryCell
)
{
    auto factoryFromTypeInfoItr = ReflectionRuntimeModule::classFactoryFromTypeInfo().find(classTypeInfo);
    auto factoryFromTypeNameItr = ReflectionRuntimeModule::classFactoryFromTypeName().find(className);
    // Make sure that both factory function registries are in sync
    debugAssert(
        (factoryFromTypeInfoItr == ReflectionRuntimeModule::classFactoryFromTypeInfo().end())
        == (factoryFromTypeNameItr == ReflectionRuntimeModule::classFactoryFromTypeName().end())
    );

    if (factoryFromTypeInfoItr == ReflectionRuntimeModule::classFactoryFromTypeInfo().end())
    {
        ReflectionRuntimeModule::classFactoryFromTypeInfo()[classTypeInfo] = { className, factoryCell };
        ReflectionRuntimeModule::classFactoryFromTypeName()[className] = { classTypeInfo, factoryCell };
    }
}

void IReflectionRuntimeModule::registerStructFactory(
    const StringID &structName, const ReflectTypeInfo *structTypeInfo, const ClassPropertyFactoryCell &factoryCell
)
{
    auto factoryFromTypeInfoItr = ReflectionRuntimeModule::structFactoryFromTypeInfo().find(structTypeInfo);
    auto factoryFromTypeNameItr = ReflectionRuntimeModule::structFactoryFromTypeName().find(structName);
    // Make sure that both factory function registries are in sync
    debugAssert(
        (factoryFromTypeInfoItr == ReflectionRuntimeModule::structFactoryFromTypeInfo().end())
        == (factoryFromTypeNameItr == ReflectionRuntimeModule::structFactoryFromTypeName().end())
    );

    if (factoryFromTypeInfoItr == ReflectionRuntimeModule::structFactoryFromTypeInfo().end())
    {
        ReflectionRuntimeModule::structFactoryFromTypeInfo()[structTypeInfo] = { structName, factoryCell };
        ReflectionRuntimeModule::structFactoryFromTypeName()[structName] = { structTypeInfo, factoryCell };
    }
}

void IReflectionRuntimeModule::registerEnumFactory(
    const StringID &enumName, const ReflectTypeInfo *enumTypeInfo, const EnumPropertyFactoryCell &factoryCell
)
{
    auto factoryFromTypeInfoItr = ReflectionRuntimeModule::enumFactoryFromTypeInfo().find(enumTypeInfo);
    auto factoryFromTypeNameItr = ReflectionRuntimeModule::enumFactoryFromTypeName().find(enumName);
    // Make sure that both factory function registries are in sync
    debugAssert(
        (factoryFromTypeInfoItr == ReflectionRuntimeModule::enumFactoryFromTypeInfo().end())
        == (factoryFromTypeNameItr == ReflectionRuntimeModule::enumFactoryFromTypeName().end())
    );

    if (factoryFromTypeInfoItr == ReflectionRuntimeModule::enumFactoryFromTypeInfo().end())
    {
        ReflectionRuntimeModule::enumFactoryFromTypeInfo()[enumTypeInfo] = { enumName, factoryCell };
        ReflectionRuntimeModule::enumFactoryFromTypeName()[enumName] = { enumTypeInfo, factoryCell };
    }
}

void IReflectionRuntimeModule::registerTypeFactory(const ReflectTypeInfo *typeInfo, const TypedPropertyFactoryCell &factoryCell)
{
    auto factoryFromTypeInfoItr = ReflectionRuntimeModule::otherTypesFactories().find(typeInfo);

    if (factoryFromTypeInfoItr == ReflectionRuntimeModule::otherTypesFactories().end())
    {
        ReflectionRuntimeModule::otherTypesFactories()[typeInfo] = factoryCell;
    }
}

std::unordered_map<const ReflectTypeInfo *, std::pair<StringID, ClassPropertyFactoryCell>> &ReflectionRuntimeModule::classFactoryFromTypeInfo()
{
    static std::unordered_map<const ReflectTypeInfo *, std::pair<StringID, ClassPropertyFactoryCell>> singletonFactoriesRegistry;
    return singletonFactoriesRegistry;
}
std::unordered_map<StringID, std::pair<const ReflectTypeInfo *, ClassPropertyFactoryCell>> &ReflectionRuntimeModule::classFactoryFromTypeName()
{
    static std::unordered_map<StringID, std::pair<const ReflectTypeInfo *, ClassPropertyFactoryCell>> singletonFactoriesRegistry;
    return singletonFactoriesRegistry;
}
const ClassProperty *ReflectionRuntimeModule::createClassProperty(const ReflectTypeInfo *typeInfo)
{
    auto propertyCreateFactoryItr = classFactoryFromTypeInfo().find(typeInfo);
    if (propertyCreateFactoryItr != classFactoryFromTypeInfo().end() && propertyCreateFactoryItr->second.second.factoryFunc)
    {
        ClassProperty *prop = propertyCreateFactoryItr->second.second.factoryFunc();

        // add without parent first, to allow initialization
        ClassTreeType::NodeIdx classIdx = dbClasses.add(prop);
        debugAssert(classIdx != ClassTreeType::InvalidIdx);
        // Set the property in db for other property access immediately
        dbClassTypes[typeInfo] = classIdx;
        dbClassTypesFromName[propertyCreateFactoryItr->second.first] = classIdx;
        // Initialize the property now
        if (propertyCreateFactoryItr->second.second.initFunc)
        {
            propertyCreateFactoryItr->second.second.initFunc(prop);
        }
        // Now setup the parent
        if (prop->baseClass)
        {
            auto parentIdxItr = dbClassTypes.find(prop->baseClass->typeInfo);
            debugAssert(parentIdxItr != dbClassTypes.end());
            dbClasses.relinkTo(classIdx, parentIdxItr->second);
        }

        // Now clear the registered factories
        classFactoryFromTypeName().erase(propertyCreateFactoryItr->second.first);
        classFactoryFromTypeInfo().erase(propertyCreateFactoryItr);
        return prop;
    }
    else
    {
        LOG_ERROR("ReflectionRuntimeModule", "%s() : Creating class property failed for type %s", __func__, *typeInfo);
    }
    return nullptr;
}
const ClassProperty *ReflectionRuntimeModule::createClassProperty(const StringID &typeName)
{
    auto propertyCreateFactoryItr = classFactoryFromTypeName().find(typeName);
    if (propertyCreateFactoryItr != classFactoryFromTypeName().end() && propertyCreateFactoryItr->second.second.factoryFunc)
    {
        ClassProperty *prop = propertyCreateFactoryItr->second.second.factoryFunc();

        ClassTreeType::NodeIdx classIdx = dbClasses.add(prop);
        debugAssert(classIdx != ClassTreeType::InvalidIdx);
        dbClassTypes[propertyCreateFactoryItr->second.first] = classIdx;
        dbClassTypesFromName[typeName] = classIdx;

        if (propertyCreateFactoryItr->second.second.initFunc)
        {
            propertyCreateFactoryItr->second.second.initFunc(prop);
        }

        // Now setup the parent
        if (prop->baseClass)
        {
            auto parentIdxItr = dbClassTypes.find(prop->baseClass->typeInfo);
            debugAssert(parentIdxItr != dbClassTypes.end());
            dbClasses.relinkTo(classIdx, parentIdxItr->second);
        }

        classFactoryFromTypeInfo().erase(propertyCreateFactoryItr->second.first);
        classFactoryFromTypeName().erase(propertyCreateFactoryItr);
        return prop;
    }
    else
    {
        LOG_ERROR("ReflectionRuntimeModule", "%s() : Creating class property failed for type %s", __func__, typeName);
    }
    return nullptr;
}

FORCE_INLINE void ReflectionRuntimeModule::createAllPendingClasses()
{
    std::vector<const ReflectTypeInfo *> typeInfos;
    typeInfos.reserve(classFactoryFromTypeInfo().size());
    for (const std::pair<ReflectTypeInfo const *const, std::pair<StringID, ClassPropertyFactoryCell>> &registeredClass :
         classFactoryFromTypeInfo())
    {
        typeInfos.emplace_back(registeredClass.first);
    }
    for (const ReflectTypeInfo *typeInfo : typeInfos)
    {
        const ClassProperty *classProp = getClassType(typeInfo);
        alertIf(classProp, "Failed creating class property for type info %s", *typeInfo);
    }
}

std::unordered_map<const ReflectTypeInfo *, std::pair<StringID, ClassPropertyFactoryCell>> &ReflectionRuntimeModule::structFactoryFromTypeInfo()
{
    static std::unordered_map<const ReflectTypeInfo *, std::pair<StringID, ClassPropertyFactoryCell>> singletonFactoriesRegistry;
    return singletonFactoriesRegistry;
}
std::unordered_map<StringID, std::pair<const ReflectTypeInfo *, ClassPropertyFactoryCell>> &ReflectionRuntimeModule::structFactoryFromTypeName()
{
    static std::unordered_map<StringID, std::pair<const ReflectTypeInfo *, ClassPropertyFactoryCell>> singletonFactoriesRegistry;
    return singletonFactoriesRegistry;
}
const ClassProperty *ReflectionRuntimeModule::createStructProperty(const ReflectTypeInfo *typeInfo)
{
    auto propertyCreateFactoryItr = structFactoryFromTypeInfo().find(typeInfo);
    if (propertyCreateFactoryItr != structFactoryFromTypeInfo().end() && propertyCreateFactoryItr->second.second.factoryFunc)
    {
        ClassProperty *prop = propertyCreateFactoryItr->second.second.factoryFunc();

        dbStructTypes[typeInfo] = prop;
        dbStructTypesFromName[propertyCreateFactoryItr->second.first] = prop;

        if (propertyCreateFactoryItr->second.second.initFunc)
        {
            propertyCreateFactoryItr->second.second.initFunc(prop);
        }

        structFactoryFromTypeName().erase(propertyCreateFactoryItr->second.first);
        structFactoryFromTypeInfo().erase(propertyCreateFactoryItr);
        return prop;
    }
    else
    {
        LOG_ERROR("ReflectionRuntimeModule", "%s() : Creating struct property failed for type %s", __func__, *typeInfo);
    }
    return nullptr;
}
const ClassProperty *ReflectionRuntimeModule::createStructProperty(const StringID &typeName)
{
    auto propertyCreateFactoryItr = structFactoryFromTypeName().find(typeName);
    if (propertyCreateFactoryItr != structFactoryFromTypeName().end() && propertyCreateFactoryItr->second.second.factoryFunc)
    {
        ClassProperty *prop = propertyCreateFactoryItr->second.second.factoryFunc();

        dbStructTypes[propertyCreateFactoryItr->second.first] = prop;
        dbStructTypesFromName[typeName] = prop;

        if (propertyCreateFactoryItr->second.second.initFunc)
        {
            propertyCreateFactoryItr->second.second.initFunc(prop);
        }

        structFactoryFromTypeInfo().erase(propertyCreateFactoryItr->second.first);
        structFactoryFromTypeName().erase(propertyCreateFactoryItr);
        return prop;
    }
    else
    {
        LOG_ERROR("ReflectionRuntimeModule", "%s() : Creating struct property failed for type %s", __func__, typeName);
    }
    return nullptr;
}

std::unordered_map<const ReflectTypeInfo *, std::pair<StringID, EnumPropertyFactoryCell>> &ReflectionRuntimeModule::enumFactoryFromTypeInfo()
{
    static std::unordered_map<const ReflectTypeInfo *, std::pair<StringID, EnumPropertyFactoryCell>> singletonFactoriesRegistry;
    return singletonFactoriesRegistry;
}
std::unordered_map<StringID, std::pair<const ReflectTypeInfo *, EnumPropertyFactoryCell>> &ReflectionRuntimeModule::enumFactoryFromTypeName()
{
    static std::unordered_map<StringID, std::pair<const ReflectTypeInfo *, EnumPropertyFactoryCell>> singletonFactoriesRegistry;
    return singletonFactoriesRegistry;
}
const EnumProperty *ReflectionRuntimeModule::createEnumProperty(const ReflectTypeInfo *typeInfo)
{
    auto propertyCreateFactoryItr = enumFactoryFromTypeInfo().find(typeInfo);
    if (propertyCreateFactoryItr != enumFactoryFromTypeInfo().end() && propertyCreateFactoryItr->second.second.factoryFunc)
    {
        EnumProperty *prop = propertyCreateFactoryItr->second.second.factoryFunc();

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
        LOG_ERROR("ReflectionRuntimeModule", "%s() : Creating enum property failed for type %s", __func__, *typeInfo);
    }
    return nullptr;
}
const EnumProperty *ReflectionRuntimeModule::createEnumProperty(const StringID &typeName)
{
    auto propertyCreateFactoryItr = enumFactoryFromTypeName().find(typeName);
    if (propertyCreateFactoryItr != enumFactoryFromTypeName().end() && propertyCreateFactoryItr->second.second.factoryFunc)
    {
        EnumProperty *prop = propertyCreateFactoryItr->second.second.factoryFunc();

        dbEnumTypes[propertyCreateFactoryItr->second.first] = prop;
        dbEnumTypesFromName[typeName] = prop;

        if (propertyCreateFactoryItr->second.second.initFunc)
        {
            propertyCreateFactoryItr->second.second.initFunc(prop);
        }

        enumFactoryFromTypeInfo().erase(propertyCreateFactoryItr->second.first);
        enumFactoryFromTypeName().erase(propertyCreateFactoryItr);
        return prop;
    }
    else
    {
        LOG_ERROR("ReflectionRuntimeModule", "%s() : Creating enum property failed for type %s", __func__, typeName);
    }
    return nullptr;
}

std::unordered_map<const ReflectTypeInfo *, TypedPropertyFactoryCell> &ReflectionRuntimeModule::otherTypesFactories()
{
    static std::unordered_map<const ReflectTypeInfo *, TypedPropertyFactoryCell> singletonFactoriesRegistry;
    return singletonFactoriesRegistry;
}
const BaseProperty *ReflectionRuntimeModule::createTypedProperty(const ReflectTypeInfo *typeInfo)
{
    auto propertyCreateFactoryItr = otherTypesFactories().find(typeInfo);
    if (propertyCreateFactoryItr != otherTypesFactories().end() && propertyCreateFactoryItr->second.factoryFunc)
    {
        BaseProperty *typeProperty = propertyCreateFactoryItr->second.factoryFunc();
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
        LOG_ERROR("ReflectionRuntimeModule", "%s() : Creating typed property failed for type %s", __func__, *typeInfo);
    }
    return nullptr;
}

const ClassProperty *ReflectionRuntimeModule::getStructType(const ReflectTypeInfo *typeInfo)
{
    const ClassProperty *retVal = nullptr;
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
const ClassProperty *ReflectionRuntimeModule::getStructType(const StringID &structName)
{
    const ClassProperty *retVal = nullptr;
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

void ReflectionRuntimeModule::getChildsOf(
    const ClassProperty *clazz, std::vector<const ClassProperty *> &outChilds, bool bRecursively /*= false*/, bool bOnlyLeafChilds /*= false */
)
{
    if (!clazz)
    {
        return;
    }

    // Create any pending classes registered but not created to get full class tree
    createAllPendingClasses();

    auto clazzIdxItr = dbClassTypes.find(clazz->typeInfo);
    debugAssert(clazzIdxItr != dbClassTypes.end());

    std::vector<ClassTreeType::NodeIdx> classChildren;
    if (bOnlyLeafChilds)
    {
        for (ClassTreeType::NodeIdx childIdx : dbClasses.getChildren(clazzIdxItr->second, bRecursively))
        {
            if (!dbClasses.hasChild(childIdx))
            {
                classChildren.emplace_back(childIdx);
            }
        }
    }
    else
    {
        dbClasses.getChildren(classChildren, clazzIdxItr->second, bRecursively);
    }

    outChilds.reserve(classChildren.size());
    for (ClassTreeType::NodeIdx childIdx : classChildren)
    {
        outChilds.emplace_back(dbClasses[childIdx]);
    }
    // LOG("Test", "Class trees : %s", dbClasses);
}

const ClassProperty *ReflectionRuntimeModule::getClassType(const ReflectTypeInfo *typeInfo)
{
    const ClassProperty *retVal = nullptr;
    auto typePropertyItr = dbClassTypes.find(typeInfo);
    if (typePropertyItr == dbClassTypes.end())
    {
        retVal = createClassProperty(typeInfo);
    }
    else
    {
        retVal = dbClasses[typePropertyItr->second];
    }
    return retVal;
}
const ClassProperty *ReflectionRuntimeModule::getClassType(const StringID &className)
{
    const ClassProperty *retVal = nullptr;
    auto typePropertyItr = dbClassTypesFromName.find(className);
    if (typePropertyItr == dbClassTypesFromName.end())
    {
        retVal = createClassProperty(className);
    }
    else
    {
        retVal = dbClasses[typePropertyItr->second];
    }
    return retVal;
}

const EnumProperty *ReflectionRuntimeModule::getEnumType(const ReflectTypeInfo *typeInfo)
{
    const EnumProperty *retVal = nullptr;
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
const EnumProperty *ReflectionRuntimeModule::getEnumType(const StringID &enumName)
{
    const EnumProperty *retVal = nullptr;
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

const BaseProperty *ReflectionRuntimeModule::getType(const ReflectTypeInfo *typeInfo)
{
    SCOPED_MUTE_LOG_SEVERITIES(Logger::AllServerity);
    const BaseProperty *retVal = nullptr;
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

void ReflectionRuntimeModule::setMetaData(
    const BaseProperty *forProperty, const std::vector<const PropertyMetaDataBase *> &propertyMeta, uint64 propertyMetaFlags
)
{
    std::vector<std::pair<PropertyMetaDataKey, const PropertyMetaDataBase *>> initializerList;
    initializerList.reserve(propertyMeta.size());
    for (const PropertyMetaDataBase *metaData : propertyMeta)
    {
        initializerList.push_back({
            {forProperty, metaData->metaType()},
            metaData
        });
    }
    if (!initializerList.empty())
    {
        propertiesMetaData.insert(initializerList.cbegin(), initializerList.cend());
    }
    if (propertyMetaFlags != 0)
    {
        propertiesMetaFlags.insert({ forProperty, propertyMetaFlags });
    }
}

std::vector<const PropertyMetaDataBase *> ReflectionRuntimeModule::getPropertyMetaData(const BaseProperty *prop) const
{
    std::vector<const PropertyMetaDataBase *> retVal;
    for (const auto &metaData : propertiesMetaData)
    {
        if (metaData.first.first == prop)
        {
            retVal.emplace_back(metaData.second);
        }
    }
    return retVal;
}

const PropertyMetaDataBase *ReflectionRuntimeModule::getPropertyMetaData(const BaseProperty *prop, const ReflectTypeInfo *typeInfo) const
{
    auto itr = propertiesMetaData.find({ prop, typeInfo });
    if (itr != propertiesMetaData.cend())
    {
        return itr->second;
    }
    return nullptr;
}

uint64 ReflectionRuntimeModule::getPropertyMetaFlags(const BaseProperty *prop) const
{
    auto itr = propertiesMetaFlags.find(prop);
    if (itr != propertiesMetaFlags.cend())
    {
        return itr->second;
    }
    return 0;
}

void ReflectionRuntimeModule::init() { initCommonProperties(); }

void ReflectionRuntimeModule::release()
{
    for (const auto &typePropertyIdx : dbClasses.getAll())
    {
        delete dbClasses[typePropertyIdx];
    }
    for (const auto &typeProperty : dbStructTypes)
    {
        delete typeProperty.second;
    }
    for (const auto &typeProperty : dbEnumTypes)
    {
        delete typeProperty.second;
    }
    for (const auto &typeProperty : dbOtherTypes)
    {
        delete typeProperty.second;
    }
    dbClasses.clear();
    dbClassTypes.clear();
    dbClassTypesFromName.clear();
    dbStructTypes.clear();
    dbStructTypesFromName.clear();
    dbEnumTypes.clear();
    dbEnumTypesFromName.clear();
    dbOtherTypes.clear();

    // Clear all meta data
    for (const auto &propertyMetaData : propertiesMetaData)
    {
        delete propertyMetaData.second;
    }
    propertiesMetaFlags.clear();
    propertiesMetaData.clear();
}