/*!
 * \file TextureAsset.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "AssetObject.h"
#include "Math/CoreMathTypedefs.h"

class Color;
class TextureBase;

class TextureAsset
    : public AssetBase
    , public ICleanupAsset
{
private:
    std::vector<Color> tempPixelData;
    UInt2 textureDimension;
    uint8 componentsCount;
    bool bIsNormalMap;
    TextureBase *texture;

public:
    /* AssetBase Implementation */
    ICleanupAsset *cleanableAsset() override;
    /* ICleanupAsset Implementation */
    void initAsset() override;
    void clearAsset() override;
    /* Overrides ends */

    void setTempPixelData(const std::vector<Color> &pixelData);
    void setTextureSize(const UInt2 &dimension);
    void setNormalMap(bool bIsNormal);
    void setChannelCount(uint8 count);
    TextureBase *getTexture() const;
    const std::vector<Color> &getPixelData() const { return tempPixelData; }
};
