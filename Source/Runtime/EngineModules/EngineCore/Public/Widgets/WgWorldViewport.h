/*!
 * \file WgViewport.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "EngineCoreExports.h"
#include "Widgets/WidgetBase.h"

class WgRenderTarget;
class WorldsManager;

namespace cbe
{
class World;
}

// TODO(Jeslas) : WgWorldViewport will be Runtime viewport widget. WgViewportImGuiLayer will be viewport in Editor
class ENGINECORE_EXPORT WgWorldViewport : public WidgetBase
{
private:
    cbe::World *world;
    WorldsManager *worldManager;

public:
    struct WgArguments
    {
        cbe::World *inWorld;
        WorldsManager *worldManager;
    };

    void construct(WgArguments args);

    virtual WgRenderTarget *getViewportRT(IRect &outViewport) = 0;

    /* WidgetBase overrides */
protected:
    void rebuildGeometry(WidgetGeomId thisId, WidgetGeomTree &geomTree) override;

public:
    void drawWidget(ShortRect clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context) override;
    bool hasWidget(SharedPtr<WidgetBase> widget) const override;
    void tick(float timeDelta) override;
    EInputHandleState inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem) override;
    EInputHandleState analogKey(AnalogStates::StateKeyType key, AnalogStates::StateInfoType state, const InputSystem *inputSystem);
    void mouseEnter(Short2 absPos, Short2 widgetRelPos, const InputSystem *inputSystem) override;
    void mouseMoved(Short2 absPos, Short2 widgetRelPos, const InputSystem *inputSystem) override;
    void mouseLeave(Short2 absPos, Short2 widgetRelPos, const InputSystem *inputSystem) override;

    /* overrides ends */
};