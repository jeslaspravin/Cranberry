#include "ModuleManager.h"
#include "PlatformFunctions.h"

ModuleManager::~ModuleManager()
{
	for (auto itr = loadedModules.begin(); itr != loadedModules.end(); ++itr) {
		delete itr->second;
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
	return loadedModules[moduleName];
}

LibPointer* ModuleManager::getOrLoadModule(String moduleName)
{
	if (!isLoaded(moduleName))
	{		
		LibPointer* library = PlatformFunctions::openLibrary(moduleName);
		if (library)
		{		
			loadedModules[moduleName] = library;
			return library;
		}
		return nullptr;
	}

	return getModule(moduleName);
}
