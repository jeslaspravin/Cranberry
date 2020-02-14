#include <wtypes.h>
#include "Core/Engine/GameEngine.h"
#include "Core/Logger/Logger.h"
#include "Core/Platform/PlatformInstances.h"
#include "Core/Platform/LFS/PlatformLFS.h"

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	::gEngine = new GameEngine();

	PlatformAppInstance appInstance;
	FileSystemFunctions::applicationDirectory(appInstance.applicationName);
	String extension;
	appInstance.applicationName = FileSystemFunctions::stripExtension(appInstance.applicationName, extension);

	appInstance.headVersion = 0;
	appInstance.majorVersion = ENGINE_VERSION;
	appInstance.subVersion = ENGINE_SUBVERSION;
	appInstance.windowsInstance = hInstance;

	Logger::log("Engine", "%s() : Engine start", __func__);
	gEngine->startup(&appInstance);

	gEngine->engineLoop();

	gEngine->quit();
	Logger::log("Engine", "%s() : Engine quit", __func__);

	delete (::gEngine);
	::gEngine = nullptr;
}