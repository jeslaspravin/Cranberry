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
#include "Config/EngineVariableTypes.h"
#include "Math/CoreMathTypedefs.h"
#include "Types/CoreTypes.h"
#include "ProgramCoreExports.h"

namespace EngineSettings
{
    // Rendering resolution and will be used to create window in windowed mode
    extern PROGRAMCORE_EXPORT EngineGlobalConfig<Size2D> screenSize;
    // Size of surface created for window by operating system, This will be updated with values while creating main window
    extern PROGRAMCORE_EXPORT EngineGlobalConfig<Size2D> surfaceSize;

    extern PROGRAMCORE_EXPORT EngineGlobalConfig<bool> fullscreenMode;

    extern PROGRAMCORE_EXPORT EngineGlobalConfig<bool> enableVsync;

    extern PROGRAMCORE_EXPORT EngineGlobalConfig<uint32> minSamplingMipLevel;
    extern PROGRAMCORE_EXPORT EngineGlobalConfig<uint32> maxPrefilteredCubeMiplevels;
    extern PROGRAMCORE_EXPORT EngineGlobalConfig<uint32> maxEnvMapSize;

    extern PROGRAMCORE_EXPORT EngineGlobalConfig<int32> pcfKernelSize;
    extern PROGRAMCORE_EXPORT EngineGlobalConfig<int32> pointPcfKernelSize;

    extern PROGRAMCORE_EXPORT EngineGlobalConfig<uint32> globalSampledTexsSize;
}