#include "RenderApi.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"
#include "../Core/Engine/GameEngine.h"
#include "GBuffersAndTextures.h"
#include "../RenderInterface/Rendering/IRenderCommandList.h"
#include "../Core/Platform/PlatformAssertionErrors.h"

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

    GBuffers::initialize();
    renderCmds = IRenderCommandList::genericInstance();
    graphicsInstance->initializeCmds(renderCmds);

    initAllShaders();
}

void RenderApi::postInit()
{
    // Process post init pre-frame render commands
    renderFrame();
}

void RenderApi::preDestroy()
{
    // Process pre-destroy other api clean up commands
    renderFrame();
}

void RenderApi::destroy()
{
    releaseAllShaders();
    GBuffers::destroy();
    delete renderCmds;
    renderCmds = nullptr;

    gEngine->appInstance().appWindowManager.destroyMain();
    graphicsInstance->unload();
    delete graphicsInstance;
    graphicsInstance = nullptr;
}

void RenderApi::renderFrame()
{
    bIsInsideRenderCommand = true;
    while (!commands.empty())
    {
        IRenderCommand* command = commands.front();
        commands.pop();

        command->execute(renderCmds, graphicsInstance);
        delete command;
    }
    bIsInsideRenderCommand = false;
}

IGraphicsInstance* RenderApi::getGraphicsInstance() const
{
    debugAssert(bIsInsideRenderCommand && "using graphics instance any where outside render commands is not allowed");
    return graphicsInstance;
}

void RenderApi::enqueueCommand(class IRenderCommand* renderCommand)
{
    if (bIsInsideRenderCommand)
    {
        renderCommand->execute(renderCmds, graphicsInstance);
        delete renderCommand;
    }
    else
    {
        commands.push(renderCommand);
    }
}

void GameEngine::issueRenderCommand(class IRenderCommand* renderCommand)
{
    renderingApi->enqueueCommand(renderCommand);
}
