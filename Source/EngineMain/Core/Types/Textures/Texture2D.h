#pragma once
#include "TexturesBase.h"

struct Texture2DCreateParams : public TextureBaseCreateParams
{
    Size2D textureSize;
    EPixelSampleCount::Type sampleCount;
    // If greater than acceptable it will be clamped, if 0 mips get auto calculated from size
    uint32 mipCount = 0;
    // Must be Size == textureSize.x * textureSize.y 
    std::vector<class Color> colorData;
    // whether colorData is encoded in sRGB or needs to be stored in sRGB
    bool bIsSrgb;
};

// Texture 2Ds are texture that will be static and gets created from certain data
class Texture2D : public TextureBase
{
public:
    std::vector<class Color> rawData;
public:
    uint32 getMipCount() const;
    void setData(const std::vector<class Color>& newData,bool bIsSrgb);

    static Texture2D* createTexture(const Texture2DCreateParams& createParams);
    static void destroyTexture(Texture2D* texture2D);
protected:
    Texture2D() = default;
    ~Texture2D() = default;
private:
    static void init(Texture2D* texture);
    static void destroy(Texture2D* texture);
};
