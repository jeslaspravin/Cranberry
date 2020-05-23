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

    AssetBase* getOrLoadAsset(const String& assetPath);
    AssetBase* getOrLoadAsset(const AssetHeader& header);
};