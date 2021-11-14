#pragma once

#include "AssetObject.h"
#include "Math/CoreMathTypedefs.h"

class TextureBase;
class LinearColor;

class EnvironmentMapAsset : public AssetBase, public ICleanupAsset
{
private:
    std::vector<LinearColor> tempPixelData;
    Size2D textureDimension;

    TextureBase* envMap;
    TextureBase* diffuseIrradMap;
    TextureBase* specularIrradMap;
public:

    /* AssetBase Implementation */
    ICleanupAsset* cleanableAsset() override;
    /* ICleanupAsset Implementation */
    void initAsset() override;
    void clearAsset() override;
    /* Overrides ends */

    void setTempPixelData(const std::vector<LinearColor>& pixelData);
    void setTextureSize(const Size2D& dimension);
    TextureBase* getEnvironmentMap() const;
    TextureBase* getSpecularIrradianceMap() const;
    TextureBase* getDiffuseIrradianceMap() const;
};
