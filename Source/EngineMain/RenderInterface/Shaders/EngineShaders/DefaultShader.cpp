#include "../Base/DrawMeshShader.h"
#include "../../../Core/Types/CoreDefines.h"
#include "../../../RenderApi/GBuffersAndTextures.h"

template<EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat>
class DefaultShader : public DrawMeshShader
{
    DECLARE_GRAPHICS_RESOURCE(DefaultShader, <ExpandArgs(VertexUsage, RenderpassFormat)>,DrawMeshShader,)
protected:
    DefaultShader()
        : BaseType(DEFAULT_SHADER_NAME)
    {
        compatibleRenderpassFormat = RenderpassFormat;
        compatibleVertex = VertexUsage;
    }
};

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(DefaultShader, <ExpandArgs(EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat)>
    , <ExpandArgs(VertexUsage, RenderpassFormat)>)

template class DefaultShader<EVertexType::Simple2, ERenderPassFormat::Multibuffers>;
template class DefaultShader<EVertexType::StaticMesh, ERenderPassFormat::Multibuffers>;

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

class DefaultShaderPipeline : public GraphicsPipeline
{
    DECLARE_GRAPHICS_RESOURCE(DefaultShaderPipeline,, GraphicsPipeline,)
private:
    DefaultShaderPipeline() = default;
public:
    DefaultShaderPipeline(const ShaderResource* shaderResource, const PipelineBase* parent);
    DefaultShaderPipeline(const ShaderResource* shaderResource);
};

DEFINE_GRAPHICS_RESOURCE(DefaultShaderPipeline)

DefaultShaderPipeline::DefaultShaderPipeline(const ShaderResource* shaderResource, const PipelineBase* parent)
    : BaseType(static_cast<const GraphicsPipelineBase*>(parent))
{}

DefaultShaderPipeline::DefaultShaderPipeline(const ShaderResource* shaderResource)
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

using DefaultShaderPipelineRegister = GenericGraphicsPipelineRegister<DefaultShaderPipeline>;
DefaultShaderPipelineRegister DEFAULT_SHADER_PIPELINE_REGISTER(DEFAULT_SHADER_NAME);
