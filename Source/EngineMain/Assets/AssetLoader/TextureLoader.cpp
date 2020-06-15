#include "TextureLoader.h"
#include "../AssetLoaderLibrary.h"
#include "../Asset/TextureAsset.h"
#include "../../Core/Types/Colors.h"
#include "../../Core/Platform/LFS/PlatformLFS.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"

#if _DEBUG
#define STBI_FAILURE_USERMSG
#elif _NDEBUG
#define STBI_NO_FAILURE_STRINGS
#endif
#define STBI_ASSERT(x) debugAssert(x);
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

TextureLoader::TextureLoader(const String& texturePath)
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
        int32 channels;
        uint8* pixelData = stbi_load_from_memory(fileData.data(), int32(fileData.size()), &dimX, &dimY, &channels, CHANNEL_NUM);
        if (pixelData == nullptr)
        {
            Logger::error("Texture Loader", "%s : Failed loading image - %s", __func__, stbi_failure_reason());
            bLoaded = false;
        }
        else
        {
            textureDimension.x = uint32(dimX);
            textureDimension.y = uint32(dimY);

            int32 pixelsCount = dimY * dimX;
            texturePixelData.resize(pixelsCount);
            for (int32 i = 0; i < pixelsCount; ++i)
            {
                int32 pixelStart = i * CHANNEL_NUM;
                texturePixelData[i] = Color(pixelData[pixelStart], pixelData[pixelStart + 1], pixelData[pixelStart + 2], pixelData[pixelStart + 3]);
            }
            bLoaded = true;

            stbi_image_free(pixelData);
        }
    }
    else
    {
        Logger::error("Texture Loader", "%s : Failed opening texture file - %s", __func__, textureFile.getFileName().getChar());
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
