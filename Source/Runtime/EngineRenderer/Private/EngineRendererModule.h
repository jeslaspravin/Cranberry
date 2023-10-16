/*!
 * \file EngineRendererModule.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "IRenderInterfaceModule.h"
#include "Modules/ModuleManager.h"

class EngineRendererModule final : public IRenderInterfaceModule
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

    void initializeGraphics(bool bComputeOnly = false);
    void finalizeGraphicsInitialization() final;
    RenderManager *getRenderManager() const final;
    DelegateHandle registerToStateEvents(RenderStateDelegate::SingleCastDelegateType &&callback) final;
    void unregisterToStateEvents(const DelegateHandle &handle) final;
    /* IModuleBase finals */
    void init() final;
    void release() final;
    /* final ends */
};