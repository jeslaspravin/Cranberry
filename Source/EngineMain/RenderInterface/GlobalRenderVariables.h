#pragma once

#include "PlatformIndependentGraphicsTypes.h"
#include "../Core/Platform/PlatformTypes.h"

class Vector2D;

namespace GlobalRenderVariables
{
    extern GraphicsDeviceConstant<bool> ENABLE_ANISOTROPY;
    extern GraphicsDeviceConstant<float> MAX_ANISOTROPY;

    //extern GraphicsDeviceConstant<bool> ENABLED_TESSELLATION;

    extern GraphicsDeviceConstant<bool> ENABLED_TIMELINE_SEMAPHORE;
    extern GraphicsDeviceConstant<uint64> MAX_TIMELINE_OFFSET;

    //in nanoseconds
    extern GraphicsDeviceConstant<uint64> MAX_SYNC_RES_WAIT_TIME;

    // GBuffer's framebuffer sample count
    extern EngineGlobalConfig<uint32> GBUFFER_SAMPLE_COUNT;
    // Filtering for shader read textures of GBuffers
    extern EngineVar<uint32> GBUFFER_FILTERING;
}