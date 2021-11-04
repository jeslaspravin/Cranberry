#pragma once
#include "../String/String.h"
#include <unordered_map>


class ModuleManager {

private:

    std::unordered_map<String, std::pair<LibPointerPtr,struct ModuleData>> loadedModules;

public:
    ModuleManager();
    ~ModuleManager();

    static ModuleManager* get();

    bool isLoaded(String moduleName);

    LibPointer* getModule(String moduleName);

    LibPointer* getOrLoadModule(String moduleName);

    std::vector<std::pair<LibPointerPtr,struct ModuleData>> getAllModuleData();
};