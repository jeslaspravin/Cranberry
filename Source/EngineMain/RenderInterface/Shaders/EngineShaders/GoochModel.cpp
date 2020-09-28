#include "GoochModel.h"
#include "../Base/DrawMeshShader.h"
#include "../../../Core/Types/CoreDefines.h"
#include "../../../RenderApi/GBuffersAndTextures.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../ShaderCore/ShaderParameterResources.h"

BEGIN_BUFFER_DEFINITION(SurfaceData)
ADD_BUFFER_TYPED_FIELD(lightPos)
ADD_BUFFER_TYPED_FIELD(highlightColor)
ADD_BUFFER_TYPED_FIELD(surfaceColor)
END_BUFFER_DEFINITION();

#define GOOCH_SHADER_NAME "GoochModel"

template<EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat>
class GoochModelShader : public DrawMeshShader
{
    DECLARE_GRAPHICS_RESOURCE(GoochModelShader, <ExpandArgs(VertexUsage, RenderpassFormat)>, DrawMeshShader, )
protected:
    GoochModelShader()
        : BaseType(GOOCH_SHADER_NAME)
    {
        compatibleRenderpassFormat = RenderpassFormat;
        compatibleVertex = VertexUsage;
    }
public:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        static SurfaceDataBufferParamInfo SURFACE_DATA_INFO;
        static const std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
        {
            { "surfaceData", &SURFACE_DATA_INFO }
        };

        for (const std::pair<String, ShaderBufferParamInfo*>& bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(GoochModelShader, <ExpandArgs(EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat)>
    , <ExpandArgs(VertexUsage, RenderpassFormat)>)

template class GoochModelShader<EVertexType::StaticMesh, ERenderPassFormat::Multibuffers>;

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
    supportedCullings.resize(2);
    supportedCullings[0] = ECullingMode::FrontFace;
    supportedCullings[1] = ECullingMode::BackFace;

    allowedDrawModes.resize(2);
    allowedDrawModes[0] = EPolygonDrawMode::Fill;
    allowedDrawModes[1] = EPolygonDrawMode::Line;

    // No alpha based blending for default shaders
    AttachmentBlendState blendState;
    blendState.bBlendEnable = false;

    bool bHasDepth = false;
    FramebufferFormat fbFormat(static_cast<const DrawMeshShader*>(shaderResource)->renderpassUsage());
    GBuffers::getFramebuffer(fbFormat, 0);
    attachmentBlendStates.reserve(fbFormat.attachments.size());
    for (EPixelDataFormat::Type attachmentFormat : fbFormat.attachments)
    {
        if (!EPixelDataFormat::isDepthFormat(attachmentFormat))
        {
            attachmentBlendStates.emplace_back(blendState);
        }
        else
        {
            bHasDepth = true;
        }
    }

    depthState.bEnableWrite = bHasDepth;
}

using GoochModelShaderPipelineRegister = GenericGraphicsPipelineRegister<GoochModelShaderPipeline>;
GoochModelShaderPipelineRegister GOOCHMODEL_SHADER_PIPELINE_REGISTER(GOOCH_SHADER_NAME);