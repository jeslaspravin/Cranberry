#pragma once

#include "PlatformIndependentGraphicsTypes.h"
#include "../Core/Platform/PlatformTypes.h"

namespace GlobalRenderVariables
{
    extern GraphicsDeviceConstant<bool> ENABLE_ANISOTROPY;
    extern GraphicsDeviceConstant<float> MAX_ANISOTROPY;

    //extern GraphicsDeviceConstant<bool> ENABLED_TESSELLATION;

    extern GraphicsDeviceConstant<bool> ENABLED_TIMELINE_SEMAPHORE;
    extern GraphicsDeviceConstant<uint64> MAX_TIMELINE_OFFSET;
}