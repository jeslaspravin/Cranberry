/*!
 * \file WgDetailsImGuiLayer.h
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Widgets/ImGui/IImGuiLayer.h"
#include "EditorTypes.h"
#include "CoreObjectDelegates.h"

class WgDetailsImGuiLayer : public IImGuiLayer
{
public:
    ObjectGetterDelegate selectionGetter;

public:
    /* IImGuiLayer overrides */

    virtual int32 layerDepth() const { return 0; }
    virtual int32 sublayerDepth() const { return 1; }
    // Draw to ImGui context
    virtual void draw(ImGuiDrawInterface *drawInterface);
    virtual bool drawDirect(const DrawDirectParams &) { return false; }

    /* WidgetBase overrides */
    void drawWidget(ShortRect /*clipBound*/, WidgetGeomId /*thisId*/, const WidgetGeomTree &, WidgetDrawContext &) override {}
    bool hasWidget(SharedPtr<WidgetBase>) const override { return false; }
    void tick(float /*timeDelta*/) override {}

    /* Overrides ends */

private:
};