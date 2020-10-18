#include "RenderTargetTextures.h"
#include "../../../RenderInterface/PlatformIndependentHeaders.h"
#include "../../Math/Math.h"
#include "../../Logger/Logger.h"
#include "../../../RenderInterface/Rendering/IRenderCommandList.h"

namespace ERenderTargetFormat
{
    template<>
    constexpr EPixelDataFormat::Type rtFormatToPixelFormat</*bIsSrgb = */true>(ERenderTargetFormat::Type rtFormat, EPixelDataFormat::Type defaultFormat)
    {
        switch (rtFormat)
        {
        case ERenderTargetFormat::RT_U8:
            return EPixelDataFormat::BGRA_U8_SRGB;
        case ERenderTargetFormat::RT_U8Packed:
            return EPixelDataFormat::ABGR8_U32_SrgbPacked;
        case ERenderTargetFormat::RT_U8_NoAlpha:
            return EPixelDataFormat::BGR_U8_SRGB;
        case ERenderTargetFormat::RT_NormalMap:
            return EPixelDataFormat::ABGR8_S32_NormPacked;
        case ERenderTargetFormat::RT_SF32:
            return EPixelDataFormat::R_SF32;
        case ERenderTargetFormat::RT_UseDefault:
            return defaultFormat;
        default:
            break;
        }
        return EPixelDataFormat::Undefined;
    }

    template<>
    constexpr EPixelDataFormat::Type rtFormatToPixelFormat</*bIsSrgb = */false>(ERenderTargetFormat::Type rtFormat, EPixelDataFormat::Type defaultFormat)
    {
        switch (rtFormat)
        {
        case ERenderTargetFormat::RT_U8:
            return EPixelDataFormat::BGRA_U8_Norm;
        case ERenderTargetFormat::RT_U8Packed:
            return EPixelDataFormat::ABGR8_U32_NormPacked;
        case ERenderTargetFormat::RT_U8_NoAlpha:
            return EPixelDataFormat::BGR_U8_Norm;
        case ERenderTargetFormat::RT_NormalMap:
            return EPixelDataFormat::ABGR8_S32_NormPacked;
        case ERenderTargetFormat::RT_SF32:
            return EPixelDataFormat::R_SF32;
        case ERenderTargetFormat::RT_UseDefault:
            return defaultFormat;
        default:
            break;
        }
        return EPixelDataFormat::Undefined;
    }
}

void RenderTargetTexture::reinitResources()
{
    if (dataFormat != rtResource->imageFormat())
    {
        RenderTargetTexture::init(this);
    }
    else
    {
        rtResource->setImageSize(textureSize);
        rtResource->setNumOfMips(mipCount);
        rtResource->setResourceName(textureName);
        textureResource->setImageSize(textureSize);
        textureResource->setNumOfMips(mipCount);
        textureResource->setResourceName(textureName);
        ENQUEUE_COMMAND(RtReinitTexture,
            {
                rtResource->reinitResources();
                cmdList->setupInitialLayout(rtResource);
                if (!bSameReadWriteTexture)
                {
                    textureResource->reinitResources();
                    cmdList->setupInitialLayout(textureResource);
                } 
            }, this);
    }
}

void RenderTargetTexture::setTextureSize(Size2D newSize)
{
    textureSize = Size3D(newSize.x, newSize.y, 1);
    markResourceDirty();
}

RenderTargetTexture* RenderTargetTexture::createTexture(const RenderTextureCreateParams& createParams)
{
    RenderTargetTexture* texture = new RenderTargetTexture();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount != 0)
    {
        texture->mipCount = Math::min((uint32)(1 + Math::floor(Math::log2(
            (float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))), createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->bIsSrgb = createParams.bIsSrgb;
    texture->bSameReadWriteTexture = createParams.bSameReadWriteTexture;
    if (createParams.bIsSrgb)
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormat<true>(createParams.format, EPixelDataFormat::BGRA_U8_Norm);
    }
    else
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormat<false>(createParams.format, EPixelDataFormat::BGRA_U8_Norm);
    }
    // Dependent values
    texture->setSampleCount(createParams.bSameReadWriteTexture? EPixelSampleCount::SampleCount1 : createParams.sampleCount);
    texture->setFilteringMode(createParams.filtering);

    RenderTargetTexture::init(texture);
    return texture;
}

void RenderTargetTexture::destroyTexture(RenderTargetTexture* texture)
{
    RenderTargetTexture::release(texture);
    delete texture;
}

void RenderTargetTexture::init(RenderTargetTexture* texture)
{
    texture->textureResource = texture->rtResource = new GraphicsRenderTargetResource(texture->dataFormat);

    texture->rtResource->setResourceName(texture->textureName);
    texture->rtResource->setShaderUsage(texture->bSameReadWriteTexture? EImageShaderUsage::Sampling : 0);
    texture->rtResource->setSampleCounts(texture->getSampleCount());
    texture->rtResource->setImageSize(texture->textureSize);
    texture->rtResource->setLayerCount(1);
    texture->rtResource->setNumOfMips(texture->mipCount);

    if (!texture->bSameReadWriteTexture)
    {
        texture->rtResource->setResourceName(texture->textureName + "_RT");

        texture->textureResource = new GraphicsImageResource(texture->dataFormat);
        texture->textureResource->setResourceName(texture->textureName);
        texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
        texture->textureResource->setSampleCounts(EPixelSampleCount::SampleCount1);
        texture->textureResource->setImageSize(texture->textureSize);
        texture->textureResource->setLayerCount(1);
        texture->textureResource->setNumOfMips(texture->mipCount);
    }

    ENQUEUE_COMMAND(RtInitTexture,
        {
            texture->rtResource->init();
            cmdList->setupInitialLayout(texture->rtResource);
            if (!texture->bSameReadWriteTexture)
            {
                texture->textureResource->init();
                cmdList->setupInitialLayout(texture->textureResource);
            }
        }, texture);
}

void RenderTargetTexture::release(RenderTargetTexture* texture)
{
    ImageResource* rtResource = texture->rtResource;
    ImageResource* textureResource = texture->textureResource;
    ENQUEUE_COMMAND(RtDestroyTexture,
        {
            textureResource->release();
            rtResource->release();

            delete rtResource;
            if (rtResource != textureResource)
            {
                delete textureResource;
            }
        }, rtResource, textureResource);

    texture->textureResource = nullptr;
    texture->rtResource = nullptr;
}
