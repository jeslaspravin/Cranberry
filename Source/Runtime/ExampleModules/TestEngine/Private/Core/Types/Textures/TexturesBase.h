/*!
 * \file TexturesBase.h
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
#include "RenderInterface/CoreGraphicsTypes.h"
#include "RenderInterface/Resources/MemoryResources.h"

class ImageResource;

struct TextureBaseCreateParams
{
    String textureName;
    ESamplerFiltering::Type filtering;
};
// Always clear using destroyTexture and create using createTexture
class TextureBase
{
private:
    ESamplerFiltering::Type sampleFiltering;
    EPixelSampleCount::Type sampleCount;

protected:
    ImageResourceRef textureResource = nullptr;
    Size3D textureSize;
    uint32 mipCount;
    EPixelDataFormat::Type dataFormat;
    String textureName;

    bool bNeedsUpdate;

protected:
    void setSampleCount(EPixelSampleCount::Type newSampleCount);
    void setFilteringMode(ESamplerFiltering::Type filtering);
    virtual void reinitResources();

public:
    ImageResourceRef getTextureResource() const { return textureResource; }
    EPixelSampleCount::Type getSampleCount() const { return sampleCount; }
    EPixelDataFormat::Type getFormat() const { return dataFormat; }
    Size2D getTextureSize() const { return textureSize; }
    ESamplerFiltering::Type filteringMode() const { return sampleFiltering; }
    const String &getTextureName() const { return textureName; }

    void markResourceDirty();

    // Each type of texture has to provide an implementation for this template
    template <typename TextureType, typename CreateParamType>
    static std::enable_if_t<std::is_base_of_v<TextureBase, TextureType>, TextureType> *createTexture(const CreateParamType &createParam)
    {
        return TextureType::createTexture(createParam);
    }
    // Each type of texture has to provide an implementation for this template
    template <typename TextureType>
    static std::enable_if_t<std::is_base_of_v<TextureBase, TextureType>, void> destroyTexture(TextureBase *texture)
    {
        return TextureType::destroyTexture(static_cast<TextureType *>(texture));
    }

protected:
    TextureBase() = default;
    ~TextureBase() = default;
};