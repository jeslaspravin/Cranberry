#include "../RenderInterface/Shaders/Base/UtilityShaders.h"
#include "../RenderInterface/Shaders/Base/GenericComputePipeline.h"

#define TESTCOMPUTE_SHADER_NAME "TestCompute"

class TestComputeShader : public ComputeShader
{
    DECLARE_GRAPHICS_RESOURCE(TestComputeShader,, ComputeShader,)

public:
    TestComputeShader()
        : BaseType(Byte3D(16,16,1), TESTCOMPUTE_SHADER_NAME)
    {}
};

DEFINE_GRAPHICS_RESOURCE(TestComputeShader)


SimpleComputePipelineRegistrar TESTCOMPUTE_SHADER_PIPELINE_REGISTER(TESTCOMPUTE_SHADER_NAME);
