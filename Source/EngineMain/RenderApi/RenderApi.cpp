#include "RenderApi.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"
#include "../Core/Engine/GameEngine.h"

void RenderApi::initialize()
{
	graphicsInstance = new GraphicInstance();
	graphicsInstance->load();
	gEngine->appInstance().appWindowManager.initMain();
	graphicsInstance->loadSurfaceDependents();
	gEngine->appInstance().appWindowManager.postInitGraphicCore();
}

void RenderApi::destroy()
{
	gEngine->appInstance().appWindowManager.destroyMain();
	graphicsInstance->unload();
	delete graphicsInstance;
	graphicsInstance = nullptr;
}
