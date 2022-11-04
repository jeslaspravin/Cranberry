/*!
 * \file WidgetRHIRenderer.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/WidgetRHIRenderer.h"
#include "Math/Math.h"
#include "IApplicationModule.h"
#include "ApplicationInstance.h"
#include "WindowManager.h"
#include "GenericAppWindow.h"
#include "Widgets/WidgetDrawContext.h"
#include "Widgets/WidgetWindow.h"
#include "RenderApi/VertexData.h"
#include "RenderApi/RenderManager.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/Rendering/CommandBuffer.h"

/**
 * For now only RHI based widget renderer exists, So this is fine
 */
WidgetRenderer *WidgetRenderer::createRenderer() { return new WidgetRHIRenderer(); }

void WidgetRHIRenderer::initialize()
{
    ENQUEUE_COMMAND(WidgetRHIRendererInit)
    (
        [this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            initializeRenderThread(cmdList, graphicsInstance, graphicsHelper);
        }
    );
}

void WidgetRHIRenderer::destroy()
{
    ENQUEUE_COMMAND(WidgetRHIRendererInit)
    (
        [this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            destroyRenderThread(cmdList, graphicsInstance, graphicsHelper);
            delete this;
        }
    );
}

void WidgetRHIRenderer::clearWindowState(const SharedPtr<WgWindow> &window)
{
    if (windowStates.empty())
    {
        return;
    }
    //  Flush before clearing, As we might be in middle of rendering in render thread
    RenderThreadEnqueuer::flushWaitRenderThread();

    auto itr = windowStates.find(window);
    if (itr != windowStates.end())
    {
        // Waiting on command is better than waiting on fence
        for (const FenceRef &fence : itr->second.perFrameSubmitFences)
        {
            if (!fence->isSignaled())
            {
                fence->waitForSignal();
            }
        }

        // Do not have to clear window canvas created frame buffer here as it will be taken care at WindowsManager
        windowStates.erase(itr);
    }
}

void WidgetRHIRenderer::initializeRenderThread(
    IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    dummyTexture = GlobalBuffers::dummyWhite2D();
}

void WidgetRHIRenderer::destroyRenderThread(
    IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    for (auto itr = windowStates.begin(); itr != windowStates.end();)
    {
        SharedPtr<WgWindow> window = itr->first;
        itr++;
        clearWindowState(window);
    }
    dummyTexture.reset();
    indices.reset();
    vertices.reset();
}

FORCE_INLINE void WidgetRHIRenderer::clearUnusedTextures()
{
    for (auto itr = textureToParamsIdx.begin(); itr != textureToParamsIdx.end();)
    {
        if (textureParams[itr->second].second)
        {
            // Reset each texture usage to false
            textureParams[itr->second].second = false;
            ++itr;
        }
        else
        {
            textureParams.reset(itr->second);
            itr = textureToParamsIdx.erase(itr);
        }
    }
}

WidgetRHIRenderer::WindowState &WidgetRHIRenderer::createWindowState(
    const SharedPtr<WgWindow> &window, GenericWindowCanvas *swapchainCanvas, IRenderCommandList *cmdList,
    const LocalPipelineContext &pipelineContext, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    WindowState &state = windowStates[window];
    // Ignore descriptors set 1 as it is for texture
    state.windowTransformParam
        = graphicsHelper->createShaderParameters(graphicsInstance, pipelineContext.getPipeline()->getParamLayoutAtSet(0), { 1 });
    state.windowTransformParam->setResourceName(window->getAppWindow()->getWindowName() + TCHAR("_WgTransform"));
    state.windowTransformParam->init();

    state.perFrameCmdBuffers.resize(swapchainCanvas->imagesCount());
    state.perFrameSubmitFences.resize(swapchainCanvas->imagesCount());
    state.readyToPresent.resize(swapchainCanvas->imagesCount());
    for (uint32 i = 0; i < swapchainCanvas->imagesCount(); ++i)
    {
        state.perFrameCmdBuffers[i] = window->getAppWindow()->getWindowName() + TCHAR("_CmdBuffer_") + String::toString(i);
        state.perFrameSubmitFences[i] = graphicsHelper->createFence(
            graphicsInstance, (window->getAppWindow()->getWindowName() + TCHAR("_Fence_") + String::toString(i)).c_str()
            );
        state.perFrameSubmitFences[i]->init();
        state.readyToPresent[i] = graphicsHelper->createSemaphore(
            graphicsInstance, (window->getAppWindow()->getWindowName() + TCHAR("_Semaphore_") + String::toString(i)).c_str()
            );
        state.readyToPresent[i]->init();
    }
    return state;
}

