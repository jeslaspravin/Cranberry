/*!
 * \file CubeMapLoader.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Assets/Asset/EnvironmentMapAsset.h"
#include "Assets/AssetLoader/StbWrapper.h"
#include "Assets/AssetLoaderLibrary.h"
#include "Logger/Logger.h"
#include "Types/Colors.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/LFS/PathFunctions.h"

class ICubeMapLoader
{
public:
    virtual bool isLoadSuccess() const = 0;
    virtual void fillCubeMapAsset(EnvironmentMapAsset *envMaps) const = 0;

    virtual ~ICubeMapLoader() = default;
};

class HDRLoader : public ICubeMapLoader
{
private:
    constexpr static uint32 CHANNEL_NUM = 4;

    String textureName;
    UInt2 textureDimension;
    std::vector<LinearColor> textureTexelData;

    bool bLoaded;

public:
    HDRLoader(const String &assetPath);
    /* ICubeMapLoader Overrides */
public:
    bool isLoadSuccess() const final;
    void fillCubeMapAsset(EnvironmentMapAsset *envMaps) const final;
    /* Overrides ends */
};

HDRLoader::HDRLoader(const String &assetPath)
{
    PlatformFile textureFile(assetPath);
    textureFile.setFileFlags(EFileFlags::Read | EFileFlags::OpenExisting);
    textureName = PathFunctions::stripExtension(textureFile.getFileName());
    if (textureFile.exists() && textureFile.openFile())
    {
        std::vector<uint8> fileData;
        textureFile.read(fileData);
        textureFile.closeFile();

        int32 dimX;
        int32 dimY;
        // Since Cartesian to spherical creates coordinates from bottom left to top right
        STB::setLoadVerticalFlipped(true);
        float *texelData = STB::loadFloatFromMemory(fileData.data(), int32(fileData.size()), &dimX, &dimY, nullptr, CHANNEL_NUM);
        STB::setLoadVerticalFlipped(false);

        if (texelData == nullptr)
        {
            LOG_ERROR("HDRLoader", "Failed loading image[{}] - {}", textureName.getChar(), STB::lastFailure());
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
        LOG_ERROR("HDRLoader", "Failed opening texture file - {}", textureFile.getFileName().getChar());
        bLoaded = false;
    }
}

bool HDRLoader::isLoadSuccess() const { return bLoaded; }

void HDRLoader::fillCubeMapAsset(EnvironmentMapAsset *envMaps) const
{
    envMaps->setTempPixelData(textureTexelData);
    envMaps->setAssetName(textureName);
    envMaps->setTextureSize(textureDimension);
}

AssetBase *AssetLoaderLibrary::loadCubeMap(const String &assetPath)
{
    ICubeMapLoader *loader = nullptr;
    EnvironmentMapAsset *envMapsAsset = nullptr;

    String extension;
    PathFunctions::stripExtension(extension, assetPath);
    if (extension.startsWith(TCHAR("hdr")))
    {
        loader = new HDRLoader(assetPath);
    }
    else
    {
        fatalAssertf(false, "Invalid Cube map asset {}", assetPath.getChar());
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