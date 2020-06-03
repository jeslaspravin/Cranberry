#include "Texture2D.h"
#include "../../../RenderInterface/PlatformIndependentHeaders.h"
#include "../../Math/Math.h"
#include "../../../RenderInterface/Rendering/IRenderCommandList.h"

uint32 Texture2D::getMipCount() const
{
    return mipCount;
}

void Texture2D::setData(const std::vector<class Color>& newData, const Color& defaultColor, bool bIsSrgb)
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
        for (; copyIndex < static_cast<int32>(newData.size()); ++copyIndex)
        {
            rawData[copyIndex] = newData[copyIndex];
        }
    }
    for (; copyIndex < static_cast<int32>(rawData.size()); ++copyIndex)
    {
        rawData[copyIndex] = defaultColor;
    }

    markResourceDirty();
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
    texture->setData(createParams.colorData, createParams.defaultColor, createParams.bIsSrgb);
    // Dependent values
    texture->setSampleCount(EPixelSampleCount::SampleCount1);// MS not possible for read only textures
    texture->setFilteringMode(createParams.filtering);

    Texture2D::init(texture);
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
            if (textureResource->isValid())
            {
                textureResource->reinitResources();
                cmdList->copyToImage(textureResource, rawData);
            }
            else
            {
                Texture2D::init(this);
            }
        }, this);
}

void Texture2D::init(Texture2D* texture)
{
    texture->textureResource = new GraphicsImageResource(texture->dataFormat);
    texture->textureResource->setResourceName(texture->textureName);
    texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
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
