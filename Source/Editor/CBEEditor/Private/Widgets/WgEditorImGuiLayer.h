/*!
 * \file WgEditorImGuiLayer.h
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

class WgEditorImGuiLayer : public IImGuiLayer
{
private:
    bool bShowAbout = false;

    std::unordered_map<std::string, ImGuiDrawInterfaceCallback> menuExtenders;

public:
    /* IImGuiLayer overrides */

    virtual int32 layerDepth() const { return 0; }
    virtual int32 sublayerDepth() const { return 0; }
    // Draw to ImGui context
    virtual void draw(ImGuiDrawInterface *drawInterface);
    virtual bool drawDirect(const DrawDirectParams &params) { return false; }

    /* WidgetBase overrides */
    void drawWidget(QuantShortBox2D clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context) override {}
    bool hasWidget(SharedPtr<WidgetBase> widget) const override { return false; }
    void tick(float timeDelta) override {}

    /* Overrides ends */

    DelegateHandle addMenuDrawExtender(const TChar *menuName, ImGuiDrawInterfaceCallback::SingleCastDelegateType &callback);
    void removeMenuExtender(const TChar *menuName, DelegateHandle handle);

private:
    void addMenubar(ImGuiDrawInterface *drawInterface);
    void aboutWindow();
};