#include "ReflectionRuntimeModule.h"
#include "Modules/ModuleManager.h"

DECLARE_MODULE(ReflectionRuntime, ReflectionRuntimeModule)

IReflectionRuntimeModule* IReflectionRuntimeModule::get()
{
    static WeakModulePtr appModule = ModuleManager::get()->getOrLoadModule("ReflectionRuntime");
    if (appModule.expired())
    {
        return nullptr;
    }
    return static_cast<IReflectionRuntimeModule*>(appModule.lock().get());
}

void ReflectionRuntimeModule::init()
{
    
}

void ReflectionRuntimeModule::release()
{
    
}

const ClassProperty* ReflectionRuntimeModule::getStructType(const ReflectTypeInfo* typeInfo)
{
    return nullptr;
}
const ClassProperty* ReflectionRuntimeModule::getStructType(const String& structName)
{
    return nullptr;
}

const ClassProperty* ReflectionRuntimeModule::getClassType(const ReflectTypeInfo* typeInfo)
{
    return nullptr;
}
const ClassProperty* ReflectionRuntimeModule::getClassType(const String& className)
{
    return nullptr;
}

const EnumProperty* ReflectionRuntimeModule::getEnumType(const ReflectTypeInfo* typeInfo)
{
    return nullptr;
}
const EnumProperty* ReflectionRuntimeModule::getEnumType(const String& enumName)
{
    return nullptr;
}

const BaseProperty* ReflectionRuntimeModule::getType(const ReflectTypeInfo* typeInfo)
{
    return nullptr;
}
