#pragma once
#include "TexturesBase.h"

// All the color format will be laid out as BGRA(Except in packed it is ABGR) 
namespace ERenderTargetFormat
{
    enum Type
    {
        RT_U8,// Unsigned int8 normalized between 0.0 to 1.0
        RT_U8Packed,
        RT_U8_NoAlpha,
        RT_SF32, // signed float
    };
}

struct RenderTextureCreateParams : public TextureBaseCreateParams
{
    Size2D textureSize;
    EPixelSampleCount::Type sampleCount;
    // If greater than acceptable it will be clamped, if 0 mips get auto calculated from size
    uint32 mipCount = 1;
    ERenderTargetFormat::Type format;
    bool bIsSrgb = false;
};

class RenderTargetTexture : public TextureBase
{
private:
    ERenderTargetFormat::Type rtFormat;
    ImageResource* rtResource = nullptr;
    bool bIsSrgb;
protected:
    RenderTargetTexture() = default;
    ~RenderTargetTexture() = default;
public:

    ERenderTargetFormat::Type getRtFormat() const { return rtFormat; }
    ImageResource* getRtTexture() const { return rtResource; }

    static RenderTargetTexture* createTexture(const RenderTextureCreateParams& createParams);
    static void destroyTexture(RenderTargetTexture* texture);
private:
    static void init(RenderTargetTexture* texture);
    static void release(RenderTargetTexture* texture);
};