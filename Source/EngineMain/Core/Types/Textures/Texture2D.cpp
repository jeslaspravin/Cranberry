#include "Texture2D.h"
#include "../../../RenderInterface/PlatformIndependentHeaders.h"


#include <glm/exponential.hpp>
#include <glm/common.hpp>

uint32 Texture2D::getMipCount() const
{
    return mipCount;
}

void Texture2D::setData(const std::vector<class Color>& newData, bool bIsSrgb)
{
    rawData.resize(glm::max((uint32)newData.size(), textureSize.x * textureSize.y));

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
        rawData[copyIndex] = ColorConst::BLACK;
    }

    if (textureResource)
    {
        // TODO(Jeslas) : copy and generate mips to image resource
    }
}

Texture2D* Texture2D::createTexture(const Texture2DCreateParams& createParams)
{
    Texture2D* texture = new Texture2D();

    texture->mipCount = createParams.mipCount;
    if (createParams.mipCount != 0)
    {
        texture->mipCount = glm::min((uint32)(1 + glm::floor(glm::log2(
            (float)glm::max(createParams.textureSize.x, createParams.textureSize.y)))), createParams.mipCount);
    }
    texture->textureSize = Size3D(createParams.textureSize.x, createParams.textureSize.y, 1);
    texture->sampleCount = createParams.sampleCount;
    texture->textureName = createParams.textureName;
    texture->setData(createParams.colorData,createParams.bIsSrgb);

    Texture2D::init(texture);
    return texture;
}

void Texture2D::destroyTexture(Texture2D* texture2D)
{
    Texture2D::destroy(texture2D);
    delete texture2D;
    texture2D = nullptr;
}

void Texture2D::init(Texture2D* texture)
{
    texture->textureResource = new GraphicsImageResource(texture->dataFormat);
    texture->textureResource->setResourceName(texture->textureName);
    texture->textureResource->setShaderUsage(EImageShaderUsage::Sampling);
    texture->textureResource->setSampleCounts(texture->sampleCount);
    texture->textureResource->setImageSize(texture->textureSize);
    texture->textureResource->setLayerCount(1);
    texture->textureResource->setNumOfMips(texture->mipCount);
    texture->textureResource->init();

    // TODO(Jeslas) : copy and generate mips to image resource
}

void Texture2D::destroy(Texture2D* texture)
{
    texture->textureResource->release();
    delete texture->textureResource;
    texture->textureResource = nullptr;
}
