#include "TextureLoader.h"
#include "../AssetLoaderLibrary.h"
#include "../Asset/TextureAsset.h"
#include "../../Core/Types/Colors.h"
#include "../../Core/Platform/LFS/PlatformLFS.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../../Core/Math/Math.h"

#if _DEBUG
#define STBI_FAILURE_USERMSG
#elif _NDEBUG
#define STBI_NO_FAILURE_STRINGS
#endif
#define STBI_ASSERT(x) debugAssert(x);
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
        uint8* pixelData = stbi_load_from_memory(fileData.data(), int32(fileData.size()), &dimX, &dimY, &channelsCount, CHANNEL_NUM);

        if (pixelData == nullptr)
        {
            Logger::error("Texture Loader", "%s() : Failed loading image - %s", __func__, stbi_failure_reason());
            bLoaded = false;
        }
        else
        {
            textureDimension.x = uint32(dimX);
            textureDimension.y = uint32(dimY);

            int32 pixelsCount = dimY * dimX;

            bIsNormal = textureName.endsWith("_N", false);
            if (bIsNormal)
            {
                Logger::debug("Texture Loader", "%s() : Texture %s is determined as normal texture based on suffix _N, Please rename texture if not intended", __func__, textureFile.getFileName().getChar());
            }
            else
            {
                int32 normalizedPixs = 0;
                // Determine if normal map
                for (int32 i = 0; i < pixelsCount; ++i)
                {
                    int32 pixelStart = i * CHANNEL_NUM;

                    auto pixel = Vector3D(pixelData[pixelStart], pixelData[pixelStart + 1], pixelData[pixelStart + 2]) * 2 / 255.0f;
                    pixel -= 1.f;
                    const float pixelLen = pixel.length();
                    // for normal map the pixels must always have value greater than zero for Zs
                    normalizedPixs += (channelsCount >= 3 && Math::isEqual(1.0f, pixelLen, 0.1f) && pixel.z() > 0.0f) ? 1 : 0;
                }
                const float normalizedPixFrac = float(normalizedPixs) / pixelsCount;
                Logger::debug("Texture Loader", "%s() : Normalization ratio %0.2f for texture %s", __func__, normalizedPixFrac, textureFile.getFileName().getChar());
                if (normalizedPixFrac > 0.25f)
                {
                    Logger::log("Texture Loader", "%s() : Texture %s is marked as normal map", __func__, textureFile.getFileName().getChar());
                    bIsNormal = true;
                }
            }

            texturePixelData.resize(pixelsCount);
            // If normal we are inverting x value to account for flip of texture in u channel along tangent axis
            if (bIsNormal)
            {
                for (int32 i = 0; i < pixelsCount; ++i)
                {
                    int32 pixelStart = i * CHANNEL_NUM;
                    texturePixelData[i] = Color(uint8(Math::clamp(255 - pixelData[pixelStart], 0, 255)), pixelData[pixelStart + 1], pixelData[pixelStart + 2], pixelData[pixelStart + 3]);
                }
            }
            else
            {
                for (int32 i = 0; i < pixelsCount; ++i)
                {
                    int32 pixelStart = i * CHANNEL_NUM;
                    texturePixelData[i] = Color(pixelData[pixelStart], pixelData[pixelStart + 1], pixelData[pixelStart + 2], pixelData[pixelStart + 3]);
                }
            }
            bLoaded = true;

            stbi_image_free(pixelData);
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
    textureAsset->setTempPixelData(texturePixelData);
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
