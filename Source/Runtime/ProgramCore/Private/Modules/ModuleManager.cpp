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
#include "Logger/Logger.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/LFS/Paths.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"

#include <filesystem>

StaticModuleInitializerRegistrant::StaticModuleInitializerRegistrant(String moduleName, SingleCastDelegate<IModuleBase *> functionPtr)
{
    ModuleManager::getModuleInitializerList()[moduleName] = functionPtr;
}

StaticModuleInitializerList &ModuleManager::getModuleInitializerList()
{
    static StaticModuleInitializerList singletonInitializerList;
    return singletonInitializerList;
}

LibHandle ModuleManager::loadFromAdditionalPaths(const TChar *modulePath) const
{
    std::filesystem::path moduleFullPath(modulePath);
    // If is relative path then it is okay to append it to available paths and do load checks
    if (moduleFullPath.is_absolute())
    {
        return nullptr;
    }

    // append prefix name if not present
    if (!String(LIB_PREFIX).empty() && !String(WCHAR_TO_TCHAR(moduleFullPath.filename().c_str())).startsWith(LIB_PREFIX, true))
    {
        moduleFullPath.replace_filename(LIB_PREFIX + String(WCHAR_TO_TCHAR(moduleFullPath.filename().c_str())));
    }
    // Add extensions
    if (!moduleFullPath.has_extension())
    {
        moduleFullPath.replace_extension(SHARED_LIB_EXTENSION);
    }

    String relativeModulePath(WCHAR_TO_TCHAR(moduleFullPath.c_str()));
    for (const String &lookAtPath : additionalLibraryPaths)
    {
        if (LibHandle library = PlatformFunctions::openLibrary(PathFunctions::combinePath(lookAtPath, relativeModulePath).getChar()))
        {
            return library;
        }
        else
        {
            LOG_WARN("ModuleManager", "Searched for %s library at %s", modulePath, lookAtPath);
        }
    }
    return nullptr;
}

ModuleManager::ModuleManager()
    : loadedLibraries()
{
    // Since Tools, Editor exists in EngineRoot/Runtime/../[Tools|Editor] We can determine the EngineRoot and go into other library locations
    // from there
    additionalLibraryPaths.insert(additionalLibraryPaths.end(), { PathFunctions::combinePath(Paths::engineRoot(), TCHAR("Runtime")) });
    PlatformHandle procHandle = PlatformFunctions::getCurrentProcessHandle();
    uint32 modulesSize;
    PlatformFunctions::getAllModules(procHandle, nullptr, modulesSize);
    std::vector<LibHandle> libptrs(modulesSize);
    PlatformFunctions::getAllModules(procHandle, libptrs.data(), modulesSize);
    libptrs.resize(modulesSize);

    for (LibHandle libPtr : libptrs)
    {
        LibraryData data;
        PlatformFunctions::getModuleInfo(procHandle, libPtr, data);
        data.name = PathFunctions::stripExtension(data.name);
        LOG_DEBUG(
            "ModuleManager", "System loaded module name : %s, Image : %s, Module size : %d", data.name.getChar(), data.imgPath.getChar(),
            data.moduleSize
        );
        loadedLibraries[data.name].first = libPtr;
        loadedLibraries[data.name].second = data;
    }
}

ModuleManager::~ModuleManager() { unloadAll(); }

ModuleManager *ModuleManager::get()
{
    static ModuleManager singletonManager;
    return &singletonManager;
}

bool ModuleManager::isLibraryLoaded(const TChar *libNameOrPath) const { return loadedLibraries.find(libNameOrPath) != loadedLibraries.end(); }

LibHandle ModuleManager::getLibrary(const TChar *libNameOrPath) const
{
    auto libItr = loadedLibraries.find(libNameOrPath);
    if (libItr == loadedLibraries.cend())
    {
        return nullptr;
    }
    return libItr->second.first;
}

