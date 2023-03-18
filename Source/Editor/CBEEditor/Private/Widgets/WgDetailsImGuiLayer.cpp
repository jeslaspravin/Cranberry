/*!
 * \file WgDetailsImGuiLayer.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/WgDetailsImGuiLayer.h"
#include "Widgets/ImGui/ImGuiLib/imgui.h"

void WgDetailsImGuiLayer::draw(ImGuiDrawInterface * /*drawInterface*/)
{
    if (ImGui::Begin("Details"))
    {}
    ImGui::End();
}
