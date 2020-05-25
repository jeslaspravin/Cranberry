#include "GBuffersAndTextures.h"
#include "../RenderInterface/GlobalRenderVariables.h"
#include "../Core/Engine/Config/EngineGlobalConfigs.h"
#include "../Core/Engine/GameEngine.h"
#include "../RenderInterface/Resources/GenericWindowCanvas.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"
#include "../RenderInterface/Rendering/IRenderCommandList.h"

std::unordered_map<FramebufferFormat, std::vector<Framebuffer*>> GBuffers::gBuffers
{
    {
        FramebufferFormat({ EPixelDataFormat::BGRA_U8_Norm, EPixelDataFormat::ABGR_S8_NormPacked, EPixelDataFormat::R_SF32 }), {}
    }
};
std::vector<Framebuffer*> GBuffers::swapchainFbs;


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
                        imageResource->setSampleCounts(EPixelSampleCount::Type(newValue));
                        imageResource->reinitResources();
                    }
                    initializeInternal(framebufferData);
                }
            }
        }
    , newValue);
}

void GBuffers::onScreenResized(Size2D oldSize, Size2D newSize)
{
    ENQUEUE_COMMAND(GBufferResize,
        {
            for (auto framebuferPair : gBuffers)
            {
                for (auto* framebufferData : framebuferPair.second)
                {
                    for (auto* imageResource : framebufferData->textures)
                    {
                        imageResource->setImageSize({ newSize.x, newSize.y, 1 });
                        imageResource->reinitResources();
                    }
                    initializeInternal(framebufferData);
                }
            }
        }
    , newSize);
}

void GBuffers::onSurfaceResized(Size2D oldSize, Size2D newSize)
{
    ENQUEUE_COMMAND(SwapchainResize,
        {
            const GenericWindowCanvas * windowCanvas = gEngine->getApplicationInstance()->appWindowManager
                .getWindowCanvas(gEngine->getApplicationInstance()->appWindowManager.getMainWindow());

            uint32 swapchainIdx = 0;
            for (Framebuffer* fb : swapchainFbs)
            {
                initializeSwapchainFb(fb, windowCanvas, swapchainIdx);
                ++swapchainIdx;
            }
        }
    , newSize);
}

void GBuffers::initialize()
{
    const GenericWindowCanvas* windowCanvas = gEngine->getApplicationInstance()->appWindowManager
        .getWindowCanvas(gEngine->getApplicationInstance()->appWindowManager.getMainWindow());
    uint32 swapchainCount = windowCanvas->imagesCount();

    const Size2D initialSize = EngineSettings::screenSize.get();
    EngineSettings::screenSize.onConfigChanged().bindStatic(&GBuffers::onScreenResized);
    EngineSettings::surfaceSize.onConfigChanged().bindStatic(&GBuffers::onSurfaceResized);
    GlobalRenderVariables::FRAME_BUFFER_SAMPLE_COUNT.onConfigChanged().bindStatic(&GBuffers::onSampleCountChanged);

    EPixelSampleCount::Type sampleCount = EPixelSampleCount::Type(GlobalRenderVariables::FRAME_BUFFER_SAMPLE_COUNT.get());
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
                ImageResource* imgResource = new GraphicsRenderTargetResource(frameBufferFormat);
                imgResource->setImageSize({ initialSize.x, initialSize.y, 1 });
                imgResource->setNumOfMips(1);
                imgResource->setSampleCounts(sampleCount);
                imgResource->setShaderUsage(EImageShaderUsage::Sampling);
                imgResource->setResourceName("GBuffer_" + EPixelDataFormat::getFormatInfo(frameBufferFormat)->formatName);
                imgResource->init();

                framebufferData->textures.push_back(imgResource);
            }
            initializeInternal(framebufferData);
            framebuferPair.second.push_back(framebufferData);
        }
    }

    for (uint32 i = 0; i < swapchainCount; ++i)
    {
        Framebuffer* fb = createFbInternal();
        initializeSwapchainFb(fb, windowCanvas, i);
        swapchainFbs.push_back(fb);
    }
}

void GBuffers::destroy()
{
    for (auto& framebuferPair : gBuffers)
    {
        for (auto* framebufferData : framebuferPair.second)
        {
            for (auto* imgResource : framebufferData->textures)
            {
                imgResource->release();
                delete imgResource;
            }
            framebufferData->textures.clear();
            delete framebufferData;
        }
        framebuferPair.second.clear();
    }
    gBuffers.clear();

    for (Framebuffer* fb : swapchainFbs)
    {
        delete fb;
    }
    swapchainFbs.clear();
}

Framebuffer* GBuffers::getFramebuffer(const FramebufferFormat& framebufferFormat, uint32 frameIdx)
{
    std::unordered_map<FramebufferFormat,std::vector<Framebuffer*>>::const_iterator framebufferItr = gBuffers.find(framebufferFormat);
    if (framebufferItr != gBuffers.cend())
    {
        return framebufferItr->second[frameIdx];
    }
    return nullptr;
}

Framebuffer* GBuffers::getSwapchainFramebuffer(uint32 frameIdx)
{
    return swapchainFbs[frameIdx];
}

