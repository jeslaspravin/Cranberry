/*!
 * \file GlobalRenderVariables.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Config/ProgramVarTypes.h"
#include "EngineRendererExports.h"
#include "Types/CoreTypes.h"

class Vector2;

namespace GlobalRenderVariables
{

/**
 * Device specific initialization variables
 */
extern ENGINERENDERER_EXPORT ProgramGlobalVar<bool> ENABLE_ANISOTROPY;
extern ENGINERENDERER_EXPORT ProgramGlobalVar<float> MAX_ANISOTROPY;

extern ENGINERENDERER_EXPORT ProgramGlobalVar<bool> ENABLE_EXTENDED_STORAGES;
extern ENGINERENDERER_EXPORT ProgramGlobalVar<bool> ENABLE_GEOMETRY_SHADERS;

// extern ENGINERENDERER_EXPORT GraphicsDeviceConstant<bool> ENABLED_TESSELLATION;
extern ENGINERENDERER_EXPORT ProgramGlobalVar<bool> ENABLE_NON_FILL_DRAWS;
extern ENGINERENDERER_EXPORT ProgramGlobalVar<bool> ENABLE_WIDE_LINES;

extern ENGINERENDERER_EXPORT ProgramGlobalVar<bool> ENABLED_TIMELINE_SEMAPHORE;
extern ENGINERENDERER_EXPORT ProgramGlobalVar<uint64> MAX_TIMELINE_OFFSET;

extern ENGINERENDERER_EXPORT ProgramGlobalVar<bool> ENABLED_RESOURCE_RUNTIME_ARRAY;
extern ENGINERENDERER_EXPORT ProgramGlobalVar<bool> ENABLED_RESOURCE_UPDATE_AFTER_BIND;
extern ENGINERENDERER_EXPORT ProgramGlobalVar<bool> ENABLED_RESOURCE_UPDATE_UNUSED;
extern ENGINERENDERER_EXPORT ProgramGlobalVar<uint32> MAX_UPDATE_AFTER_BIND_DESCRIPTORS;

extern ENGINERENDERER_EXPORT ProgramGlobalVar<uint32> MAX_INDIRECT_DRAW_COUNT;

// in nanoseconds
extern ENGINERENDERER_EXPORT ProgramGlobalVar<uint64> MAX_SYNC_RES_WAIT_TIME;

// GBuffer's framebuffer sample count
extern ENGINERENDERER_EXPORT ProgramGlobalVar<uint32> GBUFFER_SAMPLE_COUNT;
// Filtering for shader read textures of GBuffers
extern ENGINERENDERER_EXPORT ProgramGlobalVar<uint32> GBUFFER_FILTERING;

/**
 * Rendered specific initialization variables
 */
extern ENGINERENDERER_EXPORT ProgramGlobalVar<bool> GPU_DEVICE_INITIALIZED;
extern ENGINERENDERER_EXPORT ProgramGlobalVar<bool> PRESENTING_ENABLED;
extern ENGINERENDERER_EXPORT ProgramGlobalVar<bool> GPU_IS_COMPUTE_ONLY;

extern ENGINERENDERER_EXPORT ProgramGlobalVar<uint32> MIN_SAMPLINE_MIP_LEVEL;
extern ENGINERENDERER_EXPORT ProgramGlobalVar<uint32> MAX_PREFILTERED_CUBE_MIPS;
extern ENGINERENDERER_EXPORT ProgramGlobalVar<uint32> MAX_ENV_MAP_SIZE;

extern ENGINERENDERER_EXPORT ProgramGlobalVar<int32> PCF_KERNEL_SIZE;
extern ENGINERENDERER_EXPORT ProgramGlobalVar<int32> POINT_LIGHT_PCF_KERNEL_SIZE;

// Bindless/descriptor indexing texture count for globalSampledTexs field
extern ENGINERENDERER_EXPORT ProgramGlobalVar<uint32> GLOBAL_SAMPLED_TEX_NUM;

} // namespace GlobalRenderVariables