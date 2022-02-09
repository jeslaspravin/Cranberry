/*!
 * \file TextureLoader.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Assets/AssetLoader/TextureLoader.h"
#include "Assets/AssetLoader/StbWrapper.h"
#include "Assets/AssetLoaderLibrary.h"
#include "Assets/Asset/TextureAsset.h"
#include "Types/Colors.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Math/Math.h"
#include "Core/Types/Textures/ImageUtils.h"
#include "Logger/Logger.h"

#include <array>

TextureLoader::TextureLoader(const String& texturePath)
    : bIsNormal(false)
{
    PlatformFile textureFile(texturePath);
    textureFile.setFileFlags(EFileFlags::Read | EFileFlags::OpenExisting);
    textureName = PathFunctions::stripExtension(textureFile.getFileName());// Extension is passed in as dummy(same textureName)
    if (textureFile.exists() && textureFile.openFile())
    {
        std::vector<uint8> fileData;
        textureFile.read(fileData);
        textureFile.closeFile();

        int32 dimX;
        int32 dimY;
        uint8* texelData = STB::loadFromMemory(fileData.data(), int32(fileData.size()), &dimX, &dimY, &channelsCount, CHANNEL_NUM);

        if (texelData == nullptr)
        {
            LOG_ERROR("Texture Loader", "%s() : Failed loading image[%s] - %s", __func__, textureName.getChar(), STB::lastFailure());
            bLoaded = false;
        }
        else
        {
            textureDimension.x = uint32(dimX);
            textureDimension.y = uint32(dimY);

            int32 pixelsCount = dimY * dimX;

            bIsNormal = isNormalTexture(texelData);

            textureTexelData.resize(pixelsCount);
            memcpy(textureTexelData.data(), texelData, pixelsCount * CHANNEL_NUM);

            // If normal we are inverting x value to account for flip of texture in u channel along tangent axis
            if (bIsNormal)
            {
                for (int32 i = 0; i < pixelsCount; ++i)
                {
                    textureTexelData[i].setR(uint8(Math::clamp(255 - textureTexelData[i].r(), 0, 255)));
                }
            }
            bLoaded = true;

            STB::deallocStbBuffer(texelData);
        }
    }
    else
    {
        LOG_ERROR("Texture Loader", "%s() : Failed opening texture file - %s", __func__, textureFile.getFileName().getChar());
        bLoaded = false;
    }
}

bool TextureLoader::isNormalTexture(const uint8* texels) const
{
    bool isNormal = false;
    const uint32 pixelsCount = textureDimension.x * textureDimension.y;

    // New way based on histogram
    std::array<float, 32> histogram[3];
    ImageUtils::calcHistogramRGB(histogram[0].data(), histogram[1].data(), histogram[2].data(), uint32(histogram[0].size())
        , texels, textureDimension.x, textureDimension.y, CHANNEL_NUM);

    float rgMaxWeight = 0;
    uint32 rgMaxLum = 0;
    float blueMaxWeight = 0;
    uint32 blueMaxLum = 0;
    for (uint32 i = 0; i < uint32(histogram[0].size()); ++i)
    {
        const float rgWeight = histogram[0][i] * histogram[1][i];
        if (rgMaxWeight < rgWeight)
        {
            rgMaxLum = i;
            rgMaxWeight = rgWeight;
        }

        if (blueMaxWeight < histogram[2][i])
        {
            blueMaxLum = i;
            blueMaxWeight = histogram[2][i];
        }
    }
    rgMaxLum = uint32((rgMaxLum * 256 / histogram->size()) + (128 / histogram->size()));
    blueMaxLum = uint32((blueMaxLum * 256 / histogram->size()) + (128 / histogram->size()));

    // If RG is in the middle of histogram as normal map must have balanced normal shifts
    // and
    // B is in end of spectrum
    if (Math::abs(rgMaxLum - 127.5f) < 17.5f && blueMaxLum > 200)
    {
        isNormal = true;
        LOG("Texture Loader", "%s() : Texture %s with Max Red Green lum %u Max RG weight %0.3f, Max Blue lum %u Max B weight %0.3f is determined as normal texture"
            , __func__, textureName.getChar()
            , rgMaxLum, rgMaxWeight, blueMaxLum, blueMaxWeight);
    }

    // Old way - Based on normalized pixels
#if 0
    int32 normalizedPixs = 0;
    // Determine if normal map
    for (uint32 i = 0; i < pixelsCount; ++i)
    {
        const uint32 texelStart = i * CHANNEL_NUM;

        auto texel = Vector3D(texels[texelStart], texels[texelStart + 1], texels[texelStart + 2]) * 2 / 255.0f;
        texel -= 1.f;
        const float pixelLen = texel.length();
        // for normal map the pixels must always have value greater than zero for Zs
        normalizedPixs += (channelsCount >= 3 && Math::isEqual(1.0f, pixelLen, 0.1f) && texel.z() > 0.0f) ? 1 : 0;
    }
    const float normalizedPixFrac = float(normalizedPixs) / pixelsCount;
    LOG_DEBUG("Texture Loader", "%s() : Normalization ratio %0.2f for texture %s", __func__, normalizedPixFrac, textureName.getChar());
    if (normalizedPixFrac > 0.25f)
    {
        LOG("Texture Loader", "%s() : Texture %s is marked as normal map, Normalization ratio %0.2f", __func__, textureName.getChar(), normalizedPixFrac);
        isNormal = true;
    }
#endif


    if (!isNormal && textureName.endsWith(TCHAR("_N"), false))
    {
        isNormal = true;
        LOG_DEBUG("Texture Loader", "%s() : Texture %s is determined as normal texture based on suffix _N, Please rename texture if not intended", __func__, textureName.getChar());
    }
    return isNormal;
}

void TextureLoader::fillTextureAsset(TextureAsset* textureAsset) const
{
    textureAsset->setAssetName(textureName);
    textureAsset->setTextureSize(textureDimension);
    textureAsset->setTempPixelData(textureTexelData);
    textureAsset->setChannelCount(uint8(channelsCount));
    textureAsset->setNormalMap(bIsNormal);
}

bool TextureLoader::isLoadSuccess() const
{
    return bLoaded;
}

AssetBase* AssetLoaderLibrary::loadTexture(const String& assetPath)
{
    TextureLoader loader(assetPath);

    if (loader.isLoadSuccess())
    {
        TextureAsset* textureAsset = new TextureAsset();

        loader.fillTextureAsset(textureAsset);
        return textureAsset;
    }
    return nullptr;
}
