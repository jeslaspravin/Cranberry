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
#include "IRenderInterfaceModule.h"

class PlatformAppInstanceBase;
class InputSystem;
class FontManager;
class WindowManager;
namespace copat
{
class JobSystem;
}

struct AppInstanceCreateInfo
{
    void *platformAppHandle;

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

class ApplicationInstance
{
private:
    String applicationName;

    int32 majorVersion;
    int32 minorVersion;
    int32 patchVersion;
    String cmdLine;

    bool bExitNextFrame = false;
    bool bAppActive = true;

public:
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

    void startApp();
    APPLICATION_EXPORT virtual void onStart() = 0;
    // Returns true if application can continue running
    [[nodiscard]] bool appTick();
    APPLICATION_EXPORT virtual void onTick() = 0;
    void exitApp();
    APPLICATION_EXPORT virtual void onExit() = 0;
    APPLICATION_EXPORT virtual void onRendererStateEvent(ERenderStateEvent state) {}

    APPLICATION_EXPORT const String &getAppName() const { return applicationName; }
    APPLICATION_EXPORT void getVersion(int32 &majorVer, int32 &minorVer, int32 &patchVer) const
    {
        majorVer = majorVersion;
        minorVer = minorVersion;
        patchVer = patchVersion;
    }
    APPLICATION_EXPORT const String &getCmdLine() const { return cmdLine; }
};