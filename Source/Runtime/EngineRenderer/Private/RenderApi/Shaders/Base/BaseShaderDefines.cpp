/*!
 * \file BaseShaderDefines.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Logger/Logger.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderApi/Scene/RenderScene.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderApi/Shaders/Base/DrawMeshShader.h"
#include "RenderApi/Shaders/Base/ScreenspaceQuadGraphicsPipeline.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"
#include "ShaderReflected.h"

DEFINE_GRAPHICS_RESOURCE(DrawMeshShaderConfig)

String DrawMeshShaderConfig::getShaderFileName() const
{
    return getResourceName() + EVertexType::toString(vertexUsage()) + ERenderPassFormat::toString(renderpassUsage());
}

void DrawMeshShaderConfig::getSpecializationConsts(std::map<String, struct SpecializationConstantEntry> &specializationConst) const
{
    BaseType::getSpecializationConsts(specializationConst);
    RenderSceneBase::sceneViewSpecConsts(specializationConst);
    EVertexType::vertexSpecConsts(vertexUsage(), specializationConst);
}

DEFINE_GRAPHICS_RESOURCE(UniqueUtilityShaderConfig)

EVertexType::Type UniqueUtilityShaderConfig::vertexUsage() const
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
        auto is3DColor = [this]() -> bool
        {
            bool retVal = true;
            const EShaderInputAttribFormat::Type vertexAttribCheck[] = { EShaderInputAttribFormat::Float3, EShaderInputAttribFormat::Float4 };
            for (const ReflectInputOutput &refInput : getReflection()->inputs)
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
        auto isUiVertex = [this]() -> bool
        {
            bool retVal = true;
            const EShaderInputAttribFormat::Type UiAttribCheck[]
                = { EShaderInputAttribFormat::Float2, EShaderInputAttribFormat::Float2, EShaderInputAttribFormat::Float4 };
            for (const ReflectInputOutput &refInput : getReflection()->inputs)
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

    LOG_ERROR("UniqueUtilityShader", "not supported vertex format for Utility shader %s", getResourceName().getChar());
    return EVertexType::Simple2;
}

DEFINE_GRAPHICS_RESOURCE(ComputeShaderConfig)

namespace ScreenSpaceQuadPipelineConfigs
{
GraphicsPipelineConfig screenSpaceQuadConfig(String &pipelineName, const ShaderResource *shaderResource)
{
    pipelineName = TCHAR("ScreenSpaceQuad_") + shaderResource->getResourceName();
    GraphicsPipelineConfig config;
    config.supportedCullings.emplace_back(ECullingMode::BackFace);
    config.allowedDrawModes.emplace_back(EPolygonDrawMode::Fill);

    config.renderpassProps.bOneRtPerFormat = true;
    config.renderpassProps.multisampleCount = EPixelSampleCount::SampleCount1;
    config.renderpassProps.renderpassAttachmentFormat.attachments.emplace_back(EPixelDataFormat::BGRA_U8_Norm);
    config.renderpassProps.renderpassAttachmentFormat.rpFormat = ERenderPassFormat::Generic;

    config.depthState.bEnableWrite = false;
    config.depthState.compareOp = CoreGraphicsTypes::ECompareOp::Always;

    AttachmentBlendState blendState;
    blendState.bBlendEnable = false;
    config.attachmentBlendStates.emplace_back(blendState);

    return config;
}

GraphicsPipelineConfig screenSpaceQuadOverBlendConfig(String &pipelineName, const ShaderResource *shaderResource)
{
    GraphicsPipelineConfig config = screenSpaceQuadConfig(pipelineName, shaderResource);

    pipelineName = TCHAR("OverBlendedSSQuad_") + shaderResource->getResourceName();
    config.attachmentBlendStates[0].bBlendEnable = true;
    config.attachmentBlendStates[0].colorBlendOp = EBlendOp::Add;
    config.attachmentBlendStates[0].srcColorFactor = EBlendFactor::SrcAlpha;
    config.attachmentBlendStates[0].dstColorFactor = EBlendFactor::OneMinusSrcAlpha;
    config.attachmentBlendStates[0].alphaBlendOp = EBlendOp::Add;
    config.attachmentBlendStates[0].srcAlphaFactor = config.attachmentBlendStates[0].dstAlphaFactor = EBlendFactor::One;

    return config;
}

GraphicsPipelineConfig screenSpaceQuadOverBlendDepthTestedShaderConfig(String &pipelineName, const ShaderResource *shaderResource)
{
    GraphicsPipelineConfig config = screenSpaceQuadOverBlendConfig(pipelineName, shaderResource);

    pipelineName = TCHAR("OverBlendedSSQuadDepthTested_") + shaderResource->getResourceName();
    // Just add depth attachment and disable depth write
    config.renderpassProps.renderpassAttachmentFormat.attachments.emplace_back(EPixelDataFormat::D24S8_U32_DNorm_SInt);

    config.depthState.bEnableWrite = false;
    config.depthState.compareOp = CoreGraphicsTypes::ECompareOp::Greater;

    return config;
}
} // namespace ScreenSpaceQuadPipelineConfigs

namespace CommonGraphicsPipelineConfigs
{
GraphicsPipelineConfig writeGbufferShaderConfig(String &pipelineName, const ShaderResource *shaderResource)
{
    pipelineName = shaderResource->getResourceName();

    GraphicsPipelineConfig config;

    config.supportedCullings.resize(2);
    config.supportedCullings[0] = ECullingMode::FrontFace;
    config.supportedCullings[1] = ECullingMode::BackFace;

    config.allowedDrawModes.resize(2);
    config.allowedDrawModes[0] = EPolygonDrawMode::Fill;
    config.allowedDrawModes[1] = EPolygonDrawMode::Line;

    // No alpha based blending for default shaders
    AttachmentBlendState blendState;
    blendState.bBlendEnable = false;

    bool bHasDepth = false;
    FramebufferFormat fbFormat = GlobalBuffers ::getFramebufferRenderpassProps(
                                     static_cast<const DrawMeshShaderConfig *>(shaderResource->getShaderConfig())->renderpassUsage()
    )
                                     .renderpassAttachmentFormat;
    config.attachmentBlendStates.reserve(fbFormat.attachments.size());
    for (EPixelDataFormat::Type attachmentFormat : fbFormat.attachments)
    {
        if (!EPixelDataFormat::isDepthFormat(attachmentFormat))
        {
            config.attachmentBlendStates.emplace_back(blendState);
        }
        else
        {
            bHasDepth = true;
        }
    }

    config.depthState.bEnableWrite = bHasDepth;
    return config;
}
} // namespace CommonGraphicsPipelineConfigs