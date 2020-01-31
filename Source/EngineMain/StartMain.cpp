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
	appInstance.headVersion = 0;
	appInstance.majorVersion = ENGINE_VERSION;
	appInstance.subVersion = ENGINE_SUBVERSION;
	appInstance.windowsInstance = hInstance;

	PlatformAppWindow appWindow;
	appWindow.setWindowSize(1280, 720);
	appWindow.createWindow(&appInstance);

	appInstance.appWindow = &appWindow;

	Logger::log("Engine", "%s() : Engine start", __func__);
	gEngine->startup(&appInstance);

	gEngine->quit();
	Logger::log("Engine", "%s() : Engine quit", __func__);

	delete (::gEngine);
	::gEngine = nullptr;
}