/*!
 * \file AssetsManager.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Assets/Asset/AssetHeader.h"

#include <unordered_map>
#include <vector>

class AssetBase;
class String;

class AssetManager
{
private:
    std::unordered_map<AssetHeader, AssetBase *> assetsRegistered;
    std::vector<String> preloadingPaths;
    bool bIsLoaded = false;

private:
    void loadUnderPath(const String &scanPath);
    void loadUnderPathAsync(const String &scanPath);
    std::vector<AssetBase *> loadAsset(const AssetHeader &header);

public:
    AssetManager() = default;

    void load();
    void unload();
    // Post unload when all assets are clear to be destroyed
    void clearToDestroy();

    // Paths are relative to application path
    void addPathsToScan(const String &scanPath);
    // Relative to application assets root path
    AssetBase *getOrLoadAsset(const String &relAssetPath);
    AssetBase *getOrLoadAsset(const AssetHeader &header);
    AssetBase *getAsset(const String &assetName) const;

    template <EAssetType::Type AssetObjectType, typename AssetType>
    std::vector<AssetType *> getAssetsOfType() const
    {
        std::vector<AssetType *> assets;
        for (const std::pair<const AssetHeader, AssetBase *> &asset : assetsRegistered)
        {
            if (asset.first.type == AssetObjectType)
            {
                assets.emplace_back(static_cast<AssetType *>(asset.second));
            }
        }
        return assets;
    }
};