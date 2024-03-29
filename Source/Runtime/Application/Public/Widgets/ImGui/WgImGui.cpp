/*!
 * \file WgImGui.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/ImGui/WgImGui.h"
#include "Types/Platform/Threading/CoPaT/JobSystem.h"
#include "ApplicationInstance.h"
#include "IApplicationModule.h"
#include "Widgets/WidgetWindow.h"
#include "Widgets/ImGui/IImGuiLayer.h"
#include "Widgets/ImGui/ImGuiManager.h"
#include "Widgets/WidgetDrawContext.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderApi/RenderManager.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/CommandBuffer.h"
#include "RenderApi/Rendering/RenderingContexts.h"

void WgImGui::construct(const WgArguments &args)
{
    debugAssert(!args.imguiManagerName.empty());
    if (imgui)
    {
        clearResources();
    }
    imgui = new ImGuiManager(args.imguiManagerName.getChar(), args.parentImguiCntxt);
    imgui->initialize({ .bEnableDocking = args.bEnableDocking });
}

WgImGui::~WgImGui()
{
    if (imgui)
    {
        clearResources();
    }
}

void WgImGui::rebuildGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree)
{
    // Right now we are not doing anything fancy just filling ImGui to parent's size
    WidgetGeomId parentId = geomTree.getNode(thisId).parent;
    debugAssertf(geomTree.isValid(parentId), "Invalid parent for WgImGui!");
    geomTree[thisId].box = geomTree[parentId].box;
    // Set display size of ImGui
    if (imgui)
    {
        imgui->setDisplaySize(geomTree[thisId].box.size());
    }

    /**
     * Right now this code does nothing except allow drawWidget on each children of this,
     * Once app widgets and ImGui can coexists interaction wise this will be useful
     * If they never going to coexist remove this and change how we draw app widgets in drawWidget()
     */
    for (const std::pair<const int32, std::vector<SharedPtr<IImGuiLayer>>> &layersPerDepth : imgui->getLayers())
    {
        for (const SharedPtr<IImGuiLayer> &layer : layersPerDepth.second)
        {
            WidgetGeom layerGeom;
            layerGeom.widget = layer->shared_from_this();
            WidgetGeomId layerId = geomTree.add(layerGeom, thisId);
            layer->rebuildWidgetGeometry(layerId, geomTree);
        }
    }
}

