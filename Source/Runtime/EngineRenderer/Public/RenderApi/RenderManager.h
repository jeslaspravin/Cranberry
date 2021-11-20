#pragma once

#include "Types/Delegates/Delegate.h"
#include "RenderInterface/CoreGraphicsTypes.h"
#include "EngineRendererExports.h"

#include <queue>

class IGraphicsInstance;
class GlobalRenderingContextBase;
class IRenderTargetTexture;
struct GenericRenderPassProperties;
class GraphicsHelperAPI;

class ENGINERENDERER_EXPORT RenderManager
{
private:
    GlobalRenderingContextBase* globalContext;
    IGraphicsInstance* graphicsInstanceCache;

    using ImmediateExecuteCommandType = SingleCastDelegate<void, class IRenderCommandList*, IGraphicsInstance*, const GraphicsHelperAPI*>;
    class IRenderCommandList* renderCmds;
    std::queue<class IRenderCommand*> commands;

    // TODO(Jeslas) : Once multi threaded rendering is added this should be changed to some TLS value
    bool bIsInsideRenderCommand = false;

    // TODO(Commented) DelegateHandle onVsyncChangeHandle;
public:

private:
    // Helpers
    // 
    // Get generic render pass properties from Render targets
    GenericRenderPassProperties renderpassPropsFromRTs(const std::vector<IRenderTargetTexture*>& rtTextures) const;

    void createSingletons();

    void enqueueCommand(class IRenderCommand* renderCommand);
    void immediateExecCommand(ImmediateExecuteCommandType&& immediateCmd) const;
    void executeAllCmds();
public:

    void initialize(IGraphicsInstance* graphicsInstance);
    void finalizeInit();
    void destroy();

    void renderFrame(const float& timedelta);
    GlobalRenderingContextBase* getGlobalRenderingContext() const;

    // Fills pipelineContext with info necessary to render using this particular requested pipeline, with given framebuffer attachments
    void preparePipelineContext(class LocalPipelineContext* pipelineContext,
        const std::vector<IRenderTargetTexture*>& rtTextures);
    void preparePipelineContext(class LocalPipelineContext* pipelineContext);
    // Hint on render pass format in case of non generic renderpass is necessary
    void clearExternInitRtsFramebuffer(const std::vector<IRenderTargetTexture*>& rtTextures
        , ERenderPassFormat::Type rpFormat = ERenderPassFormat::Generic);

    void waitOnCommands();
    // If initializing we assume it is executing as well
    bool isExecutingCommands() const { return bIsInsideRenderCommand; }

    template <typename RenderCmdClass>
    static void issueRenderCommand(RenderManager* renderApi, typename RenderCmdClass::RenderCmdFunc &&renderCommandFn);
};

template <typename RenderCmdClass>
void RenderManager::issueRenderCommand(RenderManager* renderApi, typename RenderCmdClass::RenderCmdFunc &&renderCommandFn)
{
    if (renderApi->bIsInsideRenderCommand)
    {
        renderApi->immediateExecCommand(ImmediateExecuteCommandType::createLambda(
            [renderCommandFn](class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
            {
                renderCommandFn(cmdList, graphicsInstance, graphicsHelper);
            }
        ));
    }
    else
    {
        renderApi->enqueueCommand(new RenderCmdClass(std::forward<typename RenderCmdClass::RenderCmdFunc>(renderCommandFn)));
    }
}
