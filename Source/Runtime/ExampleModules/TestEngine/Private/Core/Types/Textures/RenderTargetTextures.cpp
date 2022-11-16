/*!
 * \file RenderTargetTextures.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderTargetTextures.h"
#include "Logger/Logger.h"
#include "Math/Math.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"

namespace ERenderTargetFormat
{
EPixelDataFormat::Type rtFormatToPixelFormatSrgb(ERenderTargetFormat::Type rtFormat, EPixelDataFormat::Type defaultFormat)
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

EPixelDataFormat::Type rtFormatToPixelFormat(ERenderTargetFormat::Type rtFormat, EPixelDataFormat::Type defaultFormat)
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
} // namespace ERenderTargetFormat

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
        ENQUEUE_COMMAND(RtReinitTexture)
        (
            [this](class IRenderCommandList *cmdList, IGraphicsInstance *, const GraphicsHelperAPI *)
            {
                rtResource->reinitResources();
                cmdList->setupInitialLayout(rtResource);
                if (!bSameReadWriteTexture)
                {
                    textureResource->reinitResources();
                    cmdList->setupInitialLayout(textureResource);
                }
            }
        );
    }
}

void RenderTargetTexture::setTextureSize(Size2D newSize)
{
    textureSize = Size3D(newSize.x, newSize.y, 1);
    markResourceDirty();
}

RenderTargetTexture *RenderTargetTexture::createTexture(const RenderTextureCreateParams &createParams)
{
    RenderTargetTexture *texture = new RenderTargetTexture();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount == 0)
    {
        texture->mipCount = Math::min(
            (uint32)(1 + Math::floor(Math::log2((float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))),
            createParams.mipCount
        );
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->bIsSrgb = createParams.bIsSrgb;
    texture->bSameReadWriteTexture = createParams.bSameReadWriteTexture;
    texture->rtFormat = createParams.format;
    texture->layerCount = 1;
    if (createParams.bIsSrgb)
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormatSrgb(createParams.format, EPixelDataFormat::BGRA_U8_Norm);
    }
    else
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormat(createParams.format, EPixelDataFormat::BGRA_U8_Norm);
    }
    // Dependent values
    texture->setSampleCount(createParams.bSameReadWriteTexture ? EPixelSampleCount::SampleCount1 : createParams.sampleCount);
    texture->setFilteringMode(createParams.filtering);

    RenderTargetTexture::init(texture);
    return texture;
}

void RenderTargetTexture::destroyTexture(RenderTargetTexture *texture)
{
    RenderTargetTexture::release(texture);
    delete texture;
}

void RenderTargetTexture::init(RenderTargetTexture *texture)
{
    ImageResourceCreateInfo imageCI{ .imageFormat = texture->dataFormat,
                                     .dimensions = texture->textureSize,
                                     .numOfMips = texture->mipCount,
                                     .layerCount = texture->layerCount };

    ENQUEUE_COMMAND(RtInitTexture)
    (
        [texture, imageCI](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            texture->textureResource = texture->rtResource = graphicsHelper->createRTImage(graphicsInstance, imageCI);
            texture->rtResource->setResourceName(texture->textureName);
            texture->rtResource->setShaderUsage(texture->bSameReadWriteTexture ? EImageShaderUsage::Sampling : 0);
            texture->rtResource->setSampleCounts(texture->getSampleCount());

            if (!texture->bSameReadWriteTexture)
            {
                texture->rtResource->setResourceName(texture->textureName + TCHAR("_RT"));

                texture->textureResource = graphicsHelper->createImage(graphicsInstance, imageCI);
                texture->textureResource->setResourceName(texture->textureName);
                texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
                texture->textureResource->setSampleCounts(EPixelSampleCount::SampleCount1);
            }

            texture->rtResource->init();
            cmdList->setupInitialLayout(texture->rtResource);
            if (!texture->bSameReadWriteTexture)
            {
                texture->textureResource->init();
                cmdList->setupInitialLayout(texture->textureResource);
            }
        }
    );
}

void RenderTargetTexture::release(RenderTargetTexture *texture)
{
    ImageResourceRef rtResource = texture->rtResource;
    ImageResourceRef textureResource = texture->textureResource;
    ENQUEUE_COMMAND(RtDestroyTexture)
    (
        [rtResource,
         textureResource](class IRenderCommandList *, IGraphicsInstance *, const GraphicsHelperAPI *)
        {
            if (textureResource.isValid())
            {
                textureResource->release();
                if (rtResource != textureResource)
                {
                    rtResource->release();
                }
            }
        }
    );

    texture->textureResource.reset();
    texture->rtResource.reset();
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

void RenderTargetTextureCube::init(RenderTargetTextureCube *texture)
{
    ImageResourceCreateInfo imageCI{ .imageFormat = texture->dataFormat, .dimensions = texture->textureSize, .numOfMips = texture->mipCount };

    ENQUEUE_COMMAND(RtInitTexture)
    (
        [texture, imageCI](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            texture->textureResource = texture->rtResource
                = graphicsHelper->createCubeRTImage(graphicsInstance, imageCI, texture->getSampleCount());
            texture->rtResource->setResourceName(texture->textureName);
            texture->rtResource->setShaderUsage(texture->bSameReadWriteTexture ? EImageShaderUsage::Sampling : 0);
            texture->rtResource->setSampleCounts(texture->getSampleCount());

            if (!texture->bSameReadWriteTexture)
            {
                texture->rtResource->setResourceName(texture->textureName + TCHAR("_RT"));

                texture->textureResource = graphicsHelper->createCubeImage(graphicsInstance, imageCI);
                texture->textureResource->setResourceName(texture->textureName);
                texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
                texture->textureResource->setSampleCounts(EPixelSampleCount::SampleCount1);
            }

            texture->rtResource->init();
            cmdList->setupInitialLayout(texture->rtResource);
            if (!texture->bSameReadWriteTexture)
            {
                texture->textureResource->init();
                cmdList->setupInitialLayout(texture->textureResource);
            }
        }
    );
}

RenderTargetTextureCube *RenderTargetTextureCube::createTexture(const RenderTextureCreateParams &createParams)
{
    RenderTargetTextureCube *texture = new RenderTargetTextureCube();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount == 0)
    {
        texture->mipCount = Math::min(
            (uint32)(1 + Math::floor(Math::log2((float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))),
            createParams.mipCount
        );
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->bIsSrgb = createParams.bIsSrgb;
    texture->bSameReadWriteTexture = createParams.bSameReadWriteTexture;
    texture->rtFormat = createParams.format;
    if (createParams.bIsSrgb)
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormatSrgb(createParams.format, EPixelDataFormat::BGRA_U8_Norm);
    }
    else
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormat(createParams.format, EPixelDataFormat::BGRA_U8_Norm);
    }
    texture->layerCount = 6;

    // Dependent values
    texture->setSampleCount(createParams.bSameReadWriteTexture ? EPixelSampleCount::SampleCount1 : createParams.sampleCount);
    texture->setFilteringMode(createParams.filtering);

    RenderTargetTextureCube::init(texture);
    return texture;
}

void RenderTargetTextureCube::destroyTexture(RenderTargetTextureCube *texture) { RenderTargetTexture::destroyTexture(texture); }

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

void RenderTargetTextureArray::destroyTexture(RenderTargetTextureArray *texture) { RenderTargetTexture::destroyTexture(texture); }

RenderTargetTextureArray *RenderTargetTextureArray::createTexture(const RenderTextureArrayCreateParams &createParams)
{
    RenderTargetTextureArray *texture = new RenderTargetTextureArray();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount == 0)
    {
        texture->mipCount = Math::min(
            (uint32)(1 + Math::floor(Math::log2((float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))),
            createParams.mipCount
        );
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->bIsSrgb = createParams.bIsSrgb;
    texture->bSameReadWriteTexture = createParams.bSameReadWriteTexture;
    texture->rtFormat = createParams.format;
    if (createParams.bIsSrgb)
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormatSrgb(createParams.format, EPixelDataFormat::BGRA_U8_Norm);
    }
    else
    {
        texture->dataFormat = ERenderTargetFormat::rtFormatToPixelFormat(createParams.format, EPixelDataFormat::BGRA_U8_Norm);
    }
    texture->setLayerCount(createParams.layerCount);

    // Dependent values
    texture->setSampleCount(createParams.bSameReadWriteTexture ? EPixelSampleCount::SampleCount1 : createParams.sampleCount);
    texture->setFilteringMode(createParams.filtering);

    RenderTargetTexture::init(texture);
    return texture;
}
