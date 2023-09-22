/*!
 * \file ImGuiManager.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/ImGui/ImGuiManager.h"
#include "Widgets/ImGui/IImGuiLayer.h"
#include "Widgets/ImGui/ImGuiLib/imgui.h"
#include "Widgets/ImGui/ImGuiLib/implot.h"
#include "Widgets/WidgetWindow.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/PlatformInputTypes.h"
#include "Types/Platform/LFS/Paths.h"
#include "Types/Platform/LFS/File/FileHelper.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "IRenderInterfaceModule.h"
#include "RenderApi/RenderManager.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderApi/ResourcesInterface/IRenderResource.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"

using namespace ImGui;

static_assert(std::is_same_v<uint32, ImGuiID>, "ImGuiID and uint32 type do not match!");
static_assert(std::is_same_v<int32, ImGuiWindowFlags>, "ImGuiWindowFlags and int32 type do not match!");
static_assert(std::is_convertible_v<ImGuiKey, int32>, "ImGuiKey and int32 type do not match!");

const int32 ImGuiManager::SIMPLE_READONLY_WINDOWFLAGS
    = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

int32 ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[512] = {};
void setupImGuiNamedKeyMapping()
{
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::BACKSPACE.keyCode] = ImGuiKey_Backspace;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::TAB.keyCode] = ImGuiKey_Tab;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::CAPS.keyCode] = ImGuiKey_CapsLock;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::ESC.keyCode] = ImGuiKey_Escape;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::ENTER.keyCode] = ImGuiKey_Enter;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::SPACE.keyCode] = ImGuiKey_Space;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::PAGEUP.keyCode] = ImGuiKey_PageUp;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::PAGEDOWN.keyCode] = ImGuiKey_PageDown;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::END.keyCode] = ImGuiKey_End;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::HOME.keyCode] = ImGuiKey_Home;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::LEFT.keyCode] = ImGuiKey_LeftArrow;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::UP.keyCode] = ImGuiKey_UpArrow;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::RIGHT.keyCode] = ImGuiKey_RightArrow;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::DOWN.keyCode] = ImGuiKey_DownArrow;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::INS.keyCode] = ImGuiKey_Insert;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::DEL.keyCode] = ImGuiKey_Delete; // Delete
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::ZERO.keyCode] = ImGuiKey_0;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::ONE.keyCode] = ImGuiKey_1;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::TWO.keyCode] = ImGuiKey_2;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::THREE.keyCode] = ImGuiKey_3;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::FOUR.keyCode] = ImGuiKey_4;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::FIVE.keyCode] = ImGuiKey_5;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::SIX.keyCode] = ImGuiKey_6;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::SEVEN.keyCode] = ImGuiKey_7;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::EIGHT.keyCode] = ImGuiKey_8;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NINE.keyCode] = ImGuiKey_9;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::A.keyCode] = ImGuiKey_A;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::B.keyCode] = ImGuiKey_B;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::C.keyCode] = ImGuiKey_C;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::D.keyCode] = ImGuiKey_D;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::E.keyCode] = ImGuiKey_E;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F.keyCode] = ImGuiKey_F;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::G.keyCode] = ImGuiKey_G;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::H.keyCode] = ImGuiKey_H;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::I.keyCode] = ImGuiKey_I;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::J.keyCode] = ImGuiKey_J;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::K.keyCode] = ImGuiKey_K;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::L.keyCode] = ImGuiKey_L;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::M.keyCode] = ImGuiKey_M;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::N.keyCode] = ImGuiKey_N;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::O.keyCode] = ImGuiKey_O;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::P.keyCode] = ImGuiKey_P;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::Q.keyCode] = ImGuiKey_Q;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::R.keyCode] = ImGuiKey_R;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::S.keyCode] = ImGuiKey_S;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::T.keyCode] = ImGuiKey_T;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::U.keyCode] = ImGuiKey_U;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::V.keyCode] = ImGuiKey_V;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::W.keyCode] = ImGuiKey_W;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::X.keyCode] = ImGuiKey_X;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::Y.keyCode] = ImGuiKey_Y;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::Z.keyCode] = ImGuiKey_Z;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUM0.keyCode] = ImGuiKey_Keypad0;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUM1.keyCode] = ImGuiKey_Keypad1;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUM2.keyCode] = ImGuiKey_Keypad2;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUM3.keyCode] = ImGuiKey_Keypad3;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUM4.keyCode] = ImGuiKey_Keypad4;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUM5.keyCode] = ImGuiKey_Keypad5;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUM6.keyCode] = ImGuiKey_Keypad6;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUM7.keyCode] = ImGuiKey_Keypad7;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUM8.keyCode] = ImGuiKey_Keypad8;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUM9.keyCode] = ImGuiKey_Keypad9;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::ASTERICK.keyCode] = ImGuiKey_KeypadMultiply;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::PLUS.keyCode] = ImGuiKey_KeypadAdd;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUMMINUS.keyCode] = ImGuiKey_KeypadSubtract;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUMFULLSTOP.keyCode] = ImGuiKey_KeypadDecimal;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUMFWDSLASH.keyCode] = ImGuiKey_KeypadDivide;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F1.keyCode] = ImGuiKey_F1;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F2.keyCode] = ImGuiKey_F2;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F3.keyCode] = ImGuiKey_F3;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F4.keyCode] = ImGuiKey_F4;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F5.keyCode] = ImGuiKey_F5;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F6.keyCode] = ImGuiKey_F6;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F7.keyCode] = ImGuiKey_F7;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F8.keyCode] = ImGuiKey_F8;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F9.keyCode] = ImGuiKey_F9;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F10.keyCode] = ImGuiKey_F10;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F11.keyCode] = ImGuiKey_F11;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F12.keyCode] = ImGuiKey_F12;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::LWIN.keyCode] = ImGuiKey_LeftSuper;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::RWIN.keyCode] = ImGuiKey_RightSuper;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::MENU.keyCode] = ImGuiKey_Menu;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F16.keyCode] = ImGuiKey_None;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F17.keyCode] = ImGuiKey_None;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F18.keyCode] = ImGuiKey_None;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F19.keyCode] = ImGuiKey_None;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F20.keyCode] = ImGuiKey_None;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F21.keyCode] = ImGuiKey_None;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F22.keyCode] = ImGuiKey_None;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F23.keyCode] = ImGuiKey_None;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::F24.keyCode] = ImGuiKey_None;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUMLOCK.keyCode] = ImGuiKey_NumLock;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::SCRLLOCK.keyCode] = ImGuiKey_ScrollLock;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::PAUSE.keyCode] = ImGuiKey_Pause;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::LSHIFT.keyCode] = ImGuiKey_LeftShift;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::RSHIFT.keyCode] = ImGuiKey_RightShift;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::LCTRL.keyCode] = ImGuiKey_LeftCtrl;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::RCTRL.keyCode] = ImGuiKey_RightCtrl;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::LALT.keyCode] = ImGuiKey_LeftAlt;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::RALT.keyCode] = ImGuiKey_RightAlt;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::SEMICOLON.keyCode] = ImGuiKey_Semicolon;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::COMMA.keyCode] = ImGuiKey_Comma;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::FULLSTOP.keyCode] = ImGuiKey_Period;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::FWDSLASH.keyCode] = ImGuiKey_Slash;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::MINUS.keyCode] = ImGuiKey_Minus;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::BACKTICK.keyCode] = ImGuiKey_GraveAccent;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::OPENSQR.keyCode] = ImGuiKey_LeftBracket;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::CLOSESQR.keyCode] = ImGuiKey_RightBracket;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::BACKSLASH.keyCode] = ImGuiKey_Backslash;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::APOSTROPHE.keyCode] = ImGuiKey_Apostrophe;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::PA1.keyCode] = ImGuiKey_None;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::CLR.keyCode] = ImGuiKey_None;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::LEFTBACKSLASH.keyCode] = ImGuiKey_Backslash;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::NUMENTER.keyCode] = ImGuiKey_KeypadEnter;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::EQUAL.keyCode] = ImGuiKey_Equal;
    ImGuiManager::APPKEYS_TO_IMGUI_NAMEDKEYS[Keys::FWDDEL.keyCode] = ImGuiKey_None;
}

const StringID ImGuiManager::TEXTURE_PARAM_NAME{ TCHAR("textureAtlas") };
const NameString ImGuiManager::IMGUI_SHADER_NAME{ TCHAR("DrawImGui") };

ImGuiManager::ImGuiManager(const TChar *managerName, ImGuiManager *parent)
    : bCaptureInput(false)
    , parentGuiManager(parent)
    , context(nullptr)
    , implotContext(nullptr)
    , name(TCHAR_TO_UTF8(managerName))
{}

ImGuiManager::ImGuiManager(const TChar *managerName)
    : bCaptureInput(false)
    , parentGuiManager(nullptr)
    , context(nullptr)
    , implotContext(nullptr)
    , name(TCHAR_TO_UTF8(managerName))
{}

void ImGuiManager::initialize(ImGuiManagerOptions opts)
{
    debugAssertf(!opts.bEnableViewport, "Viewport option is not supported!");
    IMGUI_CHECKVERSION();
    CALL_ONCE(setupImGuiNamedKeyMapping);

    if (parentGuiManager)
    {
        parentGuiManager->setCurrentContext();
        context = CreateContext(GetIO().Fonts);
    }
    else
    {
        context = CreateContext();
    }
    implotContext = ImPlot::CreateContext();
    setCurrentContext();

    ImGuiIO &io = GetIO();
    io.BackendPlatformName = name.c_str();
    io.IniFilename = nullptr;
    io.ConfigFlags = (opts.bEnableDocking ? ImGuiConfigFlags_DockingEnable : 0);
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 2;
    // fontConfig.GlyphExtraSpacing = ImVec2(1, 1);
    fontConfig.RasterizerMultiply = 1.5f;
    // io.Fonts->AddFontDefault(&fontConfig);

    String fontPath = PathFunctions::combinePath(Paths::engineRuntimeRoot(), TCHAR("Assets/Fonts/CascadiaMono-Regular.ttf"));
    if (FileSystemFunctions::fileExists(fontPath.getChar()))
    {
        io.Fonts->AddFontFromFileTTF(TCHAR_TO_UTF8(fontPath.getChar()), 14.0f, &fontConfig);
    }
    else
    {
        LOG_ERROR("ImGui", "Cannot find font file at {}", fontPath);
        fontConfig.OversampleH = 3;
        fontConfig.OversampleV = 3;
        fontConfig.RasterizerMultiply = 2;
        io.Fonts->AddFontDefault(&fontConfig);
    }

    // https://github.com/google/material-design-icons/blob/master/font/MaterialIcons-Regular.codepoints
    // fontConfig.MergeMode = true;
    // fontConfig.GlyphMinAdvanceX = 13.0f;
    // fontConfig.GlyphOffset = ImVec2(0, 2);
    // Captures the pointer so Range must be alive when building the font itself
    // static const ImWchar fontRange[] = { 0xe000, 0xf8ff, 0 };
    // io.Fonts->AddFontFromFileTTF(TCHAR_TO_UTF8(
    //    PathFunctions::combinePath(Paths::engineRuntimeRoot(), TCHAR("Assets/Fonts/MaterialIcons-Regular.ttf")).getChar()
    //    ),
    //    13.0f, &fontConfig, fontRange);

    // Setup Dear ImGui style
    StyleColorsDark();
    GetStyle().AntiAliasedLines = false;
    GetStyle().AntiAliasedFill = true;
    GetStyle().AntiAliasedLinesUseTex = true;

    GetStyle().WindowRounding = 1.0f;
    GetStyle().ChildRounding = 0.75f;
    GetStyle().FrameRounding = 0.75f;
    GetStyle().ScrollbarRounding = 1.0f;
    GetStyle().GrabRounding = 1.0f;
    GetStyle().TabRounding = 1.0f;

    setupInputs();
    setupRendering();
}

void ImGuiManager::release()
{
    releaseRendering();
    ImPlot::DestroyContext(implotContext);
    DestroyContext(context);
}

void ImGuiManager::setClipboard(void * /*userData*/, const char *text) { PlatformFunctions::setClipboard(UTF8_TO_TCHAR(text)); }

