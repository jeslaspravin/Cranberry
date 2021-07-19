#pragma once

#include "Asset/AssetHeader.h"

#include <unordered_map>
#include <vector>

class AssetBase;
class String;

class AssetManager
{
private:
    std::unordered_map<AssetHeader, AssetBase*> assetsRegistered;
    std::vector<String> preloadingPaths;
    bool bIsLoaded = false;
private:
    void loadUnderPath(const String& scanPath);
    std::vector<AssetBase*> loadAsset(const AssetHeader& header);
public:
    AssetManager() = default;

    void load();
    void unload();
    // Post unload when all assets are clear to be destroyed
    void clearToDestroy();

    // Paths are relative to application path
    void addPathsToScan(const String& scanPath);
    // Relative to application assets root path
    AssetBase* getOrLoadAsset(const String& relAssetPath);
    AssetBase* getOrLoadAsset(const AssetHeader& header);
    AssetBase* getAsset(const String& assetName) const;

    template<EAssetType::Type AssetType>
    std::vector<AssetBase*> getAssetsOfType()
    {
        std::vector<AssetBase*> assets;
        for (const std::pair<const AssetHeader, AssetBase*>& asset : assetsRegistered)
        {
            if (asset.first.type == AssetType)
            {
                assets.emplace_back(asset.second);
            }
        }
        return assets;
    }
};