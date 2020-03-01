#pragma once
#include "GraphicsResources.h"
#include "../../Core/Platform/PlatformTypes.h"


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