/*!
 * \file ApplicationSettings.h
 *
 * \author Jeslas
 * \date May 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/CoreMathTypedefs.h"
#include "Config/ProgramVarTypes.h"
#include "ApplicationExports.h"

class ApplicationInstance;
class WindowManager;

namespace ApplicationSettings
{
// Rendering resolution and will be used to create window in windowed mode
extern APPLICATION_EXPORT ProgramGlobalVar<Size2D> screenSize;
// Size of surface created for window by operating system, This will be updated with values while
// creating main window
extern APPLICATION_EXPORT ProgramOwnedVar<Size2D, WindowManager> surfaceSize;

extern APPLICATION_EXPORT ProgramOwnedVar<bool, ApplicationInstance> fullscreenMode;

extern APPLICATION_EXPORT ProgramOwnedVar<bool, ApplicationInstance> enableVsync;

extern APPLICATION_EXPORT ProgramOwnedVar<bool, ApplicationInstance> renderingOffscreen;
// Will be same as GlobalRenderVariables::GPU_IS_COMPUTE_ONLY
extern APPLICATION_EXPORT ProgramOwnedVar<bool, ApplicationInstance> computeOnly;

} // namespace ApplicationSettings