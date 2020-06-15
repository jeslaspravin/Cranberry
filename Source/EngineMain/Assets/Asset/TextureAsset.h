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
	TextureBase* getTexture() const;
};
