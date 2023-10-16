/*!
 * \file ImGuiDrawInterface.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ApplicationExports.h"
#include "ImGuiLib/imgui.h"
#include "Math/Box.h"
#include "Math/Vector2.h"
#include "Types/Colors.h"

class TextureBase;

class APPLICATION_EXPORT ImGuiDrawInterface
{
private:
    friend class ImGuiManager;

private:
    void drawQuadFilled(
        const Vector2 &min, const Vector2 &max, const Vector2 &offset, float rotInDeg, Color color = ColorConst::WHITE,
        TextureBase *texture = nullptr
    );
    void drawQuad(const Vector2 &min, const Vector2 &max, const Vector2 &offset, float rotInDeg, Color color = ColorConst::WHITE);

public:
    template <Box2Dim BoxType>
    void drawPackedRectangles(
        const BoxType *packedRects, const Color *colors, uint32 rectsCount, const typename BoxType::PointType &packedIn,
        const Color &packedInRectCol
    );

    bool
    inputText(const char *label, String *str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback = nullptr, void *user_data = nullptr);
    bool inputTextMultiline(
        const char *label, String *str, const ImVec2 &size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback = nullptr,
        void *user_data = nullptr
    );
    bool inputTextWithHint(
        const char *label, const char *hint, String *str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback = nullptr,
        void *user_data = nullptr
    );
};

template <Box2Dim BoxType>
void ImGuiDrawInterface::drawPackedRectangles(
    const BoxType *packedRects, const Color *colors, uint32 rectsCount, const typename BoxType::PointType &packedIn,
    const Color &packedInRectCol
)
{
    for (uint32 i = 0; i < rectsCount; ++i)
    {
        drawQuadFilled(
            { (float)packedRects[i].minBound.x, (float)packedRects[i].minBound.y },
            { (float)packedRects[i].maxBound.x, (float)packedRects[i].maxBound.y }, Vector2::ZERO, 0.0f, colors[i]
        );
    }
    Vector2 packedInRectSize = Vector2{ (float)packedIn.x, (float)packedIn.y };
    drawQuad(Vector2::ZERO, packedInRectSize, Vector2::ZERO, 0.0f, packedInRectCol);
    ImGui::Dummy(packedInRectSize);
}
