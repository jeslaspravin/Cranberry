#pragma once

#include "PlatformIndependentGraphicsTypes.h"
#include "../Core/Platform/PlatformTypes.h"

class Vector2D;

namespace GlobalRenderVariables
{
    extern GraphicsDeviceConstant<bool> ENABLE_ANISOTROPY;
    extern GraphicsDeviceConstant<float> MAX_ANISOTROPY;

    extern GraphicsDeviceConstant<bool> ENABLE_EXTENDED_STORAGES;
    extern GraphicsDeviceConstant<bool> ENABLE_GEOMETRY_SHADERS;

    //extern GraphicsDeviceConstant<bool> ENABLED_TESSELLATION;
    extern GraphicsDeviceConstant<bool> ENABLE_NON_FILL_DRAWS;
    extern GraphicsDeviceConstant<bool> ENABLE_WIDE_LINES;

    extern GraphicsDeviceConstant<bool> ENABLED_TIMELINE_SEMAPHORE;
    extern GraphicsDeviceConstant<uint64> MAX_TIMELINE_OFFSET;

    extern GraphicsDeviceConstant<bool> ENABLED_RESOURCE_RUNTIME_ARRAY;
    extern GraphicsDeviceConstant<bool> ENABLED_RESOURCE_UPDATE_AFTER_BIND;
    extern GraphicsDeviceConstant<bool> ENABLED_RESOURCE_UPDATE_UNUSED;
    extern GraphicsDeviceConstant<uint32> MAX_UPDATE_AFTER_BIND_DESCRIPTORS;

    extern GraphicsDeviceConstant<uint32> MAX_INDIRECT_DRAW_COUNT;

    //in nanoseconds
    extern GraphicsDeviceConstant<uint64> MAX_SYNC_RES_WAIT_TIME;

    // GBuffer's framebuffer sample count
    extern EngineGlobalConfig<uint32> GBUFFER_SAMPLE_COUNT;
    // Filtering for shader read textures of GBuffers
    extern EngineVar<uint32> GBUFFER_FILTERING;
}