const char *ImGuiManager::getClipboard(void *userData)
{
    reinterpret_cast<ImGuiManager *>(userData)->clipboard = TCHAR_TO_UTF8(PlatformFunctions::getClipboard().getChar());
    return reinterpret_cast<ImGuiManager *>(userData)->clipboard.c_str();
}

void ImGuiManager::setShaderData()
{
    ImDrawData *drawData = ImGui::GetDrawData();
    if (drawData && drawData->Valid && imguiTransformParams.isValid())
    {
        Vector2 scale = 2.0f / Vector2(drawData->DisplaySize);
        Vector2 translate = -1.0f - Vector2(drawData->DisplayPos) * scale;
        imguiTransformParams->setVector2Param(TCHAR("scale"), scale);
        imguiTransformParams->setVector2Param(TCHAR("translate"), translate);
    }
}

void ImGuiManager::recreateFontAtlas(
    class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper
)
{
    debugAssert(context == ImGui::GetCurrentContext() && parentGuiManager == nullptr);
    ImFontAtlas *fontAtlas = ImGui::GetIO().Fonts;
    uint8 *alphaVals;
    int32 textureSizeX, textureSizeY;
    fontAtlas->GetTexDataAsAlpha8(&alphaVals, &textureSizeX, &textureSizeY);
    std::vector<Color> rawData(textureSizeX * textureSizeY, ColorConst::BLACK);
    for (int32 i = 0; i < rawData.size(); ++i)
    {
        rawData[i].setR(alphaVals[i]);
    }

    ImageResourceCreateInfo imageCI{ .imageFormat = EPixelDataFormat::R_U8_Norm,
                                     .dimensions = UInt3(textureSizeX, textureSizeY, 1),
                                     .numOfMips = 1 };
    textureAtlas = graphicsHelper->createImage(graphicsInstance, imageCI);
    textureAtlas->setResourceName(UTF8_TO_TCHAR((name + "FontAtlas").c_str()));
    textureAtlas->setShaderUsage(EImageShaderUsage::Sampling);
    textureAtlas->setSampleCounts(EPixelSampleCount::SampleCount1);
    textureAtlas->init();
    cmdList->copyToImage(textureAtlas, rawData);
}

