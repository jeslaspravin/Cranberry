/*!
 * \file RenderManager.h
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
#include "RenderTaskHelpers.h"
#include "RenderInterface/CoreGraphicsTypes.h"

#include <queue>

class IGraphicsInstance;
class GlobalRenderingContextBase;
class IRenderTargetTexture;
struct GenericRenderPassProperties;
class GraphicsHelperAPI;

/**
 * All graphics or rendering related state full resources will be present here
 */
class ENGINERENDERER_EXPORT RenderManager
{
private:
    IGraphicsInstance *graphicsInstanceCache;
    const GraphicsHelperAPI *graphicsHelperCache;

    GlobalRenderingContextBase *globalContext;
    class IRenderCommandList *renderCmds;
    // TODO(Commented) DelegateHandle onVsyncChangeHandle;

public:
    RenderManager() = default;
    MAKE_TYPE_NONCOPY_NONMOVE(RenderManager)

    RenderThreadEnqTask initialize(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void finalizeInit();
    RenderThreadEnqTask destroy();

    void renderFrame(const float &timedelta);
    GlobalRenderingContextBase *getGlobalRenderingContext() const;
    IRenderCommandList *getRenderCmds() const;

    // Fills pipelineContext with info necessary to render using this particular requested pipeline, with
    // given framebuffer attachments
    void preparePipelineContext(class LocalPipelineContext *pipelineContext, const std::vector<IRenderTargetTexture *> &rtTextures);
    void preparePipelineContext(class LocalPipelineContext *pipelineContext);
    // Hint on render pass format in case of non generic render pass is necessary
    void clearExternInitRtsFramebuffer(
        const std::vector<IRenderTargetTexture *> &rtTextures, ERenderPassFormat::Type rpFormat = ERenderPassFormat::Generic
    );

private:
    /**
     *  Helpers
     */

    // Get generic render pass properties from Render targets
    GenericRenderPassProperties renderpassPropsFromRTs(const std::vector<IRenderTargetTexture *> &rtTextures) const;

    void createSingletons();
};