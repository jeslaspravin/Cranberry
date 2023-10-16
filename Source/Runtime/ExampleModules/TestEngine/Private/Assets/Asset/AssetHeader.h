/*!
 * \file AssetHeader.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "String/String.h"
#include "Types/HashTypes.h"

namespace EAssetType
{
enum Type
{
    InvalidType,
    StaticMesh,
    Texture2D,
    CubeMap
};
} // namespace EAssetType

struct AssetHeader
{
    EAssetType::Type type;
    String assetPath;
    String assetName;

    bool operator== (const AssetHeader &other) const
    {
        return type == other.type && assetPath == other.assetPath && assetName == other.assetName;
    }
};

template <>
struct std::hash<AssetHeader>
{

    _NODISCARD size_t operator() (const AssetHeader keyval) const noexcept
    {
        size_t hashVal = hash<EAssetType::Type>{}(keyval.type);
        HashUtility::hashCombine(hashVal, keyval.assetPath);
        HashUtility::hashCombine(hashVal, keyval.assetName);
        return hashVal;
    }
};