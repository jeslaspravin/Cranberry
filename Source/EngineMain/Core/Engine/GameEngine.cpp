#include "GameEngine.h"
#include "../../RenderApi/RenderApi.h"
#include "../Logger/Logger.h"
#include "../Platform/GenericAppWindow.h"
#include "../Platform/PlatformAssertionErrors.h"
#include "../Types/Time.h"


void EngineTime::engineStart()
{
    startTick = Time::timeNow();
}

void EngineTime::tickStart()
{
    initEndTick = Time::timeNow();
    frameTick = initEndTick;
    deltaTime = 0;
}

void EngineTime::progressFrame()
{
    frameCounter++;
    lastFrameTick = frameTick;
    frameTick = Time::timeNow();
    lastDeltaTime = deltaTime;
    deltaTime = Time::asSeconds(frameTick - lastFrameTick);
}

float EngineTime::getDeltaTime()
{
    return deltaTime * timeDilation;
}

//////////////////////////////////////////////////////////////////////////
//// Game Engine implementation
//////////////////////////////////////////////////////////////////////////

void GameEngine::startup(GenericAppInstance* appInstance)
{
    timeData.engineStart();

    applicationInstance = appInstance;
    renderingApi.initialize();
    applicationInstance->assetManager.load();
    onStartUp();

    // Has to be done at last after all the other rendering related systems init
    renderingApi.postInit();
}

void GameEngine::quit()
{
    bExitNextFrame = true;
    onQuit();
    applicationInstance->assetManager.unload();

    renderingApi.destroy();

    applicationInstance->assetManager.clearToDestroy();
    applicationInstance = nullptr;

    Logger::debug("GameEngine", "%s() : Engine run time in %.3f minutes", __func__
        , Time::asMinutes(Time::timeNow() - timeData.startTick));
}

void GameEngine::engineLoop()
{
    timeData.tickStart();
    Logger::debug("GameEngine", "%s() : Engine initialized in %0.3f seconds", __func__
        , Time::asSeconds(timeData.initEndTick - timeData.startTick));

    while (!isExiting())
    {
        timeData.activeTimeDilation = applicationInstance->appWindowManager.pollWindows()? 1.0f : 0.0f;
        timeData.progressFrame();
        tickEngine();
        renderingApi.renderFrame();
    }
}

void GameEngine::onStartUp()
{

}

void GameEngine::onQuit()
{

}

void GameEngine::tickEngine()
{

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

GameEngine* GameEngineWrapper::createEngineInstance()
{
    return new GameEngine();
}
