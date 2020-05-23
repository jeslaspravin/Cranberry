#include "RenderTargetTextures.h"
#include "../../../RenderInterface/PlatformIndependentHeaders.h"
#include "../../Math/Math.h"
#include "../../Logger/Logger.h"

namespace ERenderTargetFormat
{
    template<>
    constexpr EPixelDataFormat::Type rtFormatToPixelFormat</*bIsSrgb = */true>(ERenderTargetFormat::Type rtFormat)
    {
        switch (rtFormat)
        {
        case ERenderTargetFormat::RT_U8:
            return EPixelDataFormat::BGRA_U8_SRGB;
        case ERenderTargetFormat::RT_U8Packed:
            return EPixelDataFormat::ABGR_U8_SrgbPacked;
        case ERenderTargetFormat::RT_U8_NoAlpha:
            return EPixelDataFormat::BGR_U8_SRGB;
        case ERenderTargetFormat::RT_NormalMap:
            return EPixelDataFormat::A2BGR10_S32_NormPacked;
        case ERenderTargetFormat::RT_SF32:
            return EPixelDataFormat::R_SF32;
        default:
            break;
        }
        return EPixelDataFormat::Undefined;
    }

    template<>
    constexpr EPixelDataFormat::Type rtFormatToPixelFormat</*bIsSrgb = */false>(ERenderTargetFormat::Type rtFormat)
    {
        switch (rtFormat)
        {
        case ERenderTargetFormat::RT_U8:
            return EPixelDataFormat::BGRA_U8_Norm;
        case ERenderTargetFormat::RT_U8Packed:
            return EPixelDataFormat::ABGR_U8_NormPacked;
        case ERenderTargetFormat::RT_U8_NoAlpha:
            return EPixelDataFormat::BGR_U8_Norm;
        case ERenderTargetFormat::RT_NormalMap:
            return EPixelDataFormat::A2BGR10_S32_NormPacked;
        case ERenderTargetFormat::RT_SF32:
            return EPixelDataFormat::R_SF32;
        default:
            break;
        }
        return EPixelDataFormat::Undefined;
    }

    ERenderTargetFormat::Type pixelFormatToRTFormat(EPixelDataFormat::Type pixelFormat)
    {
        switch (pixelFormat)
        {
        case EPixelDataFormat::BGR_U8_Norm:
        case EPixelDataFormat::BGR_U8_SRGB:
            return ERenderTargetFormat::RT_U8_NoAlpha;
        case EPixelDataFormat::ABGR_U8_SrgbPacked:
        case EPixelDataFormat::ABGR_U8_NormPacked:
            return ERenderTargetFormat::RT_U8Packed;
        case EPixelDataFormat::BGRA_U8_Norm:
        case EPixelDataFormat::BGRA_U8_SRGB:
            return ERenderTargetFormat::RT_U8;
        case EPixelDataFormat::A2BGR10_S32_NormPacked:
            return ERenderTargetFormat::RT_NormalMap;
        case EPixelDataFormat::R_SF32:
            return ERenderTargetFormat::RT_SF32;
        default:
            break;
        }

        Logger::warn("ERenderTargetFormat", "%s() : Not support pixel format %s in Render target", __func__
            , EPixelDataFormat::getFormatInfo(pixelFormat)->formatName.getChar());
        return ERenderTargetFormat::RT_U8;
    }

}

void RenderTargetTexture::reinitResources()
{
    TextureBase::reinitResources();

    if (dataFormat != rtResource->imageFormat())
    {
        RenderTargetTexture::init(this);
    }
    else
    {
        rtResource->setImageSize(textureSize);
        rtResource->setNumOfMips(mipCount);
        rtResource->setSampleCounts(sampleCount);
        rtResource->setResourceName(textureName);

        rtResource->reinitResources();
        if (!bSameReadWriteTexture)
        {
            textureResource->reinitResources();
        }
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
    texture->sampleCount = createParams.sampleCount;
    texture->textureName = createParams.textureName;
    texture->bIsSrgb = createParams.bIsSrgb;
    texture->bSameReadWriteTexture = createParams.bSameReadWriteTexture;
    if (createParams.bIsSrgb)
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormat<true>(createParams.format);
    }
    else
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormat<false>(createParams.format);
    }

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
    texture->rtResource->setSampleCounts(texture->sampleCount);
    texture->rtResource->setImageSize(texture->textureSize);
    texture->rtResource->setLayerCount(1);
    texture->rtResource->setNumOfMips(texture->mipCount);
    texture->rtResource->init();

    if (!texture->bSameReadWriteTexture)
    {
        texture->textureResource = new GraphicsImageResource(texture->dataFormat);
        texture->textureResource->setResourceName(texture->textureName);
        texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
        texture->textureResource->setSampleCounts(texture->sampleCount);
        texture->textureResource->setImageSize(texture->textureSize);
        texture->textureResource->setLayerCount(1);
        texture->textureResource->setNumOfMips(texture->mipCount);
        texture->textureResource->init();
    }
}

void RenderTargetTexture::release(RenderTargetTexture* texture)
{
    texture->textureResource->release();
    texture->rtResource->release();

    delete texture->textureResource;
    delete texture->rtResource;

    texture->textureResource = nullptr;
    texture->rtResource = nullptr;
}
