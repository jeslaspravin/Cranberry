#include "GBuffersAndTextures.h"
#include "../RenderInterface/GlobalRenderVariables.h"
#include "../Core/Engine/Config/EngineGlobalConfigs.h"
#include "../Core/Engine/GameEngine.h"
#include "../RenderInterface/Resources/GenericWindowCanvas.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"
#include "../RenderInterface/Rendering/IRenderCommandList.h"
#include "../Core/Types/Textures/RenderTargetTextures.h"

std::unordered_map<FramebufferFormat, std::vector<Framebuffer*>> GBuffers::gBuffers
{
    {
        FramebufferFormat({ EPixelDataFormat::BGRA_U8_Norm, EPixelDataFormat::ABGR_S8_NormPacked, EPixelDataFormat::R_SF32 }), {}
    }
};

bool FramebufferFormat::operator==(const FramebufferFormat& otherFormat) const
{
    bool isEqual = false;
    if (otherFormat.attachments.size() == attachments.size())
    {
        isEqual = true;
        for (int32 index = 0; index < (int32)attachments.size(); ++index)
        {
            isEqual = isEqual && (attachments[index] == otherFormat.attachments[index]);
        }
    }

    return isEqual;
}

FramebufferFormat::FramebufferFormat(std::vector<EPixelDataFormat::Type>&& frameBuffers)
    : attachments(std::move(frameBuffers))
{}

class ImageResource* Framebuffer::getImageResource(const RenderTargetTexture * rtTexture) const
{
    return rtTexture->getRtTexture();
}

void GBuffers::onSampleCountChanged(uint32 oldValue, uint32 newValue)
{
    ENQUEUE_COMMAND(GBufferSampleCountChange, 
        {
            for (auto framebuferPair : gBuffers)
            {
                for (auto* framebufferData : framebuferPair.second)
                {
                    for (auto* imageResource : framebufferData->textures)
                    {
                        imageResource->setSampleCount(EPixelSampleCount::Type(newValue));
                    }
                    initializeInternal(framebufferData);
                }
            }
        }
    , newValue);
}

void GBuffers::onResize(Size2D oldSize, Size2D newSize)
{
    ENQUEUE_COMMAND(GBufferResize,
        {
            for (auto framebuferPair : gBuffers)
            {
                for (auto* framebufferData : framebuferPair.second)
                {
                    for (auto* imageResource : framebufferData->textures)
                    {
                        imageResource->setTextureSize(newSize);
                    }
                    initializeInternal(framebufferData);
                }
            }
        }
    , newSize);
}

void GBuffers::initialize()
{
    uint32 swapchainCount = gEngine->getApplicationInstance()->appWindowManager.getWindowCanvas(gEngine->getApplicationInstance()->appWindowManager.getMainWindow())->imagesCount();

    const Size2D initialSize = EngineSettings::screenSize.get();
    EngineSettings::screenSize.onConfigChanged().bindStatic(&GBuffers::onResize);
    EPixelSampleCount::Type sampleCount = EPixelSampleCount::Type(GlobalRenderVariables::FRAME_BUFFER_SAMPLE_COUNT.get());
    GlobalRenderVariables::FRAME_BUFFER_SAMPLE_COUNT.onConfigChanged().bindStatic(&GBuffers::onSampleCountChanged);
    for (auto& framebuferPair : gBuffers)
    {
        framebuferPair.second.clear();
        for (uint32 i = 0; i < swapchainCount; ++i)
        {
            Framebuffer* framebufferData = createFbInternal();
            if (framebufferData == nullptr)
            {
                continue;
            }
            for (auto frameBufferFormat : framebuferPair.first.attachments)
            {
                RenderTextureCreateParams createParam;
                createParam.format = ERenderTargetFormat::pixelFormatToRTFormat(frameBufferFormat);
                createParam.mipCount = 1;
                createParam.sampleCount = sampleCount;
                createParam.textureSize = initialSize;
                createParam.bSameReadWriteTexture = true;
                createParam.textureName = "GBuffer_" + EPixelDataFormat::getFormatInfo(frameBufferFormat)->formatName;

                framebufferData->textures.push_back(TextureBase::createTexture<RenderTargetTexture>(createParam));
            }
            initializeInternal(framebufferData);
            framebuferPair.second.push_back(framebufferData);
        }
    }
}

void GBuffers::destroy()
{
    for (auto& framebuferPair : gBuffers)
    {
        for (auto* framebufferData : framebuferPair.second)
        {
            for (auto* imageResource : framebufferData->textures)
            {
                TextureBase::destroyTexture<RenderTargetTexture>(imageResource);
            }
            framebufferData->textures.clear();
            delete framebufferData;
        }
        framebuferPair.second.clear();
    }
}

