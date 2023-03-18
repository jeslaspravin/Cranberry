/*!
 * \file TextureLoader.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Math/CoreMathTypedefs.h"
#include "String/String.h"

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
    bool isNormalTexture(const uint8 *texels) const;

public:
    TextureLoader(const String &texturePath);

    void fillTextureAsset(TextureAsset *textureAsset) const;
    bool isLoadSuccess() const;
};