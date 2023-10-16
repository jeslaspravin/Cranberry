/*!
 * \file WidgetWindow.h
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

    Short2 mousePos;
    SharedPtr<WidgetBase> hoveringWidget;

public:
    void construct(const WgArguments &args);

    FORCE_INLINE GenericAppWindow *getAppWindow() const { return ownerWindow; }
    float getWidgetScaling() const;
    Short2 getWidgetSize() const;
    FORCE_INLINE Short2 applyDpiScale(Short2 pt) const
    {
        return Short2(int16(Math::round(pt.x * getWidgetScaling())), int16(Math::round(pt.y * getWidgetScaling())));
    }
    FORCE_INLINE Short2 removeDpiScale(Short2 pt) const { return Short2(int16(pt.x / getWidgetScaling()), int16(pt.y / getWidgetScaling())); }
    Short2 screenToWgWindowSpace(Short2 screenPt) const;
    Short2 wgWindowToScreenSpace(Short2 windowPt) const;

    void setContent(SharedPtr<WidgetBase> widget);
    SharedPtr<WidgetBase> getContent() const { return content; }

    // Searches cached geometry tree to find widget's geometry. If not found returns default WidgetGeom
    WidgetGeom findWidgetGeom(SharedPtr<WidgetBase> widget) const;

    void drawWidget(WidgetDrawContext &context);
    void rebuildWindowGeoms();
    void clearWindow();

    /* WidgetBase overrides */
protected:
    // below 2 functions will be called from within WgWindow
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
    /* override ends */
};