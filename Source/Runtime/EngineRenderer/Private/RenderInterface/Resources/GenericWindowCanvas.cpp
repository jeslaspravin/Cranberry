#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "GenericAppWindow.h"
#include "Math/CoreMathTypedefs.h"

DEFINE_GRAPHICS_RESOURCE(GenericWindowCanvas)

void GenericWindowCanvas::setWindow(GenericAppWindow* forWindow)
{
    ownerWindow = forWindow;
}

uint32 GenericWindowCanvas::requestNextImage(SharedPtr<GraphicsSemaphore>* waitOnSemaphore, SharedPtr<GraphicsFence>* waitOnFence /*= nullptr*/)
{
    return 0;
}

String GenericWindowCanvas::getResourceName() const
{
    return ownerWindow->getWindowName();
}
