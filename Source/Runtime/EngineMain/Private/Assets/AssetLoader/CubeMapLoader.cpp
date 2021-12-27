#include "Assets/AssetLoaderLibrary.h"
#include "Assets/AssetLoader/StbWrapper.h"
#include "Types/Colors.h"
#include "Assets/Asset/EnvironmentMapAsset.h"
#include "Logger/Logger.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/PlatformAssertionErrors.h"

class ICubeMapLoader
{
public:
    virtual bool isLoadSuccess() const = 0;
    virtual void fillCubeMapAsset(EnvironmentMapAsset* envMaps) const = 0;
};

class HDRLoader : public ICubeMapLoader
{
private:
    constexpr static uint32 CHANNEL_NUM = 4;

    String textureName;
    Size2D textureDimension;
    std::vector<LinearColor> textureTexelData;

    bool bLoaded;
public:
    HDRLoader(const String& assetPath);
    /* ICubeMapLoader Overrides */
public:
    bool isLoadSuccess() const final;
    void fillCubeMapAsset(EnvironmentMapAsset* envMaps) const final;
    /* Overrides ends */
};

HDRLoader::HDRLoader(const String& assetPath)
{
    PlatformFile textureFile(assetPath);
    textureFile.setFileFlags(EFileFlags::Read | EFileFlags::OpenExisting);
    textureName = PathFunctions::stripExtension(textureFile.getFileName(), textureName);// Extension is passed in as dummy(same textureName)
    if (textureFile.exists() && textureFile.openFile())
    {
        std::vector<uint8> fileData;
        textureFile.read(fileData);
        textureFile.closeFile();

        int32 dimX;
        int32 dimY;
        // Since Cartesian to spherical creates coordinates from bottom left to top right
        STB::setLoadVerticalFlipped(true);
        float* texelData = STB::loadFloatFromMemory(fileData.data(), int32(fileData.size()), &dimX, &dimY, nullptr, CHANNEL_NUM);
        STB::setLoadVerticalFlipped(false);

        if (texelData == nullptr)
        {
            Logger::error("HDRLoader", "%s() : Failed loading image[%s] - %s", __func__, textureName.getChar(), STB::lastFailure());
            bLoaded = false;
        }
        else
        {
            textureDimension.x = uint32(dimX);
            textureDimension.y = uint32(dimY);

            int32 pixelsCount = dimY * dimX;
            textureTexelData.resize(pixelsCount);
            memcpy(textureTexelData.data(), texelData, pixelsCount * CHANNEL_NUM * sizeof(float));

            bLoaded = true;
            STB::deallocStbBuffer(texelData);
        }
    }
    else
    {
        Logger::error("HDRLoader", "%s() : Failed opening texture file - %s", __func__, textureFile.getFileName().getChar());
        bLoaded = false;
    }
}

bool HDRLoader::isLoadSuccess() const
{
    return bLoaded;
}

void HDRLoader::fillCubeMapAsset(EnvironmentMapAsset* envMaps) const
{
    envMaps->setTempPixelData(textureTexelData);
    envMaps->setAssetName(textureName);
    envMaps->setTextureSize(textureDimension);
}

AssetBase* AssetLoaderLibrary::loadCubeMap(const String& assetPath)
{
    ICubeMapLoader* loader = nullptr;
    EnvironmentMapAsset* envMapsAsset = nullptr;

    String extension;
    PathFunctions::stripExtension(assetPath, extension);
    if (extension.startsWith("hdr"))
    {
        loader = new HDRLoader(assetPath);
    }
    else
    {
        fatalAssert(false, "Invalid Cube map asset %s", assetPath.getChar());
        return nullptr;
    }

    if (loader->isLoadSuccess())
    {
        envMapsAsset = new EnvironmentMapAsset();
        loader->fillCubeMapAsset(envMapsAsset);
    }
    delete loader;
    return envMapsAsset;
}