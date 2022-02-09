/*!
 * \file ModuleManager.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Modules/ModuleManager.h"
#include "Types/Platform/PlatformFunctions.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Logger/Logger.h"

#include <filesystem>

StaticModuleInitializerRegistrant::StaticModuleInitializerRegistrant(String moduleName, SingleCastDelegate<IModuleBase*> functionPtr)
{
    ModuleManager::getModuleInitializerList()[moduleName] = functionPtr;
}

StaticModuleInitializerList& ModuleManager::getModuleInitializerList()
{
    static StaticModuleInitializerList singletonInitializerList;
    return singletonInitializerList;
}

LibPointer* ModuleManager::loadFromAdditionalPaths(String modulePath) const
{
    std::filesystem::path moduleFullPath(modulePath.getChar());
    // If is relative path then it is okay to append it to available paths and do load checks
    if (moduleFullPath.is_absolute())
    {
        return nullptr;
    }

    // append prefix name if not present
    if (!String(LIB_PREFIX).empty() && !String(moduleFullPath.filename().c_str()).startsWith(LIB_PREFIX, true))
    {
        moduleFullPath.replace_filename(LIB_PREFIX + String(moduleFullPath.filename().c_str()));
    }
    // Add extensions
    if (!moduleFullPath.has_extension())
    {
        moduleFullPath.replace_extension(SHARED_LIB_EXTENSION);
    }

    String relativeModulePath(moduleFullPath.c_str());
    for (const String& lookAtPath : additionalLibraryPaths)
    {
        if (LibPointer* library = PlatformFunctions::openLibrary(PathFunctions::combinePath(lookAtPath, relativeModulePath)))
        {
            return library;
        }
        else
        {
            LOG_WARN("ModuleManager", "%s() : Searched for %s library at %s", __func__, modulePath, lookAtPath);
        }
    }
    return nullptr;
}

ModuleManager::ModuleManager()
    : loadedLibraries()
    , additionalLibraryPaths{ TCHAR(ENGINE_MODULES_PATH) }
{
    void* procHandle = PlatformFunctions::getCurrentProcessHandle();
    uint32 modulesSize;
    PlatformFunctions::getAllModules(procHandle, nullptr, modulesSize);
    std::vector<LibPointerPtr> libptrs(modulesSize);
    PlatformFunctions::getAllModules(procHandle, libptrs.data(), modulesSize);
    libptrs.resize(modulesSize);

    for (LibPointerPtr libPtr : libptrs)
    {
        LibraryData data;
        PlatformFunctions::getModuleInfo(procHandle, libPtr, data);
        LOG_DEBUG("ModuleManager", "%s() : System loaded module name : %s, Image : %s, Module size : %d", __func__,
            data.name.getChar(), data.imgName.getChar(), data.moduleSize);
        loadedLibraries[data.name].first = libPtr;
        loadedLibraries[data.name].second = data;
    }
}

ModuleManager::~ModuleManager()
{
    for (const std::pair<const String, ModulePtr>& modulePair : loadedModuleInterfaces)
    {
        modulePair.second->release();
        LOG_DEBUG("ModuleManager", "%s() : Unloaded module %s", __func__, modulePair.first);
    }
    loadedModuleInterfaces.clear();

    for (const std::pair<const String, std::pair<LibPointerPtr, LibraryData>>& libPair : loadedLibraries) 
    {
        delete libPair.second.first;
        LOG_DEBUG("ModuleManager", "%s() : Unloaded library %s", __func__, libPair.first);
    }
    loadedLibraries.clear();
}

ModuleManager* ModuleManager::get()
{
    static ModuleManager singletonManager;
    return &singletonManager;
}

bool ModuleManager::isLibraryLoaded(String moduleName) const
{
    return loadedLibraries.find(moduleName) != loadedLibraries.end();
}

LibPointer* ModuleManager::getLibrary(String moduleName) const
{
    auto libItr = loadedLibraries.find(moduleName);
    if (libItr == loadedLibraries.cend())
    {
        return nullptr;
    }
    return libItr->second.first;
}

LibPointer* ModuleManager::getOrLoadLibrary(String modulePath)
{
    // Remove path and extension info, If any
    std::filesystem::path modulePathName(modulePath.getChar());
    modulePathName.replace_extension();
    String moduleName{ modulePathName.filename().c_str() };

    if (!isLibraryLoaded(moduleName))
    {        
        LibPointer* library = PlatformFunctions::openLibrary(modulePath);
        if (library == nullptr)
        {
            // Pass in sent path to derive abs paths from relative if any
            library = loadFromAdditionalPaths(modulePath);
        }
        if (library)
        {    
            LOG_DEBUG("ModuleManager", "%s() : Loaded Library %s from %s", __func__, moduleName, modulePath);

            loadedLibraries[moduleName].first = library;
            PlatformFunctions::getModuleInfo(PlatformFunctions::getCurrentProcessHandle(), loadedLibraries[moduleName].first,
                loadedLibraries[moduleName].second);
            return library;
        }
        return nullptr;
    }

    return getLibrary(moduleName);
}

bool ModuleManager::isModuleLoaded(String moduleName) const
{
    return loadedModuleInterfaces.find(moduleName) != loadedModuleInterfaces.cend();
}

WeakModulePtr ModuleManager::getModule(String moduleName) const
{
    auto moduleItr = loadedModuleInterfaces.find(moduleName);
    if (moduleItr == loadedModuleInterfaces.cend())
    {
        return WeakModulePtr();
    }
    return moduleItr->second;
}

bool ModuleManager::loadModule(String moduleName)
{
    return !getOrLoadModule(moduleName).expired();
}

WeakModulePtr ModuleManager::getOrLoadModule(String moduleName)
{
    WeakModulePtr existingModule = getModule(moduleName);
    ModulePtr retModule = existingModule.expired() ? nullptr : existingModule.lock();

    if (!bool(retModule))
    {
        LOG("ModuleManager", "%s() : Loading module %s", __func__, moduleName);

        auto staticInitializerItr = getModuleInitializerList().find(moduleName);
        if (staticInitializerItr != getModuleInitializerList().end())
        {
            fatalAssert(staticInitializerItr->second.isBound(), "%s() : Static initializer must be bound", __func__);
            retModule = ModulePtr(staticInitializerItr->second.invoke());
        }
#if STATIC_LINKED
        else
        {
            fatalAssert(!"Module initializer not found", "%s() : Module initializer not found", __func__);
            return existingModule;
        }
#else // STATIC_LINKED
        else
        {
            // Not specifying extension as they are auto appended by api to platform default
            LibPointerPtr libPtr = getOrLoadLibrary(moduleName);
            // Check other paths for modules
            fatalAssert(libPtr, "%s() : Failed loading module %s", __func__, moduleName);

            Function<IModuleBase*> createFuncPtr(
                (Function<IModuleBase*>::StaticDelegate)PlatformFunctions::getProcAddress(libPtr, TCHAR("createModule_") + moduleName));
            fatalAssert(createFuncPtr, "%s() : Failed find module create function for module %s", __func__, moduleName);

            retModule = ModulePtr(createFuncPtr());
        }
#endif // STATIC_LINKED
        fatalAssert(retModule, "%s() : Failed loading module interface %s", __func__, moduleName);

        retModule->init();
        loadedModuleInterfaces[moduleName] = retModule;
    }
    return retModule;
}

void ModuleManager::unloadModule(String moduleName)
{
    WeakModulePtr existingModule = getModule(moduleName);
    if (!existingModule.expired())
    {
        IModuleBase* moduleInterface = existingModule.lock().get();
        moduleInterface->release();
        loadedModuleInterfaces.erase(moduleName);
        LOG_DEBUG("ModuleManager", "%s() : Unloaded module %s", __func__, moduleName.getChar());
#if !STATIC_LINKED
        LibPointerPtr libPtr = getLibrary(moduleName);
        if (libPtr)
        {
            loadedLibraries.erase(moduleName);
            delete libPtr;
            libPtr = nullptr;
        }
#endif // STATIC_LINKED
    }
}

std::vector<std::pair<LibPointerPtr, LibraryData>> ModuleManager::getAllModuleData()
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
        LibraryData data;
        PlatformFunctions::getModuleInfo(procHandle, libPtr, data);

        if(loadedLibraries.find(data.name) != loadedLibraries.end())
            continue;

        LOG_DEBUG("ModuleManager", "%s() : System loaded module name : %s, Image : %s, Module size : %d", __func__,
            data.name.getChar(), data.imgName.getChar(), data.moduleSize);
        loadedLibraries[data.name].first = libPtr;
        loadedLibraries[data.name].second = data;
    }

    std::vector<std::pair<LibPointerPtr, LibraryData>> libraries;
    libraries.reserve(loadedLibraries.size());
    for (const std::pair<const String, std::pair<LibPointerPtr, LibraryData>>& pairs : loadedLibraries)
    {
        libraries.push_back(pairs.second);
    }
    return libraries;
}