void WidgetRHIRenderer::createVerticesAndIndices(
    uint64 indexCount, uint32 vertCount, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    if (!vertices.isValid() || !vertices->isValid() || vertices->bufferCount() < vertCount)
    {
        vertices = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, sizeof(VertexUI), vertCount);
        vertices->setResourceName(TCHAR("WidgetRHIRendererVertices"));
        vertices->setAsStagingResource(true);
        vertices->init();
    }

    if (!indices.isValid() || !indices->isValid() || indices->bufferCount() < indexCount)
    {
        indices = graphicsHelper->createReadOnlyIndexBuffer(graphicsInstance, sizeof(uint32), indexCount);
        indices->setResourceName(TCHAR("WidgetRHIRendererIndices"));
        indices->setAsStagingResource(true);
        indices->init();
    }
}

void WidgetRHIRenderer::presentWindows(const std::vector<SharedPtr<WgWindow>> &windows, std::vector<WindowCanvasRef> swapchains)
{
    debugAssert(swapchains.size() == windows.size());
    ENQUEUE_COMMAND(PresentAllWindows)
    (
        [swapchains, windows, this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            std::vector<uint32> swapchainIdxs(swapchains.size());
            std::vector<SemaphoreRef> presentWaits(windows.size());
            for (uint32 i = 0; i < swapchains.size(); ++i)
            {
                uint32 idx = swapchains[i]->currentImgIdx();
                swapchainIdxs[i] = idx;

                auto winStateItr = windowStates.find(windows[i]);
                debugAssert(winStateItr != windowStates.cend());
                presentWaits[i] = winStateItr->second.readyToPresent[swapchainIdxs[i]];
            }

            // Sending the present semaphore manually as vulkan does not support timeline semaphore
            cmdList->presentImage(swapchains, swapchainIdxs, presentWaits);
        }
    );
}

void WidgetRHIRenderer::drawWindowWidgets(std::vector<std::pair<SharedPtr<WgWindow>, WidgetDrawContext>> &&drawingContexts)
{
    ENQUEUE_COMMAND(DrawWindowWidgetsRHI)
    (
        [allDrawCtxs = std::forward<std::vector<std::pair<SharedPtr<WgWindow>, WidgetDrawContext>>>(drawingContexts),
         this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            drawWindowWidgetsRenderThread(allDrawCtxs, cmdList, graphicsInstance, graphicsHelper);
        }
    );
}

