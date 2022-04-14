/*!
 * \file EnvironmentMapAsset.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "AssetObject.h"
#include "Math/CoreMathTypedefs.h"

class TextureBase;
class LinearColor;

class EnvironmentMapAsset
    : public AssetBase
    , public ICleanupAsset
{
private:
    std::vector<LinearColor> tempPixelData;
    Size2D textureDimension;

    TextureBase *envMap;
    TextureBase *diffuseIrradMap;
    TextureBase *specularIrradMap;

public:
    /* AssetBase Implementation */
    ICleanupAsset *cleanableAsset() override;
    /* ICleanupAsset Implementation */
    void initAsset() override;
    void clearAsset() override;
    /* Overrides ends */

    void setTempPixelData(const std::vector<LinearColor> &pixelData);
    void setTextureSize(const Size2D &dimension);
    TextureBase *getEnvironmentMap() const;
    TextureBase *getSpecularIrradianceMap() const;
    TextureBase *getDiffuseIrradianceMap() const;
};
