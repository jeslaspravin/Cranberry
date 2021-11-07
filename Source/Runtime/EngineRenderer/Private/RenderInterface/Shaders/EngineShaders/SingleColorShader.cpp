#include "RenderInterface/Shaders/EngineShaders/SingleColorShader.h"
#include "RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "Types/CoreDefines.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"

#define SINGLECOLOR_SHADER_NAME "SingleColor"

struct SingleColorMeshMaterials
{
    SingleColorMeshData* meshData;
};

BEGIN_BUFFER_DEFINITION(SingleColorMeshData)
ADD_BUFFER_TYPED_FIELD(meshColor)
ADD_BUFFER_TYPED_FIELD(roughness)
ADD_BUFFER_TYPED_FIELD(metallic)
END_BUFFER_DEFINITION();

BEGIN_BUFFER_DEFINITION(SingleColorMeshMaterials)
ADD_BUFFER_STRUCT_FIELD(meshData, SingleColorMeshData)
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
        static SingleColorMeshMaterialsBufferParamInfo MESH_DATA_MATERIALS;
        static const std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
        {
            { "materials", &MESH_DATA_MATERIALS }
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

template class SingleColorShader<EVertexType::Simple2, ERenderPassFormat::Multibuffer>;
template class SingleColorShader<EVertexType::StaticMesh, ERenderPassFormat::Multibuffer>;

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
    setResourceName(shaderResource->getResourceName());
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
    FramebufferFormat fbFormat = GlobalBuffers
        ::getFramebufferRenderpassProps(static_cast<const DrawMeshShader*>(shaderResource)->renderpassUsage()).renderpassAttachmentFormat;
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
