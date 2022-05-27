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
#include "ApplicationSettings.h"
#include "CmdLine/CmdLine.h"
#include "Modules/ModuleManager.h"
#include "InputSystem/InputSystem.h"
#include "WindowManager.h"
#include "FontManager.h"

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

} // namespace ApplicationSettings

void ApplicationInstance::startApp()
{
    if (!ProgramCmdLine::get()->parse(cmdLine))
    {
        LOG_ERROR("Engine", "%s() : Invalid command line", __func__);
        ProgramCmdLine::get()->printCommandLine();
    }

    onStart();
}

void ApplicationInstance::runApp()
{
    while (!bExitNextFrame)
    {
        if (windowManager && inputSystem)
        {
            bAppActive = windowManager->pollWindows();
            inputSystem->updateInputStates();
        }
        // Handle if we requested exit during this polling
        if (bExitNextFrame)
        {
            break;
        }

        if (fontManager)
        {
            fontManager->flushUpdates();
        }
        onTick();

        // TODO(Jeslas) : Handle render frame progress and Time data?
    }
}

void ApplicationInstance::exitApp() { onExit(); }

ApplicationInstance::ApplicationInstance(const AppInstanceCreateInfo &createInfo)
    : applicationName(createInfo.applicationName)
    , majorVersion(createInfo.majorVersion)
    , minorVersion(createInfo.minorVersion)
    , patchVersion(createInfo.patchVersion)
    , cmdLine(createInfo.cmdLine)
    , platformApp(nullptr)
    , inputSystem(nullptr)
    , windowManager(nullptr)
    , fontManager(nullptr)
{
    ApplicationSettings::renderingOffscreen.set(createInfo.bRenderOffscreen);
}
