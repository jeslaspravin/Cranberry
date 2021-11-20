#include "Engine/GameEngine.h"
#include "Logger/Logger.h"
#include "GenericAppWindow.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Time.h"
#include "Modules/ModuleManager.h"
#include "IApplicationModule.h"
#include "EngineInputCoreModule.h"
#include "RenderInterface/Rendering/RenderingContexts.h"
#include "WindowManager.h"


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
    lastDeltaTime = deltaTime;

    frameTick = Time::timeNow();
    deltaTime = Time::asSeconds(frameTick - lastFrameTick);
    if (deltaTime > 2) // if Delta time is greater than 2 seconds we might have been in break so reset to old delta
    {
        deltaTime = lastDeltaTime;
    }
}

float EngineTime::getDeltaTime()
{
    return deltaTime * timeDilation;
}

//////////////////////////////////////////////////////////////////////////
//// Game Engine implementation
//////////////////////////////////////////////////////////////////////////

void GameEngine::startup(const AppInstanceCreateInfo appInstanceCI)
{
    timeData.engineStart();
    rendererModule = static_cast<IRenderInterfaceModule*>(ModuleManager::get()->getOrLoadModule("EngineRenderer").lock().get());
    renderStateChangeHandle = rendererModule->registerToStateEvents(RenderStateDelegate::SingleCastDelegateType::createObject(this, &GameEngine::onRenderStateChange));
    applicationModule = static_cast<IApplicationModule*>(ModuleManager::get()->getOrLoadModule("Application").lock().get());
    exitAppHandle = applicationModule->registerAllWindowDestroyed(SimpleDelegate::SingleCastDelegateType::createObject(this, &GameEngine::tryExitApp));
    windowSurfaceUpdateHandle = applicationModule->registerPreWindowSurfaceUpdate(AppWindowDelegate::SingleCastDelegateType::createObject(this, &GameEngine::onPreWindowurfaceUpdate));
    inputModule = static_cast<EngineInputCoreModule*>(ModuleManager::get()->getOrLoadModule("EngineInputCore").lock().get());

    applicationModule->createApplication(appInstanceCI);
    rendererModule->initializeGraphics();
    assetManager.load();

    onStartUp();
    rendererModule->finalizeGraphicsInitialization();
}

void GameEngine::quit()
{
    bExitNextFrame = true;
    onQuit();

    assetManager.unload();

    ModuleManager::get()->unloadModule("EngineInputCore");
    ModuleManager::get()->unloadModule("Application");
    ModuleManager::get()->unloadModule("EngineRenderer");
    rendererModule = nullptr;
    applicationModule = nullptr;
    inputModule = nullptr;

    assetManager.clearToDestroy();

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
        timeData.activeTimeDilation = applicationModule->pollWindows()? 1.0f : 0.0f;
        inputModule->getInputSystem()->updateInputStates();

        // Possible when window destroy event was sent
        if (isExiting())
        {
            break;
        }

        timeData.progressFrame();
        tickEngine();
        rendererModule->getRenderManager()->renderFrame(timeData.deltaTime);

        Logger::flushStream();
    }
}

void GameEngine::onRenderStateChange(ERenderStateEvent state)
{
    switch (state)
    {
    case ERenderStateEvent::PostLoadInstance:
        break;
    case ERenderStateEvent::PreinitDevice:
        break;
    case ERenderStateEvent::PostInitDevice:
        break;
    case ERenderStateEvent::PostInitGraphicsContext:
        break;
    case ERenderStateEvent::PostInititialize:
        break;
    case ERenderStateEvent::PreFinalizeInit:
        imguiManager.initialize();
        break;
    case ERenderStateEvent::PostFinalizeInit:
        break;
    case ERenderStateEvent::PreExecFrameCommands:
        imguiManager.updateFrame(timeData.deltaTime);
        break;
    case ERenderStateEvent::PreCleanupCommands:
        imguiManager.release();
        break;
    case ERenderStateEvent::Cleanup:
        break;
    case ERenderStateEvent::PostCleanupCommands:
        break;
    default:
        break;
    }
}

void GameEngine::onPreWindowurfaceUpdate(GenericAppWindow* window) const
{
    rendererModule->getRenderManager()->getGlobalRenderingContext()
        ->clearWindowCanvasFramebuffer(applicationModule->getWindowManager()->getWindowCanvas(window));
}

void GameEngine::tryExitApp()
{
    bExitNextFrame = true;
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

#if !EXPERIMENTAL
GameEngine* GameEngineWrapper::createEngineInstance()
{
    return new GameEngine();
}
#endif