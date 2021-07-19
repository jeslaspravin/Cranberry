#include "Core/Engine/GameEngine.h"
#include "Core/Logger/Logger.h"
#include "Core/Platform/PlatformInstances.h"
#include "Core/Platform/LFS/PlatformLFS.h"
#include "Core/Platform/PlatformAssertionErrors.h"
#include "Core/Platform/PlatformFunctions.h"


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

#if _WIN32

#include <wtypes.h>

//#define _CRTDBG_MAP_ALLOC
//#include <cstdlib>
//#include <crtdbg.h>

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    /*_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);*/

    PlatformAppInstance appInstance;
    FileSystemFunctions::applicationDirectory(appInstance.applicationName);
    String extension;
    appInstance.applicationName = FileSystemFunctions::stripExtension(appInstance.applicationName, extension);

    appInstance.headVersion = 0;
    appInstance.majorVersion = ENGINE_VERSION;
    appInstance.subVersion = ENGINE_SUBVERSION;
    appInstance.windowsInstance = hInstance;

    PlatformFunctions::wcharToStr(appInstance.cmdLine, pCmdLine);
    Logger::debug("CommandLine", "%s() : Command [%s]",__func__, appInstance.cmdLine.getChar());
    
    int32 exitCode = appMain(&appInstance);

    return exitCode;
}

#elif __unix__

static_assert(false, "Platform not supported!");
#elif __linux__
static_assert(false, "Platform not supported!");
#elif __APPLE__
static_assert(false, "Platform not supported!");
#endif
