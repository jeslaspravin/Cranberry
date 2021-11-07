#pragma once

#include "Types/Delegates/Delegate.h"
#include "EngineRendererExports.h"

#include <queue>

class IGraphicsInstance;
class GlobalRenderingContextBase;

class ENGINERENDERER_EXPORT RenderManager
{
private:
    IGraphicsInstance* graphicsInstance;
    GlobalRenderingContextBase* globalContext;

    class IRenderCommandList* renderCmds;
    std::queue<class IRenderCommand*> commands;

    class ImGuiManager* imGuiManager;

    // TODO(Jeslas) : Once multi threaded rendering is added this should be changed to some TLS value
    bool bIsInsideRenderCommand = false;

    DelegateHandle onVsyncChangeHandle;
public:

private:
    void createSingletons();

    void enqueueCommand(class IRenderCommand* renderCommand);
    void executeAllCmds();
public:

    void initialize();
    void postInit();
    void destroy();

    void renderFrame(const float& timedelta);
    IGraphicsInstance* getGraphicsInstance() const;
    GlobalRenderingContextBase* getGlobalRenderingContext() const;
    class ImGuiManager* getImGuiManager() const;

    void waitOnCommands();

    template <typename RenderCmdClass>
    static void issueRenderCommand(RenderManager* renderApi, typename RenderCmdClass::RenderCmdFunc &&renderCommandFn);
};

template <typename RenderCmdClass>
void RenderManager::issueRenderCommand(RenderManager* renderApi, typename RenderCmdClass::RenderCmdFunc &&renderCommandFn)
{
    if (renderApi->bIsInsideRenderCommand)
    {
        renderCommandFn(renderApi->renderCmds, renderApi->graphicsInstance);
    }
    else
    {
        renderApi->enqueueCommand(new RenderCmdClass(std::forward<typename RenderCmdClass::RenderCmdFunc>(renderCommandFn)));
    }
}
