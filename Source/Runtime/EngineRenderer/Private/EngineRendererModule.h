/*!
 * \file EngineRendererModule.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "IRenderInterfaceModule.h"

class EngineRedererModule final : public IRenderInterfaceModule
{
private:
    IGraphicsInstance *graphicsInstanceCache;
    const GraphicsHelperAPI *graphicsHelperCache;
    WeakModulePtr weakRHI;

    RenderManager *renderManager;

public:
    RenderStateDelegate renderStateEvents;

public:
    /* IRenderInterfaceModule finals */
    IGraphicsInstance *currentGraphicsInstance() const final;
    const GraphicsHelperAPI *currentGraphicsHelper() const final;

    void initializeGraphics() final;
    void finalizeGraphicsInitialization() final;
    RenderManager *getRenderManager() const final;
    DelegateHandle registerToStateEvents(RenderStateDelegate::SingleCastDelegateType callback) final;
    void unregisterToStateEvents(const DelegateHandle &handle) final;
    /* IModuleBase finals */
    void init() final;
    void release() final;
    /* final ends */
};