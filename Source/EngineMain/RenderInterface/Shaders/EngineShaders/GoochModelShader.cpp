#include "GoochModelShader.h"
#include "../../../Core/Types/CoreDefines.h"
#include "../../../RenderApi/GBuffersAndTextures.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../ShaderCore/ShaderParameterResources.h"
#include "../../GlobalRenderVariables.h"
#include "../Base/UtilityShaders.h"
#include "../../../RenderApi/Scene/RenderScene.h"

BEGIN_BUFFER_DEFINITION(GoochModelLightCommon)
ADD_BUFFER_TYPED_FIELD(lightsCount)
ADD_BUFFER_TYPED_FIELD(invLightsCount)
END_BUFFER_DEFINITION();

BEGIN_BUFFER_DEFINITION(GoochModelLightData)
ADD_BUFFER_TYPED_FIELD(warmOffsetAndPosX)
ADD_BUFFER_TYPED_FIELD(coolOffsetAndPosY)
ADD_BUFFER_TYPED_FIELD(highlightColorAndPosZ)
ADD_BUFFER_TYPED_FIELD(lightColorAndRadius)
END_BUFFER_DEFINITION();

#define GOOCH_SHADER_NAME "GoochModel"

class GoochModelShader : public UniqueUtilityShader
{
    DECLARE_GRAPHICS_RESOURCE(GoochModelShader, , UniqueUtilityShader, )
protected:
    GoochModelShader()
        : BaseType(GOOCH_SHADER_NAME)
    {}
public:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        static GoochModelLightCommonBufferParamInfo LIGHTCOMMON_INFO;
        static GoochModelLightDataBufferParamInfo LIGHTDATA_INFO;
        static const std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
        {
            { "lightCommon", &LIGHTCOMMON_INFO },
            { "light", &LIGHTDATA_INFO },
            { "viewData", RenderSceneBase::sceneViewParamInfo().at("viewData") }
        };


        for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};

DEFINE_GRAPHICS_RESOURCE(GoochModelShader)

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

class GoochModelShaderPipeline : public GraphicsPipeline
{
    DECLARE_GRAPHICS_RESOURCE(GoochModelShaderPipeline, , GraphicsPipeline, )
private:
    GoochModelShaderPipeline() = default;
public:
    GoochModelShaderPipeline(const ShaderResource* shaderResource, const PipelineBase* parent);
    GoochModelShaderPipeline(const ShaderResource* shaderResource);
};

DEFINE_GRAPHICS_RESOURCE(GoochModelShaderPipeline)

GoochModelShaderPipeline::GoochModelShaderPipeline(const ShaderResource* shaderResource, const PipelineBase* parent)
    : BaseType(static_cast<const GraphicsPipelineBase*>(parent))
{}

GoochModelShaderPipeline::GoochModelShaderPipeline(const ShaderResource* shaderResource)
    : BaseType()
{
    supportedCullings.resize(1);
    supportedCullings[0] = ECullingMode::BackFace;

    allowedDrawModes.resize(2);
    allowedDrawModes[0] = EPolygonDrawMode::Fill;

    renderpassProps.bOneRtPerFormat = false;
    renderpassProps.multisampleCount = EPixelSampleCount::Type(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
    renderpassProps.renderpassAttachmentFormat.attachments.emplace_back(EPixelDataFormat::BGRA_U8_Norm);
    renderpassProps.renderpassAttachmentFormat.rpFormat = ERenderPassFormat::Generic;

    // No alpha based blending for default shaders
    AttachmentBlendState blendState;
    blendState.bBlendEnable = false;
    attachmentBlendStates.emplace_back(blendState);

    depthState.bEnableWrite = false;
    depthState.compareOp = CoreGraphicsTypes::ECompareOp::Always;
}

using GoochModelShaderPipelineRegister = GenericGraphicsPipelineRegister<GoochModelShaderPipeline>;
GoochModelShaderPipelineRegister GOOCHMODEL_SHADER_PIPELINE_REGISTER(GOOCH_SHADER_NAME);