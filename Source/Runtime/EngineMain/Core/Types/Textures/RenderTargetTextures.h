#pragma once
#include "TexturesBase.h"

// All the color format will be laid out as BGRA(Except in packed it is ABGR) 
namespace ERenderTargetFormat
{
    enum Type
    {
        RT_UseDefault,
        RT_U8,// Unsigned int8 normalized between 0.0 to 1.0
        RT_U8Packed,
        RT_U8_NoAlpha,
        RT_NormalMap,
        RT_Depth, // signed float
    };

    template<bool bIsSrgb>
    EPixelDataFormat::Type rtFormatToPixelFormat(ERenderTargetFormat::Type, EPixelDataFormat::Type);
}

struct RenderTextureCreateParams : public TextureBaseCreateParams
{
    Size2D textureSize;
    EPixelSampleCount::Type sampleCount;
    // If greater than acceptable it will be clamped, if 0 mips get auto calculated from size
    uint32 mipCount = 1;
    ERenderTargetFormat::Type format;
    bool bIsSrgb = false;
    bool bSameReadWriteTexture = true;
};

class RenderTargetTexture : public TextureBase
{
protected:
    uint32 layerCount = 1;
    ERenderTargetFormat::Type rtFormat;
    ImageResource* rtResource = nullptr;
    bool bIsSrgb;
    // If using same texture for both reading from shader and writing to shader
    bool bSameReadWriteTexture;

    RenderTargetTexture() = default;
    ~RenderTargetTexture() = default;

    void reinitResources() override;

    static void init(RenderTargetTexture* texture);
    static void release(RenderTargetTexture* texture);
public:

    ERenderTargetFormat::Type getRtFormat() const { return rtFormat; }
    ImageResource* getRtTexture() const { return rtResource; }
    bool isSameReadWriteTexture() const { return bSameReadWriteTexture; }

    void setTextureSize(Size2D newSize);

    static RenderTargetTexture* createTexture(const RenderTextureCreateParams& createParams);
    static void destroyTexture(RenderTargetTexture* texture);
};

class RenderTargetTextureCube : public RenderTargetTexture
{
private:
protected:
    RenderTargetTextureCube() = default;
    ~RenderTargetTextureCube() = default;

    void reinitResources() override;
    static void init(RenderTargetTextureCube* texture);
public:
    static RenderTargetTextureCube* createTexture(const RenderTextureCreateParams& createParams);
    static void destroyTexture(RenderTargetTextureCube* texture);
};

struct RenderTextureArrayCreateParams : public RenderTextureCreateParams
{
    uint32 layerCount = 1;
};

class RenderTargetTextureArray : public RenderTargetTexture
{
private:
protected:
    RenderTargetTextureArray() = default;
    ~RenderTargetTextureArray() = default;
public:
    void setLayerCount(uint32 count);

    static RenderTargetTextureArray* createTexture(const RenderTextureArrayCreateParams& createParams);
    static void destroyTexture(RenderTargetTextureArray* texture);
};