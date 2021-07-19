#pragma once
#include "../../Core/String/String.h"
#include "../../Core/Types/HashTypes.h"

namespace EAssetType
{
    enum Type
    {
        InvalidType,
        StaticMesh,
        Texture2D,
        CubeMap
    };
}

struct AssetHeader
{
    EAssetType::Type type;
    // TODO(Jeslas) : Change once proper asset management is introduced.
    String assetPath;
    String assetName;

    bool operator==(const AssetHeader& other) const
    {
        return type == other.type && assetPath == other.assetPath && assetName == other.assetName;
    }
};

template <>
struct std::hash<AssetHeader> {

    _NODISCARD size_t operator()(const AssetHeader keyval) const noexcept {
        size_t hashVal = hash<EAssetType::Type>{}(keyval.type);
        HashUtility::hashCombine(hashVal, keyval.assetPath);
        HashUtility::hashCombine(hashVal, keyval.assetName);
        return hashVal;
    }
};