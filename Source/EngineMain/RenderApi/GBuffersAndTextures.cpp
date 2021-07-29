#include "GBuffersAndTextures.h"
#include "../RenderInterface/GlobalRenderVariables.h"
#include "../Core/Engine/Config/EngineGlobalConfigs.h"
#include "../Core/Engine/GameEngine.h"
#include "../RenderInterface/Resources/GenericWindowCanvas.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"
#include "../RenderInterface/Rendering/IRenderCommandList.h"
#include "../Core/Types/Textures/RenderTargetTextures.h"
#include "../Core/Math/Math.h"

//////////////////////////////////////////////////////////////////////////
// Custom Render target texture for GBuffers
//////////////////////////////////////////////////////////////////////////

struct GBufferRTCreateParams : public RenderTextureCreateParams
{
    EPixelDataFormat::Type dataFormat;
};

class GBufferRenderTexture : public RenderTargetTexture
{
public:
    static GBufferRenderTexture* createTexture(const GBufferRTCreateParams& createParams);
    static void destroyTexture(GBufferRenderTexture* texture);
};

GBufferRenderTexture* GBufferRenderTexture::createTexture(const GBufferRTCreateParams& createParams)
{
    GBufferRenderTexture* texture = new GBufferRenderTexture();

    texture->mipCount = 1;
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->bIsSrgb = createParams.bIsSrgb;
    texture->bSameReadWriteTexture = createParams.bSameReadWriteTexture;
    if (createParams.bIsSrgb)
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormat<true>(createParams.format, createParams.dataFormat);
    }
    else
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormat<false>(createParams.format, createParams.dataFormat);
    }
    // Dependent values

    // If depth texture then it should use same sample count as RT as it will not be used directly as shader read texture
    texture->setSampleCount(createParams.bSameReadWriteTexture && !EPixelDataFormat::isDepthFormat(texture->dataFormat) ? EPixelSampleCount::SampleCount1 : createParams.sampleCount);
    texture->setFilteringMode(createParams.filtering);

    RenderTargetTexture::init(texture);
    return texture;
}

void GBufferRenderTexture::destroyTexture(GBufferRenderTexture* texture)
{
    RenderTargetTexture::destroyTexture(texture);
}

//////////////////////////////////////////////////////////////////////////
// GBuffers
//////////////////////////////////////////////////////////////////////////

std::unordered_map<FramebufferFormat, std::vector<FramebufferWrapper>> GlobalBuffers::gBuffers
{
    {
        FramebufferFormat({ EPixelDataFormat::BGRA_U8_Norm, EPixelDataFormat::A2BGR10_U32_NormPacked, EPixelDataFormat::A2BGR10_U32_NormPacked, EPixelDataFormat::D24S8_U32_DNorm_SInt }, ERenderPassFormat::Multibuffers), {}
    }
};
std::vector<Framebuffer*> GlobalBuffers::swapchainFbs;

TextureBase* GlobalBuffers::dummyBlackTexture = nullptr;
TextureBase* GlobalBuffers::dummyWhiteTexture = nullptr;
TextureBase* GlobalBuffers::dummyNormalTexture = nullptr;

std::pair<BufferResource*, BufferResource*> GlobalBuffers::quadVertsInds{ nullptr,nullptr };
std::pair<BufferResource*, BufferResource*> GlobalBuffers::lineGizmoVertxInds{ nullptr,nullptr };

