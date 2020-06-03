#include "GlobalRenderVariables.h"
#include "CoreGraphicsTypes.h"

namespace GlobalRenderVariables
{
    EngineGlobalConfig<uint32> FRAME_BUFFER_SAMPLE_COUNT(EPixelSampleCount::SampleCount1);// Right now GBuffers do not support more than 1 sample count
}
