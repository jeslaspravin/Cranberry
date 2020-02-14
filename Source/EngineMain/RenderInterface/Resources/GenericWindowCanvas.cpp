#include "GenericWindowCanvas.h"
#include "../../Core/Platform/GenericAppWindow.h"

DEFINE_GRAPHICS_RESOURCE(GenericWindowCanvas)

void GenericWindowCanvas::init()
{
	BaseType::init();
}

void GenericWindowCanvas::release()
{
	BaseType::release();
}

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
