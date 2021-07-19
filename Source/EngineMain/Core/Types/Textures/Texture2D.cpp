#include "Texture2D.h"
#include "../../../RenderInterface/PlatformIndependentHeaders.h"
#include "../../Math/Math.h"
#include "../../../RenderInterface/Rendering/IRenderCommandList.h"
#include "../../../RenderApi/GBuffersAndTextures.h"

uint32 Texture2D::getMipCount() const
{
    return mipCount;
}

void Texture2D::setData(const std::vector<class Color>& newData, const Color& defaultColor)
{
    rawData.resize(Math::max((uint32)newData.size(), textureSize.x * textureSize.y));

    dataFormat = EPixelDataFormat::BGRA_U8_Norm;
    memcpy(rawData.data(), newData.data(), newData.size() * sizeof(Color));
    for (uint32 copyIndex = uint32(newData.size()); copyIndex < static_cast<uint32>(rawData.size()); ++copyIndex)
    {
        rawData[copyIndex] = defaultColor;
    }

    markResourceDirty();
}

bool Texture2D::isSrgb() const
{
    return dataFormat == EPixelDataFormat::BGRA_U8_SRGB
        || dataFormat == EPixelDataFormat::RG_U8_SRGB 
        || dataFormat == EPixelDataFormat::R_U8_SRGB;
}

