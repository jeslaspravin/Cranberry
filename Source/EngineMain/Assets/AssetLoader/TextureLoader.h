#pragma once
#include "../../Core/String/String.h"
#include "../../Core/Math/CoreMathTypedefs.h"

class TextureAsset;
class Color;

class TextureLoader
{
private:
    constexpr static uint32 CHANNEL_NUM = 4;

    String textureName;
    Size2D textureDimension;
    int32 channelsCount;
    bool bIsNormal;
    std::vector<Color> textureTexelData;

    bool bLoaded;
private:
    bool isNormalTexture(const uint8* texels) const;
public:
    TextureLoader(const String& texturePath);

    void fillTextureAsset(TextureAsset* textureAsset) const;
    bool isLoadSuccess() const;
};