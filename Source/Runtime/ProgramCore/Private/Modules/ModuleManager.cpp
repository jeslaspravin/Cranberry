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
            LOG_WARN("ModuleManager", "Searched for {} library at {}", modulePath, lookAtPath);
        }
    }
    return nullptr;
}

ModulePtr ModuleManager::tryLoadModule(const TChar *moduleName)
{
    ModulePtr retModule = nullptr;

    LOG("ModuleManager", "Loading module {}", moduleName);

    auto staticInitializerItr = getModuleInitializerList().find(moduleName);
    if (staticInitializerItr != getModuleInitializerList().end())
    {
        fatalAssertf(staticInitializerItr->second.isBound(), "Static initializer must be bound");
        retModule = ModulePtr(staticInitializerItr->second.invoke());
    }
#if STATIC_LINKED
    else
    {
        fatalAssertf(!"Module initializer not found", "Module {} initializer not found", moduleName);
        return existingModule;
    }
#else  // STATIC_LINKED
    else
    {
        // Not specifying extension as they are auto appended by api to platform default
        LibHandle libPtr = getOrLoadLibrary(moduleName);
        // Check other paths for modules
        fatalAssertf(libPtr, "Failed loading module {}", moduleName);

        String moduleCreateFuncName = TCHAR("createModule_");
        moduleCreateFuncName.append(moduleName);
        Function<IModuleBase *> createFuncPtr(
            Function<IModuleBase *>::StaticDelegate(PlatformFunctions::getProcAddress(libPtr, moduleCreateFuncName.getChar()))
        );
        fatalAssertf(createFuncPtr, "Failed find module create function({}) for module {}", moduleCreateFuncName, moduleName);

        retModule = ModulePtr(createFuncPtr());
    }
#endif // STATIC_LINKED
    if (retModule)
    {
        // Order and module must be set before calling init, this allows module's code to access module from ModuleManager inside init
        moduleLoadedOrder.emplace_back(moduleName);
        loadedModuleInterfaces[moduleName] = retModule;
        retModule->init();

        onModuleLoad.invoke(moduleName);
    }
    else
    {
        LOG_ERROR("ModuleManager", "Failed loading module interface {}", moduleName);
    }
    return retModule;
}

DEBUG_INLINE bool ModuleManager::tryUnloadModule(const TChar *moduleName)
{
    WeakModulePtr existingModule = getModule(moduleName);
    if (!existingModule.expired())
    {
        IModuleBase *moduleInterface = existingModule.lock().get();

        onModuleUnload.invoke(moduleName);
        moduleInterface->release();
        loadedModuleInterfaces.erase(moduleName);
        std::erase(moduleLoadedOrder, moduleName);
        LOG_DEBUG("ModuleManager", "Unloaded module {}", moduleName);
        return true;
    }
    return false;
}

