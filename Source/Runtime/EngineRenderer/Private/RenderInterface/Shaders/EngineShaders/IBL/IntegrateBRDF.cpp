#include "RenderInterface/Shaders/Base/UtilityShaders.h"
#include "RenderInterface/Shaders/Base/ScreenspaceQuadGraphicsPipeline.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "ShaderDataTypes.h"

#define SAMPLE_COUNT "SAMPLE_COUNT"
#define INTEGRATEBRDF_SHADER_NAME "IntegrateBRDF"

class IntegrateBRDFShader final : public ComputeShaderConfigTemplated<16, 16, 1>
{
    DECLARE_GRAPHICS_RESOURCE(IntegrateBRDFShader, , ComputeShaderConfigTemplated, <EXPAND_ARGS(16, 16, 1)>)

public:
    IntegrateBRDFShader()
        : BaseType(INTEGRATEBRDF_SHADER_NAME)
    {
        static ComputePipelineFactoryRegistrant INTEGRATEBRDF_SHADER_PIPELINE_REGISTER(getResourceName());
    }
    
    void getSpecializationConsts(std::map<String, struct SpecializationConstantEntry>& specializationConst) const final
    {
        specializationConst[SAMPLE_COUNT] = SpecializationConstUtility::fromValue(1024u);
    }
};

DEFINE_GRAPHICS_RESOURCE(IntegrateBRDFShader)

#define DRAWINTEGRATEBRDF_SHADER_NAME "DrawIntegrateBRDF"

class DrawIntegrateBRDFShader : public UniqueUtilityShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(DrawIntegrateBRDFShader, , UniqueUtilityShaderConfig, );
private:
    DrawIntegrateBRDFShader();
public:
    void getSpecializationConsts(std::map<String, struct SpecializationConstantEntry>& specializationConst) const final
    {
        specializationConst[SAMPLE_COUNT] = SpecializationConstUtility::fromValue(1024u);
    }
};

DEFINE_GRAPHICS_RESOURCE(DrawIntegrateBRDFShader)

DrawIntegrateBRDFShader::DrawIntegrateBRDFShader()
    : BaseType(DRAWINTEGRATEBRDF_SHADER_NAME)
{}

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

// Registrar
CREATE_GRAPHICS_PIPELINE_REGISTRANT(DRAWINTEGRATEBRDF_PIPELINE_REGISTER, DRAWINTEGRATEBRDF_SHADER_NAME, &ScreenSpaceQuadPipelineConfigs::screenSpaceQuadConfig);