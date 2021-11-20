#include "RenderInterface/Resources/GraphicsSyncResource.h"


void GraphicsTimelineSemaphore::waitForSignal() const
{
    if (currentValue() > 0)
        return;

    waitForSignal(1);
}

bool GraphicsTimelineSemaphore::isSignaled() const
{
    return isSignaled(1);
}

void GraphicsTimelineSemaphore::resetSignal()
{
    resetSignal(0);
}
