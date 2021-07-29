#include "TextureLoader.h"
#include "../AssetLoaderLibrary.h"
#include "../Asset/TextureAsset.h"
#include "../../Core/Types/Colors.h"
#include "../../Core/Platform/LFS/PlatformLFS.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../../Core/Math/Math.h"
#include "../../Core/Types/Textures/ImageUtils.h"

#if _DEBUG
#define STBI_FAILURE_USERMSG
#elif _NDEBUG
#define STBI_NO_FAILURE_STRINGS
#endif
#define STBI_ASSERT(x) debugAssert(x);
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <array>

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
        Logger::log("Texture Loader", "%s() : Texture %s with Max Red Green lum %u Max RG weight %0.3f, Max Blue lum %u Max B weight %0.3f is determined as normal texture"
            , __func__ , textureName.getChar()
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
    Logger::debug("Texture Loader", "%s() : Normalization ratio %0.2f for texture %s", __func__, normalizedPixFrac, textureName.getChar());
    if (normalizedPixFrac > 0.25f)
    {
        Logger::log("Texture Loader", "%s() : Texture %s is marked as normal map, Normalization ratio %0.2f", __func__, textureName.getChar(), normalizedPixFrac);
        isNormal = true;
    }
#endif

    
    if (!isNormal && textureName.endsWith("_N", false))
    {
        isNormal = true;
        Logger::debug("Texture Loader", "%s() : Texture %s is determined as normal texture based on suffix _N, Please rename texture if not intended", __func__, textureName.getChar());
    }
    return isNormal;
}

TextureLoader::TextureLoader(const String& texturePath)
    : bIsNormal(false)
{
    PlatformFile textureFile(texturePath);
    textureFile.setFileFlags(EFileFlags::Read | EFileFlags::OpenExisting);
    textureName = FileSystemFunctions::stripExtension(textureFile.getFileName(), textureName);// Extension is passed in as dummy(same textureName)
    if (textureFile.exists() && textureFile.openFile())
    {
        std::vector<uint8> fileData;
        textureFile.read(fileData);
        textureFile.closeFile();

        int32 dimX;
        int32 dimY;
        uint8* texelData = stbi_load_from_memory(fileData.data(), int32(fileData.size()), &dimX, &dimY, &channelsCount, CHANNEL_NUM);

        if (texelData == nullptr)
        {
            Logger::error("Texture Loader", "%s() : Failed loading image - %s", __func__, stbi_failure_reason());
            bLoaded = false;
        }
        else
        {
            textureDimension.x = uint32(dimX);
            textureDimension.y = uint32(dimY);

            int32 pixelsCount = dimY * dimX;

            bIsNormal = isNormalTexture(texelData);

            textureTexelData.resize(pixelsCount);
            // If normal we are inverting x value to account for flip of texture in u channel along tangent axis
            if (bIsNormal)
            {
                for (int32 i = 0; i < pixelsCount; ++i)
                {
                    int32 pixelStart = i * CHANNEL_NUM;
                    textureTexelData[i] = Color(uint8(Math::clamp(255 - texelData[pixelStart], 0, 255)), texelData[pixelStart + 1], texelData[pixelStart + 2], texelData[pixelStart + 3]);
                }
            }
            else
            {
                for (int32 i = 0; i < pixelsCount; ++i)
                {
                    int32 pixelStart = i * CHANNEL_NUM;
                    textureTexelData[i] = Color(texelData[pixelStart], texelData[pixelStart + 1], texelData[pixelStart + 2], texelData[pixelStart + 3]);
                }
            }
            bLoaded = true;

            stbi_image_free(texelData);
        }
    }
    else
    {
        Logger::error("Texture Loader", "%s() : Failed opening texture file - %s", __func__, textureFile.getFileName().getChar());
        bLoaded = false;
    }
}
#undef STBI_NO_FAILURE_STRINGS
#undef STBI_FAILURE_USERMSG
#undef STB_IMAGE_IMPLEMENTATION
#undef STBI_ASSERT

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
