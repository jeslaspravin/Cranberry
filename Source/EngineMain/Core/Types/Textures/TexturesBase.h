#pragma once
#include "../../../RenderInterface/CoreGraphicsTypes.h"
#include "../../Math/CoreMathTypedefs.h"

class ImageResource;

struct TextureBaseCreateParams
{
    String textureName;
};
// Always clear using destroyTexture and create using createTexture
class TextureBase
{
protected:
    ImageResource* textureResource = nullptr;
    Size3D textureSize;
    uint32 mipCount;
    EPixelSampleCount::Type sampleCount;
    EPixelDataFormat::Type dataFormat;
    String textureName;
public:

    ImageResource* getTextureResource() const { return textureResource; }
    EPixelSampleCount::Type getSampleCount() const { return sampleCount; }
    EPixelDataFormat::Type getFormat() const { return dataFormat; }
    const String& getTextureName() const { return textureName; }

    // Each type of texture has to provide an implementation for this template
    template<typename TextureType,typename CreateParamType>
    static std::enable_if_t<std::is_base_of_v<TextureBase, TextureType>, TextureType>* createTexture(const CreateParamType& createParam)
    {
        return TextureType::createTexture(createParam);
    }
    // Each type of texture has to provide an implementation for this template
    template<typename TextureType>
    static std::enable_if_t<std::is_base_of_v<TextureBase, TextureType>, void> destroyTexture(TextureBase* texture)
    {
        return TextureType::destroyTexture(static_cast<TextureType*>(texture));
    }

protected:
    TextureBase() = default;
    ~TextureBase() = default;
};