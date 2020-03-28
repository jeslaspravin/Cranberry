#pragma once
#include "GraphicsResources.h"
#include "../../Core/Math/CoreMathTypedefs.h"
#include "../../Core/String/String.h"


namespace EPixelDataFormat
{
    enum Type
    {
        Undefined,
        RGBA_UI8_Packed,
        RGBA_SI8_Packed,
        RGBA_UI8_SrgbPacked,
        RGBA_U8_NormPacked,     /* 0 to 255 gives 0.0f - 1.0f per comp */
        RGBA_S8_NormPacked,     /* -127 to 127 gives -1.0f - 1.0f per comp( -128 gets clamped to -127 ) */
        RGBA_U8_ScaledPacked,   /* Just converts the value directly as float 0.0f - 255.0f per comp */
        RGBA_S8_ScaledPacked,   /* Just converts the value directly as float -128.0f - 127.0f per comp */
        R_UI32,
        R_SI32,
        R_SF32
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

    String memoryResName;

public:
    virtual uint64 getResourceSize() const { return 0; }
    virtual bool isValid() { return false; }
    bool isStagingResource() const { return bIsStagingResource; }

    /* GraphicsResource overrides */
    String getResourceName() const override;
    void setResourceName(const String& name) override;
    /* overrides ends */
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