LibHandle ModuleManager::getOrLoadLibrary(const TChar *libNameOrPath)
{
    // Remove path and extension info, If any
    String moduleName{ PathFunctions::stripExtension(PathFunctions::fileOrDirectoryName(libNameOrPath)) };
    if (!isLibraryLoaded(moduleName.getChar()))
    {
        LibHandle library = PlatformFunctions::openLibrary(libNameOrPath);
        if (library == nullptr)
        {
            // Pass in sent path to derive abs paths from relative if any
            library = loadFromAdditionalPaths(libNameOrPath);
        }
        if (library)
        {
            LOG_DEBUG("ModuleManager", "Loaded Library %s from %s", moduleName, libNameOrPath);

            loadedLibraries[moduleName].first = library;
            PlatformFunctions::getModuleInfo(
                PlatformFunctions::getCurrentProcessHandle(), loadedLibraries[moduleName].first, loadedLibraries[moduleName].second
            );
            return library;
        }
        return nullptr;
    }

    return getLibrary(moduleName.getChar());
}

bool ModuleManager::unloadLibrary(const TChar *libName)
{
    if (isLibraryLoaded(libName))
    {
        LibHandle libPtr = getLibrary(libName);
        loadedLibraries.erase(libName);
        PlatformFunctions::releaseLibrary(libPtr);
        return true;
    }
    return false;
}

bool ModuleManager::isModuleLoaded(const TChar *moduleName) const
{
    return loadedModuleInterfaces.find(moduleName) != loadedModuleInterfaces.cend();
}

WeakModulePtr ModuleManager::getModule(const TChar *moduleName) const
{
    auto moduleItr = loadedModuleInterfaces.find(moduleName);
    if (moduleItr == loadedModuleInterfaces.cend())
    {
        return WeakModulePtr();
    }
    return moduleItr->second;
}

bool ModuleManager::loadModule(const TChar *moduleName) { return !getOrLoadModule(moduleName).expired(); }

WeakModulePtr ModuleManager::getOrLoadModule(const TChar *moduleName)
{
    WeakModulePtr existingModule = getModule(moduleName);
    ModulePtr retModule = existingModule.expired() ? nullptr : existingModule.lock();

    if (!bool(retModule))
    {
        LOG("ModuleManager", "Loading module %s", moduleName);

        auto staticInitializerItr = getModuleInitializerList().find(moduleName);
        if (staticInitializerItr != getModuleInitializerList().end())
        {
            fatalAssertf(staticInitializerItr->second.isBound(), "Static initializer must be bound");
            retModule = ModulePtr(staticInitializerItr->second.invoke());
        }
#if STATIC_LINKED
        else
        {
            fatalAssertf(!"Module initializer not found", "Module %s initializer not found", moduleName);
            return existingModule;
        }
#else  // STATIC_LINKED
        else
        {
            // Not specifying extension as they are auto appended by api to platform default
            LibHandle libPtr = getOrLoadLibrary(moduleName);
            // Check other paths for modules
            fatalAssertf(libPtr, "Failed loading module %s", moduleName);

            String moduleCreateFuncName = TCHAR("createModule_");
            moduleCreateFuncName.append(moduleName);
            Function<IModuleBase *> createFuncPtr(
                Function<IModuleBase *>::StaticDelegate(PlatformFunctions::getProcAddress(libPtr, moduleCreateFuncName.getChar()))
            );
            fatalAssertf(createFuncPtr, "Failed find module create function(%s) for module %s", moduleCreateFuncName, moduleName);

            retModule = ModulePtr(createFuncPtr());
        }
#endif // STATIC_LINKED
        fatalAssertf(retModule, "Failed loading module interface %s", moduleName);

        // Order and module must be set before calling init, this allows module's code to access module from ModuleManager inside init
        moduleLoadedOrder.emplace_back(moduleName);
        loadedModuleInterfaces[moduleName] = retModule;
        retModule->init();

        onModuleLoad.invoke(moduleName);
    }
    return retModule;
}

