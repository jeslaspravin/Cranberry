/*!
 * \file WgWorldImGuiLayer.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/WgWorldImGuiLayer.h"
#include "Widgets/ImGui/ImGuiLib/imgui.h"

void WgWorldImGuiLayer::draw(ImGuiDrawInterface */*drawInterface*/)
{
    if (ImGui::Begin("World"))
    {}
    ImGui::End();
}

void WgWorldImGuiLayer::setWorld(cbe::World *world) { currentWorld = world; }
