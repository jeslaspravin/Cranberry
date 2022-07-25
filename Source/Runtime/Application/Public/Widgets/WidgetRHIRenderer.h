/*!
 * \file WidgetRHIRenderer.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Widgets/WidgetRenderer.h"
#include "Types/Containers/BitArray.h"
#include "Types/Containers/SparseVector.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "RenderInterface/Resources/GraphicsSyncResource.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"

class LocalPipelineContext;
class GenericWindowCanvas;

class WidgetRHIRenderer final : public WidgetRenderer
{
private:
    struct WindowState
    {
        ShaderParametersRef windowTransformParam;
        std::vector<String> perFrameCmdBuffers;
        std::vector<FenceRef> perFrameSubmitFences;
        // Signal semaphore is necessary to present
        std::vector<SemaphoreRef> perFrameSignal;
    };

    ImageResourceRef dummyTexture;
    // Parameter and if used this frame
    SparseVector<std::pair<ShaderParametersRef, bool>, BitArraySparsityPolicy> textureParams;
    std::unordered_map<ImageResourceRef, uint32> textureToParamsIdx;
    std::unordered_map<SharedPtr<WgWindow>, WindowState> windowStates;

    /**
     * No need for per swapchain resource as vertices will be deleted and not mutated
     */
    BufferResourceRef indices;
    BufferResourceRef vertices;

    /* WidgetRenderer overrides */
public:
    void initialize() final;
    void destroy() final;
    void clearWindowState(const SharedPtr<WgWindow> &window) final;

protected:
    void drawWindowWidgets(std::vector<std::pair<SharedPtr<WgWindow>, WidgetDrawContext>> &&drawingContexts);

    /* Overrides ends */
private:
    void initializeRenderThread(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void destroyRenderThread(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);

    FORCE_INLINE void clearUnusedTextures();

    WindowState &createWindowState(
        const SharedPtr<WgWindow> &window, GenericWindowCanvas *swapchainCanvas, IRenderCommandList *cmdList,
        const LocalPipelineContext &pipelineContext, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
    );
    void createVerticesAndIndices(
        uint64 indexCount, uint32 vertCount, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
    );
    void drawWindowWidgetsRenderThread(
        const std::vector<std::pair<SharedPtr<WgWindow>, WidgetDrawContext>> &drawingContexts, IRenderCommandList *cmdList,
        IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
    );
};