EPixelDataFormat::Type Texture2D::determineDataFormat(bool bIsSrgb, bool bIsNormalMap, uint8 componentCount)
{
    EPixelDataFormat::Type dataFormat;
    if (bIsNormalMap)
    {
        dataFormat = EPixelDataFormat::A2BGR10_U32_NormPacked;
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

Texture2D* Texture2D::createTexture(const Texture2DCreateParams& createParams)
{
    Texture2D* texture = new Texture2D();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount != 0)
    {
        texture->mipCount = Math::min((uint32)(1 + Math::floor(Math::log2(
            (float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))), createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->setData(createParams.colorData, createParams.defaultColor);
    // Dependent values
    texture->setSampleCount(EPixelSampleCount::SampleCount1);// MS not possible for read only textures
    texture->setFilteringMode(createParams.filtering);

    Texture2D::init(texture, createParams.bIsNormalMap, createParams.bIsSrgb, createParams.componentsCount);
    return texture;
}

void Texture2D::destroyTexture(Texture2D* texture2D)
{
    Texture2D::destroy(texture2D);
    delete texture2D;
    texture2D = nullptr;
}

void Texture2D::reinitResources()
{
    TextureBase::reinitResources();

    ENQUEUE_COMMAND(ReinitTexture2D, 
        {
            const EPixelDataFormat::PixelFormatInfo * formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);
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
        }, this);
}

void Texture2D::init(Texture2D* texture, bool bIsNormalMap, bool bIsSrgb, uint8 componentCount)
{
    EPixelDataFormat::Type dataFormat = Texture2D::determineDataFormat(bIsSrgb, bIsNormalMap, componentCount);    

    texture->textureResource = new GraphicsImageResource(dataFormat);
    texture->textureResource->setResourceName(texture->textureName);
    texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
    texture->textureResource->setSampleCounts(texture->getSampleCount());
    texture->textureResource->setImageSize(texture->textureSize);
    texture->textureResource->setLayerCount(1);
    texture->textureResource->setNumOfMips(texture->mipCount);

    ENQUEUE_COMMAND(InitTexture2D,
        {
            texture->textureResource->init();
            if (bIsNormalMap)
            {
                cmdList->copyToImageLinearMapped(texture->textureResource, texture->rawData);
            }
            else
            {
                cmdList->copyToImage(texture->textureResource, texture->rawData);
            }
        }
        , texture, bIsNormalMap);
}

void Texture2D::destroy(Texture2D* texture)
{
    ImageResource* textureResource = texture->textureResource;
    ENQUEUE_COMMAND(DestroyTexture2D,
        {
            textureResource->release();
            delete textureResource;
        }, textureResource);

    texture->textureResource = nullptr;
}

//////////////////////////////////////////////////////////////////////////
// RW texture
//////////////////////////////////////////////////////////////////////////

void Texture2DRW::destroy(Texture2DRW* texture)
{
    ImageResource* textureResource = texture->textureResource;
    ENQUEUE_COMMAND(DestroyTexture2D,
        {
            textureResource->release();
            delete textureResource;
        }, textureResource);

    texture->textureResource = nullptr;
}

void Texture2DRW::init(Texture2DRW* texture)
{
    texture->textureResource = new GraphicsImageResource(texture->dataFormat);
    texture->textureResource->setResourceName(texture->textureName);
    texture->textureResource->setShaderUsage(texture->bIsWriteOnly? EImageShaderUsage::Writing : EImageShaderUsage::Sampling | EImageShaderUsage::Writing);
    texture->textureResource->setSampleCounts(texture->getSampleCount());
    texture->textureResource->setImageSize(texture->textureSize);
    texture->textureResource->setLayerCount(1);
    texture->textureResource->setNumOfMips(texture->mipCount);

    ENQUEUE_COMMAND(InitTexture2D,
        {
            texture->textureResource->init();
            cmdList->copyToImage(texture->textureResource, texture->rawData);
        }
    , texture);
}

void Texture2DRW::reinitResources()
{
    TextureBase::reinitResources();

    ENQUEUE_COMMAND(ReinitTexture2D,
        {
            if (textureResource->isValid())
            {
                textureResource->reinitResources();
                cmdList->copyToImage(textureResource, rawData);
            }
            else
            {
                Texture2DRW::init(this);
            }
        }, this);
}

void Texture2DRW::destroyTexture(Texture2DRW* texture2D)
{
    Texture2DRW::destroy(texture2D);
    delete texture2D;
    texture2D = nullptr;
}

Texture2DRW* Texture2DRW::createTexture(const Texture2DRWCreateParams& createParams)
{
    Texture2DRW* texture = new Texture2DRW();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount != 0)
    {
        texture->mipCount = Math::min((uint32)(1 + Math::floor(Math::log2(
            (float)Math::max(createParams.textureSize.x, createParams.textureSize.y)))), createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->textureName = createParams.textureName;
    texture->bIsWriteOnly = createParams.bIsWriteOnly;
    texture->setData(createParams.colorData, createParams.defaultColor, false);
    // Dependent values
    texture->setSampleCount(EPixelSampleCount::SampleCount1);// MS not possible for read only textures
    texture->setFilteringMode(createParams.filtering);
    texture->dataFormat = createParams.format;

    Texture2DRW::init(texture);
    return texture;
}

void Texture2DRW::setData(const std::vector<class Color>& newData, const Color& defaultColor, bool bIsSrgb)
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

uint32 Texture2DRW::getMipCount() const
{
    return mipCount;
}


void GlobalBuffers::createTexture2Ds()
{
    Texture2DCreateParams createParams;
    createParams.bIsSrgb = false;
    createParams.defaultColor = ColorConst::BLACK;
    createParams.mipCount = 1;
    createParams.textureName = "Dummy_Black";
    createParams.textureSize = Size2D(1, 1);
    dummyBlackTexture = TextureBase::createTexture<Texture2D>(createParams);

    createParams.defaultColor = ColorConst::WHITE;
    createParams.textureName = "Dummy_White";
    dummyWhiteTexture = TextureBase::createTexture<Texture2D>(createParams);
}

void GlobalBuffers::destroyTexture2Ds()
{
    TextureBase::destroyTexture<Texture2D>(dummyBlackTexture);
    dummyBlackTexture = nullptr;
    TextureBase::destroyTexture<Texture2D>(dummyWhiteTexture);
    dummyWhiteTexture = nullptr;
}