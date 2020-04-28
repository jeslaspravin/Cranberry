#pragma once
#include "EngineVariableTypes.h"
#include "../../Math/CoreMathTypedefs.h"

namespace EngineSettings
{
    // Rendering resolution(not yet implemented for frame buffers) and will be used to create window in windowed mode
    extern EngineGlobalConfig<Size2D> screenSize;
    // Size of surface created for window by operating system, This will be updated with values while creating main window
    extern EngineGlobalConfig<Size2D> surfaceSize;

    extern EngineGlobalConfig<bool> fullscreenMode;
}