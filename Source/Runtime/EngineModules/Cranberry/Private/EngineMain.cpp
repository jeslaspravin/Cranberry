/*!
 * \file EngineMain.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CmdLine/CmdLine.h"
#include "Logger/Logger.h"
#include "Memory/Memory.h"
#include "Modules/ModuleManager.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"
#include "ApplicationInstance.h"

CBE_GLOBAL_NEWDELETE_OVERRIDES
void tempTest();
int32 appMain(String cmdLine, InstanceHandle appPlatformInstance)
{
    AppInstanceCreateInfo appCI;
#if EDITOR_BUILD
    appCI.applicationName = TCHAR(MACRO_TO_STRING(ENGINE_NAME)"Editor");
#else  // EDITOR_BUILD
    appCI.applicationName = TCHAR(MACRO_TO_STRING(ENGINE_NAME));
#endif // EDITOR_BUILD
    appCI.cmdLine = cmdLine;
    appCI.majorVersion = ENGINE_VERSION;
    appCI.minorVersion = ENGINE_MINOR_VERSION;
    appCI.patchVersion = ENGINE_PATCH_VERSION;
    appCI.platformAppHandle = appPlatformInstance;
    appCI.bIsComputeOnly = false;
    appCI.bRenderOffscreen = false;
    appCI.bUseGpu = true;

    if (!ModuleManager::get()->loadModule(TCHAR("ProgramCore")))
    {
        return -1;
    }

    // Main Core
    bool bMandatoryModulesLoaded = ModuleManager::get()->loadModule(TCHAR("Application") );
    bMandatoryModulesLoaded = bMandatoryModulesLoaded && ModuleManager::get()->loadModule(TCHAR("ReflectionRuntime"));
    bMandatoryModulesLoaded = bMandatoryModulesLoaded && ModuleManager::get()->loadModule(TCHAR("CoreObjects"));
    bMandatoryModulesLoaded = bMandatoryModulesLoaded && ModuleManager::get()->loadModule(TCHAR("EngineCore"));
    fatalAssertf(bMandatoryModulesLoaded, "Loading mandatory modules failed");

    UnexpectedErrorHandler::getHandler()->registerFilter();

    tempTest();

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

#include "CBEPackage.h"
#include "CBEObjectHelpers.h"
#include "Types/Platform/LFS/Paths.h"
#include "Property/PropertyHelper.h"
#include "CoreObjectDelegates.h"
#include "Classes/ObjectTemplate.h"

void tempTest()
{
    String dir = Paths::contentDirectory();
    String name = Paths::applicationName();
    CoreObjectDelegates::broadcastContentDirectoryAdded(dir);
#if 0
    if (BasicPackagedObject *obj = CBE::load<BasicPackagedObject>(name))
    {
        LOG("Test", "Loaded object %s nameVal %s", obj->getFullPath(), obj->nameVal);
    }
    else
    {
        CBE::Package *package = CBE::Package::createPackage(name, dir);
        CBE::Package *package2 = CBE::Package::createPackage(name + TCHAR("2"), dir);

        BasicPackagedObject *packedObj2 = CBE::create<BasicPackagedObject>(name, package2);
        packedObj2->dt = 0.56;
        packedObj2->nameVal = TCHAR("Its connected object");
        packedObj2->structData = { .a = 4124111.06, .b = 2026, .testStr = "This must be connected to another package" };
        BasicPackagedObject *packedObj = CBE::create<BasicPackagedObject>(name, package);
        packedObj->dt = 0.28;
        packedObj->id = STRID("Hello Subity & Jeslas");
        packedObj->nameVal = TCHAR("Its Me Jeslas");
        packedObj->idxToStr = {
            {1, TCHAR("Jeslas Pravin")},
            {2, TCHAR("Subity Jerald")}
        };
        packedObj->structData = { .a = 8235.28, .b = 834435, .testStr = "3528" };
        packedObj->interLinked = packedObj2;

        BasicFieldSerializedObject *testTemp = CBE::create<BasicFieldSerializedObject>(name, package);
        testTemp->dt = 101.111;
        testTemp->id = STRID("HEll Let lOsE");
        testTemp->interLinked = packedObj;
        testTemp->nameVal = TCHAR("Test All field serialization!");
        testTemp->structData = { .a = 4321, .b = 1234, .testStr = "Not a default value here!" };
        testTemp->idxToStr[10] = {
            {TCHAR("ABC"), 123},
            {TCHAR("CBA"), 321}
        };
        testTemp->idxToStr[5] = {
            {TCHAR("XYZ"), 55667788},
            {TCHAR("ZYX"),     8235}
        };
        IInterfaceExample *interface1 = CBE::cast<IInterfaceExample>(static_cast<CBE::Object *>(testTemp));
        IInterfaceExample2 *interface2 = CBE::cast<IInterfaceExample2>(static_cast<CBE::Object *>(testTemp));
        IInterfaceExample2 *interface3 = CBE::cast<IInterfaceExample2>(interface1);
        BasicFieldSerializedObject *ixToClassObj = CBE::cast<BasicFieldSerializedObject>(interface1);
        CBE::Object *ix1ToClassObj = CBE::cast<CBE::Object>(interface1);
        CBE::Object *ix2ToClassObj = CBE::cast<CBE::Object>(interface2);
        BasicPackagedObject *failingCast = CBE::cast<BasicPackagedObject>(interface1);

        BasicFieldSerializedObject *copied = CBE::cast<BasicFieldSerializedObject>(CBE::duplicateObject(testTemp, package));

        CBE::save(packedObj);
        CBE::save(packedObj2);
    }
#endif
}