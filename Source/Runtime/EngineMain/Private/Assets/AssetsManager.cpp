#include "Assets/AssetsManager.h"
#include "Assets/Asset/AssetHeader.h"
#include "Assets/Asset/AssetObject.h"
#include "Assets/AssetLoaderLibrary.h"
#include "Logger/Logger.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Time.h"

void AssetManager::loadUnderPath(const String& scanPath)
{
    Logger::debug("AssetManager", "%s(): Initial asset loaded started", __func__);
    StopWatch loadTime;
    std::vector<String> foundFiles = FileSystemFunctions::listAllFiles(scanPath,true);
    for (const String& filePath : foundFiles)
    {
        AssetHeader header;
        header.assetPath = filePath.replaceAllCopy("\\","/");
        header.type = AssetLoaderLibrary::typeFromAssetPath(filePath);
        loadAsset(header);
        Logger::debug("AssetManager", "%s(): Loaded asset %s in %0.3f Seconds(not including gpu copy)", __func__, header.assetPath.getChar(), loadTime.thisLap());
        loadTime.lap();
    }
    loadTime.stop();
    Logger::debug("AssetManager", "%s(): Loaded all assets in %0.3f Seconds(not including gpu copy)", __func__, loadTime.duration());
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
        if (AssetBase* loadedTextureAsset = AssetLoaderLibrary::loadTexture(header.assetPath))
        {
            loadedAssets.emplace_back(loadedTextureAsset);
        }
        break;
    case EAssetType::CubeMap:
        if (AssetBase* loadedCubeMapAsset = AssetLoaderLibrary::loadCubeMap(header.assetPath))
        {
            loadedAssets.emplace_back(loadedCubeMapAsset);
        }
        break;
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

AssetBase* AssetManager::getOrLoadAsset(const String& relAssetPath)
{
    String newRelPath = relAssetPath.replaceAllCopy("\\", "/");
    String appName;
    AssetHeader header;
    header.assetPath = FileSystemFunctions::combinePath(FileSystemFunctions::applicationDirectory(appName), "Assets", newRelPath);
    header.type = AssetLoaderLibrary::typeFromAssetPath(newRelPath);
    header.assetName = PlatformFile(header.assetPath).getFileName();
    header.assetName = PathFunctions::stripExtension(header.assetName);

    return getOrLoadAsset(header);
}

AssetBase* AssetManager::getOrLoadAsset(const AssetHeader& header)
{
    AssetHeader newHeader = header;
    newHeader.assetPath = header.assetPath.replaceAllCopy("\\", "/");

    auto headerItr = assetsRegistered.find(newHeader);
    if (headerItr != assetsRegistered.end())
    {
        return headerItr->second;
    }

    std::vector<AssetBase*> assets = loadAsset(newHeader);
    AssetBase* returnVal = nullptr; 
    for (AssetBase* asset : assets)
    {
        if (asset->assetHeader.assetName == newHeader.assetName)
        {
            returnVal = asset;
            break;
        }
    }
    return returnVal;
}

AssetBase* AssetManager::getAsset(const String& assetName) const
{
    for (const std::pair<const AssetHeader, AssetBase*>& asset : assetsRegistered)
    {
        if (asset.first.assetName == assetName)
        {
            return asset.second;
        }
    }
    return nullptr;
}
