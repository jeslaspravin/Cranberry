#include "GameEngine.h"
#include "../../RenderApi/RenderManager.h"
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
    frameTick = lastFrameTick = initEndTick;
    deltaTime = 0;
    frameCounter = 0;
}

void EngineTime::progressFrame()
{
    frameCounter++;
    lastFrameTick = frameTick;
    frameTick = Time::timeNow();
    averageDeltaTime = Time::asSeconds(frameTick - initEndTick) / frameCounter;
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

    renderManager.initialize();

    applicationInstance->assetManager.load();
    onStartUp();

    // Has to be done at last after all the other rendering related systems init
    renderManager.postInit();
}

void GameEngine::quit()
{
    bExitNextFrame = true;
    onQuit();
    applicationInstance->assetManager.unload();
    renderManager.destroy();

    applicationInstance->assetManager.clearToDestroy();
    applicationInstance = nullptr;

    Logger::log("GameEngine", "%s() : Engine run time in %.3f minutes", __func__
        , Time::asMinutes(Time::timeNow() - timeData.startTick));
}

void GameEngine::engineLoop()
{
    timeData.tickStart();
    Logger::log("GameEngine", "%s() : Engine initialized in %0.3f seconds", __func__
        , Time::asSeconds(timeData.initEndTick - timeData.startTick));

    while (!isExiting())
    {
        timeData.activeTimeDilation = applicationInstance->appWindowManager.pollWindows()? 1.0f : 0.0f;
        timeData.progressFrame();
        tickEngine();
        renderManager.renderFrame(timeData.deltaTime);

        Logger::flushStream();
    }
}

void GameEngine::broadcastPostInitRenderEvent()
{
    if (renderPostInitEvent.isBound())
    {
        renderPostInitEvent.invoke();
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

#if !EXPERIMENTAL
GameEngine* GameEngineWrapper::createEngineInstance()
{
    return new GameEngine();
}
#endif