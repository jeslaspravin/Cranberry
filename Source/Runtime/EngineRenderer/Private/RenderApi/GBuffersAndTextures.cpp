/*!
 * \file GBuffersAndTextures.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderApi/GBuffersAndTextures.h"
#include "Math/Math.h"
#include "IRenderInterfaceModule.h"
#include "RenderApi/RenderManager.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/CommandBuffer.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderApi/Rendering/RenderingContexts.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"
#include "Types/Platform/PlatformAssertionErrors.h"

//////////////////////////////////////////////////////////////////////////
// GBuffers
//////////////////////////////////////////////////////////////////////////

// clang-format off
std::unordered_map<ERenderPassFormat::Type, FramebufferFormat::AttachmentsFormatList> GlobalBuffers::GBUFFERS_ATTACHMENT_FORMATS{
    { ERenderPassFormat::Multibuffer            , { EPixelDataFormat::BGRA_U8_Norm, EPixelDataFormat::A2BGR10_U32_NormPacked, EPixelDataFormat::A2BGR10_U32_NormPacked, EPixelDataFormat::D24S8_U32_DNorm_SInt }},
    { ERenderPassFormat::Depth                  , { EPixelDataFormat::D24S8_U32_DNorm_SInt }},
    { ERenderPassFormat::PointLightDepth        , { EPixelDataFormat::D24S8_U32_DNorm_SInt }},
    { ERenderPassFormat::DirectionalLightDepth  , { EPixelDataFormat::D24S8_U32_DNorm_SInt }}
};
// clang-format on

ImageResourceRef GlobalBuffers::dummyBlackTexture;
ImageResourceRef GlobalBuffers::dummyWhiteTexture;
ImageResourceRef GlobalBuffers::dummyCubeTexture;
ImageResourceRef GlobalBuffers::dummyNormalTexture;
ImageResourceRef GlobalBuffers::dummyDepthTexture;

ImageResourceRef GlobalBuffers::integratedBRDF;

BufferResourceRef GlobalBuffers::quadTriVertsBuffer = nullptr;

SamplerRef GlobalBuffers::nearestFiltering = nullptr;
SamplerRef GlobalBuffers::linearFiltering = nullptr;
SamplerRef GlobalBuffers::depthFiltering = nullptr;
SamplerRef GlobalBuffers::shadowFiltering = nullptr;

std::pair<BufferResourceRef, BufferResourceRef> GlobalBuffers::quadRectVertsInds{ nullptr, nullptr };
std::pair<BufferResourceRef, BufferResourceRef> GlobalBuffers::lineGizmoVertsInds{ nullptr, nullptr };

bool FramebufferFormat::operator== (const FramebufferFormat &otherFormat) const
{
    bool isEqual = rpFormat == otherFormat.rpFormat;

    // If generic then check all attachments for equality
    if (isEqual && rpFormat == ERenderPassFormat::Generic)
    {
        if (otherFormat.attachments.size() == attachments.size())
        {
            isEqual = true;
            for (int32 index = 0; index < (int32)attachments.size(); ++index)
            {
                isEqual = isEqual && (attachments[index] == otherFormat.attachments[index]);
            }
        }
        else
        {
            isEqual = false;
        }
    }
    return isEqual;
}

bool FramebufferFormat::operator< (const FramebufferFormat &otherFormat) const
{
    if (rpFormat == otherFormat.rpFormat && rpFormat == ERenderPassFormat::Generic)
    {
        const int32 minFormatCount = int32(Math::min(attachments.size(), otherFormat.attachments.size()));
        for (int32 index = 0; index < minFormatCount; ++index)
        {
            if (attachments[index] != otherFormat.attachments[index])
            {
                return attachments[index] < otherFormat.attachments[index];
            }
        }
        return attachments.size() < otherFormat.attachments.size();
    }

    return rpFormat < otherFormat.rpFormat;
}

void GlobalBuffers::initialize()
{
    IRenderInterfaceModule *renderInterface = IRenderInterfaceModule::get();
    debugAssert(renderInterface);

    IRenderCommandList *cmdList = renderInterface->getRenderManager()->getRenderCmds();
    IGraphicsInstance *graphicsInstance = renderInterface->currentGraphicsInstance();
    const GraphicsHelperAPI *graphicsHelper = renderInterface->currentGraphicsHelper();

    createTextureCubes(cmdList, graphicsInstance, graphicsHelper);
    createTexture2Ds(cmdList, graphicsInstance, graphicsHelper);
    createVertIndBuffers(cmdList, graphicsInstance, graphicsHelper);
    createSamplers(cmdList, graphicsInstance, graphicsHelper);

    generateTexture2Ds(cmdList, graphicsInstance, graphicsHelper);
}

void GlobalBuffers::destroy()
{
    destroyTextureCubes();
    destroyTexture2Ds();
    destroyVertIndBuffers();
    destroySamplers();
}

GenericRenderPassProperties GlobalBuffers::getFramebufferRenderpassProps(ERenderPassFormat::Type renderpassFormat)
{
    GenericRenderPassProperties renderpassProps;
    renderpassProps.multisampleCount = EPixelSampleCount::Type(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
    renderpassProps.bOneRtPerFormat = renderpassProps.multisampleCount == EPixelSampleCount::SampleCount1;

    std::unordered_map<ERenderPassFormat::Type, FramebufferFormat::AttachmentsFormatList>::const_iterator attachmentFormatsItr
        = GBUFFERS_ATTACHMENT_FORMATS.find(renderpassFormat);
    debugAssert(attachmentFormatsItr != GBUFFERS_ATTACHMENT_FORMATS.cend());
    renderpassProps.renderpassAttachmentFormat.attachments = attachmentFormatsItr->second;
    renderpassProps.renderpassAttachmentFormat.rpFormat = attachmentFormatsItr->first;

    return renderpassProps;
}

const FramebufferFormat::AttachmentsFormatList &GlobalBuffers::getGBufferAttachmentFormat(ERenderPassFormat::Type renderpassFormat)
{
    std::unordered_map<ERenderPassFormat::Type, FramebufferFormat::AttachmentsFormatList>::const_iterator attachmentFormatsItr
        = GBUFFERS_ATTACHMENT_FORMATS.find(renderpassFormat);
    debugAssert(attachmentFormatsItr != GBUFFERS_ATTACHMENT_FORMATS.cend());
    return attachmentFormatsItr->second;
}

void GlobalBuffers::createTexture2Ds(IRenderCommandList *, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
{
    ImageResourceCreateInfo imageCI;
    imageCI.dimensions = UInt3(1, 1, 1);
    imageCI.imageFormat = EPixelDataFormat::BGRA_U8_Norm;
    imageCI.layerCount = imageCI.numOfMips = 1;
    dummyBlackTexture = graphicsHelper->createImage(graphicsInstance, imageCI);
    dummyBlackTexture->setResourceName(TCHAR("Dummy_Black"));

    dummyWhiteTexture = graphicsHelper->createImage(graphicsInstance, imageCI);
    dummyWhiteTexture->setResourceName(TCHAR("Dummy_White"));

    dummyNormalTexture = graphicsHelper->createImage(graphicsInstance, imageCI);
    dummyNormalTexture->setResourceName(TCHAR("Dummy_Normal"));

    imageCI.imageFormat = EPixelDataFormat::D_SF32;
    dummyDepthTexture = graphicsHelper->createImage(graphicsInstance, imageCI);
    dummyDepthTexture->setResourceName(TCHAR("Dummy_Depth"));

    if (GlobalRenderVariables::ENABLE_EXTENDED_STORAGES)
    {
        // TODO(Jeslas) : Create better read only LUT
        imageCI.imageFormat = EPixelDataFormat::RG_SF16;
        imageCI.dimensions = UInt3(GlobalRenderVariables::MAX_ENV_MAP_SIZE / 2u, GlobalRenderVariables::MAX_ENV_MAP_SIZE / 2u, 1);
        integratedBRDF = graphicsHelper->createImage(graphicsInstance, imageCI);
        integratedBRDF->setShaderUsage(EImageShaderUsage::Sampling | EImageShaderUsage::Writing);
        integratedBRDF->setResourceName(TCHAR("LUT_IntegratedBRDF"));
    }
    else
    {
        LOG_ERROR("GlobalBuffers", "Cannot create integrated BRDF LUT, RG_SF16 is not supported format");
        integratedBRDF = nullptr;
    }
}

void GlobalBuffers::generateTexture2Ds(
    IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    dummyWhiteTexture->init();
    dummyBlackTexture->init();
    dummyNormalTexture->init();
    dummyDepthTexture->init();
    integratedBRDF->init();
    cmdList->setupInitialLayout(integratedBRDF);

    LocalPipelineContext integrateBrdfContext;
    integrateBrdfContext.materialName = TCHAR("IntegrateBRDF_16x16x1");
    IRenderInterfaceModule::get()->getRenderManager()->preparePipelineContext(&integrateBrdfContext);
    ShaderParametersRef integrateBrdfParams
        = graphicsHelper->createShaderParameters(graphicsInstance, integrateBrdfContext.getPipeline()->getParamLayoutAtSet(0), {});
    integrateBrdfParams->setTextureParam(TCHAR("outIntegratedBrdf"), integratedBRDF);
    integrateBrdfParams->init();

    const GraphicsResource *cmdBuffer = cmdList->startCmd(TCHAR("IntegrateBRDF"), EQueueFunction::Graphics, false);
    cmdList->cmdBindComputePipeline(cmdBuffer, integrateBrdfContext);
    cmdList->cmdBindDescriptorsSets(cmdBuffer, integrateBrdfContext, { integrateBrdfParams });
    UInt3 subgrpSize = static_cast<const ComputeShaderConfig *>(integrateBrdfContext.getPipeline()->getShaderResource()->getShaderConfig())
                           ->getSubGroupSize();
    cmdList->cmdDispatch(cmdBuffer, integratedBRDF->getImageSize().x / subgrpSize.x, integratedBRDF->getImageSize().y / subgrpSize.y);
    cmdList->cmdTransitionLayouts(cmdBuffer, { &integratedBRDF, 1 });
    cmdList->endCmd(cmdBuffer);

    CommandSubmitInfo2 submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdList->submitCmd(EQueuePriority::High, submitInfo);

    Color pixelColor = ColorConst::BLACK;
    cmdList->copyToImage(dummyBlackTexture, { &pixelColor, 1 });
    pixelColor = ColorConst::WHITE;
    cmdList->copyToImage(dummyWhiteTexture, { &pixelColor, 1 });
    pixelColor = ColorConst::BLUE;
    cmdList->copyToImage(dummyNormalTexture, { &pixelColor, 1 });
    cmdList->copyToImage(dummyDepthTexture, { &LinearColorConst::BLACK, 1 }, { .extent = dummyDepthTexture->getImageSize() });

    cmdList->finishCmd(cmdBuffer);
    cmdList->freeCmd(cmdBuffer);
    integrateBrdfParams.reset();
}

void GlobalBuffers::destroyTexture2Ds()
{
    dummyBlackTexture.reset();
    dummyWhiteTexture.reset();
    dummyNormalTexture.reset();
    dummyDepthTexture.reset();

    integratedBRDF.reset();
}

void GlobalBuffers::createSamplers(IRenderCommandList *, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
{
    SamplerCreateInfo samplerCI{
        .filtering = ESamplerFiltering::Nearest,
        .mipFiltering = ESamplerFiltering::Nearest,
        .mipLodRange = ValueRange<float>{0, float(GlobalRenderVariables::MIN_SAMPLINE_MIP_LEVEL.get())},
        .resourceName = TCHAR("NearestSampler")
    };
    samplerCI.tilingMode = { ESamplerTilingMode::Repeat, ESamplerTilingMode::Repeat, ESamplerTilingMode::Repeat };

    nearestFiltering = graphicsHelper->createSampler(graphicsInstance, samplerCI);
    nearestFiltering->init();

    samplerCI.mipFiltering = samplerCI.filtering = ESamplerFiltering::Linear;
    samplerCI.resourceName = TCHAR("LinearSampler");
    linearFiltering = graphicsHelper->createSampler(graphicsInstance, samplerCI);
    linearFiltering->init();

    samplerCI.mipFiltering = samplerCI.filtering = ESamplerFiltering::Linear;
    samplerCI.tilingMode = { ESamplerTilingMode::BorderClamp, ESamplerTilingMode::BorderClamp, ESamplerTilingMode::BorderClamp };
    samplerCI.resourceName = TCHAR("DepthSampler");
    // Depth sampling must be nearest however there is better filtering when using linear filtering
    depthFiltering = graphicsHelper->createSampler(graphicsInstance, samplerCI);
    depthFiltering->init();

    // Has to lesser comparison since we want shadow to be 1.0 only if shading texel's depth is less that shadow depth texel
    // And lesser than gives 1.0(Shadowed) if shading depth is less than texel depth
    samplerCI.useCompareOp = 1;
    samplerCI.compareOp = CoreGraphicsTypes::ECompareOp::Less;
    samplerCI.resourceName = TCHAR("ShadowSampler");
    shadowFiltering = graphicsHelper->createSampler(graphicsInstance, samplerCI);
    shadowFiltering->init();
}

void GlobalBuffers::destroySamplers()
{
    nearestFiltering.reset();
    linearFiltering.reset();
    depthFiltering.reset();
    shadowFiltering.reset();
}

void GlobalBuffers::createTextureCubes(IRenderCommandList *, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
{
    ImageResourceCreateInfo imageCI;
    imageCI.dimensions = UInt3(1, 1, 1);
    imageCI.imageFormat = EPixelDataFormat::BGRA_U8_Norm;
    imageCI.layerCount = 6;
    imageCI.numOfMips = 1;
    dummyCubeTexture = graphicsHelper->createCubeImage(graphicsInstance, imageCI);
    dummyCubeTexture->setResourceName(TCHAR("DummyCubeMap"));
}

void GlobalBuffers::destroyTextureCubes() { dummyCubeTexture.reset(); }