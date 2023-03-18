/*!
 * \file IRenderInterfaceModule.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "EngineRendererExports.h"
#include "Modules/IModuleBase.h"
#include "Types/Delegates/Delegate.h"

class IGraphicsInstance;
class GraphicsHelperAPI;
class RenderManager;

enum class ERenderStateEvent
{
    // Event when graphics instance is created and device is not initialized
    PostLoadInstance,
    PreinitDevice,
    // Post init device and Initialized surface properties
    PostInitDevice,
    // Now shaders and pipelines are initialized
    PostInitGraphicsContext,
    // At the end of render initialization, but called from within executing commands
    PostInititialize,
    // At the end of render initialization, but called from outside executing commands
    PreFinalizeInit,
    // At the end of render initialization, but called after executing all initialize commands
    PostFinalizeInit,
    // Before starting executing current frame commands
    PreExecFrameCommands,
    // Before executing clean up commands
    PreCleanupCommands,
    // While destroying
    Cleanup,
    // After executing clean up commands but before GraphicsInstance and Graphics device destroy, Do not
    // call any enqueue command here
    PostCleanupCommands
};

using RenderStateDelegate = Delegate<ERenderStateEvent>;

class ENGINERENDERER_EXPORT IRenderInterfaceModule : public IModuleBase
{
public:
    static IRenderInterfaceModule *get();

    /**
     * These are supposed to be used only inside rendering thread.
     * Do not cache this results If you are not sure of your intentions
     */
    virtual IGraphicsInstance *currentGraphicsInstance() const = 0;
    virtual const GraphicsHelperAPI *currentGraphicsHelper() const = 0;
    virtual RenderManager *getRenderManager() const = 0;

    virtual void initializeGraphics(bool bComputeOnly = false) = 0;
    virtual void finalizeGraphicsInitialization() = 0;

    virtual DelegateHandle registerToStateEvents(RenderStateDelegate::SingleCastDelegateType callback) = 0;
    virtual void unregisterToStateEvents(const DelegateHandle &handle) = 0;
};