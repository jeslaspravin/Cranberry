#include "../../Base/UtilityShaders.h"
#include "../../Base/GenericComputePipeline.h"

#define HDRITOCUBE_SHADER_NAME "HDRIToCube"

class HDRIToCubeShader : public ComputeShaderTemplated<16, 16, 1>
{
    DECLARE_GRAPHICS_RESOURCE(HDRIToCubeShader,, ComputeShaderTemplated, <ExpandArgs(16, 16, 1)>)

public:
    HDRIToCubeShader()
        : BaseType(HDRITOCUBE_SHADER_NAME)
    { 
        static SimpleComputePipelineRegistrar HDRITOCUBE_SHADER_PIPELINE_REGISTER(getResourceName());
    }
};

DEFINE_GRAPHICS_RESOURCE(HDRIToCubeShader)

