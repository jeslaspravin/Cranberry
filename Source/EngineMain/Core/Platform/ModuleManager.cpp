#include "ModuleManager.h"
#include "PlatformFunctions.h"
#include "../Logger/Logger.h"

ModuleManager::~ModuleManager()
{
	for (auto itr = loadedModules.begin(); itr != loadedModules.end(); ++itr) {
		delete itr->second;
		Logger::debug("ModuleManager", "%s() : Unloaded module %s", __func__, itr->first.getChar());
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
			Logger::debug("ModuleManager", "%s() : Loaded module %s", __func__, moduleName.getChar());
			loadedModules[moduleName] = library;
			return library;
		}
		return nullptr;
	}

	return getModule(moduleName);
}
