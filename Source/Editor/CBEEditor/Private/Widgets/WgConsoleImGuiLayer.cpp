/*!
 * \file WgConsoleImGuiLayer.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/WgConsoleImGuiLayer.h"
#include "Widgets/ImGui/ImGuiLib/imgui.h"

void WgConsoleImGuiLayer::draw(ImGuiDrawInterface * /*drawInterface*/)
{
    if (ImGui::Begin("Console"))
    {}
    ImGui::End();
}
