#include "UtilityShaders.h"
#include "DrawMeshShader.h"
#include "ScreenspaceQuadGraphicsPipeline.h"
#include "../../../Core/Logger/Logger.h"

DEFINE_GRAPHICS_RESOURCE(DrawMeshShader)

String DrawMeshShader::getShaderFileName() const
{
    return getResourceName() + EVertexType::toString(vertexUsage()) + ERenderPassFormat::toString(renderpassUsage());
}


DEFINE_GRAPHICS_RESOURCE(UniqueUtilityShader)

EVertexType::Type UniqueUtilityShader::vertexUsage() const
{
    // Since at the moment only planning to have either simple or basic vertices only
    if (getReflection()->inputs.size() == 1)
    {
        switch (getReflection()->inputs[0].data.type.vecSize)
        {
        case 2:
            return EVertexType::Simple2;
        case 3:
            return EVertexType::Simple3;
        case 4:
            return EVertexType::Simple4;
        }
    }
    else if (getReflection()->inputs.size() == 2)
    {
        return EVertexType::BasicMesh;
    }

    Logger::error("UniqueUtilityShader", "%s : not supported vertex format for Utility shader");
    return EVertexType::Simple2;
}


DEFINE_GRAPHICS_RESOURCE(ScreenSpaceQuadShaderPipeline)

ScreenSpaceQuadShaderPipeline::ScreenSpaceQuadShaderPipeline(const ShaderResource* shaderResource, const PipelineBase* parent)
    : BaseType(static_cast<const GraphicsPipelineBase*>(parent))
{}

ScreenSpaceQuadShaderPipeline::ScreenSpaceQuadShaderPipeline(const ShaderResource* shaderResource)
    : BaseType()
{
    supportedCullings.emplace_back(ECullingMode::BackFace);
    allowedDrawModes.emplace_back(EPolygonDrawMode::Fill);

    renderpassProps.bOneRtPerFormat = true;
    renderpassProps.multisampleCount = EPixelSampleCount::SampleCount1;
    renderpassProps.renderpassAttachmentFormat.attachments.emplace_back(EPixelDataFormat::BGRA_U8_Norm);
    renderpassProps.renderpassAttachmentFormat.rpFormat = ERenderPassFormat::Generic;

    depthState.bEnableWrite = false;
    depthState.compareOp = CoreGraphicsTypes::ECompareOp::Always;

    AttachmentBlendState blendState;
    blendState.bBlendEnable = false;
    attachmentBlendStates.emplace_back(blendState);
}
