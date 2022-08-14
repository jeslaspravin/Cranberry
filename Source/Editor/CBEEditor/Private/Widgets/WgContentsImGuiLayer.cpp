/*!
 * \file WgContentsImGuiLayer.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/WgContentsImGuiLayer.h"
#include "Widgets/ImGui/ImGuiLib/imgui.h"

void WgContentsImGuiLayer::draw(ImGuiDrawInterface *drawInterface)
{
    if (ImGui::Begin("Contents"))
    {}
    ImGui::End();
}
