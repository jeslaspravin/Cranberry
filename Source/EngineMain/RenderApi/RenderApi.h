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

    class ImGuiManager* imGuiManager;

    // TODO(Jeslas) : Once multi threaded rendering is added this should be changed to some TLS value
    bool bIsInsideRenderCommand = false;

    void createSingletons();
    void executeAllCmds();
public:

    void initialize();
    void postInit();
    void destroy();

    void renderFrame(const float& timedelta);
    IGraphicsInstance* getGraphicsInstance() const;
    GlobalRenderingContextBase* getGlobalRenderingContext() const;
    class ImGuiManager* getImGuiManager() const;

    void enqueueCommand(class IRenderCommand* renderCommand);
    void waitOnCommands();
};
