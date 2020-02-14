#include "GraphicsSyncResource.h"

DEFINE_GRAPHICS_RESOURCE(GraphicsSyncResource)

String GraphicsSyncResource::getResourceName() const
{
    return resourceName;
}

DEFINE_GRAPHICS_RESOURCE(GraphicsFence)
DEFINE_GRAPHICS_RESOURCE(GraphicsSemaphore)
DEFINE_GRAPHICS_RESOURCE(GraphicsTimelineSemaphore)
