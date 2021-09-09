#include "RenderManager.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"
#include "../Core/Engine/GameEngine.h"
#include "GBuffersAndTextures.h"
#include "../RenderInterface/Rendering/IRenderCommandList.h"
#include "../RenderInterface/Rendering/RenderingContexts.h"
#include "../Core/Platform/PlatformAssertionErrors.h"
#include "../Editor/Core/ImGui/ImGuiManager.h"
#include "../Core/Engine/Config/EngineGlobalConfigs.h"

void RenderManager::createSingletons()
{
    static GraphicInstance gGraphicsInstance;
    graphicsInstance = &gGraphicsInstance;

    static GlobalRenderingContext gGlobalContext;
    globalContext = &gGlobalContext;

    static ImGuiManager gImGuiManager;
    imGuiManager = &gImGuiManager;
}

void RenderManager::executeAllCmds()
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

void RenderManager::initialize()
{
    createSingletons();
    renderCmds = IRenderCommandList::genericInstance();
    graphicsInstance->load();

    ENQUEUE_COMMAND(InitRenderApi)(
        [this](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
        {
            gEngine->appInstance().appWindowManager.init();
            graphicsInstance->updateSurfaceDependents();
            graphicsInstance->initializeCmds(renderCmds);
            globalContext->initContext(graphicsInstance);

            // Below depends on devices and pipelines
            gEngine->appInstance().appWindowManager.postInitGraphicCore();
            GlobalBuffers::initialize();
        });

    imGuiManager->initialize();

    onVsyncChangeHandle = EngineSettings::enableVsync.onConfigChanged().bindLambda(
        [this](bool oldVal, bool newVal)
        {
            graphicsInstance->updateSurfaceDependents();
            gEngine->appInstance().appWindowManager.updateWindowCanvas();
        });
}

void RenderManager::postInit()
{
    gEngine->broadcastPostInitRenderEvent();
    // Process post init pre-frame render commands
    waitOnCommands();
}

void RenderManager::destroy()
{
    EngineSettings::enableVsync.onConfigChanged().unbind(onVsyncChangeHandle);

    imGuiManager->release();
    ENQUEUE_COMMAND_NODEBUG(DestroyRenderApi,
        {
            globalContext->clearContext();
            GlobalBuffers::destroy();
            gEngine->appInstance().appWindowManager.destroy();
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

void RenderManager::renderFrame(const float& timedelta)
{
    // #TODO(Jeslas): Start new frame before any commands, Since not multi-threaded it is okay to call directly here
    renderCmds->newFrame();
    imGuiManager->updateFrame(timedelta);
    executeAllCmds();
}

IGraphicsInstance* RenderManager::getGraphicsInstance() const
{
    debugAssert(bIsInsideRenderCommand && "using graphics instance any where outside render commands is not allowed");
    return graphicsInstance;
}

GlobalRenderingContextBase* RenderManager::getGlobalRenderingContext() const
{
    debugAssert(bIsInsideRenderCommand && "using non const rendering context any where outside render commands is not allowed");
    return globalContext;
}

class ImGuiManager* RenderManager::getImGuiManager() const
{
    return imGuiManager;
}

void RenderManager::enqueueCommand(class IRenderCommand* renderCommand)
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

void RenderManager::waitOnCommands()
{
    executeAllCmds();
}
