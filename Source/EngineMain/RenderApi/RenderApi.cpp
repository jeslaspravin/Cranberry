#include "RenderApi.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"
#include "../Core/Engine/GameEngine.h"
#include "GBuffersAndTextures.h"
#include "../RenderInterface/Rendering/IRenderCommandList.h"
#include "../RenderInterface/Rendering/RenderingContexts.h"
#include "../Core/Platform/PlatformAssertionErrors.h"
#include "../Editor/Core/ImGui/ImGuiManager.h"

void RenderApi::createSingletons()
{
    static GraphicInstance gGraphicsInstance;
    graphicsInstance = &gGraphicsInstance;

    static GlobalRenderingContext gGlobalContext;
    globalContext = &gGlobalContext;

    static ImGuiManager gImGuiManager;
    imGuiManager = &gImGuiManager;
}

void RenderApi::executeAllCmds()
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
            gEngine->appInstance().appWindowManager.postInitGraphicCore();
            GBuffers::initialize();
            globalContext->initContext(graphicsInstance);
        }
    , this);

    imGuiManager->initialize();
}

void RenderApi::postInit()
{
    // Process post init pre-frame render commands
    waitOnCommands();
}

void RenderApi::destroy()
{
    imGuiManager->release();
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

    std::vector<GraphicsResource*> resourceLeak;
    GraphicsResource::staticType()->allRegisteredResources(resourceLeak, true);
    if (!resourceLeak.empty())
    {
        Logger::error("GraphicsResourceLeak", "%s() : Resource leak detected", __func__);
        for (const GraphicsResource* resource : resourceLeak)
        {
            Logger::error(resource->getType()->getName(), "\t%s", resource->getResourceName().getChar());
        }
    }
}

void RenderApi::renderFrame(const float& timedelta)
{
    // #TODO(Jeslas): Start new frame before any commands, Since not multi-threaded it is okay to call directly here
    renderCmds->newFrame();
    imGuiManager->updateFrame(timedelta);
    executeAllCmds();
}

IGraphicsInstance* RenderApi::getGraphicsInstance() const
{
    debugAssert(bIsInsideRenderCommand && "using graphics instance any where outside render commands is not allowed");
    return graphicsInstance;
}

GlobalRenderingContextBase* RenderApi::getGlobalRenderingContext() const
{
    debugAssert(bIsInsideRenderCommand && "using non const rendering context any where outside render commands is not allowed");
    return globalContext;
}

class ImGuiManager* RenderApi::getImGuiManager() const
{
    return imGuiManager;
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
    executeAllCmds();
}

void GameEngine::issueRenderCommand(class IRenderCommand* renderCommand)
{
    renderingApi.enqueueCommand(renderCommand);
}
