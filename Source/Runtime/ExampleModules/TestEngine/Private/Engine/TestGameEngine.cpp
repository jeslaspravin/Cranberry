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

#include "Engine/TestGameEngine.h"
#include "GenericAppWindow.h"
#include "IApplicationModule.h"
#include "Logger/Logger.h"
#include "Types/Time.h"
#include "Modules/ModuleManager.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "WindowManager.h"
#include "RenderApi/RenderManager.h"
#include "RenderApi/Rendering/RenderingContexts.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"

void EngineTime::engineStart() { startTick = Time::timeNow(); }

void EngineTime::tickStart()
{
    initEndTick = Time::timeNow();
    frameTick = lastFrameTick = initEndTick;
}

void EngineTime::progressFrame()
{
    frameCounter++;
    lastFrameTick = frameTick;
    lastDeltaTime = deltaTime;

    frameTick = Time::timeNow();
    const TickRep deltaTicks = frameTick - lastFrameTick;
    deltaTime = Time::asSeconds(deltaTicks);
    if (deltaTime > 2) // if Delta time is greater than 2 seconds we might have been in break so reset to old delta
    {
        deltaTime = lastDeltaTime;
    }

    if (targetFrameTicks > 0)
    {
        // Since we are ticks at fixed rate
        deltaTime = Time::asSeconds(targetFrameTicks);
        if (deltaTicks >= targetFrameTicks)
        {
            // We only do double tick max, If there is more than 2 frames delta we drop 1 frame and add a frame fraction
            accumulatedSlack += deltaTicks % targetFrameTicks;
        }
        else // deltaTime is less than target so we have to sleep
        {
            PlatformThreadingFunctions::sleep(Time::asMilliSeconds(targetFrameTicks - deltaTicks));
        }
    }
}

float EngineTime::getDeltaTime() { return deltaTime * timeDilation; }

//////////////////////////////////////////////////////////////////////////
//// Game Engine implementation
//////////////////////////////////////////////////////////////////////////

void TestGameEngine::startup(ApplicationInstance *appInst)
{
    timeData.engineStart();
    rendererModule = static_cast<IRenderInterfaceModule *>(ModuleManager::get()->getOrLoadModule(TCHAR("EngineRenderer")).lock().get());
    applicationModule = static_cast<IApplicationModule *>(ModuleManager::get()->getOrLoadModule(TCHAR("Application")).lock().get());
    application = appInst;

    assetManager.load();
    imguiManager.initialize();

    onStartUp();

    bActiveWindow = application->windowManager->hasActiveWindow();
    frameRateBackup = timeData.frameRate;
    timeData.tickStart();
    LOG("GameEngine", "Engine initialized in %0.3f seconds", Time::asSeconds(timeData.initEndTick - timeData.startTick));
}

void TestGameEngine::quit()
{
    onQuit();

    assetManager.unload();
    ENQUEUE_COMMAND_NODEBUG(
        EngineQuit, { assetManager.clearToDestroy(); }, this
    );
    imguiManager.release();
    // We are not yet ready for 100% multi threaded renderer
    RenderThreadEnqueuer::flushWaitRenderThread();

    LOG("GameEngine", "Engine run time in %.3f minutes", Time::asMinutes(Time::timeNow() - timeData.startTick));
}

void TestGameEngine::engineLoop()
{
    // timeData.activeTimeDilation = applicationModule->pollWindows() ? 1.0f : 0.0f;

    bool bDoubleTick = timeData.targetFrameTicks > 0 && (timeData.accumulatedSlack / timeData.targetFrameTicks);
    timeData.accumulatedSlack %= timeData.targetFrameTicks;
    for (int32 i = bDoubleTick; i >= 0; --i)
    {
        // Technically only should tick logic twice not the renderer
        tickEngine();
        if (!application->windowManager->getMainWindow()->isMinimized())
        {
            ENQUEUE_COMMAND_NODEBUG(
                Engineloop,
                {
                    rendererModule->getRenderManager()->renderFrame(timeData.deltaTime);
                    imguiManager.updateFrame(timeData.deltaTime);
                },
                this
            );
            // We are not yet ready for 100% multi threaded renderer
            RenderThreadEnqueuer::flushWaitRenderThread();
        }
    }
    if (bActiveWindow && !application->windowManager->hasActiveWindow())
    {
        frameRateBackup = uint8(timeData.frameRate);
        timeData.setTargetFrameRate(5);
        bActiveWindow = false;
    }
    else if (!bActiveWindow && application->windowManager->hasActiveWindow())
    {
        timeData.setTargetFrameRate(frameRateBackup);
        bActiveWindow = true;
    }

    Logger::flushStream();
    timeData.progressFrame();
}

void TestGameEngine::onStartUp() {}

void TestGameEngine::onQuit() {}

void TestGameEngine::tickEngine() {}

#if !EXPERIMENTAL
TestGameEngine *GameEngineWrapper::createEngineInstance() { return new TestGameEngine(); }
#endif