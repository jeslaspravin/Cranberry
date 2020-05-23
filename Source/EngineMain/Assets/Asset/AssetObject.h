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

    AssetHeader assetHeader;
public:
    virtual ~AssetBase() = default;
    virtual ICleanupAsset* cleanableAsset() { return nullptr; }

    void setAssetName(const String& name);
};