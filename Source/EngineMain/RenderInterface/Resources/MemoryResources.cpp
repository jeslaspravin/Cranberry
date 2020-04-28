#include "MemoryResources.h"
#include "../../Core/Math/Math.h"

DEFINE_GRAPHICS_RESOURCE(MemoryResource)

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


ImageResource::ImageResource(EPixelDataFormat::Type imageFormat)
    : MemoryResource(imageFormat)
{

}

void ImageResource::setLayerCount(uint32 count)
{
    layerCount = Math::max<uint32>(count, 1);
}

bool EPixelDataFormat::isDepthFormat(Type dataFormat)
{
    return EPixelDataFormat::DepthFormatBegin <= dataFormat && EPixelDataFormat::DepthFormatEnd >= dataFormat;
}

bool EPixelDataFormat::isStencilFormat(Type dataFormat)
{
    return EPixelDataFormat::StencilDepthEnd <= dataFormat && EPixelDataFormat::StencilDepthEnd >= dataFormat;
}
