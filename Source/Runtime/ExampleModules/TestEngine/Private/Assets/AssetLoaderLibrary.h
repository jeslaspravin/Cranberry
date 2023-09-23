/*!
 * \file AssetLoaderLibrary.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Assets/Asset/AssetHeader.h"

class AssetBase;

class AssetLoaderLibrary
{
private:
    AssetLoaderLibrary() = default;

public:
    // If asset does not exists in the given path then generic type of that particular asset is returned
    // based on extension
    static EAssetType::Type typeFromAssetPath(const String &assetPath);

    static void loadStaticMesh(const String &assetPath, std::vector<AssetBase *> &staticMeshes);
    static AssetBase *loadTexture(const String &assetPath);
    static AssetBase *loadCubeMap(const String &assetPath);
};
