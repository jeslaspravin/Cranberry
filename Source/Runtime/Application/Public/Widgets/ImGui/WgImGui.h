/*!
 * \file WgImGui.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
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
class RenderManager;

class APPLICATION_EXPORT WgImGui : public WidgetBase
{
public:
    struct WgArguments
    {
        String imguiManagerName;
        ImGuiManager *parentImguiCntxt = nullptr;
        bool bEnableDocking = false;
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

    WeakPtr<WgWindow> wgWindow;
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
    void drawWidget(ShortRect clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context) override;
    bool hasWidget(SharedPtr<WidgetBase> widget) const override;

    void tick(float timeDelta) override;
    EInputHandleState inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem) override;
    EInputHandleState analogKey(AnalogStates::StateKeyType key, AnalogStates::StateInfoType state, const InputSystem *inputSystem) override;
    void mouseEnter(Short2 absPos, Short2 widgetRelPos, const InputSystem *inputSystem) override;
    void mouseMoved(Short2 absPos, Short2 widgetRelPos, const InputSystem *inputSystem) override;
    void mouseLeave(Short2 absPos, Short2 widgetRelPos, const InputSystem *inputSystem) override;

    /* Overrides ends */
private:
    FORCE_INLINE String getCmdBufferBaseName() const;
    void flushFreeResources(const String &cmdBufferBaseName, bool bClearRtFbs) const;
    void clearResources();
    void regenerateFrameRt(Short2 widgetSize, Short2 textureSize);
    static void deleteRTDeferred(WgRenderTarget rt, RenderManager *renderMan);
};