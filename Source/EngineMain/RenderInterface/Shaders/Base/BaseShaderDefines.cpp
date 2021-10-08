#include "UtilityShaders.h"
#include "DrawMeshShader.h"
#include "ScreenspaceQuadGraphicsPipeline.h"
#include "GenericComputePipeline.h"
#include "../../../Core/Logger/Logger.h"
#include "../../../RenderApi/Scene/RenderScene.h"

DEFINE_GRAPHICS_RESOURCE(DrawMeshShader)

String DrawMeshShader::getShaderFileName() const
{
    return getResourceName() + EVertexType::toString(vertexUsage()) + ERenderPassFormat::toString(renderpassUsage());
}

void DrawMeshShader::getSpecializationConsts(std::map<String, struct SpecializationConstantEntry>& specializationConst) const
{
    BaseType::getSpecializationConsts(specializationConst);
    RenderSceneBase::sceneViewSpecConsts(specializationConst);
    EVertexType::vertexSpecConsts(vertexUsage(), specializationConst);
}

DEFINE_GRAPHICS_RESOURCE(UniqueUtilityShader)

EVertexType::Type UniqueUtilityShader::vertexUsage() const
{
    EVertexType::Type overridenType = vertexUsed();
    if (overridenType != EVertexType::MaxVertexType)
    {
        return overridenType;
    }

    if (getReflection()->inputs.empty())
    {
        return EVertexType::NoVertex;
    }
    else if (getReflection()->inputs.size() == 1)
    {
        switch (getReflection()->inputs[0].data.type.vecSize)
        {
        case 2:
            return EVertexType::Simple2;
        case 3:
            return EVertexType::Simple3;
        }
    }
    else if (getReflection()->inputs.size() == 2)
    {
        auto is3DColor = [this]()->bool
        {
            bool retVal = true;
            const EShaderInputAttribFormat::Type vertexAttribCheck[] = { EShaderInputAttribFormat::Float3, EShaderInputAttribFormat::Float4 };
            for (const ReflectInputOutput& refInput : getReflection()->inputs)
            {
                retVal = retVal && EShaderInputAttribFormat::getInputFormat(refInput.data.type) == vertexAttribCheck[refInput.data.location];
            }
            return retVal;
        };

        if (is3DColor())
        {
            return EVertexType::Simple3DColor;
        }
        return EVertexType::BasicMesh;
    }
    else if (getReflection()->inputs.size() == 3)
    {
        auto isUiVertex = [this]()->bool
        {
            bool retVal = true;
            const EShaderInputAttribFormat::Type UiAttribCheck[] = { EShaderInputAttribFormat::Float2, EShaderInputAttribFormat::Float2,EShaderInputAttribFormat::Float4 };
            for (const ReflectInputOutput& refInput : getReflection()->inputs)
            {
                retVal = retVal && EShaderInputAttribFormat::getInputFormat(refInput.data.type) == UiAttribCheck[refInput.data.location];
            }
            return retVal;
        };

        if (isUiVertex())
        {
            return EVertexType::UI;
        }
    }

    Logger::error("UniqueUtilityShader", "%s() : not supported vertex format for Utility shader %s", __func__, getResourceName().getChar());
    return EVertexType::Simple2;
}

DEFINE_GRAPHICS_RESOURCE(ComputeShader)

DEFINE_GRAPHICS_RESOURCE(SimpleComputePipeline)

SimpleComputePipeline::SimpleComputePipeline(const PipelineBase* parent)
    : BaseType(static_cast<const ComputePipeline*>(parent))
{}

SimpleComputePipeline::SimpleComputePipeline(const ShaderResource* shaderResource)
    : BaseType()
{
    setPipelineShader(shaderResource);
    setResourceName("SimpleComp_" + shaderResource->getResourceName());
}

DEFINE_GRAPHICS_RESOURCE(ScreenSpaceQuadShaderPipeline)

ScreenSpaceQuadShaderPipeline::ScreenSpaceQuadShaderPipeline(const PipelineBase* parent)
    : BaseType(static_cast<const GraphicsPipelineBase*>(parent))
{}

ScreenSpaceQuadShaderPipeline::ScreenSpaceQuadShaderPipeline(const ShaderResource* shaderResource)
    : BaseType()
{
    setPipelineShader(shaderResource);
    setResourceName("ScreenSpaceQuad_" + shaderResource->getResourceName());
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

DEFINE_GRAPHICS_RESOURCE(OverBlendedSSQuadShaderPipeline)

OverBlendedSSQuadShaderPipeline::OverBlendedSSQuadShaderPipeline(const ShaderResource* shaderResource)
    : BaseType(shaderResource)
{
    setResourceName("OverBlendedSSQuad_" + shaderResource->getResourceName());

    attachmentBlendStates[0].bBlendEnable = true;
    attachmentBlendStates[0].colorBlendOp = EBlendOp::Add;
    attachmentBlendStates[0].srcColorFactor = EBlendFactor::SrcAlpha;
    attachmentBlendStates[0].dstColorFactor = EBlendFactor::OneMinusSrcAlpha;
    attachmentBlendStates[0].alphaBlendOp = EBlendOp::Add;
    attachmentBlendStates[0].srcAlphaFactor = attachmentBlendStates[0].dstAlphaFactor = EBlendFactor::One;
}

DEFINE_GRAPHICS_RESOURCE(OverBlendedSSQuadWithDepthTestPipeline)
OverBlendedSSQuadWithDepthTestPipeline::OverBlendedSSQuadWithDepthTestPipeline(const ShaderResource* shaderResource)
    : BaseType(shaderResource)
{
    setResourceName("OverBlendedSSQuadDepthTested_" + shaderResource->getResourceName());
    // Just add depth attachment and disable depth write
    renderpassProps.renderpassAttachmentFormat.attachments.emplace_back(EPixelDataFormat::D24S8_U32_DNorm_SInt);

    depthState.bEnableWrite = false;
    depthState.compareOp = CoreGraphicsTypes::ECompareOp::Greater;
}