void ImGuiManager::setCurrentContext()
{
    SetCurrentContext(context);
    ImPlot::SetCurrentContext(implotContext);
}

ImageResourceRef ImGuiManager::getFontTextureAtlas() const { return parentGuiManager ? parentGuiManager->getFontTextureAtlas() : textureAtlas; }

ShaderParametersRef ImGuiManager::getFontAtlasParam() const
{
    return parentGuiManager ? parentGuiManager->getFontAtlasParam() : imguiFontAtlasParams;
}

ShaderParametersRef ImGuiManager::getTextureParam(ImageResourceRef textureUsed)
{
    if (parentGuiManager)
    {
        return parentGuiManager->getTextureParam(textureUsed);
    }
    else
    {
        auto itr = textureParams.find(textureUsed);
        if (itr != textureParams.end())
        {
            activeTextureParams.insert(itr->second);
            return itr->second;
        }
        return nullptr;
    }
}

ShaderParametersRef ImGuiManager::createTextureParam(
    ImageResourceRef texture, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper,
    const class LocalPipelineContext &pipelineContext
)
{
    if (parentGuiManager)
    {
        return parentGuiManager->createTextureParam(texture, graphicsInstance, graphicsHelper, pipelineContext);
    }
    else
    {
        ShaderParametersRef params
            = graphicsHelper->createShaderParameters(graphicsInstance, pipelineContext.getPipeline()->getParamLayoutAtSet(0), { 0 });
        params->setTextureParam(TEXTURE_PARAM_NAME, texture, GlobalBuffers::linearSampler());
        params->setResourceName(UTF8_TO_TCHAR(name.c_str()) + String(TCHAR("_")) + texture->getResourceName());
        params->init();

        textureParams[texture] = params;

        return params;
    }
}

