/*!
 * \file WgImGui.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/ImGui/WgImGui.h"
#include "ApplicationInstance.h"
#include "IApplicationModule.h"
#include "Widgets/WidgetWindow.h"
#include "Widgets/ImGui/IImGuiLayer.h"
#include "Widgets/ImGui/ImGuiManager.h"
#include "Widgets/WidgetDrawContext.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/CommandBuffer.h"

void WgImGui::construct(const WgArguments &args)
{
    debugAssert(!args.imguiManagerName.empty());
    if (imgui)
    {
        imgui->release();
        delete imgui;
    }
    imgui = new ImGuiManager(args.imguiManagerName.getChar());
    imgui->initialize();
}

WgImGui::~WgImGui()
{
    if (imgui)
    {
        imgui->release();
        delete imgui;
        imgui = nullptr;
    }
}

void WgImGui::rebuildGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree)
{
    // Right now we are not doing anything fancy just filling ImGui to parent's size
    WidgetGeomId parentId = geomTree.getNode(thisId).parent;
    debugAssertf(geomTree.isValid(parentId), "Invalid parent for WgImGui!");
    geomTree[thisId].box = geomTree[parentId].box;

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

void WgImGui::drawWidget(QuantShortBox2D clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context)
{
    if (!imgui)
    {
        return;
    }
    // Increment first if this is first invocation window change logic will reset back to 0
    imageIdx++;

    SharedPtr<WgWindow> window = findWidgetParentWindow(shared_from_this());
    debugAssertf(window, "Invalid window for WgImGui!");

    Short2D widgetSize = geomTree[thisId].box.size();
    Short2D textureSize = Short2D(int16(widgetSize.x * window->getWidgetScaling()), int16(widgetSize.y * window->getWidgetScaling()));
    debugAssertf(widgetSize.x >= 0 && widgetSize.y >= 0, "Widget size is invalid [%d, %d]", widgetSize.x, widgetSize.y);
    bool bRegenRt = false;
    bool bFlushCmdBuffers = false;
    uint32 bufferingCount = swapchainBuffered.size();
    if (wgWindow != window)
    {
        // If changing window from another window, Have to wait until all previous rendering finished
        bFlushCmdBuffers = bool(wgWindow);

        ApplicationInstance *app = IApplicationModule::get()->getApplication();
        WindowCanvasRef windowCanvas = app->getWindowCanvas(window);
        debugAssert(windowCanvas);

        bufferingCount = windowCanvas->imagesCount();
        bRegenRt = bufferingCount != swapchainBuffered.size();
        wgWindow = window;
        imageIdx = 0;
    }
    // Check if resized
    if (swapchainBuffered.size() > imageIdx)
    {
        const WgRenderTarget &rtResources = swapchainBuffered[imageIdx].rt;
        ImageResourceRef rtImage = rtResources.renderTargetResource();
        if (textureSize.x != rtImage->getImageSize().x || textureSize.y != rtImage->getImageSize().y)
        {
            bRegenRt = true;
        }
    }

    String cmdBufferNameBase = imgui->getName() + TCHAR("_");
    if (bFlushCmdBuffers && !swapchainBuffered.empty())
    {
        ENQUEUE_RENDER_COMMAND(FlushWgImGuiCmds)
        (
            [cmdBufferNameBase,
             this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
            {
                for (uint32 i = 0; i < swapchainBuffered.size(); ++i)
                {
                    const GraphicsResource *cmdBuffer = cmdList->getCmdBuffer(cmdBufferNameBase + String::toString(i));
                    if (cmdBuffer)
                    {
                        cmdList->finishCmd(cmdBuffer);
                        cmdList->freeCmd(cmdBuffer);
                    }
                }
            }
        );

        swapchainBuffered.resize(bufferingCount);
    }

    if (bRegenRt)
    {
        FrameBufferedData &perFrameData = swapchainBuffered[imageIdx];

        WgRenderTargetCI ci;
        ci.sampleCount = EPixelSampleCount::SampleCount1; // No need for more than one sample for UI?
        ci.textureName = (cmdBufferNameBase + String::toString(imageIdx)).c_str();
        ci.textureSize = textureSize;
        perFrameData.rt.init(ci);

        // Any cmdList dependents initialized and wait
        if (!swapchainBuffered[imageIdx].semaphore.isValid())
        {
            ENQUEUE_RENDER_COMMAND(WgImGuiRegenResources)
            (
                [cmdBufferNameBase,
                 this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
                {
                    swapchainBuffered[imageIdx].semaphore = graphicsHelper->createSemaphore(
                        graphicsInstance, (cmdBufferNameBase + TCHAR("Sema") + String::toString(imageIdx)).c_str()
                        );
                    swapchainBuffered[imageIdx].semaphore->init();
                }
            );
        }

        // Wait until image/any resources is ready, This wont happen often
        RenderThreadEnqueuer::flushWaitRenderThread();
        imgui->setDisplaySize(widgetSize);
    }

    // Just draw this imgui widget above all layer, If any widget wants to draw below ImGui can just draw without any layer push
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

            layer->drawWidget(clipBound.getIntersectionBox(layerGeom.box), layerGeomId, geomTree, context);
        }
    }
    // Ensure layers determined by incrementing is matching actual layers
    debugAssert(layerCount == imgui->getLayers().size());
    // Draw ImGui output texture
    context.beginLayer();
    context.addWaitCondition(swapchainBuffered[imageIdx].semaphore);
    context.drawBox(geomTree[thisId].box, swapchainBuffered[imageIdx].rt.renderResource(), clipBound);
    context.endLayer();
    for (uint32 i = 0; i < layerCount; ++i)
    {
        context.endLayer();
    }

    ENQUEUE_RENDER_COMMAND(DrawWgImGui)
    (
        [cmdBufferNameBase,
         this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            String cmdBufferName = cmdBufferNameBase + String::toString(imageIdx);
            // No need to finish before starting as it will be taken care by window widget
            const GraphicsResource *cmdBuffer = cmdList->startCmd(cmdBufferName, EQueueFunction::Graphics, true);

            drawImGui(cmdBuffer, cmdList, graphicsInstance, graphicsHelper);

            cmdList->endCmd(cmdBuffer);

            CommandSubmitInfo submitInfo;
            submitInfo.cmdBuffers.emplace_back(cmdBuffer);
            submitInfo.signalSemaphores.emplace_back(swapchainBuffered[imageIdx].semaphore);
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

void WgImGui::mouseEnter(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem)
{
    if (imgui)
    {
        imgui->mouseEnter(absPos, widgetRelPos, inputSystem);
    }
}
void WgImGui::mouseMoved(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem)
{
    if (imgui)
    {
        imgui->mouseMoved(absPos, widgetRelPos, inputSystem);
    }
}
void WgImGui::mouseLeave(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem)
{
    if (imgui)
    {
        imgui->mouseLeave(absPos, widgetRelPos, inputSystem);
    }
}

void WgImGui::drawImGui(
    const GraphicsResource *cmdBuffer, class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
    const GraphicsHelperAPI *graphicsHelper
)
{
    if (imgui)
    {
        ImGuiDrawingContext drawingContext;
        drawingContext.bClearRt = true;
        drawingContext.cmdBuffer = cmdBuffer;
        WgRenderTarget rt = swapchainBuffered[imageIdx].rt;
        drawingContext.rtTexture = &rt;
        imgui->draw(cmdList, graphicsInstance, graphicsHelper, drawingContext);
    }
}
