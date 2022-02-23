/*!
 * \file ImGuiDrawInterface.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "ImGuiDrawInterface.h"
#include "Math/MathGeom.h"
#include "ImGuiLib/imgui_internal.h"

void ImGuiDrawInterface::drawQuadFilled(const Vector2D& min, const Vector2D& max
    , const Vector2D& offset, float rotInDeg, Color color /*= ColorConst::WHITE*/, TextureBase* texture /*= nullptr*/)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    // ImGui::GetCursorPos() - Gives pos relative to window pos without any scrolling as it adds scroll and negates relative screen pos
    Vector2D contentStart = Vector2D(ImGui::GetCursorScreenPos()) + Vector2D(ImGui::GetWindowPos()) + offset;

    Vector2D size = max - min;
    Vector2D a = MathGeom::transform2d(min, contentStart, rotInDeg);
    Vector2D b = MathGeom::transform2d(min + Vector2D(size.x(), 0), contentStart, rotInDeg);
    Vector2D c = MathGeom::transform2d(min + Vector2D(size.x(), size.y()), contentStart, rotInDeg);
    Vector2D d = MathGeom::transform2d(min + Vector2D(0, size.y()), contentStart, rotInDeg);
    
    //Rect bb(a);
    //bb.grow(b);
    //bb.grow(c);
    //bb.grow(d);
    //ImGui::ItemSize(size);
    //ImGui::ItemAdd(ImRect(bb.minBound, bb.maxBound), 0);
    if (texture)
    {
        drawList->PushTextureID(texture);
        drawList->PrimReserve(6, 4);
        drawList->PrimQuadUV(a, b, c, d
            , { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }
            , color);
        drawList->PopTextureID();
    }
    else
    {
        drawList->AddQuadFilled(a, b, c, d, color);
    }
}

void ImGuiDrawInterface::drawQuad(const Vector2D& min, const Vector2D& max
    , const Vector2D& offset, float rotInDeg, Color color /*= ColorConst::WHITE*/)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    // ImGui::GetCursorPos() - Gives pos relative to window pos without any scrolling as it adds scroll and negates relative screen pos
    Vector2D contentStart = Vector2D(ImGui::GetCursorScreenPos()) + Vector2D(ImGui::GetWindowPos()) + offset;

    Vector2D size = max - min;
    Vector2D a = MathGeom::transform2d(min, contentStart, rotInDeg);
    Vector2D b = MathGeom::transform2d(min + Vector2D(size.x(), 0), contentStart, rotInDeg);
    Vector2D c = MathGeom::transform2d(min + Vector2D(size.x(), size.y()), contentStart, rotInDeg);
    Vector2D d = MathGeom::transform2d(min + Vector2D(0, size.y()), contentStart, rotInDeg);

    //Rect bb(a);
    //bb.grow(b);
    //bb.grow(c);
    //bb.grow(d);
    //ImGui::ItemSize(size);
    //ImGui::ItemAdd(ImRect(bb.minBound, bb.maxBound), 0);
    drawList->AddQuad(a, b, c, d, color);
}
