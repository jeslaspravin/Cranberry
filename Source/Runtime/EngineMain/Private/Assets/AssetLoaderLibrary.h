#pragma once
#include "Assets/Asset/AssetHeader.h"

class AssetBase;

class AssetLoaderLibrary
{
private:
    AssetLoaderLibrary() = default;

public:
    // If asset does not exists in the given path then generic type of that particular asset is returned based on extension
    static EAssetType::Type typeFromAssetPath(const String& assetPath);

    static void loadStaticMesh(const String& assetPath, std::vector<AssetBase*>& staticMeshes);
    static AssetBase* loadTexture(const String& assetPath);
    static AssetBase* loadCubeMap(const String& assetPath);
};
