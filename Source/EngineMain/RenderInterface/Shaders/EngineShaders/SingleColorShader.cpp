#include "SingleColorShader.h"
#include "../Base/DrawMeshShader.h"
#include "../../../Core/Types/CoreDefines.h"
#include "../../../RenderApi/GBuffersAndTextures.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../ShaderCore/ShaderParameterResources.h"

#define SINGLECOLOR_SHADER_NAME "SingleColor"

BEGIN_BUFFER_DEFINITION(SingleColorMeshData)
ADD_BUFFER_TYPED_FIELD(meshColor)
END_BUFFER_DEFINITION();

template<EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat>
class SingleColorShader : public DrawMeshShader
{
    DECLARE_GRAPHICS_RESOURCE(SingleColorShader, <ExpandArgs(VertexUsage, RenderpassFormat)>,DrawMeshShader,)
protected:
    SingleColorShader()
        : BaseType(SINGLECOLOR_SHADER_NAME)
    {
        compatibleRenderpassFormat = RenderpassFormat;
        compatibleVertex = VertexUsage;
    }

    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        static SingleColorMeshDataBufferParamInfo MESH_DATA;
        static const std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
        {
            { "meshData", &MESH_DATA }
        };


        for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

            debugAssert(foundDescBinding != bindingBuffers.end());

            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(SingleColorShader, <ExpandArgs(EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat)>
    , <ExpandArgs(VertexUsage, RenderpassFormat)>)

template class SingleColorShader<EVertexType::Simple2, ERenderPassFormat::Multibuffers>;
template class SingleColorShader<EVertexType::StaticMesh, ERenderPassFormat::Multibuffers>;

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

class SingleColorShaderPipeline : public GraphicsPipeline
{
    DECLARE_GRAPHICS_RESOURCE(SingleColorShaderPipeline,, GraphicsPipeline,)
private:
    SingleColorShaderPipeline() = default;
public:
    SingleColorShaderPipeline(const PipelineBase* parent);
    SingleColorShaderPipeline(const ShaderResource* shaderResource);
};

DEFINE_GRAPHICS_RESOURCE(SingleColorShaderPipeline)

SingleColorShaderPipeline::SingleColorShaderPipeline(const PipelineBase* parent)
    : BaseType(static_cast<const GraphicsPipelineBase*>(parent))
{}

SingleColorShaderPipeline::SingleColorShaderPipeline(const ShaderResource* shaderResource)
    : BaseType()
{
    setPipelineShader(shaderResource);
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

using SingleColorShaderPipelineRegistrar = GenericPipelineRegistrar<SingleColorShaderPipeline>;
SingleColorShaderPipelineRegistrar SINGLECOLOR_SHADER_PIPELINE_REGISTER(SINGLECOLOR_SHADER_NAME);

