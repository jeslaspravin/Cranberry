/*!
 * \file GlobalRenderVariables.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/CoreGraphicsTypes.h"

namespace GlobalRenderVariables
{
ProgramGlobalVar<uint32> GBUFFER_SAMPLE_COUNT(EPixelSampleCount::SampleCount1);
ProgramGlobalVar<uint32> GBUFFER_FILTERING(ESamplerFiltering::Linear);
} // namespace GlobalRenderVariables

namespace GlobalRenderVariables
{
ProgramGlobalVar<bool> ENABLE_ANISOTROPY;
ProgramGlobalVar<float> MAX_ANISOTROPY(0);

ProgramGlobalVar<bool> ENABLE_EXTENDED_STORAGES;
ProgramGlobalVar<bool> ENABLE_GEOMETRY_SHADERS;

// extern ProgramGlobalVar<bool> ENABLED_TESSELLATION;
ProgramGlobalVar<bool> ENABLE_NON_FILL_DRAWS;
ProgramGlobalVar<bool> ENABLE_WIDE_LINES;

ProgramGlobalVar<bool> ENABLED_RESOURCE_RUNTIME_ARRAY;
ProgramGlobalVar<bool> ENABLED_RESOURCE_UPDATE_AFTER_BIND;
ProgramGlobalVar<bool> ENABLED_RESOURCE_UPDATE_UNUSED;
ProgramGlobalVar<uint32> MAX_UPDATE_AFTER_BIND_DESCRIPTORS;

ProgramGlobalVar<uint32> MAX_INDIRECT_DRAW_COUNT;

ProgramGlobalVar<bool> ENABLED_TIMELINE_SEMAPHORE;
ProgramGlobalVar<uint64> MAX_TIMELINE_OFFSET(0);

ProgramGlobalVar<uint64> MAX_SYNC_RES_WAIT_TIME(500000000); /*500ms*/

/**
 *
 */
ProgramGlobalVar<bool> GPU_DEVICE_INITIALIZED{ false };
ProgramGlobalVar<bool> PRESENTING_ENABLED{ false };
ProgramGlobalVar<bool> GPU_IS_COMPUTE_ONLY{ false };

ProgramGlobalVar<uint32> MIN_SAMPLINE_MIP_LEVEL(10u);
ProgramGlobalVar<uint32> MAX_PREFILTERED_CUBE_MIPS(8u);
ProgramGlobalVar<uint32> MAX_ENV_MAP_SIZE(1024u);

ProgramGlobalVar<int32> PCF_KERNEL_SIZE(3);
ProgramGlobalVar<int32> POINT_LIGHT_PCF_KERNEL_SIZE(4);

ProgramGlobalVar<uint32> GLOBAL_SAMPLED_TEX_NUM(128u);
} // namespace GlobalRenderVariables
