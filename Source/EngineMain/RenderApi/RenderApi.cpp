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
    renderCmds = IRenderCommandList::genericInstance();
    graphicsInstance->load();

    ENQUEUE_COMMAND(InitRenderApi,
        {
            gEngine->appInstance().appWindowManager.initMain();
            graphicsInstance->loadSurfaceDependents();
            graphicsInstance->initializeCmds(renderCmds);
            gEngine->appInstance().appWindowManager.postInitGraphicCore();
            GBuffers::initialize();
            initAllShaders();
        }
    , this);
}

void RenderApi::postInit()
{
    // Process post init pre-frame render commands
    waitOnCommands();
}

void RenderApi::destroy()
{
    ENQUEUE_COMMAND(DestroyRenderApi,
        {
            releaseAllShaders();
            GBuffers::destroy();
            gEngine->appInstance().appWindowManager.destroyMain();
        }
        , this);

    // Executing commands one last time
    waitOnCommands();
    delete renderCmds;
    renderCmds = nullptr;

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
    if (bIsInsideRenderCommand && renderCmds)
    {
        renderCommand->execute(renderCmds, graphicsInstance);
        delete renderCommand;
    }
    else
    {
        commands.push(renderCommand);
    }
}

void RenderApi::waitOnCommands()
{
    renderFrame();
}

void GameEngine::issueRenderCommand(class IRenderCommand* renderCommand)
{
    renderingApi->enqueueCommand(renderCommand);
}