ShaderParametersRef ImGuiManager::findFreeTextureParam(ImageResourceRef textureUsed)
{
    if (!freeTextureParams.empty())
    {
        ShaderParametersRef retVal = freeTextureParams.front();
        textureParams[textureUsed] = retVal;
        freeTextureParams.pop();
        retVal->setTextureParam(TEXTURE_PARAM_NAME, textureUsed, GlobalBuffers::linearSampler());
        return retVal;
    }
    return nullptr;
}

void ImGuiManager::setupInputs()
{
    ImGuiIO &io = GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    io.ClipboardUserData = this;
    io.GetClipboardTextFn = &ImGuiManager::getClipboard;
    io.SetClipboardTextFn = &ImGuiManager::setClipboard;

    bCaptureInput = false;
}

void ImGuiManager::updateTextureParameters()
{
    // In parent GUI manager
    if (!parentGuiManager)
    {
        for (auto itr = textureParams.cbegin(); itr != textureParams.cend();)
        {
            if (activeTextureParams.find(itr->second) == activeTextureParams.cend())
            {
                // FIXME(Jeslas) : If user action leads to this parameter being reset faster than cmd buffer ends this will trigger shader param
                // update before used in cmd finished
                itr->second->setTextureParam(TEXTURE_PARAM_NAME, GlobalBuffers::dummyWhite2D());
                freeTextureParams.push(itr->second);
                itr = textureParams.erase(itr);
            }
            else
            {
                ++itr;
            }
        }
        activeTextureParams.clear();
    }

    ImDrawData *drawData = ImGui::GetDrawData();

    // Update used texture resources
    if (drawData && drawData->Valid)
    {
        texturesUsed.clear();
        for (int32 cmdListIdx = 0; cmdListIdx < drawData->CmdListsCount; ++cmdListIdx)
        {
            const ImDrawList *uiCmdList = drawData->CmdLists[cmdListIdx];
            for (int32 cmdIdx = 0; cmdIdx < uiCmdList->CmdBuffer.Size; ++cmdIdx)
            {
                const ImDrawCmd &drawCmd = uiCmdList->CmdBuffer[cmdIdx];
                if (drawCmd.TextureId)
                {
                    ShaderParametersRef perDrawTexture = getTextureParam(static_cast<ImageResource *>(drawCmd.TextureId));
                    if (perDrawTexture.isValid())
                    {
                        texturesUsed.insert(perDrawTexture.reference());
                    }
                    else
                    {
                        perDrawTexture = findFreeTextureParam(static_cast<ImageResource *>(drawCmd.TextureId));
                        if (perDrawTexture.isValid())
                        {
                            texturesUsed.insert(perDrawTexture.reference());
                        }
                        else
                        {
                            texturesToCreate.insert(static_cast<ImageResource *>(drawCmd.TextureId));
                        }
                    }
                }
            }
        }
    }
}