void WidgetRHIRenderer::drawWindowWidgetsRenderThread(
    const std::vector<std::pair<SharedPtr<WgWindow>, WidgetDrawContext>> &drawingContexts, IRenderCommandList *cmdList,
    IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    ApplicationInstance *app = IApplicationModule::get()->getApplication();
    WindowManager *windowsManager = app->windowManager;
    RenderManager *renderManager = IRenderInterfaceModule::get()->getRenderManager();

    // Data for drawing
    struct WgDrawCmd
    {
        uint64 indicesOffset;
        uint64 indicesCount;
        uint32 textureDescIdx;
        QuantShortBox2D scissor;
    };
    // 1:1 to windows, Request next image for all windows
    std::vector<SemaphoreRef, CBEStlStackAllocatorExclusive<SemaphoreRef>> swapchainSemaphores(
        drawingContexts.size(), CBEStlStackAllocatorExclusive<SemaphoreRef>{ app->getRenderFrameAllocator() }
    );
    std::vector<WindowState *, CBEStlStackAllocatorExclusive<WindowState *>> statePerWnd(
        drawingContexts.size(), CBEStlStackAllocatorExclusive<WindowState *>{ app->getRenderFrameAllocator() }
    );
    std::vector<std::vector<WgDrawCmd>, CBEStlStackAllocatorExclusive<std::vector<WgDrawCmd>>> drawCmdsPerWnd(
        drawingContexts.size(), CBEStlStackAllocatorExclusive<std::vector<WgDrawCmd>>{ app->getRenderFrameAllocator() }
    );
    std::vector<const GraphicsResource *, CBEStlStackAllocatorExclusive<const GraphicsResource *>> cmdBufferPerWnd(
        drawingContexts.size(), CBEStlStackAllocatorExclusive<const GraphicsResource *>{ app->getRenderFrameAllocator() }
    );
    std::vector<LocalPipelineContext, CBEStlStackAllocatorExclusive<LocalPipelineContext>> pipelineCntxPerWnd(
        drawingContexts.size(), CBEStlStackAllocatorExclusive<LocalPipelineContext>{ app->getRenderFrameAllocator() }
    );

    // Setting up resources
    {
        // TODO(Jeslas) : Change from IMGui shader, We can do better with Descriptor indexing as we now accept only quads
        // Everything below this needs to be rewritten on changing pipeline or including non quad primitives
        // Dummy prepare to get pipeline layout for ShaderParameters
        LocalPipelineContext pipelineContext;
        pipelineContext.materialName = TCHAR("DrawImGui");
        pipelineContext.forVertexType = EVertexType::UI;
        pipelineContext.windowCanvas = windowsManager->getWindowCanvas(drawingContexts[0].first->getAppWindow());
        pipelineContext.swapchainIdx = pipelineContext.windowCanvas->currentImgIdx();
        renderManager->preparePipelineContext(&pipelineContext);
        if (!pipelineContext.getPipeline())
        {
            LOG_ERROR("WidgetRHIRenderer", "Failed to find %s and its related pipelines!", pipelineContext.materialName);
            return;
        }

        // for preallocating indices and vertices
        uint32 totalNoOfQuads = 0;
        // Setup per window parameters and texture parameters
        for (uint32 i = 0; i < drawingContexts.size(); ++i)
        {
            WindowCanvasRef swapchainCanvas = windowsManager->getWindowCanvas(drawingContexts[i].first->getAppWindow());
            debugAssert(!drawingContexts[i].second.perVertexPos().empty() && swapchainCanvas.isValid());
            if (!windowStates.contains(drawingContexts[i].first))
            {
                statePerWnd[i] = &createWindowState(
                    drawingContexts[i].first, swapchainCanvas.reference(), cmdList, pipelineContext, graphicsInstance, graphicsHelper
                );
            }
            else
            {
                statePerWnd[i] = &windowStates[drawingContexts[i].first];
            }
            // Fill pipeline's context
            pipelineCntxPerWnd[i] = pipelineContext;
            pipelineCntxPerWnd[i].swapchainIdx = swapchainCanvas->requestNextImage(&swapchainSemaphores[i]);
            pipelineCntxPerWnd[i].windowCanvas = swapchainCanvas;
            renderManager->preparePipelineContext(&pipelineCntxPerWnd[i]);

            // Fill window parameters to shader, This will be Uploaded in next frame. 0th frame params will be invalid
            Short2D windowSize = drawingContexts[i].first->getWidgetSize();
            Vector2D scale = 2.0f / Vector2D(windowSize);
            // -1.0f - Offset * scale, So that vertices gets translated to viewport, but in our case we always stay within the window
            Vector2D translate = Vector2D(-1.0f);
            statePerWnd[i]->windowTransformParam->setVector2Param(STRID("scale"), scale);
            statePerWnd[i]->windowTransformParam->setVector2Param(STRID("translate"), translate);

            totalNoOfQuads += drawingContexts[i].second.perQuadTexture().size();
            for (ImageResourceRef texture : drawingContexts[i].second.perQuadTexture())
            {
                if (!texture.isValid())
                {
                    texture = dummyTexture;
                }

                // Create or get texture param descriptors set
                if (textureToParamsIdx.contains(texture))
                {
                    textureParams[textureToParamsIdx[texture]].second = true;
                }
                else
                {
                    uint32 idx = uint32(textureParams.get());
                    textureToParamsIdx[texture] = idx;
                    // Ignore descriptors set 0 as it is for transforms
                    auto &pair = textureParams[idx];
                    pair.first = graphicsHelper->createShaderParameters(
                        graphicsInstance, pipelineContext.getPipeline()->getParamLayoutAtSet(0), { 0 }
                    );
                    pair.second = true;
                    textureParams[idx].first->setTextureParam(STRID("textureAtlas"), texture, GlobalBuffers::linearSampler());
                    textureParams[idx].first->setResourceName(
                        drawingContexts[i].first->getAppWindow()->getWindowName()
                        + TCHAR("_") + texture->getResourceName()
                        );
                    textureParams[idx].first->init();
                }
            }
        }

        // Setup vertices, indices and fill draw commands
        createVerticesAndIndices(totalNoOfQuads * 6, totalNoOfQuads * 4, graphicsInstance, graphicsHelper);
        ArrayView<VertexUI> verticesView((VertexUI *)graphicsHelper->borrowMappedPtr(graphicsInstance, vertices), vertices->bufferCount());
        ArrayView<uint32> indicesView((uint32 *)graphicsHelper->borrowMappedPtr(graphicsInstance, indices), indices->bufferCount());
        // Offset of quad vertices inserted so far into vertices array
        uint32 quadIdxOffset = 0;
        for (uint32 i = 0; i < drawingContexts.size(); ++i)
        {
            const WidgetDrawContext &drawingCtx = drawingContexts[i].second;

            struct WgDrawCmdsPerLayer
            {
                uint32 drawCmdIdx;
                std::vector<uint32> quadIdxs;
            };
            // Unique draw commands per each layers and texture
            std::map<std::pair<uint32, ImageResourceRef>, WgDrawCmdsPerLayer> uniqueDrawIndexed;
            // For each layer collecting unique textured draw commands, This will then used to fill vertices and indices
            uint32 layerIdx = 0;
            for (const std::vector<ValueRange<uint32>> &layerVerts : drawingCtx.allLayerVertRange())
            {
                for (const ValueRange<uint32> &verticesRange : layerVerts)
                {
                    debugAssertf(
                        verticesRange.minBound % 4 == 0 && (verticesRange.maxBound + 1) % 4 == 0,
                        "Vertices are not quad aligned(Check if any vertices are not quad)"
                    );
                    // Finding texture used from Quad instance index
                    for (uint32 vertIdx = verticesRange.minBound; vertIdx < verticesRange.maxBound; vertIdx += 4)
                    {
                        uint32 quadIdx = vertIdx / 4;
                        // Only if quad is big enough
                        if (drawingCtx.perQuadClipping()[quadIdx].size() == Short2D(0))
                        {
                            continue;
                        }

                        ImageResourceRef img
                            = drawingCtx.perQuadTexture()[quadIdx].isValid() ? drawingCtx.perQuadTexture()[quadIdx] : dummyTexture;
                        std::pair<uint32, ImageResourceRef> uniqPair = { layerIdx, img };
                        WgDrawCmdsPerLayer &layerDrawCmd = uniqueDrawIndexed[uniqPair];
                        if (layerDrawCmd.quadIdxs.empty())
                        {
                            layerDrawCmd.drawCmdIdx = uint32(drawCmdsPerWnd[i].size());
                            drawCmdsPerWnd[i].emplace_back();
                        }
                        layerDrawCmd.quadIdxs.push_back(quadIdx);
                    }
                }
                layerIdx++;
            }

            for (const std::pair<const std::pair<uint32, ImageResourceRef>, WgDrawCmdsPerLayer> &uniqDraw : uniqueDrawIndexed)
            {
                debugAssert(!uniqDraw.second.quadIdxs.empty());
                WgDrawCmd &drawCmd = drawCmdsPerWnd[i][uniqDraw.second.drawCmdIdx];
                drawCmd.indicesCount = uniqDraw.second.quadIdxs.size() * 6;
                drawCmd.indicesOffset = quadIdxOffset * 6;
                drawCmd.scissor = drawingCtx.perQuadClipping()[uniqDraw.second.quadIdxs[0]];
                drawCmd.textureDescIdx = textureToParamsIdx[uniqDraw.first.second];
                // For each of quad fill the vertices, indices and expand the drawCmd.Scissor
                uint32 outBaseIndex = 0;
                for (uint32 quadIdx : uniqDraw.second.quadIdxs)
                {
                    drawCmd.scissor += drawingCtx.perQuadClipping()[quadIdx];

                    uint32 outBaseVertIdx = quadIdxOffset * 4;
                    indicesView[drawCmd.indicesOffset + outBaseIndex + 0] = outBaseVertIdx + 0;
                    indicesView[drawCmd.indicesOffset + outBaseIndex + 1] = outBaseVertIdx + 1;
                    indicesView[drawCmd.indicesOffset + outBaseIndex + 2] = outBaseVertIdx + 3;
                    indicesView[drawCmd.indicesOffset + outBaseIndex + 3] = outBaseVertIdx + 3;
                    indicesView[drawCmd.indicesOffset + outBaseIndex + 4] = outBaseVertIdx + 1;
                    indicesView[drawCmd.indicesOffset + outBaseIndex + 5] = outBaseVertIdx + 2;
                    outBaseIndex += 6;

                    uint32 inBaseVertIdx = quadIdx * 4;
                    for (uint32 vertIdx = 0; vertIdx < 4; ++vertIdx)
                    {
                        verticesView[outBaseVertIdx + vertIdx].position = Vector2D(
                            float(drawingCtx.perVertexPos()[inBaseVertIdx + vertIdx].x),
                            float(drawingCtx.perVertexPos()[inBaseVertIdx + vertIdx].y)
                        );
                        verticesView[outBaseVertIdx + vertIdx].uv = drawingCtx.perVertexUV()[inBaseVertIdx + vertIdx];
                        verticesView[outBaseVertIdx + vertIdx].color = drawingCtx.perVertexColor()[inBaseVertIdx + vertIdx];
                    }
                    quadIdxOffset++;
                }
                // Convert scissor to window scaled size
                drawCmd.scissor.minBound = drawingContexts[i].first->applyDpiScale(drawCmd.scissor.minBound);
                drawCmd.scissor.maxBound = drawingContexts[i].first->applyDpiScale(drawCmd.scissor.maxBound);
            }
        }

        graphicsHelper->flushMappedPtr(graphicsInstance, std::vector<BufferResourceRef>{ vertices, indices });
        graphicsHelper->returnMappedPtr(graphicsInstance, vertices);
        graphicsHelper->returnMappedPtr(graphicsInstance, indices);
    }

    // Render the widget draw commands
    RenderPassClearValue clearValue;
    clearValue.colors.emplace_back(LinearColorConst::WHITE_Transparent);
    clearValue.depth = 1;

    RenderPassAdditionalProps additionalParams;
    additionalParams.bUsedAsPresentSource = true;

    GraphicsPipelineState pipelineState;
    pipelineState.pipelineQuery.cullingMode = ECullingMode::BackFace;
    pipelineState.pipelineQuery.drawMode = EPolygonDrawMode::Fill;

    for (uint32 i = 0; i < drawingContexts.size(); ++i)
    {
        uint32 width, height;
        drawingContexts[i].first->getAppWindow()->windowSize(width, height);
        QuantizedBox2D renderArea{ Int2D(0), Int2D(width, height) };

        LocalPipelineContext &pipelineContext = pipelineCntxPerWnd[i];
        WindowState *windowState = statePerWnd[i];

        // Wait until corresponding previous frame draw is done
        cmdList->finishCmd(windowState->perFrameCmdBuffers[pipelineContext.swapchainIdx]);

        const GraphicsResource *cmdBuffer
            = cmdList->startCmd(windowState->perFrameCmdBuffers[pipelineContext.swapchainIdx], EQueueFunction::Graphics, true);
        cmdBufferPerWnd[i] = cmdBuffer;

        SCOPED_STR_CMD_MARKER(cmdList, cmdBuffer, TCHAR("WidgetRHIRender_") + drawingContexts[i].first->getAppWindow()->getWindowName());
        cmdList->cmdBeginRenderPass(cmdBuffer, pipelineContext, renderArea, additionalParams, clearValue);
        cmdList->cmdSetViewportAndScissor(cmdBuffer, renderArea, renderArea);
        cmdList->cmdBindVertexBuffer(cmdBuffer, 0, vertices, 0);
        cmdList->cmdBindIndexBuffer(cmdBuffer, indices, 0);
        cmdList->cmdBindGraphicsPipeline(cmdBuffer, pipelineContext, pipelineState);
        cmdList->cmdBindDescriptorsSets(cmdBuffer, pipelineContext, windowState->windowTransformParam);
        for (const WgDrawCmd &drawCmd : drawCmdsPerWnd[i])
        {
            QuantizedBox2D scissor{
                Int2D{drawCmd.scissor.minBound.x, drawCmd.scissor.minBound.y},
                Int2D{drawCmd.scissor.maxBound.x, drawCmd.scissor.maxBound.y}
            };
            cmdList->cmdSetScissor(cmdBuffer, scissor);
            cmdList->cmdBindDescriptorsSets(cmdBuffer, pipelineContext, textureParams[drawCmd.textureDescIdx].first);

            cmdList->cmdDrawIndexed(cmdBuffer, drawCmd.indicesOffset, drawCmd.indicesCount);
        }
        cmdList->cmdEndRenderPass(cmdBuffer);

        cmdList->endCmd(cmdBuffer);

        CommandSubmitInfo submitInfo;
        submitInfo.cmdBuffers.emplace_back(cmdBuffer);
        submitInfo.waitOn.emplace_back(swapchainSemaphores[i], INDEX_TO_FLAG_MASK(EPipelineStages::FragmentShaderStage));
        submitInfo.signalSemaphores = {
            {windowState->readyToPresent[pipelineContext.swapchainIdx], INDEX_TO_FLAG_MASK(EPipelineStages::ColorAttachmentOutput)}
        };
        for (const SemaphoreRef &semaphore : drawingContexts[i].second.allWaitOnSemaphores())
        {
            submitInfo.waitOn.emplace_back(semaphore, INDEX_TO_FLAG_MASK(EPipelineStages::FragmentShaderStage));
        }
        cmdList->submitCmd(EQueuePriority::High, submitInfo, windowState->perFrameSubmitFences[pipelineContext.swapchainIdx]);
    }

    clearTexturesCounter++;
    if (clearTexturesCounter == CLEAR_EVERY)
    {
        clearTexturesCounter = 0;
        clearUnusedTextures();
    }
}
