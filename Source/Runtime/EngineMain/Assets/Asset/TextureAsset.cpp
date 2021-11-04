#include "TextureAsset.h"
#include "../../Core/Types/Textures/Texture2D.h"

ICleanupAsset* TextureAsset::cleanableAsset()
{
    return this;
}

void TextureAsset::initAsset()
{
    // TODO(Jeslas) : Check the data and categorize the texture as normal map or height map or color texture etc...
    Texture2DCreateParams createParams;
    createParams.filtering = ESamplerFiltering::Linear;
    createParams.mipCount = 0;
    createParams.textureName = assetHeader.assetName;
    createParams.textureSize = textureDimension;
    createParams.colorData = tempPixelData;
    createParams.bIsSrgb = false;
    createParams.componentsCount = componentsCount;
    createParams.bIsNormalMap = bIsNormalMap;
    createParams.defaultColor = ColorConst::BLACK;

    texture = TextureBase::createTexture<Texture2D>(createParams);
}

void TextureAsset::clearAsset()
{
    TextureBase::destroyTexture<Texture2D>(texture);
    texture = nullptr;
}

void TextureAsset::setTempPixelData(const std::vector<Color>& pixelData)
{
    tempPixelData = pixelData;
}

void TextureAsset::setTextureSize(const Size2D& dimension)
{
    textureDimension = dimension;
}

void TextureAsset::setNormalMap(bool bIsNormal)
{
    bIsNormalMap = bIsNormal;
}

void TextureAsset::setChannelCount(uint8 count)
{
    componentsCount = count;
}

TextureBase* TextureAsset::getTexture() const
{
    return texture;
}

