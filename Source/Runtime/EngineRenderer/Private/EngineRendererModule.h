#pragma once

#include "IRenderInterfaceModule.h"

class EngineRedererModule final : public IRenderInterfaceModule
{
private:
    IGraphicsInstance* graphicsInstanceCache;
    const GraphicsHelperAPI* graphicsHelperCache;
    WeakModulePtr weakRHI;
public:
    RenderStateDelegate renderStateEvents;
public:

    /* IRenderInterfaceModule finals */
    IGraphicsInstance* currentGraphicsInstance() const final;
    const GraphicsHelperAPI* currentGraphicsHelper() const final;

    void initializeGraphics() final;
    void finalizeGraphicsInitialization() final;
    RenderManager* getRenderManager() const;
    DelegateHandle registerToStateEvents(RenderStateDelegate::SingleCastDelegateType callback) final;
    void unregisterToStateEvents(const DelegateHandle& handle) final;
    /* IModuleBase finals */
    void init() final;
    void release() final;
    /* final ends */
};