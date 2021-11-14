#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/CoreGraphicsTypes.h"

namespace GlobalRenderVariables
{
    EngineGlobalConfig<uint32> GBUFFER_SAMPLE_COUNT(EPixelSampleCount::SampleCount1);
    EngineGlobalConfig<uint32> GBUFFER_FILTERING(ESamplerFiltering::Linear);
}

namespace GlobalRenderVariables
{
    EngineGlobalConfig<bool> ENABLE_ANISOTROPY;
    EngineGlobalConfig<float> MAX_ANISOTROPY(0);

    EngineGlobalConfig<bool> ENABLE_EXTENDED_STORAGES;
    EngineGlobalConfig<bool> ENABLE_GEOMETRY_SHADERS;

    //extern EngineGlobalConfig<bool> ENABLED_TESSELLATION;
    EngineGlobalConfig<bool> ENABLE_NON_FILL_DRAWS;
    EngineGlobalConfig<bool> ENABLE_WIDE_LINES;

    EngineGlobalConfig<bool> ENABLED_RESOURCE_RUNTIME_ARRAY;
    EngineGlobalConfig<bool> ENABLED_RESOURCE_UPDATE_AFTER_BIND;
    EngineGlobalConfig<bool> ENABLED_RESOURCE_UPDATE_UNUSED;
    EngineGlobalConfig<uint32> MAX_UPDATE_AFTER_BIND_DESCRIPTORS;

    EngineGlobalConfig<uint32> MAX_INDIRECT_DRAW_COUNT;

    EngineGlobalConfig<bool> ENABLED_TIMELINE_SEMAPHORE;
    EngineGlobalConfig<uint64> MAX_TIMELINE_OFFSET(0);

    EngineGlobalConfig<uint64> MAX_SYNC_RES_WAIT_TIME(500000000);/*500ms*/
}
