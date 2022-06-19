/*!
 * \file AssetsManager.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Assets/AssetsManager.h"
#include "Assets/Asset/AssetHeader.h"
#include "Assets/Asset/AssetObject.h"
#include "Assets/AssetLoaderLibrary.h"
#include "Logger/Logger.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/LFS/Paths.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Time.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"
#include "Types/Platform/Threading/CoPaT/CoroutineWait.h"
#include "Types/Platform/Threading/CoPaT/CoroutineAwaitAll.h"

void AssetManager::loadUnderPath(const String &scanPath)
{
    LOG_DEBUG("AssetManager", "Initial asset load started");
    StopWatch loadTime;
    std::vector<String> foundFiles = FileSystemFunctions::listAllFiles(scanPath, true);
    for (const String &filePath : foundFiles)
    {
        AssetHeader header;
        header.assetPath = PathFunctions::asGenericPath(filePath);
        header.type = AssetLoaderLibrary::typeFromAssetPath(filePath);
        for (AssetBase *asset : loadAsset(header))
        {
            assetsRegistered[asset->assetHeader] = asset;
        }
        LOG_DEBUG("AssetManager", "Loaded asset %s in %0.3f Seconds(not including gpu copy)", header.assetPath.getChar(), loadTime.thisLap());
        loadTime.lap();
    }
    loadTime.stop();
    LOG_DEBUG("AssetManager", "Loaded all assets in %0.3f Seconds(not including gpu copy)", loadTime.duration());
}

void AssetManager::loadUnderPathAsync(const String &scanPath)
{
    LOG("AssetManager", "Initial asset load started");
    StopWatch loadTime;
    std::vector<String> foundFiles = FileSystemFunctions::listAllFiles(scanPath, true);
    std::vector<decltype(loadAssetAsync(std::declval<AssetHeader>()))> asyncTasks;
    asyncTasks.reserve(foundFiles.size());
    for (const String &filePath : foundFiles)
    {
        AssetHeader header;
        header.assetPath = PathFunctions::asGenericPath(filePath);
        header.type = AssetLoaderLibrary::typeFromAssetPath(filePath);
        asyncTasks.emplace_back(loadAssetAsync(header));
    }
    copat::AwaitAllTasks<std::vector<decltype(loadAssetAsync(std::declval<AssetHeader>()))>> allAwaits
        = copat::awaitAllTasks(std::move(asyncTasks));
    for (auto &awaitable : copat::waitOnAwaitable(allAwaits))
    {
        for (AssetBase *asset : awaitable.getReturnValue())
        {
            assetsRegistered[asset->assetHeader] = asset;
        }
    }

    loadTime.stop();
    LOG("AssetManager", "Loaded all assets in %0.3f Seconds(not including gpu copy)", loadTime.duration());
}

copat::JobSystemReturnableTask<std::vector<AssetBase *>, true, copat::EJobThreadType::WorkerThreads>
    AssetManager::loadAssetAsync(AssetHeader header)
{
    StopWatch loadTime;
    std::vector<AssetBase *> assets = loadAsset(header);
    LOG_DEBUG("AssetManager", "Loaded asset %s in %0.3f Seconds(not including gpu copy)", header.assetPath.getChar(), loadTime.duration());
    co_return std::move(assets);
}

std::vector<AssetBase *> AssetManager::loadAsset(const AssetHeader &header)
{
    std::vector<AssetBase *> loadedAssets;
    switch (header.type)
    {
    case EAssetType::StaticMesh:
        AssetLoaderLibrary::loadStaticMesh(header.assetPath, loadedAssets);
        break;
    case EAssetType::Texture2D:
        if (AssetBase *loadedTextureAsset = AssetLoaderLibrary::loadTexture(header.assetPath))
        {
            loadedAssets.emplace_back(loadedTextureAsset);
        }
        break;
    case EAssetType::CubeMap:
        if (AssetBase *loadedCubeMapAsset = AssetLoaderLibrary::loadCubeMap(header.assetPath))
        {
            loadedAssets.emplace_back(loadedCubeMapAsset);
        }
        break;
    case EAssetType::InvalidType:
    default:
        break;
    }
    for (AssetBase *asset : loadedAssets)
    {
        asset->assetHeader.assetPath = header.assetPath;
        asset->assetHeader.type = header.type;
        if (asset->cleanableAsset())
        {
            asset->cleanableAsset()->initAsset();
        }
    }
    return loadedAssets;
}

void AssetManager::load()
{
    String appPath = Paths::applicationDirectory();
    // Default path
    addPathsToScan(TCHAR("Assets"));
    for (const String &scanPath : preloadingPaths)
    {
        String scanFullPath = PathFunctions::combinePath(appPath, scanPath);
        loadUnderPathAsync(scanFullPath);
        // loadUnderPath(scanFullPath);
    }
}

void AssetManager::unload()
{
    for (std::pair<const AssetHeader, AssetBase *> &assetPair : assetsRegistered)
    {
        if (assetPair.second->cleanableAsset())
        {
            assetPair.second->cleanableAsset()->clearAsset();
        }
    }
}

void AssetManager::clearToDestroy()
{
    for (const std::pair<const AssetHeader, AssetBase *> &assetPair : assetsRegistered)
    {
        delete assetPair.second;
    }
    assetsRegistered.clear();
}

void AssetManager::addPathsToScan(const String &scanPath)
{
    preloadingPaths.push_back(scanPath);
    if (bIsLoaded)
    {
        loadUnderPathAsync(scanPath);
        // loadUnderPath(scanPath);
    }
}

AssetBase *AssetManager::getOrLoadAsset(const String &relAssetPath)
{
    String newRelPath = PathFunctions::asGenericPath(relAssetPath);

    AssetHeader header;
    header.assetPath = PathFunctions::combinePath(Paths::applicationDirectory(), TCHAR("Assets"), newRelPath);
    header.type = AssetLoaderLibrary::typeFromAssetPath(newRelPath);
    header.assetName = PlatformFile(header.assetPath).getFileName();
    header.assetName = PathFunctions::stripExtension(header.assetName);

    return getOrLoadAsset(header);
}

AssetBase *AssetManager::getOrLoadAsset(const AssetHeader &header)
{
    AssetHeader newHeader = header;
    newHeader.assetPath = PathFunctions::asGenericPath(header.assetPath);

    auto headerItr = assetsRegistered.find(newHeader);
    if (headerItr != assetsRegistered.end())
    {
        return headerItr->second;
    }

    std::vector<AssetBase *> assets = loadAsset(newHeader);
    AssetBase *returnVal = nullptr;
    for (AssetBase *asset : assets)
    {
        if (asset->assetHeader.assetName == newHeader.assetName)
        {
            returnVal = asset;
            break;
        }
    }
    return returnVal;
}

AssetBase *AssetManager::getAsset(const String &assetName) const
{
    for (const std::pair<const AssetHeader, AssetBase *> &asset : assetsRegistered)
    {
        if (asset.first.assetName == assetName)
        {
            return asset.second;
        }
    }
    return nullptr;
}
