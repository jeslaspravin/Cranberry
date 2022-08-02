/*!
 * \file NullWidget.h
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

class WgNullWidget final : public WidgetBase
{
protected:
    void rebuildGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree) override { geomTree[thisId].box = QuantShortBox2D{ Short2D{ 0 } }; }

public:
    void drawWidget(QuantShortBox2D clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context) override {}
    bool hasWidget(SharedPtr<WidgetBase> widget) const override { return false; }
    void tick(float timeDelta) override {}
    EInputHandleState inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem) override
    {
        return EInputHandleState::NotHandled;
    }
    EInputHandleState analogKey(AnalogStates::StateKeyType key, AnalogStates::StateInfoType state, const InputSystem *inputSystem)
    {
        return EInputHandleState::NotHandled;
    }
    void mouseEnter(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) override {}
    void mouseMoved(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) override {}
    void mouseLeave(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) override {}

    APPLICATION_EXPORT static SharedPtr<WgNullWidget> nullWidget;
};