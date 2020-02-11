#include "GenericWindowCanvas.h"

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
