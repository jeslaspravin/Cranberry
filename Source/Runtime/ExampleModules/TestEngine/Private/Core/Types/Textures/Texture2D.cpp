/*!
 * \file Texture2D.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Texture2D.h"
#include "Logger/Logger.h"
#include "Math/Math.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/GraphicsIntance.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"

uint32 Texture2D::getMipCount() const { return mipCount; }

void Texture2D::setData(const std::vector<class Color> &newData, const Color &defaultColor)
{
    rawData.resize(Math::max((uint32)newData.size(), textureSize.x * textureSize.y));

    dataFormat = EPixelDataFormat::BGRA_U8_Norm;
    memcpy(rawData.data(), newData.data(), newData.size() * sizeof(Color));
    for (uint32 copyIndex = uint32(newData.size()); copyIndex < static_cast<uint32>(rawData.size());
         ++copyIndex)
    {
        rawData[copyIndex] = defaultColor;
    }

    markResourceDirty();
}

bool Texture2D::isSrgb() const
{
    return dataFormat == EPixelDataFormat::BGRA_U8_SRGB || dataFormat == EPixelDataFormat::RG_U8_SRGB
           || dataFormat == EPixelDataFormat::R_U8_SRGB;
}

EPixelDataFormat::Type Texture2D::determineDataFormat(
    bool bIsSrgb, bool bIsNormalMap, uint8 componentCount)
{
    EPixelDataFormat::Type dataFormat;
    if (bIsNormalMap)
    {
        // EPixelDataFormat::A2BGR10_U32_NormPacked is taking too long to be copied due to bit
        // manipulations Change this to EPixelDataFormat::A2BGR10_U32_NormPacked after custom
        // serialized assets are added to engine
        dataFormat = EPixelDataFormat::BGRA_U8_Norm;
        // dataFormat = EPixelDataFormat::A2BGR10_U32_NormPacked;
    }
    else
    {
        switch (componentCount)
        {
        case 1:
            dataFormat = bIsSrgb ? EPixelDataFormat::R_U8_SRGB : EPixelDataFormat::R_U8_Norm;
            break;
        case 2:
            dataFormat = bIsSrgb ? EPixelDataFormat::RG_U8_SRGB : EPixelDataFormat::RG_U8_Norm;
            break;
        case 3:
        case 4:
        default:
            dataFormat = bIsSrgb ? EPixelDataFormat::BGRA_U8_SRGB : EPixelDataFormat::BGRA_U8_Norm;
            break;
        }
    }
    return dataFormat;
}

Texture2D *Texture2D::createTexture(const Texture2DCreateParams &createParams)
{
    Texture2D *texture = new Texture2D();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount != 0)
    {
        texture->mipCount = Math::min(
            (uint32)(1
                     + Math::floor(Math::log2(
                         (float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))),
            createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->setData(createParams.colorData, createParams.defaultColor);
    // Dependent values
    texture->setSampleCount(EPixelSampleCount::SampleCount1); // MS not possible for read only textures
    texture->setFilteringMode(createParams.filtering);

    Texture2D::init(
        texture, createParams.bIsNormalMap, createParams.bIsSrgb, createParams.componentsCount);
    return texture;
}

void Texture2D::destroyTexture(Texture2D *texture2D)
{
    Texture2D::destroy(texture2D);
    delete texture2D;
    texture2D = nullptr;
}

void Texture2D::reinitResources()
{
    TextureBase::reinitResources();

    ENQUEUE_COMMAND(ReinitTexture2D)
    (
        [this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
            const GraphicsHelperAPI *graphicsHelper)
        {
            const EPixelDataFormat::PixelFormatInfo *formatInfo
                = EPixelDataFormat::getFormatInfo(dataFormat);
            bool bIsNormalMap = (formatInfo->componentSize[uint8(EPixelComponent::R)] > 8);
            if (textureResource->isValid())
            {
                textureResource->reinitResources();
                if (bIsNormalMap)
                {
                    cmdList->copyToImageLinearMapped(textureResource, rawData);
                }
                else
                {
                    cmdList->copyToImage(textureResource, rawData);
                }
            }
            else
            {
                Texture2D::init(this, bIsNormalMap, isSrgb(), formatInfo->componentCount);
            }
        });
}

void Texture2D::init(Texture2D *texture, bool bIsNormalMap, bool bIsSrgb, uint8 componentCount)
{
    EPixelDataFormat::Type dataFormat
        = Texture2D::determineDataFormat(bIsSrgb, bIsNormalMap, componentCount);
    ImageResourceCreateInfo imageCI{ .imageFormat = dataFormat,
        .dimensions = texture->textureSize,
        .numOfMips = texture->mipCount,
        .layerCount = 1 };

    ENQUEUE_COMMAND(InitTexture2D)
    (
        [texture, bIsNormalMap, imageCI](IRenderCommandList *cmdList,
            IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            texture->textureResource = graphicsHelper->createImage(graphicsInstance, imageCI);
            texture->textureResource->setResourceName(texture->textureName);
            texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
            texture->textureResource->setSampleCounts(texture->getSampleCount());
            texture->textureResource->init();

            if (bIsNormalMap)
            {
                cmdList->copyToImageLinearMapped(texture->textureResource, texture->rawData);
            }
            else
            {
                cmdList->copyToImage(texture->textureResource, texture->rawData);
            }
        });
}

void Texture2D::destroy(Texture2D *texture)
{
    ImageResourceRef textureResource = texture->textureResource;
    ENQUEUE_COMMAND(DestroyTexture2D)
    ([textureResource](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
         const GraphicsHelperAPI *graphicsHelper) { textureResource->release(); });

    texture->textureResource.reset();
}

//////////////////////////////////////////////////////////////////////////
// RW texture
//////////////////////////////////////////////////////////////////////////

void Texture2DRW::destroy(Texture2DRW *texture)
{
    ImageResourceRef textureResource = texture->textureResource;
    ENQUEUE_COMMAND(DestroyTexture2D)
    ([textureResource](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
         const GraphicsHelperAPI *graphicsHelper) { textureResource->release(); });

    texture->textureResource = nullptr;
}

void Texture2DRW::init(Texture2DRW *texture)
{
    ImageResourceCreateInfo imageCI{ .imageFormat = texture->dataFormat,
        .dimensions = texture->textureSize,
        .numOfMips = texture->mipCount,
        .layerCount = 1 };

    ENQUEUE_COMMAND(InitTexture2D)
    (
        [texture, imageCI](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
            const GraphicsHelperAPI *graphicsHelper)
        {
            texture->textureResource = graphicsHelper->createImage(graphicsInstance, imageCI);
            texture->textureResource->setResourceName(texture->textureName);
            texture->textureResource->setShaderUsage(
                texture->bIsWriteOnly ? EImageShaderUsage::Writing
                                      : EImageShaderUsage::Sampling | EImageShaderUsage::Writing);
            texture->textureResource->setSampleCounts(texture->getSampleCount());
            texture->textureResource->init();
            cmdList->setupInitialLayout(texture->textureResource);
            // #TODO(Jeslas) : Should we copy linear mapped for floats?
            if (!(texture->bIsWriteOnly || EPixelDataFormat::isDepthFormat(texture->dataFormat)
                    || EPixelDataFormat::isFloatingFormat(texture->dataFormat)))
            {
                cmdList->copyToImage(texture->textureResource, texture->rawData);
            }
        });
}

void Texture2DRW::reinitResources()
{
    TextureBase::reinitResources();

    ENQUEUE_COMMAND(ReinitTexture2D)
    (
        [this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
            const GraphicsHelperAPI *graphicsHelper)
        {
            if (textureResource->isValid())
            {
                textureResource->reinitResources();
                cmdList->setupInitialLayout(textureResource);
                // #TODO(Jeslas) : Should we copy linear mapped for floats?
                if (!(bIsWriteOnly || EPixelDataFormat::isDepthFormat(dataFormat)
                        || EPixelDataFormat::isFloatingFormat(dataFormat)))
                {
                    cmdList->copyToImage(textureResource, rawData);
                }
            }
            else
            {
                Texture2DRW::init(this);
            }
        });
}

void Texture2DRW::destroyTexture(Texture2DRW *texture2D)
{
    if (texture2D)
    {
        Texture2DRW::destroy(texture2D);
        delete texture2D;
        texture2D = nullptr;
    }
}

Texture2DRW *Texture2DRW::createTexture(const Texture2DRWCreateParams &createParams)
{
    Texture2DRW *texture = new Texture2DRW();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount == 0)
    {
        texture->mipCount = Math::min(
            (uint32)(1
                     + Math::floor(Math::log2(
                         (float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))),
            createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->bIsWriteOnly = createParams.bIsWriteOnly;
    texture->setData(createParams.colorData, createParams.defaultColor, false);
    // Dependent values
    texture->setSampleCount(EPixelSampleCount::SampleCount1); // MS not possible for read only textures
    texture->setFilteringMode(createParams.filtering);
    texture->dataFormat = createParams.format;

    Texture2DRW::init(texture);
    return texture;
}

void Texture2DRW::setData(
    const std::vector<class Color> &newData, const Color &defaultColor, bool bIsSrgb)
{
    rawData.resize(Math::max((uint32)newData.size(), textureSize.x * textureSize.y));

    int32 copyIndex = 0;
    if (bIsSrgb)
    {
        dataFormat = EPixelDataFormat::RGBA_U8_SRGB;
        for (; copyIndex < static_cast<int32>(newData.size()); ++copyIndex)
        {
            rawData[copyIndex] = newData[copyIndex].toSrgb();
        }
    }
    else
    {
        dataFormat = EPixelDataFormat::RGBA_U8_Norm;
        memcpy(rawData.data(), newData.data(), newData.size() * sizeof(Color));
    }
    for (; copyIndex < static_cast<int32>(rawData.size()); ++copyIndex)
    {
        rawData[copyIndex] = defaultColor;
    }

    markResourceDirty();
}

uint32 Texture2DRW::getMipCount() const { return mipCount; }
