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
#include "ApplicationSettings.h"
#include "IApplicationModule.h"
#include "Widgets/WidgetWindow.h"
#include "Widgets/ImGui/ImGuiManager.h"
#include "InputSystem/Keys.h"
#include "InputSystem/InputSystem.h"
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
    imguiManager = new ImGuiManager(TCHAR("TestEngine"));
    imguiManager->initialize({});
    // Moved out surface resize feeding from ImGuiManager to here
    // Using surface size
    float dpiScaleFactor = IApplicationModule::get()->getApplication()->windowManager->getMainWindow()->dpiScale();
    Vector2D displaySize
        = Vector2D(float(ApplicationSettings::surfaceSize.get().x), float(ApplicationSettings::surfaceSize.get().y)) / dpiScaleFactor;
    imguiManager->setDisplaySize(Short2D(int16(displaySize.x()), int16(displaySize.y())));
    surfaceResizeHandle = ApplicationSettings::surfaceSize.onConfigChanged().bindLambda(
        [this](Size2D oldSize, Size2D newSize)
        {
            float dpiScaleFactor = IApplicationModule::get()->getApplication()->windowManager->getMainWindow()->dpiScale();
            Vector2D displaySize = Vector2D(float(newSize.x), float(newSize.y)) / dpiScaleFactor;
            imguiManager->setDisplaySize(Short2D(int16(displaySize.x()), int16(displaySize.y())));
        }
    );

    onStartUp();

    timeData.tickStart();
    LOG("GameEngine", "Engine initialized in %0.3f seconds", Time::asSeconds(timeData.initEndTick - timeData.startTick));
}

void TestGameEngine::quit()
{
    onQuit();

    assetManager.unload();
    imguiManager->release();
    ApplicationSettings::surfaceSize.onConfigChanged().unbind(surfaceResizeHandle);
    ENQUEUE_COMMAND(EngineQuit)
    (
        [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            assetManager.clearToDestroy();
            delete imguiManager;
        }
    );
    // We are not yet ready for 100% multi threaded renderer
    RenderThreadEnqueuer::flushWaitRenderThread();

    LOG("GameEngine", "Engine run time in %.3f minutes", Time::asMinutes(Time::timeNow() - timeData.startTick));
}

void TestGameEngine::engineLoop()
{
    // timeData.activeTimeDilation = applicationModule->pollWindows() ? 1.0f : 0.0f;
    tickEngine();
    if (!application->windowManager->getMainWindow()->isMinimized())
    {
        // Moved out key input feeding from ImGuiManager to here
        for (Keys::StateKeyType key : Keys::Range())
        {
            auto state = application->inputSystem->keyState(*key);
            if (state->keyWentUp || state->keyWentDown)
            {
                imguiManager->inputKey(key, *state, application->inputSystem);
            }
        }
        for (AnalogStates::StateKeyType key : AnalogStates::Range())
        {
            const AnalogStates::StateInfoType *state = application->inputSystem->analogState(key);
            if (state->acceleration != 0.0f || state->currentValue != 0.0f)
            {
                imguiManager->analogKey(key, *state, application->inputSystem);
            }
        }
        Short2D relMousePos = application->getMainWindow()->screenToWgWindowSpace(Short2D(
            int16(application->inputSystem->analogState(AnalogStates::AbsMouseX)->currentValue),
            int16(application->inputSystem->analogState(AnalogStates::AbsMouseY)->currentValue)
        ));
        imguiManager->mouseMoved(relMousePos, relMousePos, application->inputSystem);

        ENQUEUE_COMMAND(Engineloop)
        (
            [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
            {
                imguiManager->updateFrame(timeData.deltaTime);
            }
        );
        // We are not yet ready for 100% multi threaded renderer
        RenderThreadEnqueuer::flushWaitRenderThread();
    }

    timeData.progressFrame();
}

void TestGameEngine::onStartUp() {}

void TestGameEngine::onQuit() {}

void TestGameEngine::tickEngine() {}

#if !EXPERIMENTAL
TestGameEngine *GameEngineWrapper::createEngineInstance()
{
    static SharedPtr<TestGameEngine> engineInst = std::make_shared<TestGameEngine>();
    return engineInst.get();
}
#endif