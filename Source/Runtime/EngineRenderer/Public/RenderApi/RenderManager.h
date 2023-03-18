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
#include "Types/Containers/ArrayView.h"

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

    copat::JobSystemTask initialize(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void finalizeInit();
    copat::JobSystemTask destroy();

    void renderFrame(float timedelta);
    GlobalRenderingContextBase *getGlobalRenderingContext() const;
    IRenderCommandList *getRenderCmds() const;

    // Fills pipelineContext with info necessary to render using this particular requested pipeline, with
    // given framebuffer attachments
    void preparePipelineContext(class LocalPipelineContext *pipelineContext, ArrayView<const IRenderTargetTexture *> rtTextures);
    void preparePipelineContext(class LocalPipelineContext *pipelineContext);
    // Hint on render pass format in case of non generic render pass is necessary
    void clearExternInitRtsFramebuffer(
        ArrayView<const IRenderTargetTexture *> rtTextures, ERenderPassFormat::Type rpFormat = ERenderPassFormat::Generic
    );

private:
    void createSingletons();
};