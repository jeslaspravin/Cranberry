#pragma once
#include "GraphicsResources.h"
#include "../../Core/Platform/PlatformTypes.h"
#include <glm/detail/type_vec3.hpp>

class MemoryResource : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(MemoryResource, , GraphicsResource, )
public:
    virtual uint64 getResourceSize() { return 0; }
};

class BufferResource : public MemoryResource
{
    DECLARE_GRAPHICS_RESOURCE(BufferResource, , MemoryResource, )
};

namespace EImageDataFormat
{
    enum Type
    {

    };
}
class ImageResource : public MemoryResource
{
    DECLARE_GRAPHICS_RESOURCE(ImageResource, , MemoryResource, )

protected:
    EImageDataFormat::Type dataFormat;
    uint32 numOfMips = 0;
    glm::vec<3,uint32, glm::defaultp> dimensions = { 256,256,1 };
    uint32 layerCount = 1;
};