#pragma once

#include "Modules/IModuleBase.h"
#include "String/String.h"
#include "Memory/SmartPointers.h"
#include "Types/Platform/GenericPlatformTypes.h"
#include "Types/Delegates/Delegate.h"

#include <unordered_map>

#define MODULE_CREATE_FUNCTION_CPP(ModuleName, ModuleClass) IModuleBase* createModule_##ModuleName() \
    { \
        return new ModuleClass(); \
    }

#define DECLARE_STATIC_LINKED_MODULE(ModuleName, ModuleClass) \
    MODULE_CREATE_FUNCTION_CPP(ModuleName, ModuleClass) \
    static StaticModuleInitializerRegistrant staticModuleInitializerRegistrant_##ModuleName{ #ModuleName ,  SingleCastDelegate<IModuleBase*>::createStatic(&createModule_##ModuleName) };

#if STATIC_LINKED
#define DECLARE_MODULE(ModuleName, ModuleClass) \
    DECLARE_STATIC_LINKED_MODULE(ModuleName, ModuleClass)
#else // STATIC_LINKED
#define DECLARE_MODULE(ModuleName, ModuleClass) \
    extern "C" \
    { \
        DLL_EXPORT MODULE_CREATE_FUNCTION_CPP(ModuleName, ModuleClass) \
    }
#endif // STATIC_LINKED

struct StaticModuleInitializerRegistrant
{
    StaticModuleInitializerRegistrant(String moduleName, SingleCastDelegate<IModuleBase*> functionPtr);
};
using StaticModuleInitializerList = std::unordered_map<String, SingleCastDelegate<IModuleBase*>>;

using ModulePtr = SharedPtr<IModuleBase>;
using WeakModulePtr = WeakPtr<IModuleBase>;

class PROGRAMCORE_EXPORT ModuleManager 
{
private:
    // Contains modules loaded as shared libraries
    std::unordered_map<String, std::pair<LibPointerPtr, LibraryData>> loadedLibraries;

    // Contains module interface implementation provided by a module
    std::unordered_map<String, ModulePtr> loadedModuleInterfaces;
    // Library paths to look for a library if not found in initial OS environment paths search
    std::vector<String> additionalLibraryPaths;
private:
    friend StaticModuleInitializerRegistrant;
    static StaticModuleInitializerList& getModuleInitializerList();

    /*
    * Checks and appends/prepends library prefix and extension as required if not present already
    */
    LibPointer* loadFromAdditionalPaths(String modulePath) const;

    ModuleManager();
public:
    ~ModuleManager();

    static ModuleManager* get();

    // Library loading functions
    bool isLibraryLoaded(String moduleName) const;
    LibPointer* getLibrary(String moduleName) const;
    LibPointer* getOrLoadLibrary(String modulePath);

    // Engine/Game module loading functions
    bool isModuleLoaded(String moduleName) const;
    WeakModulePtr getModule(String moduleName) const;
    bool loadModule(String moduleName);
    WeakModulePtr getOrLoadModule(String moduleName);
    void unloadModule(String moduleName);

    std::vector<std::pair<LibPointerPtr,struct LibraryData>> getAllModuleData();
};