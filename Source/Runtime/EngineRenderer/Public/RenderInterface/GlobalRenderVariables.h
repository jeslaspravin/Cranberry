/*!
 * \file GlobalRenderVariables.h
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
#include "Types/CoreTypes.h"
#include "EngineRendererExports.h"

class Vector2D;

namespace GlobalRenderVariables
{
    extern ENGINERENDERER_EXPORT EngineGlobalConfig<bool> ENABLE_ANISOTROPY;
    extern ENGINERENDERER_EXPORT EngineGlobalConfig<float> MAX_ANISOTROPY;

    extern ENGINERENDERER_EXPORT EngineGlobalConfig<bool> ENABLE_EXTENDED_STORAGES;
    extern ENGINERENDERER_EXPORT EngineGlobalConfig<bool> ENABLE_GEOMETRY_SHADERS;

    //extern ENGINERENDERER_EXPORT GraphicsDeviceConstant<bool> ENABLED_TESSELLATION;
    extern ENGINERENDERER_EXPORT EngineGlobalConfig<bool> ENABLE_NON_FILL_DRAWS;
    extern ENGINERENDERER_EXPORT EngineGlobalConfig<bool> ENABLE_WIDE_LINES;

    extern ENGINERENDERER_EXPORT EngineGlobalConfig<bool> ENABLED_TIMELINE_SEMAPHORE;
    extern ENGINERENDERER_EXPORT EngineGlobalConfig<uint64> MAX_TIMELINE_OFFSET;

    extern ENGINERENDERER_EXPORT EngineGlobalConfig<bool> ENABLED_RESOURCE_RUNTIME_ARRAY;
    extern ENGINERENDERER_EXPORT EngineGlobalConfig<bool> ENABLED_RESOURCE_UPDATE_AFTER_BIND;
    extern ENGINERENDERER_EXPORT EngineGlobalConfig<bool> ENABLED_RESOURCE_UPDATE_UNUSED;
    extern ENGINERENDERER_EXPORT EngineGlobalConfig<uint32> MAX_UPDATE_AFTER_BIND_DESCRIPTORS;

    extern ENGINERENDERER_EXPORT EngineGlobalConfig<uint32> MAX_INDIRECT_DRAW_COUNT;

    //in nanoseconds
    extern ENGINERENDERER_EXPORT EngineGlobalConfig<uint64> MAX_SYNC_RES_WAIT_TIME;

    // GBuffer's framebuffer sample count
    extern ENGINERENDERER_EXPORT EngineGlobalConfig<uint32> GBUFFER_SAMPLE_COUNT;
    // Filtering for shader read textures of GBuffers
    extern ENGINERENDERER_EXPORT EngineGlobalConfig<uint32> GBUFFER_FILTERING;
}