/*!
 * \file ApplicationInstance.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ApplicationExports.h"
#include "String/String.h"
#include "Types/CoreTypes.h"
#include "Types/Time.h"
#include "Math/CoreMathTypedefs.h"
#include "Memory/StackAllocator.h"
#include "Types/Platform/PlatformTypes.h"
#include "IRenderInterfaceModule.h"

#include <map>

class PlatformAppInstanceBase;
class InputSystem;
class FontManager;
class WindowManager;
class IRenderCommandList;
class GraphicsHelperAPI;
class IGraphicsInstance;
class WidgetDrawContext;
class WgWindow;
class WidgetBase;
class GenericAppWindow;
class GenericWindowCanvas;
class WidgetRenderer;

template <typename PtrType>
class ReferenceCountPtr;
using WindowCanvasRef = ReferenceCountPtr<GenericWindowCanvas>;

namespace copat
{
class JobSystem;
struct NormalFuncAwaiter;
} // namespace copat

struct AppInstanceCreateInfo
{
    InstanceHandle platformAppHandle;

    String applicationName;
    // This cmdLine will be used as reference inside ProgramCmdLine
    String cmdLine;

    int32 majorVersion;
    int32 minorVersion;
    int32 patchVersion;

    // If this application uses GPU modules, This combined with below 2 boolean flags controls how EngineRenderer module gets loaded/initialized
    bool bUseGpu = true;
    // Switches off window and any Presenting related logics
    bool bRenderOffscreen = false;
    // Switches off dedicated Graphics pipelines
    bool bIsComputeOnly = false;
};

struct ApplicationTimeData
{
    // Global
    TickRep startTick;
    TickRep initEndTick;
    uint64 frameCounter = 0;

    /* Global time dilation */
    float timeDilation = 1;

    // -1 means no limit, Ensures that delta time never goes below frame rate limit,
    // inactiveTicksBackup will be same as frameLimitsTicks when app is active
    TickRep inactiveTicksBackup = -1;
    TickRep frameLimitsTicks = -1;

    // Per frame data
    TickRep lastFrameTick;
    TickRep frameTick;

    // In Seconds
    // Start with 100FPS
    float deltaTime = 0.01f;
    float smoothedDeltaTime = 0.01f;

    // For smoothed delta time
    float deltaTimeAccum = 0;
    float prevDeltaTimes[60];
    uint8 lastDelTimeIdx = 0;

    void setApplicationState(bool bActive);
    void setFramesLimit(uint8 framesLimit);

    void appStart();
    void tickStart();
    void progressFrame();

    float getDeltaTime() const { return deltaTime * timeDilation; }
    float getSmoothedDeltaTime() const { return smoothedDeltaTime * timeDilation; }
    float getLastDeltaTime() const { return prevDeltaTimes[lastDelTimeIdx] * timeDilation; }
};

class ApplicationInstance
{
private:
    String applicationName;
    String cmdLine;

    int32 majorVersion;
    int32 minorVersion;
    int32 patchVersion;

    bool bExitNextFrame = false;
    bool bAppActive = true;

    // Gets reset every frame and starts fresh for next frame
    StackAllocator<EThreadSharing::ThreadSharing_Exclusive> frameAllocator;
    StackAllocator<EThreadSharing::ThreadSharing_Exclusive> renderFrameAllocator;

    DelegateHandle onWindowDestroyHandle;
    SharedPtr<WgWindow> lastHoverWnd;
    std::map<GenericAppWindow *, SharedPtr<WgWindow>> windowWidgets;
    WidgetRenderer *wgRenderer;

public:
    ApplicationTimeData timeData;

    PlatformAppInstanceBase *platformApp;
    // Input system and window manager will be valid only if we are rendering to screen
    InputSystem *inputSystem;
    WindowManager *windowManager;

    FontManager *fontManager;
    copat::JobSystem *jobSystem;

public:
    ApplicationInstance() = delete;
    APPLICATION_EXPORT ApplicationInstance(const AppInstanceCreateInfo &createInfo);

    APPLICATION_EXPORT void requestExit() { bExitNextFrame = true; }
    // When to call exit next frame?
    // If you already started rendering jobs based on objects that will gets destroyed immediately(Without waiting for render thread jobs) after
    // exiting
    APPLICATION_EXPORT void exitNextFrame();

    void startApp();
    virtual void onStart() = 0;
    // Returns true if application can continue running
    NODISCARD bool appTick();
    virtual void onTick() = 0;
    void exitApp();
    virtual void onExit() = 0;
    virtual void onRendererStateEvent(ERenderStateEvent /*state*/) {}

    FORCE_INLINE const String &getAppName() const { return applicationName; }
    FORCE_INLINE void getVersion(int32 &majorVer, int32 &minorVer, int32 &patchVer) const;
    FORCE_INLINE const String &getCmdLine() const { return cmdLine; }
    // Must be used in main thread and within application tick only
    APPLICATION_EXPORT StackAllocator<EThreadSharing::ThreadSharing_Exclusive> &getFrameAllocator();
    // Must be used in render thread
    APPLICATION_EXPORT StackAllocator<EThreadSharing::ThreadSharing_Exclusive> &getRenderFrameAllocator();

    // Window related functions
    APPLICATION_EXPORT SharedPtr<WgWindow> getMainWindow() const;
    APPLICATION_EXPORT WindowCanvasRef getWindowCanvas(SharedPtr<WgWindow> window) const;
    APPLICATION_EXPORT SharedPtr<WgWindow> getActiveWindow() const;
    FORCE_INLINE SharedPtr<WgWindow> getHoveringWindow() const { return lastHoverWnd; }
    APPLICATION_EXPORT bool hasActiveWindow() const;
    APPLICATION_EXPORT SharedPtr<WgWindow> createWindow(UInt2 size, const TChar *name, SharedPtr<WgWindow> parent);
    APPLICATION_EXPORT void destroyWindow(SharedPtr<WgWindow> window);
    // Widget queries
    APPLICATION_EXPORT bool isAWindow(SharedPtr<WidgetBase> widget);
    // Finds widget using hasWidget, Then rebuilds widget of a window if not found. returns null if nothing could find widget
    // Avoid calling this often as it leads to rebuilding widget tree in some cases
    APPLICATION_EXPORT SharedPtr<WgWindow> findWidgetParentWindow(SharedPtr<WidgetBase> widget);

private:
    SharedPtr<WgWindow> createWindowWidget(GenericAppWindow *appWindow) const;
    void onWindowDestroyed(GenericAppWindow *appWindow);

    void tickWindowWidgets();
    NODISCARD std::vector<SharedPtr<WgWindow>> drawWindowWidgets();
    void presentDrawnWnds(const std::vector<SharedPtr<WgWindow>> &windowsDrawn);
    void clearWidgets();

    void startNextFrame();
};

FORCE_INLINE void ApplicationInstance::getVersion(int32 &majorVer, int32 &minorVer, int32 &patchVer) const
{
    majorVer = majorVersion;
    minorVer = minorVersion;
    patchVer = patchVersion;
}