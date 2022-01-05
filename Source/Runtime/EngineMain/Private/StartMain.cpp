/*!
 * \file StartMain.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Engine/GameEngine.h"
#include "Logger/Logger.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"
#include "Assets/AssetsManager.h"
#include "Modules/ModuleManager.h"

#include "IReflectionRuntime.h"
#include "TestReflectionGen.h"


int appMain(String cmdLine, void* appPlatformInstance)
{
    AppInstanceCreateInfo appCI;
    appCI.applicationName = MACRO_TO_STRING(ENGINE_NAME);
    appCI.cmdLine = cmdLine;
    appCI.majorVersion = ENGINE_VERSION;
    appCI.minorVersion = ENGINE_MINOR_VERSION;
    appCI.patchVersion = ENGINE_PATCH_VERSION;
    appCI.platformAppHandle = appPlatformInstance;

    // Main Core
    bool bMandatoryModulesLoaded = ModuleManager::get()->loadModule("ProgramCore")
        && ModuleManager::get()->loadModule("ReflectionRuntime")
        && ModuleManager::get()->loadModule("CoreObjects");
    fatalAssert(bMandatoryModulesLoaded, "Loading mandatory modules failed");

    UnexpectedErrorHandler::getHandler()->registerFilter();

    Logger::log("Engine", "%s() : Engine start", __func__);
    gEngine->startup(appCI);

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

    String cmdLine;
    PlatformFunctions::wcharToStr(cmdLine, pCmdLine);
    Logger::debug("CommandLine", "%s() : Command [%s]", __func__, cmdLine.getChar());

    int32 exitCode = appMain(cmdLine, hInstance);

    return exitCode;
}

#elif PLATFORM_LINUX
static_assert(false, "Platform not supported!");
#elif PLATFORM_APPLE
static_assert(false, "Platform not supported!");
#endif
