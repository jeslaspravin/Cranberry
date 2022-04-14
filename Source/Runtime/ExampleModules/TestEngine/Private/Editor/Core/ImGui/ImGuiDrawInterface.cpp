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
#include "ImGuiLib/imgui_internal.h"
#include "Math/MathGeom.h"

void ImGuiDrawInterface::drawQuadFilled(const Vector2D &min, const Vector2D &max, const Vector2D &offset,
    float rotInDeg, Color color /*= ColorConst::WHITE*/, TextureBase *texture /*= nullptr*/)
{
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    // ImGui::GetCursorPos() - Gives pos relative to window pos without any scrolling as it adds scroll
    // and negates relative screen pos
    Vector2D contentStart
        = Vector2D(ImGui::GetCursorScreenPos()) + Vector2D(ImGui::GetWindowPos()) + offset;

    Vector2D size = max - min;
    Vector2D a = MathGeom::transform2d(min, contentStart, rotInDeg);
    Vector2D b = MathGeom::transform2d(min + Vector2D(size.x(), 0), contentStart, rotInDeg);
    Vector2D c = MathGeom::transform2d(min + Vector2D(size.x(), size.y()), contentStart, rotInDeg);
    Vector2D d = MathGeom::transform2d(min + Vector2D(0, size.y()), contentStart, rotInDeg);

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

void ImGuiDrawInterface::drawQuad(const Vector2D &min, const Vector2D &max, const Vector2D &offset,
    float rotInDeg, Color color /*= ColorConst::WHITE*/)
{
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    // ImGui::GetCursorPos() - Gives pos relative to window pos without any scrolling as it adds scroll
    // and negates relative screen pos
    Vector2D contentStart
        = Vector2D(ImGui::GetCursorScreenPos()) + Vector2D(ImGui::GetWindowPos()) + offset;

    Vector2D size = max - min;
    Vector2D a = MathGeom::transform2d(min, contentStart, rotInDeg);
    Vector2D b = MathGeom::transform2d(min + Vector2D(size.x(), 0), contentStart, rotInDeg);
    Vector2D c = MathGeom::transform2d(min + Vector2D(size.x(), size.y()), contentStart, rotInDeg);
    Vector2D d = MathGeom::transform2d(min + Vector2D(0, size.y()), contentStart, rotInDeg);

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
bool ImGuiDrawInterface::inputText(const char *label, String *str, ImGuiInputTextFlags flags,
    ImGuiInputTextCallback callback /*= nullptr*/, void *user_data /*= nullptr*/)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.str = str;
    cb_user_data.chainCallback = callback;
    cb_user_data.chainCallbackUserData = user_data;
    return ImGui::InputText(
        label, (char *)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
}

bool ImGuiDrawInterface::inputTextMultiline(const char *label, String *str, const ImVec2 &size,
    ImGuiInputTextFlags flags, ImGuiInputTextCallback callback /*= nullptr*/,
    void *user_data /*= nullptr*/)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.str = str;
    cb_user_data.chainCallback = callback;
    cb_user_data.chainCallbackUserData = user_data;
    return ImGui::InputTextMultiline(
        label, (char *)str->c_str(), str->capacity() + 1, size, flags, InputTextCallback, &cb_user_data);
}

bool ImGuiDrawInterface::inputTextWithHint(const char *label, const char *hint, String *str,
    ImGuiInputTextFlags flags, ImGuiInputTextCallback callback /*= nullptr*/,
    void *user_data /*= nullptr*/)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.str = str;
    cb_user_data.chainCallback = callback;
    cb_user_data.chainCallbackUserData = user_data;
    return ImGui::InputTextWithHint(
        label, hint, (char *)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
}
