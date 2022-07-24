/*!
 * \file WgImGui.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Widgets/WidgetBase.h"
#include "Widgets/WgRenderTarget.h"
#include "RenderInterface/Resources/GraphicsSyncResource.h"

class ImGuiManager;
class GraphicsResource;
class IRenderCommandList;
class IGraphicsInstance;
class GraphicsHelperAPI;

class APPLICATION_EXPORT WgImGui : public WidgetBase
{
public:
    struct WgArguments
    {
        String imguiManagerName;
    };

private:
    struct FrameBufferedData
    {
        WgRenderTarget rt;
        // Do not need fence as window widget's fence will take care of sync as long as we are in same window
        SemaphoreRef semaphore;
    };
    std::vector<FrameBufferedData> swapchainBuffered;
    uint32 imageIdx = 0;

    SharedPtr<WgWindow> wgWindow;
    ImGuiManager *imgui;

public:
    void construct(const WgArguments &args);
    ~WgImGui();

    ImGuiManager &getImGuiManager()
    {
        debugAssert(imgui);
        return *imgui;
    }

    /* WidgetBase overrides */
protected:
    void rebuildGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree) override;

public:
    void drawWidget(QuantShortBox2D clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context) override;
    bool hasWidget(SharedPtr<WidgetBase> widget) const override;

    void tick(float timeDelta) override;
    EInputHandleState inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem) override;
    void mouseEnter(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) override;
    void mouseMoved(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) override;
    void mouseLeave(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) override;

    /* Overrides ends */
private:
    void drawImGui(
        const GraphicsResource *cmdBuffer, class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance,
        const GraphicsHelperAPI *graphicsHelper
    );
};