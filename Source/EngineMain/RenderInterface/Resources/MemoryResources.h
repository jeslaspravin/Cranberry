#pragma once
#include "GraphicsResources.h"
#include "../../Core/Math/CoreMathTypedefs.h"
#include "../../Core/String/String.h"


namespace EPixelDataFormat
{
    enum Type
    {
        Undefined,
        RGBA_U8_Packed,
        RGBA_S8_Packed,
        RGBA_U8_SrgbPacked,
        RGBA_U8_NormPacked,
        RGBA_S8_NormPacked,
        RGBA_U8_ScaledPacked,
        RGBA_S8_ScaledPacked
    };

    struct ImageFormatInfo
    {
        uint32 format;
        uint32 pixelDataSize;
        String formatName;
    };

    const ImageFormatInfo* getFormatInfo(EPixelDataFormat::Type dataFormat);
}

namespace EPixelSampleCount
{
    enum Type
    {
        SampleCount1 = 0x01,
        SampleCount2 = 0x02,
        SampleCount4 = 0x04,
        SampleCount8 = 0x08,
        SampleCount16 = 0x10,
        SampleCount32 = 0x20,
        SampleCount64 = 0x40,
    };
}

namespace EImageShaderUsage
{
    enum Type
    {
        Sampling = 0x01,
        Writing = 0x02
    };
}

class MemoryResource : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(MemoryResource, , GraphicsResource, )

protected:
    // For image this is always used for buffer this is used only in special cases
    EPixelDataFormat::Type dataFormat;
    bool bIsStagingResource;

    MemoryResource(EPixelDataFormat::Type resourceFormat = EPixelDataFormat::Undefined) 
        : dataFormat(resourceFormat)
        , bIsStagingResource(false)
    {}

public:
    virtual uint64 getResourceSize() const { return 0; }
    virtual bool isValid() { return false; }
    bool isStagingResource() const { return bIsStagingResource; }
};

class BufferResource : public MemoryResource
{
    DECLARE_GRAPHICS_RESOURCE(BufferResource, , MemoryResource, )

public:
    // TODO(Jeslas) : Check if this is needed later point of development
    void setAsStagingResource(bool isStaging) { bIsStagingResource = isStaging; }
};

class ImageResource : public MemoryResource
{
    DECLARE_GRAPHICS_RESOURCE(ImageResource, , MemoryResource, )

protected:
    Size3D dimensions = { 256,256,1 };
    uint32 numOfMips = 0;
    uint32 layerCount = 1;
    EPixelSampleCount::Type sampleCounts = EPixelSampleCount::SampleCount1;
    uint32 shaderUsage = EImageShaderUsage::Sampling;
    bool isRenderTarget = false;

    ImageResource() = default;
public:
    ImageResource(EPixelDataFormat::Type imageFormat);

    void setLayerCount(uint32 count);
    uint32 getLayerCount() const { return layerCount; }

    void setSampleCounts(EPixelSampleCount::Type samples) { sampleCounts = samples; }
    void setNumOfMips(uint32 mipCount) { numOfMips = mipCount; }
    uint32 getNumOfMips() const { return numOfMips; };

    void setShaderUsage(uint32 usage) { shaderUsage = usage; }
    void setImageSize(const Size3D& imageSize) { dimensions = imageSize; }
    const Size3D& getImageSize() const { return dimensions; }
};