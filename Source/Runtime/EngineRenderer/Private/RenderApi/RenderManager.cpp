/*!
 * \file RenderManager.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderApi/RenderManager.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"
#include "EngineRendererModule.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderApi/ResourcesInterface/IRenderResource.h"
#include "RenderApi/Rendering/RenderingContexts.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/GraphicsIntance.h"
#include "RenderInterface/Rendering/FramebufferTypes.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/Resources/MemoryResources.h"

GenericRenderPassProperties RenderManager::renderpassPropsFromRTs(const std::vector<IRenderTargetTexture *> &rtTextures) const
{
    GenericRenderPassProperties renderpassProperties;
    renderpassProperties.renderpassAttachmentFormat.rpFormat = ERenderPassFormat::Generic;
    if (!rtTextures.empty())
    {
        // Since all the textures in a same framebuffer must have same properties on below two
        renderpassProperties.bOneRtPerFormat = rtTextures[0]->renderResource() == rtTextures[0]->renderTargetResource();
        renderpassProperties.multisampleCount = static_cast<ImageResource *>(rtTextures[0]->renderTargetResource().reference())->sampleCount();

        renderpassProperties.renderpassAttachmentFormat.attachments.reserve(rtTextures.size());
        for (const IRenderTargetTexture *const &rtTexture : rtTextures)
        {
            renderpassProperties.renderpassAttachmentFormat.attachments.emplace_back(
                static_cast<ImageResource *>(rtTexture->renderTargetResource().reference())->imageFormat()
            );
        }
    }

    return renderpassProperties;
}

void RenderManager::createSingletons() { globalContext = graphicsHelperCache->createGlobalRenderingContext(); }

RenderThreadEnqTask RenderManager::initialize(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
{
    graphicsInstanceCache = graphicsInstance;
    graphicsHelperCache = graphicsHelper;
    auto engineRendererModule = static_cast<EngineRendererModule *>(IRenderInterfaceModule::get());
    debugAssert(engineRendererModule);

    createSingletons();
    renderCmds = IRenderCommandList::genericInstance();
    graphicsInstanceCache->load();
    // Load instance done
    engineRendererModule->renderStateEvents.invoke(ERenderStateEvent::PostLoadInstance);

    return RenderThreadEnqueuer::execInRenderThreadAwaitable(
        [this,
         engineRendererModule](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            engineRendererModule->renderStateEvents.invoke(ERenderStateEvent::PreinitDevice);
            graphicsInstance->updateSurfaceDependents();
            graphicsInstance->initializeCmds(renderCmds);
            engineRendererModule->renderStateEvents.invoke(ERenderStateEvent::PostInitDevice);

            globalContext->initContext(graphicsInstance, graphicsHelper);
            engineRendererModule->renderStateEvents.invoke(ERenderStateEvent::PostInitGraphicsContext);

            // Below depends on devices and pipelines
            GlobalBuffers::initialize();

            engineRendererModule->renderStateEvents.invoke(ERenderStateEvent::PostInititialize);
        }
    );

    // TODO(Commented) onVsyncChangeHandle = EngineSettings::enableVsync.onConfigChanged().bindLambda(
    // TODO(Commented)     [this](bool oldVal, bool newVal)
    // TODO(Commented)     {
    // TODO(Commented)         graphicsInstance->updateSurfaceDependents();
    // TODO(Commented)         gEngine->appInstance().appWindowManager.updateWindowCanvas();
    // TODO(Commented)     });
}

void RenderManager::finalizeInit()
{
    auto engineRendererModule = static_cast<EngineRendererModule *>(IRenderInterfaceModule::get());
    debugAssert(engineRendererModule);

    engineRendererModule->renderStateEvents.invoke(ERenderStateEvent::PreFinalizeInit);
    RenderThreadEnqueuer::flushWaitRenderThread();
    engineRendererModule->renderStateEvents.invoke(ERenderStateEvent::PostFinalizeInit);
}

RenderThreadEnqTask RenderManager::destroy()
{
    debugAssert(IRenderInterfaceModule::get());
    auto engineRendererModule = static_cast<EngineRendererModule *>(IRenderInterfaceModule::get());

    // TODO(Commented) EngineSettings::enableVsync.onConfigChanged().unbind(onVsyncChangeHandle);
    engineRendererModule->renderStateEvents.invoke(ERenderStateEvent::PreCleanupCommands);

    return RenderThreadEnqueuer::execInRenderThreadAwaitable(
        [this,
         engineRendererModule](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            engineRendererModule->renderStateEvents.invoke(ERenderStateEvent::Cleanup);

            globalContext->clearContext();
            GlobalBuffers::destroy();

            delete renderCmds;
            renderCmds = nullptr;

            engineRendererModule->renderStateEvents.invoke(ERenderStateEvent::PostCleanupCommands);

            graphicsInstanceCache->unload();

            std::vector<GraphicsResource *> resourceLeak;
            GraphicsResource::staticType()->allRegisteredResources(resourceLeak, true);
            if (!resourceLeak.empty())
            {
                LOG_ERROR("GraphicsResourceLeak", "Resource leak detected");
                for (const GraphicsResource *resource : resourceLeak)
                {
                    LOG_ERROR(
                        "GraphicsResourceLeak", "\tType:%s, Resource Name %s", resource->getType()->getName(),
                        resource->getResourceName().getChar()
                    );
                }
            }
        }
    );
}

void RenderManager::renderFrame(const float &timedelta)
{
    // TODO(Jeslas): Start new frame before any commands, Since not multi-threaded it is okay to call
    // directly here
    renderCmds->newFrame(timedelta);

    debugAssert(IRenderInterfaceModule::get());
    auto engineRendererModule = static_cast<EngineRendererModule *>(IRenderInterfaceModule::get());
    engineRendererModule->renderStateEvents.invoke(ERenderStateEvent::PreExecFrameCommands);
}

GlobalRenderingContextBase *RenderManager::getGlobalRenderingContext() const
{
    ASSERT_INSIDE_RENDERTHREAD();
    return globalContext;
}

IRenderCommandList *RenderManager::getRenderCmds() const
{
    ASSERT_INSIDE_RENDERTHREAD();
    return renderCmds;
}

FORCE_INLINE void
    rtTexturesToFrameAttachments(const std::vector<IRenderTargetTexture *> &rtTextures, std::vector<ImageResourceRef> &frameAttachments)
{
    frameAttachments.clear();

    bool bHasResolves = rtTextures[0]->renderTargetResource() != rtTextures[0]->renderResource();
    for (const IRenderTargetTexture *const &rtTexture : rtTextures)
    {
        frameAttachments.emplace_back(ImageResourceRef(rtTexture->renderTargetResource()));

        // Since depth formats do not have resolve
        if (bHasResolves
            && !EPixelDataFormat::isDepthFormat(static_cast<ImageResource *>(rtTexture->renderTargetResource().reference())->imageFormat()))
        {
            frameAttachments.emplace_back(rtTexture->renderResource());
        }
    }
}

void RenderManager::preparePipelineContext(class LocalPipelineContext *pipelineContext, const std::vector<IRenderTargetTexture *> &rtTextures)
{
    GenericRenderPassProperties renderpassProps = renderpassPropsFromRTs(rtTextures);
    if (rtTextures.empty())
    {
        LOG_ERROR("RenderManager", "RT textures cannot be empty(Necessary to find GenericRenderPassProperties)");
        return;
    }

    rtTexturesToFrameAttachments(rtTextures, pipelineContext->frameAttachments);
    globalContext->preparePipelineContext(pipelineContext, renderpassProps);
}

void RenderManager::preparePipelineContext(class LocalPipelineContext *pipelineContext)
{
    globalContext->preparePipelineContext(pipelineContext, {});
}

void RenderManager::clearExternInitRtsFramebuffer(
    const std::vector<IRenderTargetTexture *> &rtTextures, ERenderPassFormat::Type rpFormat /*= ERenderPassFormat::Generic*/
)
{
    GenericRenderPassProperties renderpassProps = renderpassPropsFromRTs(rtTextures);
    renderpassProps.renderpassAttachmentFormat.rpFormat = rpFormat;

    std::vector<ImageResourceRef> frameAttachments;
    rtTexturesToFrameAttachments(rtTextures, frameAttachments);
    globalContext->clearExternInitRtsFramebuffer(frameAttachments, renderpassProps);
}
