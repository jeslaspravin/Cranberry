/*!
 * \file IImGuiLayer.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Types/CoreTypes.h"
#include "Widgets/WidgetBase.h"

class ImGuiDrawInterface;
class GraphicsResource;
class IRenderCommandList;
class IGraphicsInstance;
class GraphicsHelperAPI;
class WgRenderTarget;

/**
 * Individual layer could have it's own Widget as well
 */
class IImGuiLayer : public WidgetBase
{
public:
    struct DrawDirectParams
    {
        /**
         * Must be filled to FALSE if this draws to RT and RenderPass clears the entire RT
         * This is there to avoid having to set layout for RT and do cmdClearImage(), Instead clear at first draw
         */
        bool &inOutClearRt;
        WgRenderTarget *rt;
        const GraphicsResource *cmdBuffer;
        IRenderCommandList *cmdList;
        IGraphicsInstance *graphicsInstance;
        const GraphicsHelperAPI *graphicsHelper;
    };

public:
    virtual int32 layerDepth() const = 0;
    virtual int32 sublayerDepth() const = 0;
    // Draw to ImGui context
    virtual void draw(ImGuiDrawInterface *drawInterface) = 0;

    /**
     * Directly draw inside the command buffer this is initially created to draw directly to ImGui render target
     * This function will be called from render thread only. Whatever drawn will be drawn before/under ImGui widgets are drawn
     *
     * Returns true if this layer did any draw
     */
    virtual bool drawDirect(const DrawDirectParams & /*params*/) { return false; }

    /**
     * If you want to draw some widget on top of ImGui widgets, Note: No interaction is allowed this is for visual only now
     * This function is similar to drawWidget except that drawWidget draws under ImGui widgets
     */
    virtual void drawOnImGui(WidgetDrawContext & /*context*/) {}

    /* WidgetBase overrides */
    // Mixing ImGui and my own widgets is a sin and so to amend for it I disabled all geometry and inputs
    // ImGui handles all for now. Draw only widgets are supported inside ImGui
protected:
    void rebuildGeometry(WidgetGeomId /*thisId*/, WidgetGeomTree & /*geomTree*/) final {}

public:
    void
    drawWidget(ShortRect /*clipBound*/, WidgetGeomId /*thisId*/, const WidgetGeomTree & /*geomTree*/, WidgetDrawContext & /*context*/)
        override
    {}
    bool hasWidget(SharedPtr<WidgetBase> /*widget*/) const override { return false; }

    void tick(float /*timeDelta*/) override {}
    EInputHandleState inputKey(Keys::StateKeyType /*key*/, Keys::StateInfoType /*state*/, const InputSystem * /*inputSystem*/) final
    {
        return EInputHandleState::NotHandled;
    }
    EInputHandleState
    analogKey(AnalogStates::StateKeyType /*key*/, AnalogStates::StateInfoType /*state*/, const InputSystem * /*inputSystem*/) final
    {
        return EInputHandleState::NotHandled;
    }
    void mouseEnter(Short2 /*absPos*/, Short2 /*widgetRelPos*/, const InputSystem * /*inputSystem*/) final {}
    void mouseMoved(Short2 /*absPos*/, Short2 /*widgetRelPos*/, const InputSystem * /*inputSystem*/) final {}
    void mouseLeave(Short2 /*absPos*/, Short2 /*widgetRelPos*/, const InputSystem * /*inputSystem*/) final {}

    /* Overrides ends */
};
