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
#include "Engine/Config/EngineGlobalConfigs.h"
#include "Math/Math.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/CommandBuffer.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/Rendering/RenderingContexts.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "RenderInterface/Shaders/Base/UtilityShaders.h"
#include "Types/Platform/PlatformAssertionErrors.h"

//////////////////////////////////////////////////////////////////////////
// GBuffers
//////////////////////////////////////////////////////////////////////////

std::unordered_map<ERenderPassFormat::Type, FramebufferFormat::AttachmentsFormatList> GlobalBuffers::GBUFFERS_ATTACHMENT_FORMATS{
    {          ERenderPassFormat::Multibuffer,
     { EPixelDataFormat::BGRA_U8_Norm, EPixelDataFormat::A2BGR10_U32_NormPacked, EPixelDataFormat::A2BGR10_U32_NormPacked,
     EPixelDataFormat::D24S8_U32_DNorm_SInt }                                            },
    {                ERenderPassFormat::Depth, { EPixelDataFormat::D24S8_U32_DNorm_SInt }},
    {      ERenderPassFormat::PointLightDepth, { EPixelDataFormat::D24S8_U32_DNorm_SInt }},
    {ERenderPassFormat::DirectionalLightDepth, { EPixelDataFormat::D24S8_U32_DNorm_SInt }}
};

ImageResourceRef GlobalBuffers::dummyBlackTexture;
ImageResourceRef GlobalBuffers::dummyWhiteTexture;
ImageResourceRef GlobalBuffers::dummyCubeTexture;
ImageResourceRef GlobalBuffers::dummyNormalTexture;
ImageResourceRef GlobalBuffers::integratedBRDF;

BufferResourceRef GlobalBuffers::quadTriVerts = nullptr;
std::pair<BufferResourceRef, BufferResourceRef> GlobalBuffers::quadRectVertsInds{ nullptr, nullptr };
std::pair<BufferResourceRef, BufferResourceRef> GlobalBuffers::lineGizmoVertxInds{ nullptr, nullptr };

bool FramebufferFormat::operator==(const FramebufferFormat &otherFormat) const
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

bool FramebufferFormat::operator<(const FramebufferFormat &otherFormat) const
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
    ENQUEUE_COMMAND(InitializeGlobalBuffers)
    (
        [](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            createTextureCubes(cmdList, graphicsInstance, graphicsHelper);
            createTexture2Ds(cmdList, graphicsInstance, graphicsHelper);
            createVertIndBuffers(cmdList, graphicsInstance, graphicsHelper);

            generateTexture2Ds();
        }
    );
}

void GlobalBuffers::destroy()
{
    ENQUEUE_COMMAND(DestroyGlobalBuffers)
    (
        [](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            destroyTextureCubes();
            destroyTexture2Ds();
            destroyVertIndBuffers();
        }
    );
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

void GlobalBuffers::createTexture2Ds(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
{
    ImageResourceCreateInfo imageCI;
    imageCI.dimensions = Size3D(1, 1, 1);
    imageCI.imageFormat = EPixelDataFormat::BGRA_U8_Norm;
    imageCI.layerCount = imageCI.numOfMips = 1;
    dummyBlackTexture = graphicsHelper->createImage(graphicsInstance, imageCI);
    dummyBlackTexture->setResourceName(TCHAR("Dummy_Black"));

    dummyWhiteTexture = graphicsHelper->createImage(graphicsInstance, imageCI);
    dummyWhiteTexture->setResourceName(TCHAR("Dummy_White"));

    dummyNormalTexture = graphicsHelper->createImage(graphicsInstance, imageCI);
    dummyNormalTexture->setResourceName(TCHAR("Dummy_Normal"));

    if (GlobalRenderVariables::ENABLE_EXTENDED_STORAGES)
    {
        // #TODO(Jeslas) : Create better read only LUT
        imageCI.imageFormat = EPixelDataFormat::RG_SF16;
        imageCI.dimensions = Size3D(EngineSettings::maxEnvMapSize / 2u, EngineSettings::maxEnvMapSize / 2u, 1);
        integratedBRDF = graphicsHelper->createImage(graphicsInstance, imageCI);
        integratedBRDF->setShaderUsage(EImageShaderUsage::Sampling | EImageShaderUsage::Writing);
        integratedBRDF->setResourceName(TCHAR("LUT_IntegratedBRDF"));
    }
    else
    {
        LOG_ERROR("GlobalBuffers", "%s(): Cannot create integrated BRDF LUT, RG_SF16 is not supported format", __func__);
        integratedBRDF = nullptr;
    }
}

void GlobalBuffers::generateTexture2Ds()
{
    ENQUEUE_COMMAND(GenerateTextures2D)
    (
        [](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            dummyWhiteTexture->init();
            dummyBlackTexture->init();
            dummyNormalTexture->init();
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
            Size3D subgrpSize
                = static_cast<const ComputeShaderConfig *>(integrateBrdfContext.getPipeline()->getShaderResource()->getShaderConfig())
                      ->getSubGroupSize();
            cmdList->cmdDispatch(cmdBuffer, integratedBRDF->getImageSize().x / subgrpSize.x, integratedBRDF->getImageSize().y / subgrpSize.y);
            cmdList->cmdTransitionLayouts(cmdBuffer, { integratedBRDF });
            cmdList->endCmd(cmdBuffer);

            CommandSubmitInfo2 submitInfo;
            submitInfo.cmdBuffers.emplace_back(cmdBuffer);
            cmdList->submitCmd(EQueuePriority::High, submitInfo);

            cmdList->copyToImage(dummyBlackTexture, { ColorConst::BLACK });
            cmdList->copyToImage(dummyWhiteTexture, { ColorConst::WHITE });
            cmdList->copyToImage(dummyNormalTexture, { ColorConst::BLUE });

            cmdList->finishCmd(cmdBuffer);
            cmdList->freeCmd(cmdBuffer);
            integrateBrdfParams.reset();
        }
    );
}

void GlobalBuffers::destroyTexture2Ds()
{
    dummyBlackTexture.reset();
    dummyWhiteTexture.reset();
    dummyNormalTexture.reset();

    integratedBRDF.reset();
}

void GlobalBuffers::createTextureCubes(
    IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    ImageResourceCreateInfo imageCI;
    imageCI.dimensions = Size3D(1, 1, 1);
    imageCI.imageFormat = EPixelDataFormat::BGRA_U8_Norm;
    imageCI.layerCount = 6;
    imageCI.numOfMips = 1;
    dummyCubeTexture = graphicsHelper->createCubeImage(graphicsInstance, imageCI);
    dummyCubeTexture->setResourceName(TCHAR("DummyCubeMap"));
}

void GlobalBuffers::destroyTextureCubes() { dummyCubeTexture.reset(); }