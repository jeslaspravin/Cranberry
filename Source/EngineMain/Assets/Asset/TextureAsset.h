#pragma once
#include "AssetObject.h"
#include "../../Core/Math/CoreMathTypedefs.h"

class Color;
class TextureBase;

class TextureAsset : public AssetBase, public ICleanupAsset
{
private:
    std::vector<Color> tempPixelData;
    Size2D textureDimension;
    uint8 componentsCount;
    bool bIsNormalMap;
    TextureBase* texture;
public:

    /* AssetBase Implementation */
    ICleanupAsset* cleanableAsset() override;
    /* ICleanupAsset Implementation */
    void initAsset() override;
    void clearAsset() override;
    /* Overrides ends */

    void setTempPixelData(const std::vector<Color>& pixelData);
    void setTextureSize(const Size2D& dimension);
    void setNormalMap(bool bIsNormal);
    void setChannelCount(uint8 count);
    TextureBase* getTexture() const;
    const std::vector<Color>& getPixelData() const { return tempPixelData; }
};
