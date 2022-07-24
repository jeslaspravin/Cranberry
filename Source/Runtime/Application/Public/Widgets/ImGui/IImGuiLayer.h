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

/**
 * Individual layer could have it's own Widget as well
 */
class IImGuiLayer : public WidgetBase
{
public:
    virtual int32 layerDepth() const = 0;
    virtual int32 sublayerDepth() const = 0;
    virtual void draw(ImGuiDrawInterface *drawInterface) = 0;

    /* WidgetBase overrides */
protected:
    void rebuildGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree) override {}

public:
    void drawWidget(QuantShortBox2D clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context) override {}
    bool hasWidget(SharedPtr<WidgetBase> widget) const override { return false; }

    void tick(float timeDelta) override {}
    EInputHandleState inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem) override
    {
        return EInputHandleState::NotHandled;
    }
    void mouseEnter(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) override {}
    void mouseMoved(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) override {}
    void mouseLeave(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) override {}

    /* Overrides ends */
};
