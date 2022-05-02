/*!
 * \file EngineGlobalConfigs.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Config/ProgramVarTypes.h"
#include "Math/CoreMathTypedefs.h"
#include "ProgramCoreExports.h"
#include "Types/CoreTypes.h"

// #TODO(Jeslas) : move this out of ProgramCore
namespace EngineSettings
{
// Rendering resolution and will be used to create window in windowed mode
extern PROGRAMCORE_EXPORT ProgramGlobalVar<Size2D> screenSize;
// Size of surface created for window by operating system, This will be updated with values while
// creating main window
extern PROGRAMCORE_EXPORT ProgramGlobalVar<Size2D> surfaceSize;

extern PROGRAMCORE_EXPORT ProgramGlobalVar<bool> fullscreenMode;

extern PROGRAMCORE_EXPORT ProgramGlobalVar<bool> enableVsync;

extern PROGRAMCORE_EXPORT ProgramGlobalVar<uint32> minSamplingMipLevel;
extern PROGRAMCORE_EXPORT ProgramGlobalVar<uint32> maxPrefilteredCubeMiplevels;
extern PROGRAMCORE_EXPORT ProgramGlobalVar<uint32> maxEnvMapSize;

extern PROGRAMCORE_EXPORT ProgramGlobalVar<int32> pcfKernelSize;
extern PROGRAMCORE_EXPORT ProgramGlobalVar<int32> pointPcfKernelSize;

extern PROGRAMCORE_EXPORT ProgramGlobalVar<uint32> globalSampledTexsSize;
} // namespace EngineSettings