ModuleManager::ModuleManager()
    : loadedLibraries()
{
    additionalLibraryPaths.insert(additionalLibraryPaths.end(), { Paths::engineRuntimeRoot() });
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
            "ModuleManager", "System loaded module name : {}, Image : {}, Module size : {}", data.name.getChar(), data.imgPath.getChar(),
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

void ModuleManager::addAdditionalLibPath(const TChar *dir)
{
    String libDir = dir;
    std::filesystem::path dirPath(dir);
    if (dirPath.is_relative())
    {
        libDir = PathFunctions::combinePath(Paths::engineRoot(), dir);
    }

    auto itr = std::find(additionalLibraryPaths.cbegin(), additionalLibraryPaths.cend(), libDir);
    if (itr == additionalLibraryPaths.cend())
    {
        additionalLibraryPaths.emplace_back(std::move(libDir));
    }
}

void ModuleManager::removedAdditionalLibPath(const TChar *dir)
{
    String libDir = dir;
    std::filesystem::path dirPath(dir);
    if (dirPath.is_relative())
    {
        libDir = PathFunctions::combinePath(Paths::engineRoot(), dir);
    }
    auto itr = std::find(additionalLibraryPaths.cbegin(), additionalLibraryPaths.cend(), libDir);
    if (itr != additionalLibraryPaths.cend())
    {
        additionalLibraryPaths.erase(itr);
    }
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
            LOG_DEBUG("ModuleManager", "Loaded Library {} from {}", moduleName, libNameOrPath);

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

IModuleBase *ModuleManager::getModulePtr(const TChar *moduleName) const
{
    auto moduleItr = loadedModuleInterfaces.find(moduleName);
    if (moduleItr == loadedModuleInterfaces.cend())
    {
        return nullptr;
    }
    return moduleItr->second.get();
}

bool ModuleManager::loadModule(const TChar *moduleName) { return getOrLoadModulePtr(moduleName); }

WeakModulePtr ModuleManager::getOrLoadModule(const TChar *moduleName)
{
    WeakModulePtr existingModule = getModule(moduleName);
    ModulePtr retModule = existingModule.expired() ? nullptr : existingModule.lock();

    if (!bool(retModule))
    {
        retModule = tryLoadModule(moduleName);
        fatalAssertf(retModule, "Failed loading module interface {}", moduleName);
    }
    return retModule;
}

IModuleBase *ModuleManager::getOrLoadModulePtr(const TChar *moduleName)
{
    IModuleBase *retModule = getModulePtr(moduleName);
    if (retModule == nullptr)
    {
        ModulePtr modulePtr = tryLoadModule(moduleName);
        fatalAssertf(modulePtr, "Failed loading module interface {}", moduleName);
        retModule = modulePtr.get();
    }
    return retModule;
}

void ModuleManager::unloadModule(const TChar *moduleName) { tryUnloadModule(moduleName); }

void ModuleManager::unloadModule(const TChar *moduleName, bool bUnloadLib)
{
    if (tryUnloadModule(moduleName) && bUnloadLib)
    {
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
        if (!tryUnloadModule(rItr->getChar()))
        {
            LOG_DEBUG("ModuleManager", "Module {} is already unloaded in one of module that was unload", *rItr);
            continue;
        }
    }
    // Make a local copy and erase loadedModuleInterfaces
    LoadedModulesMap strandedModule = std::move(loadedModuleInterfaces);
    for (const std::pair<const String, ModulePtr> &modulePair : strandedModule)
    {
        onModuleUnload.invoke(modulePair.first);
        modulePair.second->release();
        LOG_DEBUG("ModuleManager", "Unloaded module {}", modulePair.first);
    }

#if !STATIC_LINKED
    // First unload all libraries in reverse of loaded order
    for (auto rItr = localModuleLoadOrder.crbegin(); rItr != localModuleLoadOrder.crend(); ++rItr)
    {
        unloadLibrary(rItr->getChar());
        LOG_DEBUG("ModuleManager", "Unloaded library {}", *rItr);
    }
#endif // STATIC_LINKED
    for (const std::pair<const String, std::pair<LibHandle, LibraryData>> &libPair : loadedLibraries)
    {
        PlatformFunctions::releaseLibrary(libPair.second.first);
        LOG_DEBUG("ModuleManager", "Unloaded library {}", libPair.first);
    }
    loadedLibraries.clear();
}

void ModuleManager::releaseModule(const TChar *moduleName)
{
    WeakModulePtr existingModule = getModule(moduleName);
    if (!existingModule.expired())
    {
        IModuleBase *moduleInterface = existingModule.lock().get();

        onModuleUnload.invoke(moduleName);
        moduleInterface->release();
        loadedModuleInterfaces.erase(moduleName);
        std::erase(moduleLoadedOrder, moduleName);
        LOG_DEBUG("ModuleManager", "Released module {}", moduleName);
    }
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
        {
            continue;
        }

        LOG_DEBUG(
            "ModuleManager", "System loaded module name : {}, Image : {}, Module size : {}", data.name.getChar(), data.imgPath.getChar(),
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