void ImGuiManager::updateRenderResources(
    class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper,
    const class LocalPipelineContext &pipelineContext
)
{
    ImDrawData *drawData = ImGui::GetDrawData();
    // Setting up vertex and index buffers
    {
        if (!vertexBuffer.isValid() || vertexBuffer->bufferCount() < uint32(drawData->TotalVtxCount))
        {
            vertexBuffer = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, int32(sizeof(ImDrawVert)), drawData->TotalVtxCount);
            vertexBuffer->setAsStagingResource(true);
            vertexBuffer->setResourceName(UTF8_TO_TCHAR((name + "Vertices").c_str()));
            vertexBuffer->init();
        }
        if (!idxBuffer.isValid() || idxBuffer->bufferCount() < uint32(drawData->TotalIdxCount))
        {
            idxBuffer = graphicsHelper->createReadOnlyIndexBuffer(graphicsInstance, int32(sizeof(ImDrawIdx)), drawData->TotalIdxCount);
            idxBuffer->setAsStagingResource(true);
            idxBuffer->setResourceName(UTF8_TO_TCHAR((name + "Indices").c_str()));
            idxBuffer->init();
        }
        std::vector<BatchCopyBufferData> bufferCopies;
        uint32 vertOffset = 0;
        uint32 idxOffset = 0;
        for (int32 n = 0; n < drawData->CmdListsCount; ++n)
        {
            const ImDrawList *drawCmdList = drawData->CmdLists[n];
            BatchCopyBufferData &vertCpy = bufferCopies.emplace_back();
            vertCpy.dst = vertexBuffer;
            vertCpy.dstOffset = vertOffset;
            vertCpy.dataToCopy = drawCmdList->VtxBuffer.Data;
            vertCpy.size = drawCmdList->VtxBuffer.Size * vertexBuffer->bufferStride();
            vertOffset += vertCpy.size;

            BatchCopyBufferData &idxCpy = bufferCopies.emplace_back();
            idxCpy.dst = idxBuffer;
            idxCpy.dstOffset = idxOffset;
            idxCpy.dataToCopy = drawCmdList->IdxBuffer.Data;
            idxCpy.size = drawCmdList->IdxBuffer.Size * idxBuffer->bufferStride();
            idxOffset += idxCpy.size;
        }
        cmdList->copyToBuffer(bufferCopies);
    }

    // only in parent GUI
    if (parentGuiManager == nullptr)
    {
        if (!getFontTextureAtlas().isValid())
        {
            recreateFontAtlas(cmdList, graphicsInstance, graphicsHelper);
        }
        if (!getFontAtlasParam().isValid())
        {
            // As 0 contains all sets in utility shader, ignore 0 as it is unique to each GUI manager
            imguiFontAtlasParams
                = graphicsHelper->createShaderParameters(graphicsInstance, pipelineContext.getPipeline()->getParamLayoutAtSet(0), { 0 });
            ImageViewInfo viewInfo;
            viewInfo.componentMapping.g = EPixelComponentMapping::R;
            viewInfo.componentMapping.b = EPixelComponentMapping::R;
            viewInfo.componentMapping.a = EPixelComponentMapping::R;
            imguiFontAtlasParams->setTextureParam(TEXTURE_PARAM_NAME, getFontTextureAtlas(), GlobalBuffers::linearSampler());
            imguiFontAtlasParams->setTextureParamViewInfo(TEXTURE_PARAM_NAME, viewInfo);
            imguiFontAtlasParams->setResourceName(UTF8_TO_TCHAR((name + "Desc_").c_str()) + getFontTextureAtlas()->getResourceName());
            imguiFontAtlasParams->init();
        }
    }
    if (!imguiTransformParams.isValid())
    {
        imguiTransformParams
            = graphicsHelper->createShaderParameters(graphicsInstance, pipelineContext.getPipeline()->getParamLayoutAtSet(0), { 1 });
        imguiTransformParams->setResourceName(UTF8_TO_TCHAR((name + "_TX").c_str()));
        setShaderData();
        imguiTransformParams->init();
    }
    // Create necessary texture parameters
    for (const ImageResourceRef &texture : texturesToCreate)
    {
        texturesUsed.insert(createTextureParam(texture, graphicsInstance, graphicsHelper, pipelineContext).reference());
    }
    texturesToCreate.clear();
}

