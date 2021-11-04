#include "GlobalRenderVariables.h"
#include "CoreGraphicsTypes.h"

namespace GlobalRenderVariables
{
    EngineGlobalConfig<uint32> GBUFFER_SAMPLE_COUNT(EPixelSampleCount::SampleCount1);
    EngineVar<uint32> GBUFFER_FILTERING(ESamplerFiltering::Linear);
}