void WgImGui::drawWidget(ShortRect clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context)
{
    if (!imgui)
    {
        return;
    }
    // Increment first if this is first invocation window change logic will reset back to 0
    imageIdx++;

    SharedPtr<WgWindow> window = findWidgetParentWindow(shared_from_this());
    debugAssertf(window, "Invalid window for WgImGui!");

    Short2 widgetSize = geomTree[thisId].box.size();
    Short2 textureSize = window->applyDpiScale(widgetSize);
    debugAssertf(widgetSize.x >= 0 && widgetSize.y >= 0, "Widget size is invalid [{}, {}]", widgetSize.x, widgetSize.y);
    bool bRegenRt = false;
    bool bFlushCmdBuffers = false;
    uint32 bufferingCount = uint32(swapchainBuffered.size());
    if (wgWindow.expired() || wgWindow.lock() != window)
    {
        // If changing window from another window, Have to wait until all previous rendering finished
        bFlushCmdBuffers = !wgWindow.expired();

        ApplicationInstance *app = IApplicationModule::get()->getApplication();
        WindowCanvasRef windowCanvas = app->getWindowCanvas(window);
        debugAssert(windowCanvas.isValid());

        bufferingCount = windowCanvas->imagesCount();
        bRegenRt = bufferingCount != swapchainBuffered.size();
        wgWindow = window;
        imageIdx = 0;
    }
    else
    {
        imageIdx %= swapchainBuffered.size();
    }
    // Check if resized
    if (swapchainBuffered.size() > imageIdx)
    {
        const WgRenderTarget &rtResources = swapchainBuffered[imageIdx].rt;
        ImageResourceRef rtImage = rtResources.renderTargetResource();
        if (rtImage == nullptr || uint32(textureSize.x) != rtImage->getImageSize().x || uint32(textureSize.y) != rtImage->getImageSize().y)
        {
            bRegenRt = true;
        }
    }

    String cmdBufferNameBase = getCmdBufferBaseName();
    if (bFlushCmdBuffers && !swapchainBuffered.empty())
    {
        flushFreeResources(cmdBufferNameBase, bRegenRt);
    }
    swapchainBuffered.resize(bufferingCount);

    if (bRegenRt)
    {
        regenerateFrameRt(widgetSize, textureSize);
    }

    // Just draw this imgui widget after all layer, If any widget wants to draw below ImGui can just draw without any layer push
    uint32 layerCount = 0;
    if (!imgui->getLayers().empty())
    {
        layerCount = 1;
        int32 currlayerDepth = imgui->getLayers().cbegin()->first;
        context.beginLayer();

        std::vector<WidgetGeomId> layerGeomIds = geomTree.getChildren(thisId);
        for (WidgetGeomId layerGeomId : layerGeomIds)
        {
            const WidgetGeom &layerGeom = geomTree[layerGeomId];
            SharedPtr<IImGuiLayer> layer = std::static_pointer_cast<IImGuiLayer>(layerGeom.widget);
            debugAssert(layer);

            if (layer->layerDepth() != currlayerDepth)
            {
                // Only increase must happen
                debugAssert(layer->layerDepth() > currlayerDepth);
                currlayerDepth = layer->layerDepth();
                layerCount++;
                context.beginLayer();
            }
            // Draws to widget draw commands
            layer->drawWidget(clipBound.getIntersectionBox(layerGeom.box), layerGeomId, geomTree, context);
        }

        // Ensure layers determined by incrementing is matching actual layers
        debugAssert(layerCount == imgui->getLayers().size());
        // Draw ImGui output texture
        context.addWaitCondition(swapchainBuffered[imageIdx].semaphore);
        context.drawBox(geomTree[thisId].box, swapchainBuffered[imageIdx].rt.renderResource(), clipBound);

        // Drawing on top of ImGui widgets
        for (WidgetGeomId layerGeomId : layerGeomIds)
        {
            const WidgetGeom &layerGeom = geomTree[layerGeomId];
            SharedPtr<IImGuiLayer> layer = std::static_pointer_cast<IImGuiLayer>(layerGeom.widget);
            debugAssert(layer);

            layer->drawOnImGui(context);
        }

        // Pop all layers
        for (uint32 i = 0; i < layerCount; ++i)
        {
            context.endLayer();
        }
    }

    ENQUEUE_RENDER_COMMAND(DrawWgImGui)
    (
        [cmdBufferNameBase, this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            String cmdBufferName = cmdBufferNameBase + String::toString(imageIdx);
            String layerDrawCmdBufferName = cmdBufferName + TCHAR("_LayerDraw");

            WgRenderTarget rt = swapchainBuffered[imageIdx].rt;
            bool bClearRt = true;

            // Must finish imgui draw cmd buffer first as it uses layerDrawCmdBuffer
            cmdList->finishCmd(cmdBufferName);
            cmdList->finishCmd(layerDrawCmdBufferName);

            // Drawing layers in separate command buffer so that dependencies can be auto resolved using cmdBarrierResources()
            const GraphicsResource *layerDrawCmdBuffer = cmdList->startCmd(layerDrawCmdBufferName, EQueueFunction::Graphics, true);
            {
                SCOPED_CMD_MARKER(cmdList, layerDrawCmdBuffer, DrawImGuiLayer);
                IImGuiLayer::DrawDirectParams layerDrawParams
                    = { bClearRt, &rt, layerDrawCmdBuffer, cmdList, graphicsInstance, graphicsHelper };
                for (const std::pair<const int32, std::vector<SharedPtr<IImGuiLayer>>> &layersPerDepth : imgui->getLayers())
                {
                    for (const SharedPtr<IImGuiLayer> &layer : layersPerDepth.second)
                    {
                        bool bDrawn = layer->drawDirect(layerDrawParams);
                        // If Drawn then bClearRt must be false(ie cleared already)
                        debugAssertf(
                            !bDrawn || !bClearRt,
                            "First draw must clear RT, It appears that RT is not cleared! or inOutClearRt is not set to false after clear!"
                        );
                    }
                }
            }
            cmdList->endCmd(layerDrawCmdBuffer);
            CommandSubmitInfo2 layerDrawSubmitInfo;
            layerDrawSubmitInfo.cmdBuffers.emplace_back(layerDrawCmdBuffer);
            cmdList->submitCmd(EQueuePriority::High, layerDrawSubmitInfo);
            TimelineSemaphoreRef layerDrawComplete = cmdList->getCmdSignalSemaphore(layerDrawCmdBuffer);

            // Now draw the imgui widgets
            const GraphicsResource *cmdBuffer = cmdList->startCmd(cmdBufferName, EQueueFunction::Graphics, true);
            {
                SCOPED_CMD_MARKER(cmdList, cmdBuffer, DrawWgImGui);
                ImGuiDrawingContext drawingContext;
                drawingContext.bClearRt = bClearRt;
                drawingContext.cmdBuffer = cmdBuffer;
                drawingContext.rtTexture = &rt;
                imgui->draw(cmdList, graphicsInstance, graphicsHelper, drawingContext);
            }
            cmdList->endCmd(cmdBuffer);

            CommandSubmitInfo submitInfo;
            submitInfo.cmdBuffers.emplace_back(cmdBuffer);
            submitInfo.signalSemaphores = {
                {swapchainBuffered[imageIdx].semaphore, INDEX_TO_FLAG_MASK(EPipelineStages::ColorAttachmentOutput)}
            };
            submitInfo.waitOnTimelines.emplace_back(layerDrawComplete, INDEX_TO_FLAG_MASK(EPipelineStages::FragmentShaderStage), 1);
            cmdList->submitCmd(EQueuePriority::High, submitInfo, nullptr);
        }
    );
}

