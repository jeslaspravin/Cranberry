#include "RenderApi.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"
#include "../Core/Engine/GameEngine.h"

void RenderApi::initAllShaders()
{
    std::vector<GraphicsResource*> shaderResources;
    GraphicsShaderResource::staticType()->allChildDefaultResources(shaderResources, true);
    for (GraphicsResource* shader : shaderResources)
    {
        shader->init();
    }
}

void RenderApi::releaseAllShaders()
{
    std::vector<GraphicsResource*> shaderResources;
    GraphicsShaderResource::staticType()->allChildDefaultResources(shaderResources, true);
    for (GraphicsResource* shader : shaderResources)
    {
        shader->release();
    }
}

void RenderApi::initialize()
{
    graphicsInstance = new GraphicInstance();
    graphicsInstance->load();
    gEngine->appInstance().appWindowManager.initMain();
    graphicsInstance->loadSurfaceDependents();
    gEngine->appInstance().appWindowManager.postInitGraphicCore();
    initAllShaders();
}

void RenderApi::destroy()
{
    releaseAllShaders();
    gEngine->appInstance().appWindowManager.destroyMain();
    graphicsInstance->unload();
    delete graphicsInstance;
    graphicsInstance = nullptr;
}
