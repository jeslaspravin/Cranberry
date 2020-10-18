#include "AssetObject.h"

void AssetBase::setAssetName(const String& name)
{
    assetHeader.assetName = name;
}

const String& AssetBase::assetName() const
{
    return assetHeader.assetName;
}
