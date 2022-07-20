/*!
 * \file WidgetBase.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ApplicationExports.h"
#include "InputSystem/Keys.h"
#include "Types/Containers/FlatTree.h"
#include "Memory/SmartPointers.h"
#include "Math/Box.h"

class InputSystem;
class WidgetBase;
class WgWindow;
class WidgetDrawContext;

struct WidgetGeom
{
    SharedPtr<WidgetBase> widget = nullptr;
    QuantShortBox2D box;
};
using WidgetGeomTree = FlatTree<WidgetGeom, uint32>;
using WidgetGeomId = WidgetGeomTree::NodeIdx;

enum class EInputHandleState
{
    Processed,
    NotHandled
};

class APPLICATION_EXPORT WidgetBase : public std::enable_shared_from_this<WidgetBase>
{
protected:
    SharedPtr<WidgetBase> parentWidget = nullptr;

public:
    virtual ~WidgetBase() = default;
    // rebuildGeometry and drawWidget is recursive. Each widget must call its sub widgets
    virtual void rebuildGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree) = 0;
    virtual void drawWidget(QuantShortBox2D clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context) = 0;

    // below virtual functions are non recursive. Overrides do not have to call its child as children will be processed before parent
    virtual void tick(float timeDelta) = 0;
    virtual EInputHandleState inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem) = 0;
    // absPos are not screen position but position relative to widget's window and widgetRelPos is this widget relative position
    virtual void mouseEnter(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) = 0;
    virtual void mouseMoved(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) = 0;
    virtual void mouseLeave(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem) = 0;

    static SharedPtr<WgWindow> findWidgetParentWindow(SharedPtr<WidgetBase> widget);

protected:
    static float getWidgetScaling(SharedPtr<WidgetBase> widget);
    /**
     * Gets widget's geometry in this frame, Avoid calling this often as it traverses the geometry tree to find it
     */
    static WidgetGeom getWidgetGeom(SharedPtr<WidgetBase> widget);

    // From root window widget at 0 to finding widget at (n - 1)
    static std::vector<SharedPtr<WidgetBase>> getWidgetChain(SharedPtr<WidgetBase> widget);
    static void setupParent(SharedPtr<WidgetBase> childWidget, SharedPtr<WidgetBase> parentWidget) { childWidget->parentWidget = parentWidget; }
};