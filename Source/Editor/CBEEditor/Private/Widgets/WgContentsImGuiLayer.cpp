/*!
 * \file WgContentsImGuiLayer.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/WgContentsImGuiLayer.h"
#include "Widgets/ImGui/ImGuiLib/imgui.h"

void WgContentsImGuiLayer::draw(ImGuiDrawInterface * /*drawInterface*/)
{
    if (ImGui::Begin("Contents"))
    {}
    ImGui::End();
}
