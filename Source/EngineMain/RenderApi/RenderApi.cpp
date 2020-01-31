#include "RenderApi.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"

void RenderApi::initialize()
{
	graphicsInstance = new GraphicInstance();
	graphicsInstance->load();
}

void RenderApi::destroy()
{
	graphicsInstance->unload();
	delete graphicsInstance;
	graphicsInstance = nullptr;
}
