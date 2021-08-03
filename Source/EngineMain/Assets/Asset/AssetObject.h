#pragma once
#include "AssetHeader.h"

class String;

class ICleanupAsset
{
public:
    // Graphics resource related functions
    virtual void initAsset() = 0;
    virtual void clearAsset() = 0;
};

class AssetBase
{
private:
    friend class AssetManager;
protected:
    AssetHeader assetHeader;
public:
    virtual ~AssetBase() = default;
    virtual ICleanupAsset* cleanableAsset() { return nullptr; }

    void setAssetName(const String& name);
    const String& assetName() const;
};

template <bool bAscending>
struct SortAssetByName
{
    constexpr bool operator()(const AssetBase* lhs, const AssetBase* rhs) const
    {
        if constexpr (bAscending)
        {
            return lhs->assetName() < rhs->assetName();
        }
        else
        {
            return lhs->assetName() > rhs->assetName();
        }
    }
};