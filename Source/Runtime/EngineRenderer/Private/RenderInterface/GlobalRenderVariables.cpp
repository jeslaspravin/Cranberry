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
} // namespace GlobalRenderVariables
