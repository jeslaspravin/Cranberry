/*!
 * \file WgWorldViewport.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/WgWorldViewport.h"

void WgWorldViewport::construct(WgArguments args)
{
    world = args.inWorld;
    worldManager = args.worldManager;
}

void WgWorldViewport::rebuildGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree)
{
    // If parent set the viewport size then skip setting it ourself
    if (geomTree[thisId].box.isValidAABB())
    {
        return;
    }
    // Occupy the entire parent size
    geomTree[thisId].box = geomTree[geomTree.getNode(thisId).parent].box;
}

void WgWorldViewport::
    drawWidget(ShortRect /*clipBound*/, WidgetGeomId /*thisId*/, const WidgetGeomTree & /*geomTree*/, WidgetDrawContext & /*context*/)
{
    // TODO(Jeslas) :
}

bool WgWorldViewport::hasWidget(SharedPtr<WidgetBase> /*widget*/) const { return false; }

void WgWorldViewport::tick(float /*timeDelta*/) {}

EInputHandleState WgWorldViewport::inputKey(Keys::StateKeyType /*key*/, Keys::StateInfoType /*state*/, const InputSystem * /*inputSystem*/)
{
    return EInputHandleState::NotHandled;
}

EInputHandleState
WgWorldViewport::analogKey(AnalogStates::StateKeyType /*key*/, AnalogStates::StateInfoType /*state*/, const InputSystem * /*inputSystem*/)
{
    return EInputHandleState::NotHandled;
}

void WgWorldViewport::mouseEnter(Short2 /*absPos*/, Short2 /*widgetRelPos*/, const InputSystem * /*inputSystem*/) {}

void WgWorldViewport::mouseMoved(Short2 /*absPos*/, Short2 /*widgetRelPos*/, const InputSystem * /*inputSystem*/) {}

void WgWorldViewport::mouseLeave(Short2 /*absPos*/, Short2 /*widgetRelPos*/, const InputSystem * /*inputSystem*/) {}
