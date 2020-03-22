#include "GameEngine.h"
#include "../../RenderApi/RenderApi.h"
#include "../Logger/Logger.h"
#include <assert.h>
#include "../Platform/GenericAppWindow.h"

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
    assert(applicationInstance);
    return applicationInstance->applicationName;
}

void GameEngine::getVersion(int32& head, int32& major, int32& sub) const
{
    assert(applicationInstance);
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
    assert(GameEngine::applicationInstance);
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