void ImGuiManager::setupRendering()
{
    ImGuiIO &io = GetIO();
    // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    // texture atlas can be used from parent
    if (parentGuiManager)
    {
        textureAtlas = nullptr;
    }
    else
    {
        ENQUEUE_RENDER_COMMAND(SetupImGui)
        (
            [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
            {
                recreateFontAtlas(cmdList, graphicsInstance, graphicsHelper);
            }
        );
    }
}

void ImGuiManager::releaseRendering()
{
    ENQUEUE_RENDER_COMMAND(ReleaseImGui)
    (
        [this](class IRenderCommandList * /*cmdList*/, IGraphicsInstance * /*graphicsInstance*/, const GraphicsHelperAPI * /*graphicsHelper*/)
        {
            if (textureAtlas.isValid())
            {
                textureAtlas.reset();
            }
            if (imguiFontAtlasParams.isValid())
            {
                imguiFontAtlasParams.reset();
            }
            if (imguiTransformParams.isValid())
            {
                imguiTransformParams.reset();
            }
            vertexBuffer.reset();
            idxBuffer.reset();

            if (!parentGuiManager)
            {
                textureParams.clear();
                while (!freeTextureParams.empty())
                {
                    freeTextureParams.pop();
                }
                activeTextureParams.clear();
            }
        }
    );
}

void ImGuiManager::draw(
    class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper,
    const ImGuiDrawingContext &drawingContext
)
{
    setCurrentContext();

    ImDrawData *drawData = ImGui::GetDrawData();
    if (!drawData || !drawingContext.rtTexture || drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
    {
        return;
    }
    // If not doing fresh draw(ie clearing) we do not even have to start the render pass and other update tasks if there is nothing to draw
    if (!drawingContext.bClearRt && drawData->CmdListsCount == 0)
    {
        return;
    }

    SCOPED_CMD_MARKER(cmdList, drawingContext.cmdBuffer, DrawImGui);
    LocalPipelineContext pipelineContext;
    pipelineContext.materialName = IMGUI_SHADER_NAME;
    pipelineContext.forVertexType = EVertexType::UI;

    const IRenderTargetTexture *rtPtr = drawingContext.rtTexture;
    IRenderInterfaceModule::get()->getRenderManager()->preparePipelineContext(&pipelineContext, { &rtPtr, 1 });

    updateRenderResources(cmdList, graphicsInstance, graphicsHelper, pipelineContext);

    //////////////////////////////////////////////////////////////////////////
    /// Drawing
    //////////////////////////////////////////////////////////////////////////

    ImageResource *rtTexture = static_cast<ImageResource *>(drawingContext.rtTexture->renderTargetResource().get());
    IRect viewport = drawingContext.viewport;
    if (!viewport.isValidAABB())
    {
        viewport.minBound = Int2(0, 0);
        // doing like this because even if ImGui size is different from framebuffer we can still draw
        viewport.maxBound = Int2(rtTexture->getImageSize().x, rtTexture->getImageSize().y);
    }

    Vector2 uiToFbDispScale = Vector2(float(viewport.maxBound.x), float(viewport.maxBound.y)) / Vector2(drawData->DisplaySize);

    // Render UI
    RenderPassAdditionalProps additionalProps;
    additionalProps.bAllowUndefinedLayout = drawingContext.bClearRt;
    additionalProps.colorAttachmentLoadOp = additionalProps.depthLoadOp = additionalProps.stencilLoadOp
        = drawingContext.bClearRt ? EAttachmentOp::LoadOp::Clear : EAttachmentOp::LoadOp::Load;

    RenderPassClearValue clearVal;
    clearVal.colors = { LinearColorConst::BLACK_Transparent, LinearColorConst::BLACK_Transparent };

    // Barrier resources once
    std::vector<ShaderParametersRef> texturesUsedVector(texturesUsed.cbegin(), texturesUsed.cend());
    texturesUsed.clear();
    cmdList->cmdBarrierResources(drawingContext.cmdBuffer, texturesUsedVector);

    cmdList->cmdBeginRenderPass(drawingContext.cmdBuffer, pipelineContext, viewport, additionalProps, clearVal);
    {
        GraphicsPipelineQueryParams query;
        query.cullingMode = ECullingMode::BackFace;
        query.drawMode = EPolygonDrawMode::Fill;
        cmdList->cmdBindGraphicsPipeline(drawingContext.cmdBuffer, pipelineContext, { query });
        cmdList->cmdBindDescriptorsSets(drawingContext.cmdBuffer, pipelineContext, imguiTransformParams.reference());
        if (vertexBuffer->bufferCount() > 0 && idxBuffer->bufferCount() > 0)
        {
            cmdList->cmdBindVertexBuffer(drawingContext.cmdBuffer, 0, vertexBuffer, 0);
            cmdList->cmdBindIndexBuffer(drawingContext.cmdBuffer, idxBuffer);
        }

        int32 vertOffset = 0;
        uint32 idxOffset = 0;
        for (int32 cmdListIdx = 0; cmdListIdx < drawData->CmdListsCount; ++cmdListIdx)
        {
            const ImDrawList *uiCmdList = drawData->CmdLists[cmdListIdx];
            for (int32 cmdIdx = 0; cmdIdx < uiCmdList->CmdBuffer.Size; ++cmdIdx)
            {
                const ImDrawCmd &drawCmd = uiCmdList->CmdBuffer[cmdIdx];
                if (drawCmd.UserCallback != nullptr)
                {
                    LOG_WARN("ImGui", "Commands with callback is not supported");
                    debugAssert(drawCmd.UserCallback != nullptr);
                    continue;
                }
                // All vertex, clip data is in display texel coordinates + Display pos(in case of
                // multi monitor setup)
                IRect scissor(
                    /*.minBound = */ Int2(
                        int32((drawCmd.ClipRect.x - drawData->DisplayPos.x) * uiToFbDispScale.x()),
                        int32((drawCmd.ClipRect.y - drawData->DisplayPos.y) * uiToFbDispScale.y())
                    ),
                    /*.maxBound = */ Int2(
                        int32((drawCmd.ClipRect.z - drawData->DisplayPos.x) * uiToFbDispScale.x()),
                        int32((drawCmd.ClipRect.w - drawData->DisplayPos.y) * uiToFbDispScale.y())
                    )
                );
                if (scissor.intersect(viewport))
                {
                    scissor = scissor.getIntersectionBox(viewport, false);

                    ShaderParametersRef perDrawTexture = getFontAtlasParam();
                    if (drawCmd.TextureId)
                    {
                        perDrawTexture = getTextureParam(static_cast<ImageResource *>(drawCmd.TextureId));
                        fatalAssertf(perDrawTexture.isValid(), "Failed getting texture parameters for imgui");
                    }
                    cmdList->cmdBindDescriptorsSets(drawingContext.cmdBuffer, pipelineContext, perDrawTexture.reference());
                    cmdList->cmdSetViewportAndScissor(drawingContext.cmdBuffer, viewport, scissor);
                    cmdList->cmdDrawIndexed(
                        drawingContext.cmdBuffer, idxOffset + drawCmd.IdxOffset, drawCmd.ElemCount, 0, 1, vertOffset + drawCmd.VtxOffset
                    );
                }
            }
            vertOffset += uiCmdList->VtxBuffer.Size;
            idxOffset += uiCmdList->IdxBuffer.Size;
        }
    }
    cmdList->cmdEndRenderPass(drawingContext.cmdBuffer);
}

void ImGuiManager::updateFrame(float deltaTime)
{
    setCurrentContext();
    ImGuiIO &io = GetIO();
    io.DeltaTime = deltaTime;
    bCaptureInput = io.WantCaptureKeyboard || io.WantCaptureMouse;
    if (io.DisplaySize.x <= 1.0f || io.DisplaySize.y <= 1.0f)
    {
        return;
    }

    ImGui::NewFrame();
    for (std::pair<const int32, std::vector<SharedPtr<IImGuiLayer>>> &imGuiLayers : drawLayers)
    {
        for (SharedPtr<IImGuiLayer> &layer : imGuiLayers.second)
        {
            layer->draw(&drawInterface);
        }
    }
    ImGui::Render();

    updateTextureParameters();
    setShaderData();
}

void ImGuiManager::setDisplaySize(Short2 newSize)
{
    setCurrentContext();
    GetIO().DisplaySize = ImVec2(newSize.x, newSize.y);
}

void ImGuiManager::addFont(const String &fontAssetPath, float fontSize)
{
    if (parentGuiManager)
    {
        parentGuiManager->addFont(fontAssetPath, fontSize);
    }
    else
    {
        setCurrentContext();

        // TODO(Jeslas) : Load from asset manager
        std::vector<uint8> fontData;
        FileHelper::readBytes(fontData, fontAssetPath);
        GetIO().Fonts->AddFontFromMemoryTTF(fontData.data(), int32(fontData.size()), fontSize);

        getFontTextureAtlas().reset();
        imguiFontAtlasParams.reset();
    }
}

void ImGuiManager::addLayer(SharedPtr<IImGuiLayer> layer)
{
    std::vector<SharedPtr<IImGuiLayer>> &layers = drawLayers[layer->layerDepth()];
    auto itr = std::find(layers.cbegin(), layers.cend(), layer);

    if (itr == layers.cend())
    {
        layers.emplace_back(layer);
        std::sort(
            layers.begin(), layers.end(),
            [](const SharedPtr<IImGuiLayer> &lhs, const SharedPtr<IImGuiLayer> &rhs) -> bool
            {
                return lhs->sublayerDepth() < rhs->sublayerDepth();
            }
        );
    }
}

void ImGuiManager::removeLayer(SharedPtr<IImGuiLayer> layer)
{
    std::vector<SharedPtr<IImGuiLayer>> &layers = drawLayers[layer->layerDepth()];
    auto itr = std::find(layers.cbegin(), layers.cend(), layer);
    if (itr != layers.cend())
    {
        // no need to sort
        layers.erase(itr);
    }
}

bool ImGuiManager::inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem)
{
    setCurrentContext();
    ImGuiIO &io = GetIO();

    if (Keys::isMouseKey(key->keyCode))
    {
        io.AddMouseButtonEvent(key->keyCode - Keys::LMB.keyCode, state.isPressed);
    }
    else
    {
        io.AddKeyEvent((ImGuiKey)APPKEYS_TO_IMGUI_NAMEDKEYS[key->keyCode], state.isPressed);

        Utf32 keyChar = inputSystem->keyChar(*key);
        if (state.keyWentDown && keyChar != 0)
        {
            io.AddInputCharacter(keyChar);
        }

        switch (key->keyCode)
        {
        case EKeyCode::KEY_LCTRL:
        case EKeyCode::KEY_RCTRL:
            io.AddKeyEvent(ImGuiMod_Ctrl, state.isPressed);
            break;
        case EKeyCode::KEY_LSHIFT:
        case EKeyCode::KEY_RSHIFT:
            io.AddKeyEvent(ImGuiMod_Shift, state.isPressed);
            break;
        case EKeyCode::KEY_LALT:
        case EKeyCode::KEY_RALT:
            io.AddKeyEvent(ImGuiMod_Alt, state.isPressed);
            break;
        case EKeyCode::KEY_LWIN:
        case EKeyCode::KEY_RWIN:
            io.AddKeyEvent(ImGuiMod_Super, state.isPressed);
            break;
        default:
            break;
        };
    }
    return bCaptureInput;
}

bool ImGuiManager::analogKey(AnalogStates::StateKeyType key, AnalogStates::StateInfoType state, const InputSystem * /*inputSystem*/)
{
    setCurrentContext();
    ImGuiIO &io = GetIO();

    switch (key)
    {
    case AnalogStates::ScrollWheelX:
        // It is for some reason inverted in ImGui -1 means to right, +1 means to left
        io.AddMouseWheelEvent(-state.currentValue, 0.0f);
        return true;
        break;
    case AnalogStates::ScrollWheelY:
        io.AddMouseWheelEvent(0.0f, state.currentValue);
        return true;
        break;
    default:
        break;
    }
    return io.WantCaptureMouse;
}

void ImGuiManager::updateMouse(Short2 /*absPos*/, Short2 widgetRelPos, const InputSystem * /*inputSystem*/)
{
    setCurrentContext();
    ImGuiIO &io = GetIO();

    io.AddMousePosEvent(widgetRelPos.x, widgetRelPos.y);
}

void ImGuiManager::mouseEnter(Short2 absPos, Short2 widgetRelPos, const InputSystem *inputSystem)
{
    updateMouse(absPos, widgetRelPos, inputSystem);
}

void ImGuiManager::mouseMoved(Short2 absPos, Short2 widgetRelPos, const InputSystem *inputSystem)
{
    updateMouse(absPos, widgetRelPos, inputSystem);
}

void ImGuiManager::mouseLeave(Short2 absPos, Short2 widgetRelPos, const InputSystem *inputSystem)
{
    updateMouse(absPos, widgetRelPos, inputSystem);
}
