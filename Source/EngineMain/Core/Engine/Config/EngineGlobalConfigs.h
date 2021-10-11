#pragma once
#include "EngineVariableTypes.h"
#include "../../Math/CoreMathTypedefs.h"

namespace EngineSettings
{
    // Rendering resolution and will be used to create window in windowed mode
    extern EngineGlobalConfig<Size2D> screenSize;
    // Size of surface created for window by operating system, This will be updated with values while creating main window
    extern EngineGlobalConfig<Size2D> surfaceSize;

    extern EngineGlobalConfig<bool> fullscreenMode;

    extern EngineGlobalConfig<bool> enableVsync;

    extern EngineGlobalConfig<uint32> minSamplingMipLevel;
    extern EngineGlobalConfig<uint32> maxPrefilteredCubeMiplevels;
    extern EngineGlobalConfig<uint32> maxEnvMapSize;

    extern EngineGlobalConfig<uint32> globalSampledTexsSize;
}