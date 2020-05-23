#include "GlobalRenderVariables.h"
#include "CoreGraphicsTypes.h"

namespace GlobalRenderVariables
{
    EngineGlobalConfig<uint32> FRAME_BUFFER_SAMPLE_COUNT(EPixelSampleCount::SampleCount8);
}