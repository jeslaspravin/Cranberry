#include "RenderInterface/Resources/QueueResource.h"

DEFINE_GRAPHICS_RESOURCE(QueueResourceBase)

void QueueResourceBase::init()
{
    BaseType::init();
    reinitResources();
}

