#pragma once
#include "../String/String.h"
#include <unordered_map>


class ModuleManager {

private:

	std::unordered_map<String, LibPointer*> loadedModules;

public:
	~ModuleManager();

	static ModuleManager* get();

	bool isLoaded(String moduleName);

	LibPointer* getModule(String moduleName);

	LibPointer* getOrLoadModule(String moduleName);

};