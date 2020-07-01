#pragma once

#include <queue>

class IGraphicsInstance;
class GlobalRenderingContextBase;

class RenderApi
{
private:
    IGraphicsInstance* graphicsInstance;
    GlobalRenderingContextBase* globalContext;

    class IRenderCommandList* renderCmds;
    std::queue<class IRenderCommand*> commands;

    // TODO(Jeslas) : Once multi threaded rendering is added this should be changed to some TLS value
    bool bIsInsideRenderCommand = false;

    void createSingletons();
public:

    void initialize();
    void postInit();
    void destroy();

    void renderFrame();
    IGraphicsInstance* getGraphicsInstance() const;

    void enqueueCommand(class IRenderCommand* renderCommand);
    void waitOnCommands();
};
