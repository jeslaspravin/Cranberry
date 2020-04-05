#include "GameEngine.h"
#include "../../RenderApi/RenderApi.h"
#include "../Logger/Logger.h"
#include "../Platform/GenericAppWindow.h"
#include "../Platform/PlatformAssertionErrors.h"

void GameEngine::startup(GenericAppInstance* appInstance)
{    
    applicationInstance = appInstance;
    renderingApi=UniquePtr<RenderApi>(new RenderApi());
    renderingApi->initialize();
    onStartUp();
}

void GameEngine::engineLoop()
{
    while (!isExiting())
    {
        tickEngine();
    }
}

void GameEngine::onStartUp()
{

}

void GameEngine::quit()
{
    bExitNextFrame = true;
    onQuit();
    renderingApi->destroy();
    renderingApi.release();
    applicationInstance = nullptr;
}

void GameEngine::requestExit()
{
    bExitNextFrame = true;
}

const String& GameEngine::getAppName() const
{
    debugAssert(applicationInstance);
    return applicationInstance->applicationName;
}

void GameEngine::getVersion(int32& head, int32& major, int32& sub) const
{
    debugAssert(applicationInstance);
    head = applicationInstance->headVersion;
    major = applicationInstance->majorVersion;
    sub = applicationInstance->subVersion;
}

const GenericAppInstance* GameEngine::getApplicationInstance() const
{
    return applicationInstance;
}

GenericAppInstance& GameEngine::appInstance() const
{
    debugAssert(GameEngine::applicationInstance);
    return *applicationInstance;
}

void GameEngine::onQuit()
{
    
}

void GameEngine::tickEngine()
{
    
}

GameEngine* GameEngineWrapper::createEngineInstance()
{
    return new GameEngine();
}
