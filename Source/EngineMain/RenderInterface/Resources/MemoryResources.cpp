#include "MemoryResources.h"

#include <glm/common.hpp>

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
    layerCount = glm::max<uint32>(count, 1);
}
