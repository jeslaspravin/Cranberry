#include "RenderTargetTextures.h"
#include "../../../RenderInterface/PlatformIndependentHeaders.h"

#include <glm/exponential.hpp>
#include <glm/common.hpp>

template<bool bIsSrgb>
EPixelDataFormat::Type RtFormatToPixelFormat(ERenderTargetFormat::Type);

template<>
constexpr EPixelDataFormat::Type RtFormatToPixelFormat</*bIsSrgb = */true>(ERenderTargetFormat::Type rtFormat)
{
	switch (rtFormat)
	{
    case ERenderTargetFormat::RT_U8:
        return EPixelDataFormat::BGRA_U8_SRGB;
        break;
    case ERenderTargetFormat::RT_U8Packed:
        return EPixelDataFormat::ABGR_U8_SrgbPacked;
        break;
    case ERenderTargetFormat::RT_U8_NoAlpha:
        return EPixelDataFormat::BGR_U8_SRGB;
        break;
    case ERenderTargetFormat::RT_SF32:
        return EPixelDataFormat::R_SF32;
        break;
    default:
        break;
	}
    return EPixelDataFormat::Undefined;
}

template<>
constexpr EPixelDataFormat::Type RtFormatToPixelFormat</*bIsSrgb = */false>(ERenderTargetFormat::Type rtFormat)
{
    switch (rtFormat)
    {
    case ERenderTargetFormat::RT_U8:
        return EPixelDataFormat::BGRA_U8_Norm;
        break;
    case ERenderTargetFormat::RT_U8Packed:
        return EPixelDataFormat::ABGR_U8_NormPacked;
        break;
    case ERenderTargetFormat::RT_U8_NoAlpha:
        return EPixelDataFormat::BGR_U8_Norm;
        break;
    case ERenderTargetFormat::RT_SF32:
        return EPixelDataFormat::R_SF32;
        break;
    default:
        break;
    }
    return EPixelDataFormat::Undefined;
}


RenderTargetTexture* RenderTargetTexture::createTexture(const RenderTextureCreateParams& createParams)
{
    RenderTargetTexture* texture = new RenderTargetTexture();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount != 0)
    {
        texture->mipCount = glm::min((uint32)(1 + glm::floor(glm::log2(
            (float)glm::max(createParams.textureSize.x, createParams.textureSize.y)))), createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->sampleCount = createParams.sampleCount;
    texture->textureName = createParams.textureName;
    texture->bIsSrgb = createParams.bIsSrgb;
    if (createParams.bIsSrgb)
    {
        texture->dataFormat = RtFormatToPixelFormat<true>(createParams.format);
    }
    else
    {
        texture->dataFormat = RtFormatToPixelFormat<false>(createParams.format);
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
    texture->textureResource = new GraphicsImageResource(texture->dataFormat);
    texture->rtResource = new GraphicsRenderTargetResource(texture->dataFormat);

    texture->textureResource->setResourceName(texture->textureName);
    texture->rtResource->setResourceName(texture->textureName);

    texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);

    texture->textureResource->setSampleCounts(texture->sampleCount);
    texture->rtResource->setSampleCounts(texture->sampleCount);

    texture->textureResource->setImageSize(texture->textureSize);
    texture->rtResource->setImageSize(texture->textureSize);

    texture->textureResource->setLayerCount(1);
    texture->rtResource->setLayerCount(1);

    texture->textureResource->setNumOfMips(texture->mipCount);
    texture->rtResource->setNumOfMips(texture->mipCount);

    texture->textureResource->init();
    texture->rtResource->init();
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