void ModuleManager::unloadModule(const TChar *moduleName)
{
    WeakModulePtr existingModule = getModule(moduleName);
    if (!existingModule.expired())
    {
        IModuleBase *moduleInterface = existingModule.lock().get();

        onModuleUnload.invoke(moduleName);
        moduleInterface->release();
        loadedModuleInterfaces.erase(moduleName);
        std::erase(moduleLoadedOrder, moduleName);
        LOG_DEBUG("ModuleManager", "Unloaded module %s", moduleName);
#if !STATIC_LINKED
        unloadLibrary(moduleName);
#endif // STATIC_LINKED
    }
}

void ModuleManager::unloadAll()
{
    // Make a local copy and erase moduleLoadedOrder
    std::vector<String> localModuleLoadOrder = std::move(moduleLoadedOrder);
    for (auto rItr = localModuleLoadOrder.crbegin(); rItr != localModuleLoadOrder.crend(); ++rItr)
    {
        auto moduleItr = loadedModuleInterfaces.find(*rItr);
        if (moduleItr == loadedModuleInterfaces.end())
        {
            LOG_DEBUG("ModuleManager", "Module %s is already unloaded in one of module that was unload", *rItr);
            continue;
        }

        onModuleUnload.invoke(*rItr);
        moduleItr->second->release();
        loadedModuleInterfaces.erase(moduleItr);
        LOG_DEBUG("ModuleManager", "Unloaded module %s", *rItr);
    }
    // Make a local copy and erase loadedModuleInterfaces
    LoadedModulesMap strandedModule = std::move(loadedModuleInterfaces);
    for (const std::pair<const String, ModulePtr> &modulePair : strandedModule)
    {
        onModuleUnload.invoke(modulePair.first);
        modulePair.second->release();
        LOG_DEBUG("ModuleManager", "Unloaded module %s", modulePair.first);
    }

#if !STATIC_LINKED
    // First unload all libraries in reverse of loaded order
    for (auto rItr = localModuleLoadOrder.crbegin(); rItr != localModuleLoadOrder.crend(); ++rItr)
    {
        unloadLibrary(rItr->getChar());
        LOG_DEBUG("ModuleManager", "Unloaded library %s", *rItr);
    }
#endif // STATIC_LINKED
    for (const std::pair<const String, std::pair<LibHandle, LibraryData>> &libPair : loadedLibraries)
    {
        PlatformFunctions::releaseLibrary(libPair.second.first);
        LOG_DEBUG("ModuleManager", "Unloaded library %s", libPair.first);
    }
    loadedLibraries.clear();
}

std::vector<std::pair<LibHandle, LibraryData>> ModuleManager::getAllModuleData()
{
    // Had to do every time because that loading can happen anytime in program
    PlatformHandle procHandle = PlatformFunctions::getCurrentProcessHandle();
    uint32 modulesCount;
    PlatformFunctions::getAllModules(procHandle, nullptr, modulesCount);
    std::vector<LibHandle> libptrs(modulesCount);
    PlatformFunctions::getAllModules(procHandle, libptrs.data(), modulesCount);
    libptrs.resize(modulesCount);

    for (LibHandle libPtr : libptrs)
    {
        LibraryData data;
        PlatformFunctions::getModuleInfo(procHandle, libPtr, data);
        data.name = PathFunctions::stripExtension(data.name);

        if (loadedLibraries.find(data.name) != loadedLibraries.end())
            continue;

        LOG_DEBUG(
            "ModuleManager", "System loaded module name : %s, Image : %s, Module size : %d", data.name.getChar(), data.imgPath.getChar(),
            data.moduleSize
        );
        loadedLibraries[data.name].first = libPtr;
        loadedLibraries[data.name].second = data;
    }

    std::vector<std::pair<LibHandle, LibraryData>> libraries;
    libraries.reserve(loadedLibraries.size());
    for (const std::pair<const String, std::pair<LibHandle, LibraryData>> &pairs : loadedLibraries)
    {
        libraries.push_back(pairs.second);
    }
    return libraries;
}