bool FramebufferFormat::operator==(const FramebufferFormat& otherFormat) const
{
    bool isEqual = rpFormat == otherFormat.rpFormat;

    // If generic then check all attachments for equality
    if(isEqual && rpFormat == ERenderPassFormat::Generic)
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

bool FramebufferFormat::operator<(const FramebufferFormat& otherFormat) const
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

FramebufferFormat::FramebufferFormat(std::vector<EPixelDataFormat::Type>&& frameBuffers, ERenderPassFormat::Type renderpassFormat)
    : attachments(std::move(frameBuffers))
    , rpFormat(renderpassFormat)
{}

void GlobalBuffers::onSampleCountChanged(uint32 oldValue, uint32 newValue)
{
    ENQUEUE_COMMAND(GBufferSampleCountChange,LAMBDA_BODY
        (
            cmdList->waitIdle();

            const Size2D & screenSize = EngineSettings::screenSize.get();
            const EPixelSampleCount::Type sampleCount = EPixelSampleCount::Type(newValue);
            const bool bCanHaveResolves = sampleCount != EPixelSampleCount::SampleCount1;

            for (std::pair<const FramebufferFormat, std::vector<FramebufferWrapper>>& framebufferPair : gBuffers)
            {
                for (FramebufferWrapper& framebufferData : framebufferPair.second)
                {
                    for (GBufferRenderTexture* rtTexture : framebufferData.rtTextures)
                    {
                        TextureBase::destroyTexture<GBufferRenderTexture>(rtTexture);
                    }
                    framebufferData.framebuffer->textures.clear();
                    framebufferData.rtTextures.clear();

                    for (const EPixelDataFormat::Type& framebufferFormat : framebufferPair.first.attachments)
                    {
                        GBufferRTCreateParams rtCreateParam;
                        rtCreateParam.bSameReadWriteTexture = !bCanHaveResolves || EPixelDataFormat::isDepthFormat(framebufferFormat);
                        rtCreateParam.filtering = ESamplerFiltering::Type(GlobalRenderVariables::GBUFFER_FILTERING.get());
                        rtCreateParam.format = ERenderTargetFormat::RT_UseDefault;
                        rtCreateParam.dataFormat = framebufferFormat;
                        rtCreateParam.sampleCount = sampleCount;
                        rtCreateParam.textureSize = { screenSize.x, screenSize.y };
                        rtCreateParam.textureName = "GBuffer_" + EPixelDataFormat::getFormatInfo(framebufferFormat)->formatName;

                        GBufferRenderTexture* rtTexture = TextureBase::createTexture<GBufferRenderTexture>(rtCreateParam);

                        framebufferData.framebuffer->textures.emplace_back(rtTexture->getRtTexture());
                        if (!rtCreateParam.bSameReadWriteTexture)
                        {
                            framebufferData.framebuffer->textures.emplace_back(rtTexture->getTextureResource());
                        }

                        framebufferData.rtTextures.emplace_back(rtTexture);
                    }
                    framebufferData.framebuffer->bHasResolves = bCanHaveResolves;
                    initializeFb(framebufferData.framebuffer, screenSize);
                }
            }
        )
    , newValue);
}

void GlobalBuffers::onScreenResized(Size2D newSize)
{
    ENQUEUE_COMMAND(GBufferResize,
        {
            cmdList->waitIdle();

            for (const std::pair<const FramebufferFormat, std::vector<FramebufferWrapper>>& framebufferPair : gBuffers)
            {
                for (const FramebufferWrapper& framebufferData : framebufferPair.second)
                {
                    framebufferData.framebuffer->textures.clear();
                    for (GBufferRenderTexture* rtTexture : framebufferData.rtTextures)
                    {
                        rtTexture->setTextureSize({ newSize.x, newSize.y });
                        framebufferData.framebuffer->textures.emplace_back(rtTexture->getRtTexture());

                        if (!rtTexture->isSameReadWriteTexture())
                        {
                            framebufferData.framebuffer->textures.emplace_back(rtTexture->getTextureResource());
                        }
                    }
                    initializeFb(framebufferData.framebuffer, newSize);
                }
            }
        }
    , newSize);
}

void GlobalBuffers::onSurfaceResized(Size2D newSize)
{
    ENQUEUE_COMMAND(SwapchainResize,
        {
            cmdList->waitIdle();
            const GenericWindowCanvas * windowCanvas = gEngine->getApplicationInstance()->appWindowManager
                .getWindowCanvas(gEngine->getApplicationInstance()->appWindowManager.getMainWindow());

            uint32 swapchainIdx = 0;
            for (Framebuffer* fb : swapchainFbs)
            {
                initializeSwapchainFb(fb, windowCanvas, newSize, swapchainIdx);
                ++swapchainIdx;
            }
        }
    , newSize);
}

void GlobalBuffers::initialize()
{
    const GenericWindowCanvas* windowCanvas = gEngine->getApplicationInstance()->appWindowManager
        .getWindowCanvas(gEngine->getApplicationInstance()->appWindowManager.getMainWindow());
    uint32 swapchainCount = windowCanvas->imagesCount();

    const Size2D& initialSize = EngineSettings::screenSize.get();
    GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.onConfigChanged().bindStatic(&GlobalBuffers::onSampleCountChanged);

    EPixelSampleCount::Type sampleCount = EPixelSampleCount::Type(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
    const bool bCanHaveResolves = sampleCount != EPixelSampleCount::SampleCount1;

    for (std::pair<const FramebufferFormat, std::vector<FramebufferWrapper>>& framebufferPair : gBuffers)
    {
        framebufferPair.second.clear();
        for (uint32 i = 0; i < swapchainCount; ++i)
        {
            FramebufferWrapper framebufferData;
            framebufferData.framebuffer = createFbInstance();
            if (framebufferData.framebuffer == nullptr)
            {
                continue;
            }
            for (const EPixelDataFormat::Type& framebufferFormat : framebufferPair.first.attachments)
            {
                GBufferRTCreateParams rtCreateParam;
                rtCreateParam.bSameReadWriteTexture = !bCanHaveResolves || EPixelDataFormat::isDepthFormat(framebufferFormat);

                rtCreateParam.filtering = ESamplerFiltering::Type(GlobalRenderVariables::GBUFFER_FILTERING.get());
                rtCreateParam.format = ERenderTargetFormat::RT_UseDefault;
                rtCreateParam.dataFormat = framebufferFormat;
                rtCreateParam.sampleCount = sampleCount;
                rtCreateParam.textureSize = { initialSize.x, initialSize.y };
                rtCreateParam.textureName = "GBuffer_" + EPixelDataFormat::getFormatInfo(framebufferFormat)->formatName;

                GBufferRenderTexture* rtTexture = TextureBase::createTexture<GBufferRenderTexture>(rtCreateParam);

                framebufferData.framebuffer->textures.emplace_back(rtTexture->getRtTexture());
                // Since there cannot be any resolve for depth texture as of Vulkan 1.2.135 we do not have resolve attachment for depth
                if (!rtCreateParam.bSameReadWriteTexture)
                {
                    framebufferData.framebuffer->textures.emplace_back(rtTexture->getTextureResource());
                }

                framebufferData.rtTextures.emplace_back(rtTexture);
            }
            framebufferData.framebuffer->bHasResolves = bCanHaveResolves;
            initializeFb(framebufferData.framebuffer, initialSize);
            framebufferPair.second.emplace_back(framebufferData);
        }
    }

    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        Framebuffer* fb = createFbInstance();
        fb->bHasResolves = false;
        initializeSwapchainFb(fb, windowCanvas, EngineSettings::surfaceSize.get(), i);
        swapchainFbs.emplace_back(fb);
    }

    createTexture2Ds();
    ENQUEUE_COMMAND(InitializeGlobalBuffers,
        { 
            createVertIndBuffers(cmdList, graphicsInstance);
        });
}

void GlobalBuffers::destroy()
{
    for (std::pair<const FramebufferFormat, std::vector<FramebufferWrapper>>& framebufferPair : gBuffers)
    {
        for (FramebufferWrapper& framebufferData : framebufferPair.second)
        {
            for (GBufferRenderTexture* rtTexture : framebufferData.rtTextures)
            {
                TextureBase::destroyTexture<GBufferRenderTexture>(rtTexture);
            }
            destroyFbInstance(framebufferData.framebuffer);
        }
        framebufferPair.second.clear();
    }
    gBuffers.clear();

    for (Framebuffer* fb : swapchainFbs)
    {
        destroyFbInstance(fb);
    }
    swapchainFbs.clear();

    destroyTexture2Ds();
    ENQUEUE_COMMAND(DestroyGlobalBuffers,
        {
            destroyVertIndBuffers(cmdList, graphicsInstance);
        });
}

Framebuffer* GlobalBuffers::getFramebuffer(FramebufferFormat& framebufferFormat, uint32 frameIdx)
{
    std::unordered_map<FramebufferFormat,std::vector<FramebufferWrapper>>::const_iterator framebufferItr = gBuffers.find(framebufferFormat);
    if (framebufferItr != gBuffers.cend())
    {
        framebufferFormat = framebufferItr->first;
        return framebufferItr->second[frameIdx].framebuffer;
    }
    return nullptr;
}

Framebuffer* GlobalBuffers::getFramebuffer(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx)
{
    std::unordered_map<FramebufferFormat, std::vector<FramebufferWrapper>>::const_iterator framebufferItr = gBuffers.find(FramebufferFormat(renderpassFormat));
    if (framebufferItr != gBuffers.cend())
    {
        return framebufferItr->second[frameIdx].framebuffer;
    }
    return nullptr;
}

std::vector<RenderTargetTexture*> GlobalBuffers::getFramebufferRts(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx)
{
    std::vector<RenderTargetTexture*> rts;
    std::unordered_map<FramebufferFormat, std::vector<FramebufferWrapper>>::const_iterator framebufferItr = gBuffers.find(FramebufferFormat(renderpassFormat));
    if (framebufferItr != gBuffers.cend())
    {
        for (auto* rt : framebufferItr->second[frameIdx].rtTextures)
        {
            rts.emplace_back(rt);
        }
    }
    return rts;
}

Framebuffer* GlobalBuffers::getSwapchainFramebuffer(uint32 frameIdx)
{
    return swapchainFbs[frameIdx];
}

void GlobalBuffers::destroyFbInstance(const Framebuffer* fb)
{
    delete fb;
}
