/*!
 * \file WgWorldImGuiLayer.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Widgets/ImGui/IImGuiLayer.h"
#include "EditorTypes.h"
#include "CoreObjectDelegates.h"
#include "ObjectPtrs.h"
#include "Classes/World.h"

class WgWorldImGuiLayer : public IImGuiLayer
{
public:
    ObjectGetterDelegate selectionGetter;
    ObjectSetterDelegate onSelected;

private:
    cbe::WeakObjPtr<cbe::World> currentWorld;

public:
    /* IImGuiLayer overrides */

    virtual int32 layerDepth() const { return 0; }
    virtual int32 sublayerDepth() const { return 1; }
    // Draw to ImGui context
    virtual void draw(ImGuiDrawInterface *drawInterface);
    virtual bool drawDirect(const DrawDirectParams &/*params*/) { return false; }

    /* WidgetBase overrides */
    void drawWidget(QuantShortBox2D /*clipBound*/, WidgetGeomId /*thisId*/, const WidgetGeomTree &, WidgetDrawContext &) override {}
    bool hasWidget(SharedPtr<WidgetBase> /*widget*/) const override { return false; }
    void tick(float /*timeDelta*/) override {}

    /* Overrides ends */

    void setWorld(cbe::World *world);

private:
};