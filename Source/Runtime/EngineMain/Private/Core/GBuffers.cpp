#include "Core/GBuffers.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Core/Types/Textures/RenderTargetTextures.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "Engine/Config/EngineGlobalConfigs.h"


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


std::unordered_map<FramebufferFormat, std::vector<FramebufferWrapper>> GBuffers::gBuffers
{
    { FramebufferFormat(GlobalBuffers::getGBufferAttachmentFormat(ERenderPassFormat::Multibuffer), ERenderPassFormat::Multibuffer), {}}
};


void GBuffers::onSampleCountChanged(uint32 oldValue, uint32 newValue)
{
    ENQUEUE_COMMAND(GBufferSampleCountChange)(
        [newValue](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
        {
            cmdList->flushAllcommands();

            const Size2D& screenSize = EngineSettings::screenSize.get();
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
                    
                        framebufferData.rtTextures.emplace_back(rtTexture);
                    }
                }
            }
        }
    );
}

void GBuffers::onScreenResized(Size2D newSize)
{
    ENQUEUE_COMMAND(GBufferResize)(
        [newSize](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
        {
            cmdList->flushAllcommands();

            for (const std::pair<const FramebufferFormat, std::vector<FramebufferWrapper>>& framebufferPair : gBuffers)
            {
                for (const FramebufferWrapper& framebufferData : framebufferPair.second)
                {
                    for (GBufferRenderTexture* rtTexture : framebufferData.rtTextures)
                    {
                        rtTexture->setTextureSize({ newSize.x, newSize.y });
                    }
                }
            }
        }
    );
}

void GBuffers::initialize(int32 swapchainCount)
{
    const Size2D& initialSize = EngineSettings::screenSize.get();
    GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.onConfigChanged().bindStatic(&GBuffers::onSampleCountChanged);

    EPixelSampleCount::Type sampleCount = EPixelSampleCount::Type(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
    const bool bCanHaveResolves = sampleCount != EPixelSampleCount::SampleCount1;

    for (std::pair<const FramebufferFormat, std::vector<FramebufferWrapper>>& framebufferPair : gBuffers)
    {
        framebufferPair.second.clear();
        for (uint32 i = 0; i < swapchainCount; ++i)
        {
            FramebufferWrapper framebufferData;
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

                framebufferData.rtTextures.emplace_back(rtTexture);
            }
            framebufferPair.second.emplace_back(framebufferData);
        }
    }
}

void GBuffers::destroy()
{
    for (std::pair<const FramebufferFormat, std::vector<FramebufferWrapper>>& framebufferPair : gBuffers)
    {
        for (FramebufferWrapper& framebufferData : framebufferPair.second)
        {
            for (GBufferRenderTexture* rtTexture : framebufferData.rtTextures)
            {
                TextureBase::destroyTexture<GBufferRenderTexture>(rtTexture);
            }
        }
        framebufferPair.second.clear();
    }
    gBuffers.clear();
}

std::vector<IRenderTargetTexture*> GBuffers::getFramebufferRts(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx)
{
    std::vector<IRenderTargetTexture*> rts;
    std::unordered_map<FramebufferFormat, std::vector<FramebufferWrapper>>::const_iterator framebufferItr
        = gBuffers.find(FramebufferFormat(renderpassFormat));
    if (framebufferItr != gBuffers.cend() && (framebufferItr->second.size() > frameIdx))
    {
        for (auto* rt : framebufferItr->second[frameIdx].rtTextures)
        {
            rts.emplace_back(rt);
        }
    }
    return rts;
}

std::vector<ImageResourceRef> GBuffers::getFramebufferAttachments(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx)
{
    std::vector<ImageResourceRef> rts;
    std::unordered_map<FramebufferFormat, std::vector<FramebufferWrapper>>::const_iterator framebufferItr
        = gBuffers.find(FramebufferFormat(renderpassFormat));
    if (framebufferItr != gBuffers.cend() && (framebufferItr->second.size() > frameIdx))
    {
        for (auto* rt : framebufferItr->second[frameIdx].rtTextures)
        {
            rts.emplace_back(rt->renderTargetResource());
            if (!rt->isSameReadWriteTexture())
            {
                rts.emplace_back(rt->getTextureResource());
            }
        }
    }
    return rts;
}
