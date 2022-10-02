/*!
 * \file WgViewportImGuiLayer.h
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
#include "Types/Camera/Camera.h"
#include "EditorTypes.h"

class WorldViewport;

class WgViewportImGuiLayer : public IImGuiLayer
{
private:
    bool bDrawingViewport = false;
    QuantShortBox2D viewportRegion;

    Camera defaultCamera;
    SharedPtr<WorldViewport> worldViewport;

public:
    WgViewportImGuiLayer();

    /* IImGuiLayer overrides */

    int32 layerDepth() const override { return 0; }
    int32 sublayerDepth() const override { return 1; }
    // Draw to ImGui context
    void draw(ImGuiDrawInterface *drawInterface) override;
    bool drawDirect(const DrawDirectParams &params) override;
    void drawOnImGui(WidgetDrawContext &context) override;

    /* WidgetBase overrides */
    void drawWidget(QuantShortBox2D clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context) override;
    bool hasWidget(SharedPtr<WidgetBase> widget) const override { return false; }
    void tick(float timeDelta) override {}
    /* Overrides ends */

    void setWorldViewport(SharedPtr<WorldViewport> inViewport) { worldViewport = inViewport; }

private:
    void navigateScene();
};