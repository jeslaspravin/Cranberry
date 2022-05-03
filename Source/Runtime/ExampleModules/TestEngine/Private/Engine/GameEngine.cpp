/*!
 * \file GameEngine.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Engine/GameEngine.h"
#include "GenericAppWindow.h"
#include "IApplicationModule.h"
#include "Logger/Logger.h"
#include "Modules/ModuleManager.h"
#include "RenderInterface/Rendering/RenderingContexts.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Time.h"
#include "WindowManager.h"

void EngineTime::engineStart() { startTick = Time::timeNow(); }

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

float EngineTime::getDeltaTime() { return deltaTime * timeDilation; }

//////////////////////////////////////////////////////////////////////////
//// Game Engine implementation
//////////////////////////////////////////////////////////////////////////

void GameEngine::startup(ApplicationInstance *appInst)
{
    timeData.engineStart();
    rendererModule = static_cast<IRenderInterfaceModule *>(ModuleManager::get()->getOrLoadModule(TCHAR("EngineRenderer")).lock().get());
    applicationModule = static_cast<IApplicationModule *>(ModuleManager::get()->getOrLoadModule(TCHAR("Application")).lock().get());
    application = appInst;

    assetManager.load();

    onStartUp();

    timeData.tickStart();
    LOG("GameEngine", "%s() : Engine initialized in %0.3f seconds", __func__, Time::asSeconds(timeData.initEndTick - timeData.startTick));
}

void GameEngine::quit()
{
    onQuit();

    assetManager.unload();
    ENQUEUE_COMMAND_NODEBUG(
        EngineQuit, { assetManager.clearToDestroy(); }, this
    );

    LOG("GameEngine", "%s() : Engine run time in %.3f minutes", __func__, Time::asMinutes(Time::timeNow() - timeData.startTick));
}

void GameEngine::engineLoop()
{
    // timeData.activeTimeDilation = applicationModule->pollWindows() ? 1.0f : 0.0f;

    timeData.progressFrame();
    tickEngine();
    rendererModule->getRenderManager()->renderFrame(timeData.deltaTime);

    Logger::flushStream();
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
        imguiManager.initialize();
        break;
    case ERenderStateEvent::PreFinalizeInit:
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

void GameEngine::onStartUp() {}

void GameEngine::onQuit() {}

void GameEngine::tickEngine() {}

#if !EXPERIMENTAL
GameEngine *GameEngineWrapper::createEngineInstance() { return new GameEngine(); }
#endif