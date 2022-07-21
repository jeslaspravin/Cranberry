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

#include "CmdLine/CmdLine.h"
#include "Engine/TestGameEngine.h"
#include "Logger/Logger.h"
#include "Memory/Memory.h"
#include "Modules/ModuleManager.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"
#include "ApplicationInstance.h"
#include "IApplicationModule.h"

CBE_GLOBAL_NEWDELETE_OVERRIDES

class TestEngineApplication : public ApplicationInstance
{
public:
    TestEngineApplication(const AppInstanceCreateInfo &createInfo)
        : ApplicationInstance(createInfo)
    {}

    void onStart() override
    {
        LOG("Engine", "Engine start");
        gEngine->startup(this);
    }
    void onTick() override { gEngine->engineLoop(); }
    void onExit() override
    {
        gEngine->quit();
        LOG("Engine", "Engine quit");
    }
};

int appMain(String cmdLine, InstanceHandle appPlatformInstance)
{
    AppInstanceCreateInfo appCI;
    // appCI.applicationName = TCHAR(MACRO_TO_STRING(ENGINE_NAME));
    appCI.applicationName = TCHAR("TestEngine");
    appCI.cmdLine = cmdLine;
    appCI.majorVersion = ENGINE_VERSION;
    appCI.minorVersion = ENGINE_MINOR_VERSION;
    appCI.patchVersion = ENGINE_PATCH_VERSION;
    appCI.platformAppHandle = appPlatformInstance;
    appCI.bIsComputeOnly = false;
    appCI.bRenderOffscreen = false;
    appCI.bUseGpu = true;

    // Main Core
    bool bMandatoryModulesLoaded = ModuleManager::get()->loadModule(
        TCHAR("ProgramCore")
        )
        && ModuleManager::get()->loadModule(TCHAR("ReflectionRuntime")) && ModuleManager::get()->loadModule(TCHAR("CoreObjects"));
    fatalAssertf(bMandatoryModulesLoaded, "Loading mandatory modules failed");

    UnexpectedErrorHandler::getHandler()->registerFilter();

    IApplicationModule *appModule = IApplicationModule::get();
    if (appModule)
    {
        appModule->startApplication<TestEngineApplication>(appCI);
    }

    ModuleManager::get()->unloadAll();
    UnexpectedErrorHandler::getHandler()->unregisterFilter();
    Logger::flushStream();

    return 0;
}

#if PLATFORM_WINDOWS

#include "WindowsCommonHeaders.h"

//#define _CRTDBG_MAP_ALLOC
//#include <cstdlib>
//#include <crtdbg.h>

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    /*_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);*/

    String cmdLine{ WCHAR_TO_TCHAR(pCmdLine) };
    Logger::initialize();
    LOG_DEBUG("CommandLine", "Command [%s]", cmdLine.getChar());

    int32 exitCode = appMain(cmdLine, hInstance);

    Logger::shutdown();
    return exitCode;
}

#elif PLATFORM_LINUX
#error "Platform not supported!"
#elif PLATFORM_APPLE
#error "Platform not supported!"
#endif
