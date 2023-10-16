/*!
 * \file ImGuiDrawInterface.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/ImGui/ImGuiDrawInterface.h"
#include "Widgets/ImGui/ImGuiLib/imgui_internal.h"
#include "Math/MathGeom.h"

void ImGuiDrawInterface::drawQuadFilled(
    const Vector2 &min, const Vector2 &max, const Vector2 &offset, float rotInDeg, Color color /*= ColorConst::WHITE*/,
    TextureBase *texture /*= nullptr*/
)
{
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    // ImGui::GetCursorPos() - Gives pos relative to window pos without any scrolling as it adds scroll
    // and negates relative screen pos
    Vector2 contentStart = Vector2(ImGui::GetCursorScreenPos()) + Vector2(ImGui::GetWindowPos()) + offset;

    Vector2 size = max - min;
    Vector2 a = MathGeom::transform2d(min, contentStart, rotInDeg);
    Vector2 b = MathGeom::transform2d(min + Vector2(size.x(), 0), contentStart, rotInDeg);
    Vector2 c = MathGeom::transform2d(min + Vector2(size.x(), size.y()), contentStart, rotInDeg);
    Vector2 d = MathGeom::transform2d(min + Vector2(0, size.y()), contentStart, rotInDeg);

    // Rect bb(a);
    // bb.grow(b);
    // bb.grow(c);
    // bb.grow(d);
    // ImGui::ItemSize(size);
    // ImGui::ItemAdd(ImRect(bb.minBound, bb.maxBound), 0);
    if (texture)
    {
        drawList->PushTextureID(texture);
        drawList->PrimReserve(6, 4);
        drawList->PrimQuadUV(a, b, c, d, { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, color);
        drawList->PopTextureID();
    }
    else
    {
        drawList->AddQuadFilled(a, b, c, d, color);
    }
}

void ImGuiDrawInterface::
    drawQuad(const Vector2 &min, const Vector2 &max, const Vector2 &offset, float rotInDeg, Color color /*= ColorConst::WHITE*/)
{
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    // ImGui::GetCursorPos() - Gives pos relative to window pos without any scrolling as it adds scroll
    // and negates relative screen pos
    Vector2 contentStart = Vector2(ImGui::GetCursorScreenPos()) + Vector2(ImGui::GetWindowPos()) + offset;

    Vector2 size = max - min;
    Vector2 a = MathGeom::transform2d(min, contentStart, rotInDeg);
    Vector2 b = MathGeom::transform2d(min + Vector2(size.x(), 0), contentStart, rotInDeg);
    Vector2 c = MathGeom::transform2d(min + Vector2(size.x(), size.y()), contentStart, rotInDeg);
    Vector2 d = MathGeom::transform2d(min + Vector2(0, size.y()), contentStart, rotInDeg);

    // Rect bb(a);
    // bb.grow(b);
    // bb.grow(c);
    // bb.grow(d);
    // ImGui::ItemSize(size);
    // ImGui::ItemAdd(ImRect(bb.minBound, bb.maxBound), 0);
    drawList->AddQuad(a, b, c, d, color);
}

//////////////////////////////////////////////////////////////////////////
/// String support for input texts, Copied from misc/cpp/imgui_stdlib.cpp
//////////////////////////////////////////////////////////////////////////

struct InputTextCallback_UserData
{
    String *str;
    ImGuiInputTextCallback chainCallback;
    void *chainCallbackUserData;
};

static int InputTextCallback(ImGuiInputTextCallbackData *data)
{
    InputTextCallback_UserData *uData = (InputTextCallback_UserData *)data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need
        // to set them back to what we want.
        std::string *str = uData->str;
        IM_ASSERT(data->Buf == str->c_str());
        str->resize(data->BufTextLen);
        data->Buf = (char *)str->c_str();
    }
    else if (uData->chainCallback)
    {
        // Forward to user callback, if any
        data->UserData = uData->chainCallbackUserData;
        return uData->chainCallback(data);
    }
    return 0;
}
bool ImGuiDrawInterface::inputText(
    const char *label, String *str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback /*= nullptr*/, void *user_data /*= nullptr*/
)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.str = str;
    cb_user_data.chainCallback = callback;
    cb_user_data.chainCallbackUserData = user_data;
    return ImGui::InputText(label, (char *)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
}

bool ImGuiDrawInterface::inputTextMultiline(
    const char *label, String *str, const ImVec2 &size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback /*= nullptr*/,
    void *user_data /*= nullptr*/
)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.str = str;
    cb_user_data.chainCallback = callback;
    cb_user_data.chainCallbackUserData = user_data;
    return ImGui::InputTextMultiline(label, (char *)str->c_str(), str->capacity() + 1, size, flags, InputTextCallback, &cb_user_data);
}

bool ImGuiDrawInterface::inputTextWithHint(
    const char *label, const char *hint, String *str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback /*= nullptr*/,
    void *user_data /*= nullptr*/
)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.str = str;
    cb_user_data.chainCallback = callback;
    cb_user_data.chainCallbackUserData = user_data;
    return ImGui::InputTextWithHint(label, hint, (char *)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
}
