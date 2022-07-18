/*!
 * \file ApplicationInstance.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "ApplicationInstance.h"
#include "IApplicationModule.h"
#include "ApplicationSettings.h"
#include "Modules/ModuleManager.h"
#include "InputSystem/InputSystem.h"
#include "WindowManager.h"
#include "FontManager.h"
#include "Widgets/WidgetRenderer.h"
#include "Types/Platform/Threading/PlatformThreading.h"

namespace ApplicationSettings
{
// Rendering resolution and will be used to create window in windowed mode
ProgramGlobalVar<Size2D> screenSize{ Size2D(1280, 720) };
// Size of surface created for window by operating system, This will be updated with values while
// creating main window
ProgramOwnedVar<Size2D, WindowManager> surfaceSize;

ProgramOwnedVar<bool, ApplicationInstance> fullscreenMode{ false };

ProgramOwnedVar<bool, ApplicationInstance> enableVsync{ true };

ProgramOwnedVar<bool, ApplicationInstance> renderingOffscreen{ false };
ProgramOwnedVar<bool, ApplicationInstance> computeOnly{ false };
ProgramOwnedVar<bool, ApplicationInstance> usingGpu{ true };

} // namespace ApplicationSettings

void ApplicationTimeData::setApplicationState(bool bActive)
{
    if (bActive)
    {
        frameLimitsTicks = inactiveTicksBackup;
        return;
    }
    frameLimitsTicks = Time::fromSeconds(1.0f / 5.0f);
}

void ApplicationTimeData::setFramesLimit(uint8 framesLimit)
{
    if (framesLimit == 0)
    {
        inactiveTicksBackup = frameLimitsTicks = -1;
        return;
    }
    inactiveTicksBackup = frameLimitsTicks = Time::fromSeconds(1.0f / float(framesLimit));
}

void ApplicationTimeData::appStart() { startTick = Time::timeNow(); }

void ApplicationTimeData::tickStart()
{
    for (uint32 i = 0; i < ARRAY_LENGTH(prevDeltaTimes); ++i)
    {
        prevDeltaTimes[i] = deltaTime;
    }
    deltaTimeAccum = ARRAY_LENGTH(prevDeltaTimes) * deltaTime;

    initEndTick = Time::timeNow();
    frameTick = lastFrameTick = initEndTick;
}

void ApplicationTimeData::progressFrame()
{
    frameCounter++;
    lastFrameTick = frameTick;
    const float lastDeltaTime = deltaTime;

    frameTick = Time::timeNow();
    TickRep deltaTicks = frameTick - lastFrameTick;
    deltaTime = Time::asSeconds(deltaTicks);
    if (deltaTime > 2) // if Delta time is greater than 2 seconds we might have been in break so reset to old delta
    {
        deltaTime = lastDeltaTime;
    }

    // If we are faster than frame limit when inactive sleep for few ms
    while (frameLimitsTicks > 0 && deltaTicks < frameLimitsTicks)
    {
        TickRep sleepDur = Time::asMilliSeconds(frameLimitsTicks - deltaTicks);
        // If we are trying to sleep more than 32ms(30fps) then use sleep else just loop
        if (sleepDur > 32)
        {
            PlatformThreadingFunctions::sleep(sleepDur);
        }
        else
        {
            // Just push thread to staged state(giving up its time slice and waits for next time slice)
            PlatformThreadingFunctions::sleep(0);
        }
        frameTick = Time::timeNow();
        deltaTicks = frameTick - lastFrameTick;
        deltaTime = Time::asSeconds(deltaTicks);
    }

    deltaTimeAccum += lastDeltaTime - prevDeltaTimes[lastDelTimeIdx];
    prevDeltaTimes[lastDelTimeIdx] = lastDeltaTime;
    lastDelTimeIdx = (lastDelTimeIdx + 1) % ARRAY_LENGTH(prevDeltaTimes);
    smoothedDeltaTime = deltaTimeAccum / ARRAY_LENGTH(prevDeltaTimes);
}

ApplicationInstance::ApplicationInstance(const AppInstanceCreateInfo &createInfo)
    : applicationName(createInfo.applicationName)
    , cmdLine(createInfo.cmdLine)
    , majorVersion(createInfo.majorVersion)
    , minorVersion(createInfo.minorVersion)
    , patchVersion(createInfo.patchVersion)
    , lastHoverWnd(nullptr)
    , wgRenderer(nullptr)
    , platformApp(nullptr)
    , inputSystem(nullptr)
    , windowManager(nullptr)
    , fontManager(nullptr)
    , jobSystem(nullptr)
{
    debugAssertf(!createInfo.bRenderOffscreen, "Offscreen rendering is not supported!");
    ApplicationSettings::renderingOffscreen.set(createInfo.bRenderOffscreen);
    ApplicationSettings::computeOnly.set(createInfo.bIsComputeOnly);
    ApplicationSettings::usingGpu.set(createInfo.bUseGpu);
}

void ApplicationInstance::startApp()
{
    if (windowManager)
    {
        debugAssert(windowManager->getMainWindow());
        windowWidgets[windowManager->getMainWindow()] = createWindowWidget(windowManager->getMainWindow());
    }
    // TODO(Jeslas) : If ever rendering off screen just create new proxy window with proxy window canvas(like swapchain) and setup window widget
    if (ApplicationSettings::usingGpu && !ApplicationSettings::computeOnly)
    {
        wgRenderer = WidgetRenderer::createRenderer();
        fatalAssertf(wgRenderer, "Failed creating WidgetRenderer!");
        wgRenderer->initialize();
    }

    onWindowDestroyHandle = IApplicationModule::get()->registerOnWindowDestroyed(
        AppWindowDelegate::SingleCastDelegateType::createObject(this, &ApplicationInstance::onWindowDestroyed)
    );
    onStart();

    timeData.tickStart();
    LOG("ApplicationInstance", "%s initialized in %0.3f seconds", applicationName, Time::asSeconds(timeData.initEndTick - timeData.startTick));
}

bool ApplicationInstance::appTick()
{
    startNewFrame();

    if (windowManager && inputSystem)
    {
        bAppActive = windowManager->pollWindows();
        inputSystem->updateInputStates();
        timeData.setApplicationState(bAppActive);
    }

    // Handle if we requested exit during this polling
    if (bExitNextFrame)
    {
        return false;
    }

    // Do any none App Tick dependent rendering/ticks here
    if (fontManager)
    {
        fontManager->flushUpdates();
    }
    if (hasActiveWindow())
    {
        tickWindowWidgets();
    }

    // Application tick
    onTick();

    // Rendering widgets after application tick to allow application updates to be visible, No idea if it is right choice!
    if (wgRenderer)
    {
        auto drawnWnds = drawWindowWidgets();
        if (!ApplicationSettings::renderingOffscreen)
        {
            presentDrawnWnds(drawnWnds);
        }
    }

    // Below must be last executed
    Logger::flushStream();
    timeData.progressFrame();
    return !bExitNextFrame;
}

void ApplicationInstance::exitApp()
{
    onExit();
    if (wgRenderer)
    {
        wgRenderer->destroy();
        // Will be deleted after destroyed in render thread
        wgRenderer = nullptr;
    }
    IApplicationModule::get()->unregisterOnWindowDestroyed(onWindowDestroyHandle);
    windowWidgets.clear();

    LOG("ApplicationInstance", "%s run time %.3f minutes", applicationName, Time::asMinutes(Time::timeNow() - timeData.startTick));
}