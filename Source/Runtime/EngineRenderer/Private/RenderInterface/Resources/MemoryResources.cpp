#include "RenderInterface/Resources/MemoryResources.h"
#include "Math/Math.h"

DEFINE_GRAPHICS_RESOURCE(MemoryResource)

void MemoryResource::addRef()
{
    refCounter.fetch_add(1);
}

void MemoryResource::removeRef()
{
    uint32 count = refCounter.fetch_sub(1);
    if (count == 1)
    {
        release();
        delete this;
    }
}

uint32 MemoryResource::refCount() const
{
    return refCounter.load();
}

String MemoryResource::getResourceName() const
{
    return memoryResName;
}

void MemoryResource::setResourceName(const String& name)
{
    memoryResName = name;
}

DEFINE_GRAPHICS_RESOURCE(BufferResource)
DEFINE_GRAPHICS_RESOURCE(ImageResource)


ImageResource::ImageResource(ImageResourceCreateInfo createInfo)
    : MemoryResource(createInfo.imageFormat)
    , dimensions(createInfo.dimensions)
    , numOfMips(createInfo.numOfMips)
    , layerCount(createInfo.layerCount)
{}

uint32 ImageResource::mipCountFromDim()
{
    return (uint32)(1 + Math::floor(Math::log2((float)Math::max(dimensions.x, dimensions.y, dimensions.z))));
}

void ImageResource::setLayerCount(uint32 count)
{
    layerCount = Math::max<uint32>(count, 1);
}

void ImageResource::setSampleCounts(EPixelSampleCount::Type samples)
{
    sampleCounts = samples;
}

void ImageResource::setNumOfMips(uint32 mipCount)
{
    numOfMips = mipCount;
}

void ImageResource::setShaderUsage(uint32 usage)
{
    shaderUsage = usage;
}

void ImageResource::setImageSize(const Size3D& imageSize)
{
    dimensions = imageSize;
}

bool EPixelDataFormat::isDepthFormat(Type dataFormat)
{
    return EPixelDataFormat::DepthFormatBegin <= dataFormat && EPixelDataFormat::DepthFormatEnd >= dataFormat;
}

bool EPixelDataFormat::isStencilFormat(Type dataFormat)
{
    return EPixelDataFormat::StencilDepthEnd <= dataFormat && EPixelDataFormat::StencilDepthEnd >= dataFormat;
}

bool EPixelDataFormat::isFloatingFormat(Type dataFormat)
{
    return EPixelDataFormat::FloatFormatBegin <= dataFormat && EPixelDataFormat::FloatFormatEnd >= dataFormat;
}