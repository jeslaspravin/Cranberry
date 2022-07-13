/*!
 * \file WidgetWindow.h
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

class GenericAppWindow;

class APPLICATION_EXPORT WgWindow : public WidgetBase
{
public:
    struct WgArguments
    {
        GenericAppWindow *ownerWindow;
        SharedPtr<WidgetBase> content;
        float scaling = 1.0f;
    };

private:
    WidgetGeomTree allWidgetGeoms;

    GenericAppWindow *ownerWindow;
    SharedPtr<WidgetBase> content;
    float scaling;

    Short2D mousePos;
    SharedPtr<WidgetBase> hoveringWidget;

public:
    void construct(const WgArguments &args);

    FORCE_INLINE GenericAppWindow *getAppWindow() const { return ownerWindow; }
    Short2D getWidgetSize() const;
    float getWidgetScaling() const;
    Short2D screenToWindowSpace(Short2D screenPt) const;
    Short2D windowToScreenSpace(Short2D windowPt) const;

    WidgetGeom findWidgetGeom(SharedPtr<WidgetBase> widget) const;

    void drawWidget(WidgetDrawContext &context);
    void setContent(SharedPtr<WidgetBase> widget);

    /* WidgetBase overrides */
    // below 2 functions will be called from within WgWindow
    void rebuildGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree) override;
    void drawWidget(QuantShortBox2D clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context) override;

    void tick(float timeDelta) override;
    EInputHandleState inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem) override;
    void mouseEnter(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) override;
    void mouseMoved(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) override;
    void mouseLeave(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) override;
    /* override ends */
};