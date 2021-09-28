#include "../Base/DrawMeshShader.h"
#include "../../../Core/Types/CoreDefines.h"
#include "../../../RenderApi/GBuffersAndTextures.h"
#include "ShadowDepthDraw.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../ShaderCore/ShaderParameterResources.h"

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
public:
    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const override
    {
        if constexpr (RenderpassFormat != ERenderPassFormat::DirectionalLightDepth && RenderpassFormat != ERenderPassFormat::PointLightDepth)
        {
            return;
        }

        const std::map<String, ShaderBufferParamInfo*>* paramInfo = nullptr;
        if constexpr(RenderpassFormat == ERenderPassFormat::DirectionalLightDepth)
        {
            static const std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
            {
                { "lightViews",  DirectionalShadowCascadeViews::paramInfo() }
            };

            paramInfo = &SHADER_PARAMS_INFO;
        }
        else if constexpr (RenderpassFormat == ERenderPassFormat::PointLightDepth)
        {
            static const std::map<String, ShaderBufferParamInfo*> SHADER_PARAMS_INFO
            {
                { "lightViews",  PointShadowDepthViews::paramInfo() }
            };

            paramInfo = &SHADER_PARAMS_INFO;
        }

        if (paramInfo != nullptr)
        {
            for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : *paramInfo)
            {
                auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

                debugAssert(foundDescBinding != bindingBuffers.end());

                foundDescBinding->second->bufferParamInfo = bufferInfo.second;
            }
        }
    }
};

DEFINE_TEMPLATED_GRAPHICS_RESOURCE(DefaultShader, <ExpandArgs(EVertexType::Type VertexUsage, ERenderPassFormat::Type RenderpassFormat)>
    , <ExpandArgs(VertexUsage, RenderpassFormat)>)

template class DefaultShader<EVertexType::Simple2, ERenderPassFormat::Multibuffer>;

template class DefaultShader<EVertexType::StaticMesh, ERenderPassFormat::Multibuffer>;
template class DefaultShader<EVertexType::StaticMesh, ERenderPassFormat::Depth>;
template class DefaultShader<EVertexType::StaticMesh, ERenderPassFormat::PointLightDepth>;
template class DefaultShader<EVertexType::StaticMesh, ERenderPassFormat::DirectionalLightDepth>;

//////////////////////////////////////////////////////////////////////////
/// Pipeline registration
//////////////////////////////////////////////////////////////////////////

class DefaultShaderPipeline : public GraphicsPipeline
{
    DECLARE_GRAPHICS_RESOURCE(DefaultShaderPipeline,, GraphicsPipeline,)
private:
    DefaultShaderPipeline() = default;
public:
    DefaultShaderPipeline(const PipelineBase* parent);
    DefaultShaderPipeline(const ShaderResource* shaderResource);
};

DEFINE_GRAPHICS_RESOURCE(DefaultShaderPipeline)

DefaultShaderPipeline::DefaultShaderPipeline(const PipelineBase* parent)
    : BaseType(static_cast<const GraphicsPipelineBase*>(parent))
{}

DefaultShaderPipeline::DefaultShaderPipeline(const ShaderResource* shaderResource)
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

using DefaultShaderPipelineRegistrar = GenericPipelineRegistrar<DefaultShaderPipeline>;
DefaultShaderPipelineRegistrar DEFAULT_SHADER_PIPELINE_REGISTER(DEFAULT_SHADER_NAME);

