#include "Core/Engine/GameEngine.h"
#include "Core/Logger/Logger.h"
#include "Core/Platform/PlatformInstances.h"
#include "Core/Platform/LFS/PlatformLFS.h"
#include "Core/Platform/PlatformAssertionErrors.h"


int appMain(GenericAppInstance* appInstance)
{
    ::gEngine = new GameEngine();
    UnexpectedErrorHandler::getHandler()->registerFilter();

    Logger::log("Engine", "%s() : Engine start", __func__);
    gEngine->startup(appInstance);

    gEngine->engineLoop();

    UnexpectedErrorHandler::getHandler()->unregisterFilter();
    gEngine->quit();
    Logger::log("Engine", "%s() : Engine quit", __func__);

    delete (::gEngine);
    ::gEngine = nullptr;

    return 0;
}

#if _WIN32

#include <wtypes.h>

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    PlatformAppInstance appInstance;
    FileSystemFunctions::applicationDirectory(appInstance.applicationName);
    String extension;
    appInstance.applicationName = FileSystemFunctions::stripExtension(appInstance.applicationName, extension);

    appInstance.headVersion = 0;
    appInstance.majorVersion = ENGINE_VERSION;
    appInstance.subVersion = ENGINE_SUBVERSION;
    appInstance.windowsInstance = hInstance;

    // TODO(JESLAS) : Change wide char to Ansi conversion to something robust(Not so important)
    size_t cmdLen = wcslen(pCmdLine) + 1;
    String cmdLine;
    cmdLine.resize(cmdLen * 2);
    size_t writtenLength; 
    wcstombs_s(&writtenLength, cmdLine.data(), cmdLine.length(), pCmdLine, _TRUNCATE);
    cmdLine.resize(writtenLength);
    appInstance.cmdLine = cmdLine;

    if ((writtenLength - cmdLen) > 1)
    {
        Logger::warn("CommandLine", "%s() : Command line has non-ansi characters, They are not accepted [command] = %s",
            __func__, appInstance.cmdLine.getChar());
    }
    Logger::debug("CommandLine", "%s() : Command [%s]",__func__, appInstance.cmdLine.getChar());
    
    return appMain(&appInstance);
}

#elif __unix__

static_assert(false, "Platform not supported!");
#elif __linux__
static_assert(false, "Platform not supported!");
#elif __APPLE__
static_assert(false, "Platform not supported!");
#endif
