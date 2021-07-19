#include "TexturedShader.h"
#include "../Base/DrawMeshShader.h"
#include "../../../Core/Types/CoreDefines.h"
#include "../../../RenderApi/GBuffersAndTextures.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../ShaderCore/ShaderParameterResources.h"

#define TEXTURED_SHADER_NAME "Textured"

BEGIN_BUFFER_DEFINITION(TexturedMeshData)
ADD_BUFFER_TYPED_FIELD(meshColor)
ADD_BUFFER_TYPED_FIELD(rm_uvScale)
END_BUFFER_DEFINITION();

template<EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat>
class TexturedShader : public DrawMeshShader
{
    DECLARE_GRAPHICS_RESOURCE(TexturedShader, <ExpandArgs(VertexUsage, RenderpassFormat)>, DrawMeshShader, )
protected:
    TexturedShader()
        : BaseType(TEXTURED_SHADER_NAME)
    {
        compatibleRenderpassFormat = RenderpassFormat;
        compatibleVertex = VertexUsage;
    }

    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        static TexturedMeshDataBufferParamInfo MESH_DATA;
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

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(TexturedShader, <ExpandArgs(EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat)>
    , <ExpandArgs(VertexUsage, RenderpassFormat)>)

template class TexturedShader<EVertexType::StaticMesh, ERenderPassFormat::Multibuffers>;

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

class TexturedShaderPipeline : public GraphicsPipeline
{
    DECLARE_GRAPHICS_RESOURCE(TexturedShaderPipeline, , GraphicsPipeline, )
private:
    TexturedShaderPipeline() = default;
public:
    TexturedShaderPipeline(const PipelineBase* parent);
    TexturedShaderPipeline(const ShaderResource* shaderResource);
};

DEFINE_GRAPHICS_RESOURCE(TexturedShaderPipeline)

TexturedShaderPipeline::TexturedShaderPipeline(const PipelineBase* parent)
    : BaseType(static_cast<const GraphicsPipelineBase*>(parent))
{}

TexturedShaderPipeline::TexturedShaderPipeline(const ShaderResource* shaderResource)
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
    GlobalBuffers::getFramebuffer(fbFormat, 0);
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

using TexturedShaderPipelineRegistrar = GenericPipelineRegistrar<TexturedShaderPipeline>;
TexturedShaderPipelineRegistrar TEXTURED_SHADER_PIPELINE_REGISTER(TEXTURED_SHADER_NAME);
