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
        case ERenderTargetFormat::RT_Depth:
            return EPixelDataFormat::D24S8_U32_DNorm_SInt;
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
        case ERenderTargetFormat::RT_Depth:
            return EPixelDataFormat::D24S8_U32_DNorm_SInt;
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
        // Since Texture format can only be set in constructor
        RenderTargetTexture::release(this);
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
        textureResource->setLayerCount(layerCount);
        ENQUEUE_COMMAND(RtReinitTexture)(
            [this](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
            {
                rtResource->reinitResources();
                cmdList->setupInitialLayout(rtResource);
                if (!bSameReadWriteTexture)
                {
                    textureResource->reinitResources();
                    cmdList->setupInitialLayout(textureResource);
                } 
            });
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
    if (createParams.mipCount == 0)
    {
        texture->mipCount = Math::min((uint32)(1 + Math::floor(Math::log2(
            (float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))), createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->bIsSrgb = createParams.bIsSrgb;
    texture->bSameReadWriteTexture = createParams.bSameReadWriteTexture;
    texture->rtFormat = createParams.format;
    texture->layerCount = 1;
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
    texture->rtResource->setLayerCount(texture->layerCount);
    texture->rtResource->setNumOfMips(texture->mipCount);

    if (!texture->bSameReadWriteTexture)
    {
        texture->rtResource->setResourceName(texture->textureName + "_RT");

        texture->textureResource = new GraphicsImageResource(texture->dataFormat);
        texture->textureResource->setResourceName(texture->textureName);
        texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
        texture->textureResource->setSampleCounts(EPixelSampleCount::SampleCount1);
        texture->textureResource->setImageSize(texture->textureSize);
        texture->textureResource->setLayerCount(texture->layerCount);
        texture->textureResource->setNumOfMips(texture->mipCount);
    }

    ENQUEUE_COMMAND(RtInitTexture)(
        [texture](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            texture->rtResource->init();
            cmdList->setupInitialLayout(texture->rtResource);
            if (!texture->bSameReadWriteTexture)
            {
                texture->textureResource->init();
                cmdList->setupInitialLayout(texture->textureResource);
            }
        });
}

void RenderTargetTexture::release(RenderTargetTexture* texture)
{
    ImageResource* rtResource = texture->rtResource;
    ImageResource* textureResource = texture->textureResource;
    ENQUEUE_COMMAND(RtDestroyTexture)(
        [rtResource, textureResource](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            if(textureResource)
            {
                textureResource->release();
                if (rtResource != textureResource)
                {
                    delete textureResource;
                }
            }
            if(rtResource)
            {
                rtResource->release();
                delete rtResource;
            }
        });

    texture->textureResource = nullptr;
    texture->rtResource = nullptr;
}

//////////////////////////////////////////////////////////////////////////
/// RenderTargetTextureCube
//////////////////////////////////////////////////////////////////////////

void RenderTargetTextureCube::reinitResources()
{
    if (dataFormat != rtResource->imageFormat())
    {
        // Since Texture format can only be set in constructor
        RenderTargetTexture::release(this);
        RenderTargetTextureCube::init(this);
    }
    else
    {
        RenderTargetTexture::reinitResources();
    }
}

void RenderTargetTextureCube::init(RenderTargetTextureCube* texture)
{

    texture->textureResource = texture->rtResource = new GraphicsCubeRTImageResource(texture->dataFormat);

    texture->rtResource->setResourceName(texture->textureName);
    texture->rtResource->setShaderUsage(texture->bSameReadWriteTexture ? EImageShaderUsage::Sampling : 0);
    texture->rtResource->setSampleCounts(texture->getSampleCount());
    texture->rtResource->setImageSize(texture->textureSize);
    texture->rtResource->setNumOfMips(texture->mipCount);

    if (!texture->bSameReadWriteTexture)
    {
        texture->rtResource->setResourceName(texture->textureName + "_RT");

        texture->textureResource = new GraphicsCubeImageResource(texture->dataFormat);
        texture->textureResource->setResourceName(texture->textureName);
        texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
        texture->textureResource->setSampleCounts(EPixelSampleCount::SampleCount1);
        texture->textureResource->setImageSize(texture->textureSize);
        texture->textureResource->setNumOfMips(texture->mipCount);
    }

    ENQUEUE_COMMAND(RtInitTexture)(
        [texture](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            texture->rtResource->init();
            cmdList->setupInitialLayout(texture->rtResource);
            if (!texture->bSameReadWriteTexture)
            {
                texture->textureResource->init();
                cmdList->setupInitialLayout(texture->textureResource);
            }
        });
}

RenderTargetTextureCube* RenderTargetTextureCube::createTexture(const RenderTextureCreateParams& createParams)
{
    RenderTargetTextureCube* texture = new RenderTargetTextureCube();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount == 0)
    {
        texture->mipCount = Math::min((uint32)(1 + Math::floor(Math::log2(
            (float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))), createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->bIsSrgb = createParams.bIsSrgb;
    texture->bSameReadWriteTexture = createParams.bSameReadWriteTexture;
    texture->rtFormat = createParams.format;
    if (createParams.bIsSrgb)
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormat<true>(createParams.format, EPixelDataFormat::BGRA_U8_Norm);
    }
    else
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormat<false>(createParams.format, EPixelDataFormat::BGRA_U8_Norm);
    }
    texture->layerCount = 6;

    // Dependent values
    texture->setSampleCount(createParams.bSameReadWriteTexture ? EPixelSampleCount::SampleCount1 : createParams.sampleCount);
    texture->setFilteringMode(createParams.filtering);

    RenderTargetTextureCube::init(texture);
    return texture;
}

void RenderTargetTextureCube::destroyTexture(RenderTargetTextureCube* texture)
{
    RenderTargetTexture::destroyTexture(texture);
}

//////////////////////////////////////////////////////////////////////////
/// RenderTargetTextureArray
//////////////////////////////////////////////////////////////////////////

void RenderTargetTextureArray::setLayerCount(uint32 count)
{
    if (layerCount != count)
    {
        layerCount = count;
        markResourceDirty();
    }
}

void RenderTargetTextureArray::destroyTexture(RenderTargetTextureArray* texture)
{
    RenderTargetTexture::destroyTexture(texture);
}

RenderTargetTextureArray* RenderTargetTextureArray::createTexture(const RenderTextureArrayCreateParams& createParams)
{
    RenderTargetTextureArray* texture = new RenderTargetTextureArray();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount == 0)
    {
        texture->mipCount = Math::min((uint32)(1 + Math::floor(Math::log2(
            (float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))), createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->bIsSrgb = createParams.bIsSrgb;
    texture->bSameReadWriteTexture = createParams.bSameReadWriteTexture;
    texture->rtFormat = createParams.format;
    if (createParams.bIsSrgb)
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormat<true>(createParams.format, EPixelDataFormat::BGRA_U8_Norm);
    }
    else
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormat<false>(createParams.format, EPixelDataFormat::BGRA_U8_Norm);
    }
    texture->setLayerCount(createParams.layerCount);

    // Dependent values
    texture->setSampleCount(createParams.bSameReadWriteTexture ? EPixelSampleCount::SampleCount1 : createParams.sampleCount);
    texture->setFilteringMode(createParams.filtering);

    RenderTargetTexture::init(texture);
    return texture;
}
