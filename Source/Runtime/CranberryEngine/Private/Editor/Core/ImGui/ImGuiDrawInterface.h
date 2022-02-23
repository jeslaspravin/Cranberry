/*!
 * \file ImGuiDrawInterface.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Math/Vector2D.h"
#include "Types/Colors.h"
#include "Math/Box.h"
#include "ImGuiLib/imgui.h"

class TextureBase;

class ImGuiDrawInterface
{
private:
    friend class ImGuiManager;
private:
    void drawQuadFilled(const Vector2D& min, const Vector2D& max
        , const Vector2D& offset, float rotInDeg, Color color = ColorConst::WHITE, TextureBase* texture = nullptr);
    void drawQuad(const Vector2D& min, const Vector2D& max
        , const Vector2D& offset, float rotInDeg, Color color = ColorConst::WHITE);
public:
    template <Box2DType BoxType>
    void drawPackedRectangles(const BoxType* packedRects, const Color* colors, uint32 rectsCount, const typename BoxType::PointType& packedIn, const Color& packedInRectCol);
};

template <Box2DType BoxType> 
void ImGuiDrawInterface::drawPackedRectangles(const BoxType* packedRects, const Color* colors, uint32 rectsCount, const typename BoxType::PointType& packedIn, const Color& packedInRectCol)
{    
    for (uint32 i = 0; i < rectsCount; ++i)
    {
        drawQuadFilled({ (float)packedRects[i].minBound.x, (float)packedRects[i].minBound.y }
            , { (float)packedRects[i].maxBound.x, (float)packedRects[i].maxBound.y }
        , Vector2D::ZERO, 0.0f, colors[i]);
    }
    Vector2D packedInRectSize = Vector2D{ (float)packedIn.x, (float)packedIn.y };
    drawQuad(Vector2D::ZERO, packedInRectSize, Vector2D::ZERO, 0.0f, packedInRectCol);
    ImGui::Dummy(packedInRectSize);
}
