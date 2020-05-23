#include "AssetsManager.h"
#include "Asset/AssetHeader.h"
#include "Asset/AssetObject.h"
#include "AssetLoaderLibrary.h"
#include "../Core/Logger/Logger.h"
#include "../Core/Platform/LFS/PlatformLFS.h"

void AssetManager::loadUnderPath(const String& scanPath)
{
    std::vector<String> foundFiles = FileSystemFunctions::listAllFiles(scanPath,true);
    for (const String& filePath : foundFiles)
    {
        AssetHeader header;
        header.assetPath = filePath;
        header.type = AssetLoaderLibrary::typeFromAssetPath(filePath);
        loadAsset(header);
    }
}

std::vector<AssetBase*> AssetManager::loadAsset(const AssetHeader& header)
{
    std::vector<AssetBase*> loadedAssets;
    switch (header.type)
    {
    case EAssetType::StaticMesh:
        AssetLoaderLibrary::loadStaticMesh(header.assetPath, loadedAssets);
        break;
    case EAssetType::Texture2D:
        Logger::error("AssetManager", "Texture 2D loading is not yet implemented");
    case EAssetType::InvalidType:
    default:
        break;
    }
    for (AssetBase* asset : loadedAssets)
    {
        asset->assetHeader.assetPath = header.assetPath;
        asset->assetHeader.type = header.type;
        assetsRegistered[asset->assetHeader] = asset;
        if (asset->cleanableAsset())
        {
            asset->cleanableAsset()->initAsset();
        }
    }
    return loadedAssets;
}

void AssetManager::load()
{
    String appPath;
    appPath = FileSystemFunctions::applicationDirectory(appPath);
    // Default path
    addPathsToScan("Assets");
    for (const String& scanPath : preloadingPaths)
    {
        String scanFullPath = FileSystemFunctions::combinePath(appPath, scanPath);
        loadUnderPath(scanFullPath);
    }
}

void AssetManager::unload()
{
    for (std::pair<const AssetHeader, AssetBase*>& assetPair : assetsRegistered)
    {
        if (assetPair.second->cleanableAsset())
        {
            assetPair.second->cleanableAsset()->clearAsset();
        }
    }
}

void AssetManager::clearToDestroy()
{
    for (const std::pair<const AssetHeader, AssetBase*>& assetPair : assetsRegistered)
    {
        delete assetPair.second;
    }
    assetsRegistered.clear();
}

void AssetManager::addPathsToScan(const String& scanPath)
{
    preloadingPaths.push_back(scanPath);
    if (bIsLoaded)
    {
        loadUnderPath(scanPath);
    }
}

AssetBase* AssetManager::getOrLoadAsset(const String& assetPath)
{
    AssetHeader header;
    header.assetPath = assetPath;
    header.type = AssetLoaderLibrary::typeFromAssetPath(assetPath);
    header.assetName = PlatformFile(assetPath).getFileName();
    String extension;
    header.assetName = FileSystemFunctions::stripExtension(header.assetName, extension);

    return getOrLoadAsset(header);
}

AssetBase* AssetManager::getOrLoadAsset(const AssetHeader& header)
{
    auto headerItr = assetsRegistered.find(header);
    if (headerItr != assetsRegistered.end())
    {
        return headerItr->second;
    }

    std::vector<AssetBase*> assets = loadAsset(header);
    AssetBase* returnVal = nullptr; 
    for (AssetBase* asset : assets)
    {
        if (asset->assetHeader.assetName == header.assetName)
        {
            returnVal = asset;
            break;
        }
    }
    return returnVal;
}
