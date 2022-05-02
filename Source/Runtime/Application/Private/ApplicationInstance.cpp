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

namespace ApplicationSettings
{
// Rendering resolution and will be used to create window in windowed mode
ProgramGlobalVar<Size2D> screenSize(Size2D(1280, 720));
// Size of surface created for window by operating system, This will be updated with values while
// creating main window
ProgramOwnedVar<Size2D, WindowManager> surfaceSize;

ProgramOwnedVar<bool, ApplicationInstance> fullscreenMode(false);

ProgramOwnedVar<bool, ApplicationInstance> enableVsync(true);

} // namespace ApplicationSettings

ApplicationInstance::ApplicationInstance(const AppInstanceCreateInfo &createInfo)
    : applicationName(createInfo.applicationName)
    , majorVersion(createInfo.majorVersion)
    , minorVersion(createInfo.minorVersion)
    , patchVersion(createInfo.patchVersion)
    , cmdLine(createInfo.cmdLine)
    , fontManager(InitType_DefaultInit)
{}
