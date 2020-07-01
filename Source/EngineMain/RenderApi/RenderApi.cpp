#include "RenderApi.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"
#include "../Core/Engine/GameEngine.h"
#include "GBuffersAndTextures.h"
#include "../RenderInterface/Rendering/IRenderCommandList.h"
#include "../RenderInterface/Rendering/RenderingContexts.h"
#include "../Core/Platform/PlatformAssertionErrors.h"

void RenderApi::createSingletons()
{
    static GraphicInstance gGraphicsInstance;
    graphicsInstance = &gGraphicsInstance;

    static GlobalRenderingContext gGlobalContext;
    globalContext = &gGlobalContext;
}

void RenderApi::initialize()
{
    createSingletons();
    renderCmds = IRenderCommandList::genericInstance();
    graphicsInstance->load();

    ENQUEUE_COMMAND(InitRenderApi,
        {
            gEngine->appInstance().appWindowManager.initMain();
            graphicsInstance->loadSurfaceDependents();
            graphicsInstance->initializeCmds(renderCmds);
            globalContext->initContext(graphicsInstance);
            gEngine->appInstance().appWindowManager.postInitGraphicCore();
            GBuffers::initialize();
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
            globalContext->clearContext();
            GBuffers::destroy();
            gEngine->appInstance().appWindowManager.destroyMain();
        }
        , this);

    // Executing commands one last time
    waitOnCommands();
    delete renderCmds;
    renderCmds = nullptr;

    graphicsInstance->unload();
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
    renderingApi.enqueueCommand(renderCommand);
}