bool WgImGui::hasWidget(SharedPtr<WidgetBase> widget) const
{
    if (imgui)
    {
        for (const std::pair<const int32, std::vector<SharedPtr<IImGuiLayer>>> &layersPerDepth : imgui->getLayers())
        {
            for (const SharedPtr<IImGuiLayer> &layer : layersPerDepth.second)
            {
                if (layer->hasWidget(widget))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void WgImGui::tick(float timeDelta)
{
    if (imgui)
    {
        // Draws to ImGui draw commands along with updates
        imgui->updateFrame(timeDelta);
    }
}

EInputHandleState WgImGui::inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem)
{
    if (imgui)
    {
        return imgui->inputKey(key, state, inputSystem) ? EInputHandleState::Processed : EInputHandleState::NotHandled;
    }
    return EInputHandleState::NotHandled;
}

EInputHandleState WgImGui::analogKey(AnalogStates::StateKeyType key, AnalogStates::StateInfoType state, const InputSystem *inputSystem)
{
    if (imgui)
    {
        return imgui->analogKey(key, state, inputSystem) ? EInputHandleState::Processed : EInputHandleState::NotHandled;
    }
    return EInputHandleState::NotHandled;
}

void WgImGui::mouseEnter(Short2 absPos, Short2 widgetRelPos, const InputSystem *inputSystem)
{
    if (imgui)
    {
        imgui->mouseEnter(absPos, widgetRelPos, inputSystem);
    }
}
void WgImGui::mouseMoved(Short2 absPos, Short2 widgetRelPos, const InputSystem *inputSystem)
{
    if (imgui)
    {
        imgui->mouseMoved(absPos, widgetRelPos, inputSystem);
    }
}
void WgImGui::mouseLeave(Short2 absPos, Short2 widgetRelPos, const InputSystem *inputSystem)
{
    if (imgui)
    {
        imgui->mouseLeave(absPos, widgetRelPos, inputSystem);
    }
}

FORCE_INLINE String WgImGui::getCmdBufferBaseName() const { return imgui->getName() + TCHAR("_"); }

void WgImGui::flushFreeResources(const String &cmdBufferBaseName, bool bClearRtFbs) const
{
    std::vector<WgRenderTarget> rts;
    rts.reserve(swapchainBuffered.size());
    for (const FrameBufferedData &frameData : swapchainBuffered)
    {
        if (frameData.rt.renderTargetResource().isValid())
        {
            rts.emplace_back(frameData.rt);
        }
    }
    ENQUEUE_RENDER_COMMAND(FreeWgImGuiCmds)
    (
        [cmdBufferBaseName, bClearRtFbs,
         rts](IRenderCommandList *cmdList, IGraphicsInstance * /*graphicsInstance*/, const GraphicsHelperAPI * /*graphicsHelper*/)
        {
            SizeT bufferingCount = rts.size();
            for (SizeT i = 0; i < bufferingCount; ++i)
            {
                String cmdBufferName = cmdBufferBaseName + String::toString(i);
                String layerDrawCmdBufferName = cmdBufferName + TCHAR("_LayerDraw");

                // First have to finish and free this as it uses layerDrawCmdBuffer
                const GraphicsResource *cmdBuffer = cmdList->getCmdBuffer(cmdBufferName);
                if (cmdBuffer)
                {
                    cmdList->finishCmd(cmdBuffer);
                    cmdList->freeCmd(cmdBuffer);
                }

                const GraphicsResource *layerDrawCmdBuffer = cmdList->getCmdBuffer(layerDrawCmdBufferName);
                if (layerDrawCmdBuffer)
                {
                    cmdList->finishCmd(layerDrawCmdBuffer);
                    cmdList->freeCmd(layerDrawCmdBuffer);
                }
            }
            if (bClearRtFbs)
            {
                RenderManager *renderMan = IRenderInterfaceModule::get()->getRenderManager();
                for (SizeT i = 0; i < bufferingCount; ++i)
                {
                    deleteRTDeferred(rts[i], renderMan);
                }
            }
        }
    );
}

void WgImGui::clearResources()
{
    flushFreeResources(getCmdBufferBaseName(), true);
    imgui->release();
    ENQUEUE_RENDER_COMMAND(ClearWgImGui)
    (
        [imgui
         = imgui](IRenderCommandList * /*cmdList*/, IGraphicsInstance * /*graphicsInstance*/, const GraphicsHelperAPI * /*graphicsHelper*/)
        {
            delete imgui;
        }
    );
    imgui = nullptr;
}

void WgImGui::regenerateFrameRt(Short2 widgetSize, Short2 textureSize)
{
    FrameBufferedData &perFrameData = swapchainBuffered[imageIdx];
    WgRenderTarget rtToClear = perFrameData.rt;

    WgRenderTargetCI ci;
    ci.sampleCount = EPixelSampleCount::SampleCount1; // No need for more than one sample for UI?
    ci.textureName = (getCmdBufferBaseName() + String::toString(imageIdx)).c_str();
    ci.textureSize = textureSize;
    perFrameData.rt.init(ci);

    // Any cmdList dependents initialized and wait
    ENQUEUE_RENDER_COMMAND(WgImGuiRegenResources)
    (
        [rtToClear, this](IRenderCommandList * /*cmdList*/, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            String cmdBufferNameBase = getCmdBufferBaseName();
            if (!swapchainBuffered[imageIdx].semaphore.isValid())
            {
                swapchainBuffered[imageIdx].semaphore = graphicsHelper->createSemaphore(
                    graphicsInstance, (cmdBufferNameBase + TCHAR("Sema") + String::toString(imageIdx)).c_str()
                    );
                swapchainBuffered[imageIdx].semaphore->init();
            }

            if (rtToClear.renderTargetResource().isValid())
            {
                RenderManager *renderMan = IRenderInterfaceModule::get()->getRenderManager();
                graphicsHelper->markForDeletion(
                    graphicsInstance, SimpleSingleCastDelegate::createStatic(&WgImGui::deleteRTDeferred, rtToClear, renderMan),
                    EDeferredDelStrategy::FrameCount
                );
            }
        }
    );

    // Wait until image/any resources is ready, This wont happen often
    RenderThreadEnqueuer::flushWaitRenderThread();
    imgui->setDisplaySize(widgetSize);
}

void WgImGui::deleteRTDeferred(WgRenderTarget rt, RenderManager *renderMan)
{
    ASSERT_INSIDE_RENDERTHREAD();

    if (rt.renderTargetResource().isValid() && rt.renderResource().isValid())
    {
        // Clear RT's Framebuffer
        const IRenderTargetTexture *rtPtr = &rt;
        renderMan->clearExternInitRtsFramebuffer({ &rtPtr, 1 });

        std::vector<ImageResourceRef> attachments{ rt.renderTargetResource(), rt.renderResource() };
        debugAssertf(
            !renderMan->getGlobalRenderingContext()->hasAnyFbUsingRts(attachments),
            "Some framebuffer are missed when clearing ImGui RT, RT might never gets cleared!"
        );
    }
}
