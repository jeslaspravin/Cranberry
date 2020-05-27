#include "ModuleManager.h"
#include "PlatformFunctions.h"
#include "../Logger/Logger.h"

ModuleManager::ModuleManager():loadedModules()
{
    void* procHandle = PlatformFunctions::getCurrentProcessHandle();
    uint32 modulesSize;
    PlatformFunctions::getAllModules(procHandle, nullptr, modulesSize);
    std::vector<LibPointerPtr> libptrs(modulesSize);
    PlatformFunctions::getAllModules(procHandle, libptrs.data(), modulesSize);
    libptrs.resize(modulesSize);

    for (LibPointerPtr libPtr : libptrs)
    {
        ModuleData data;
        PlatformFunctions::getModuleInfo(procHandle, libPtr, data);
        Logger::debug("ModuleManager", "%s() : System loaded module name : %s, Image : %s, Module size : %d", __func__,
            data.name.getChar(), data.imgName.getChar(), data.moduleSize);
        loadedModules[data.name].first = libPtr;
        loadedModules[data.name].second = data;
    }
}

ModuleManager::~ModuleManager()
{
    for (const std::pair<String, std::pair<LibPointerPtr, ModuleData>>& modulePair : loadedModules) {
        delete modulePair.second.first;
        Logger::debug("ModuleManager", "%s() : Unloaded module %s", __func__, modulePair.first.getChar());
    }
    loadedModules.clear();
}

ModuleManager* ModuleManager::get()
{
    static ModuleManager singletonManager;
    return &singletonManager;
}

bool ModuleManager::isLoaded(String moduleName)
{
    return loadedModules.find(moduleName) != loadedModules.end();
}

LibPointer* ModuleManager::getModule(String moduleName)
{
    if (!isLoaded(moduleName))
    {
        return nullptr;
    }
    return loadedModules[moduleName].first;
}

LibPointer* ModuleManager::getOrLoadModule(String moduleName)
{
    if (!isLoaded(moduleName))
    {        
        LibPointer* library = PlatformFunctions::openLibrary(moduleName);
        if (library)
        {    
            Logger::debug("ModuleManager", "%s() : Loaded module %s", __func__, moduleName.getChar());
            loadedModules[moduleName].first = library;
            PlatformFunctions::getModuleInfo(PlatformFunctions::getCurrentProcessHandle(), loadedModules[moduleName].first,
                loadedModules[moduleName].second);
            return library;
        }
        return nullptr;
    }

    return getModule(moduleName);
}

std::vector<std::pair<LibPointerPtr, ModuleData>> ModuleManager::getAllModuleData()
{
    // Had to do every time because that loading can happen anytime in program
    void* procHandle = PlatformFunctions::getCurrentProcessHandle();
    uint32 modulesSize;
    PlatformFunctions::getAllModules(procHandle, nullptr, modulesSize);
    std::vector<LibPointerPtr> libptrs(modulesSize);
    PlatformFunctions::getAllModules(procHandle, libptrs.data(), modulesSize);
    libptrs.resize(modulesSize);

    for (LibPointerPtr libPtr : libptrs)
    {
        ModuleData data;
        PlatformFunctions::getModuleInfo(procHandle, libPtr, data);

        if(loadedModules.find(data.name) != loadedModules.end())
            continue;

        Logger::debug("ModuleManager", "%s() : System loaded module name : %s, Image : %s, Module size : %d", __func__,
            data.name.getChar(), data.imgName.getChar(), data.moduleSize);
        loadedModules[data.name].first = libPtr;
        loadedModules[data.name].second = data;
    }

    std::vector<std::pair<LibPointerPtr, ModuleData>> libraries;
    libraries.reserve(loadedModules.size());
    for (const std::pair<String, std::pair<LibPointerPtr, ModuleData>>& pairs : loadedModules)
    {
        libraries.push_back(pairs.second);
    }
    return libraries;
}
