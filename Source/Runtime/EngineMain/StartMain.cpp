#include "Engine/GameEngine.h"
#include "Logger/Logger.h"
#include "PlatformInstances.h"
#include "LFS/PlatformLFS.h"
#include "PlatformAssertionErrors.h"
#include "PlatformFunctions.h"
#include "Core/Engine/WindowManager.h"
#include "Assets/AssetsManager.h"


int appMain(GenericAppInstance* appInstance)
{
    UnexpectedErrorHandler::getHandler()->registerFilter();

    Logger::log("Engine", "%s() : Engine start", __func__);
    gEngine->startup(appInstance);

    Logger::flushStream();
    gEngine->engineLoop();

    gEngine->quit();
    Logger::log("Engine", "%s() : Engine quit", __func__);
    UnexpectedErrorHandler::getHandler()->unregisterFilter();
    Logger::flushStream();
    
    return 0;
}

#if PLATFORM_WINDOWS

#include <wtypes.h>

//#define _CRTDBG_MAP_ALLOC
//#include <cstdlib>
//#include <crtdbg.h>

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    /*_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);*/

    PlatformAppInstance appInstance;
    WindowManager appWindowManager;
    AssetManager assetManager;
    appInstance.appWindowManager = &appWindowManager;
    appInstance.assetManager = &assetManager;
    FileSystemFunctions::applicationDirectory(appInstance.applicationName);
    String extension;
    appInstance.applicationName = FileSystemFunctions::stripExtension(appInstance.applicationName, extension);

    appInstance.headVersion = 0;
    appInstance.majorVersion = ENGINE_VERSION;
    appInstance.subVersion = ENGINE_MINOR_VERSION;
    appInstance.windowsInstance = hInstance;

    PlatformFunctions::wcharToStr(appInstance.cmdLine, pCmdLine);
    Logger::debug("CommandLine", "%s() : Command [%s]",__func__, appInstance.cmdLine.getChar());
    
    int32 exitCode = appMain(&appInstance);

    return exitCode;
}

#elif PLATFORM_LINUX
static_assert(false, "Platform not supported!");
#elif PLATFORM_APPLE
static_assert(false, "Platform not supported!");
#endif
