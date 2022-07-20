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
#include "ApplicationInstance.h"
#include "RenderApi/RenderManager.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderApi/ResourcesInterface/IRenderResource.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"

using namespace ImGui;

const StringID ImGuiManager::TEXTURE_PARAM_NAME{ TCHAR("textureAtlas") };
const NameString ImGuiManager::IMGUI_SHADER_NAME{ TCHAR("DrawImGui") };

ImGuiManager::ImGuiManager(const TChar *managerName, ImGuiManager *parent, SharedPtr<WidgetBase> inWidget)
    : parentGuiManager(parent)
    , wgWindow(WidgetBase::findWidgetParentWindow(inWidget))
    , name(TCHAR_TO_UTF8(managerName))
    , widget(inWidget)
{}

ImGuiManager::ImGuiManager(const TChar *managerName, SharedPtr<WidgetBase> inWidget)
    : parentGuiManager(nullptr)
    , wgWindow(WidgetBase::findWidgetParentWindow(inWidget))
    , name(TCHAR_TO_UTF8(managerName))
    , widget(inWidget)
{}

void ImGuiManager::initialize()
{
    fatalAssertf(getWindowWidget() && widget, "Invalid widgets that contains ImGui");

    IMGUI_CHECKVERSION();
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
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 3;
    fontConfig.OversampleV = 3;
    fontConfig.GlyphExtraSpacing = ImVec2(1, 1);
    fontConfig.RasterizerMultiply = 2.0f;
    io.Fonts->AddFontDefault(&fontConfig);

    // fontConfig.OversampleH = 2;
    // fontConfig.OversampleV = 2;
    // fontConfig.GlyphExtraSpacing = ImVec2(1, 1);
    // fontConfig.RasterizerMultiply = 1.25f;
    // io.Fonts->AddFontFromFileTTF("D:/Workspace/VisualStudio/Cranberry/Build/Debug/Assets/Fonts/CascadiaMono-Bold.ttf"
    //     , 13.0f, &fontConfig);

    // Setup Dear ImGui style
    StyleColorsDark();
    GetStyle().AntiAliasedLines = false;
    GetStyle().WindowRounding = 0.15f;
    GetStyle().AntiAliasedFill = true;
    GetStyle().AntiAliasedLinesUseTex = true;

    setupInputs();
    setupRendering();
}

void ImGuiManager::release()
{
    releaseRendering();
    ImPlot::DestroyContext(implotContext);
    DestroyContext(context);
}

void ImGuiManager::setClipboard(void *userData, const char *text) { PlatformFunctions::setClipboard(UTF8_TO_TCHAR(text)); }

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
        Vector2D scale = 2.0f / Vector2D(drawData->DisplaySize);
        Vector2D translate = -1.0f - Vector2D(drawData->DisplayPos) * scale;
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
                                     .dimensions = Size3D(textureSizeX, textureSizeY, 1),
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

SharedPtr<WgWindow> ImGuiManager::getWindowWidget() const { return parentGuiManager ? parentGuiManager->getWindowWidget() : wgWindow; }

void ImGuiManager::setupInputs()
{
    ImGuiIO &io = GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
    io.KeyMap[ImGuiKey_Tab] = Keys::TAB.keyCode;
    io.KeyMap[ImGuiKey_LeftArrow] = Keys::LEFT.keyCode;
    io.KeyMap[ImGuiKey_RightArrow] = Keys::RIGHT.keyCode;
    io.KeyMap[ImGuiKey_UpArrow] = Keys::UP.keyCode;
    io.KeyMap[ImGuiKey_DownArrow] = Keys::DOWN.keyCode;
    io.KeyMap[ImGuiKey_PageUp] = Keys::PAGEUP.keyCode;
    io.KeyMap[ImGuiKey_PageDown] = Keys::PAGEDOWN.keyCode;
    io.KeyMap[ImGuiKey_Home] = Keys::HOME.keyCode;
    io.KeyMap[ImGuiKey_End] = Keys::END.keyCode;
    io.KeyMap[ImGuiKey_Insert] = Keys::INS.keyCode;
    io.KeyMap[ImGuiKey_Delete] = Keys::DEL.keyCode;
    io.KeyMap[ImGuiKey_Backspace] = Keys::BACKSPACE.keyCode;
    io.KeyMap[ImGuiKey_Space] = Keys::SPACE.keyCode;
    io.KeyMap[ImGuiKey_Enter] = Keys::ENTER.keyCode;
    io.KeyMap[ImGuiKey_Escape] = Keys::ESC.keyCode;
    io.KeyMap[ImGuiKey_KeyPadEnter] = Keys::NUMENTER.keyCode;
    io.KeyMap[ImGuiKey_A] = Keys::A.keyCode;
    io.KeyMap[ImGuiKey_C] = Keys::C.keyCode;
    io.KeyMap[ImGuiKey_V] = Keys::V.keyCode;
    io.KeyMap[ImGuiKey_X] = Keys::X.keyCode;
    io.KeyMap[ImGuiKey_Y] = Keys::Y.keyCode;
    io.KeyMap[ImGuiKey_Z] = Keys::Z.keyCode;

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
            const ImDrawList *cmdList = drawData->CmdLists[n];
            BatchCopyBufferData &vertCpy = bufferCopies.emplace_back();
            vertCpy.dst = vertexBuffer;
            vertCpy.dstOffset = vertOffset;
            vertCpy.dataToCopy = cmdList->VtxBuffer.Data;
            vertCpy.size = cmdList->VtxBuffer.Size * vertexBuffer->bufferStride();
            vertOffset += vertCpy.size;

            BatchCopyBufferData &idxCpy = bufferCopies.emplace_back();
            idxCpy.dst = idxBuffer;
            idxCpy.dstOffset = idxOffset;
            idxCpy.dataToCopy = cmdList->IdxBuffer.Data;
            idxCpy.size = cmdList->IdxBuffer.Size * idxBuffer->bufferStride();
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
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset
                                                               // field, allowing for large meshes.
    // texture atlas can be used from parent
    if (parentGuiManager)
    {
        textureAtlas = nullptr;
    }
    else
    {
        ENQUEUE_COMMAND(SetupImGui)
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
    ENQUEUE_COMMAND(ReleaseImGui)
    (
        [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            if (textureAtlas)
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
    if (!drawData || !drawingContext.rtTexture || drawData->CmdListsCount == 0 || drawData->DisplaySize.x <= 0.0f
        || drawData->DisplaySize.y <= 0.0f)
    {
        return;
    }

    SCOPED_CMD_MARKER(cmdList, drawingContext.cmdBuffer, DrawImGui);
    LocalPipelineContext pipelineContext;
    pipelineContext.materialName = IMGUI_SHADER_NAME;
    pipelineContext.forVertexType = EVertexType::UI;
    IRenderInterfaceModule::get()->getRenderManager()->preparePipelineContext(&pipelineContext, { drawingContext.rtTexture });

    updateRenderResources(cmdList, graphicsInstance, graphicsHelper, pipelineContext);

    //////////////////////////////////////////////////////////////////////////
    /// Drawing
    //////////////////////////////////////////////////////////////////////////

    ImageResource *rtTexture = static_cast<ImageResource *>(drawingContext.rtTexture->renderTargetResource().get());
    QuantizedBox2D viewport;
    viewport.minBound = Int2D(0, 0);
    // doing like this because even if ImGui size is different from framebuffer we can still draw
    viewport.maxBound = Int2D(rtTexture->getImageSize().x, rtTexture->getImageSize().y);

    Vector2D uiToFbDispScale = Vector2D(float(viewport.maxBound.x), float(viewport.maxBound.y)) / Vector2D(drawData->DisplaySize);

    // Render UI
    RenderPassAdditionalProps additionalProps;
    additionalProps.bAllowUndefinedLayout = false;
    additionalProps.colorAttachmentLoadOp = EAttachmentOp::LoadOp::Load;
    additionalProps.depthLoadOp = EAttachmentOp::LoadOp::Load;
    additionalProps.stencilLoadOp = EAttachmentOp::LoadOp::Load;

    RenderPassClearValue clearVal;

    // Barrier resources once
    cmdList->cmdBarrierResources(drawingContext.cmdBuffer, texturesUsed);
    texturesUsed.clear();

    cmdList->cmdBeginRenderPass(drawingContext.cmdBuffer, pipelineContext, viewport, additionalProps, clearVal);
    {
        GraphicsPipelineQueryParams query;
        query.cullingMode = ECullingMode::BackFace;
        query.drawMode = EPolygonDrawMode::Fill;
        cmdList->cmdBindGraphicsPipeline(drawingContext.cmdBuffer, pipelineContext, { query });
        cmdList->cmdBindVertexBuffers(drawingContext.cmdBuffer, 0, { vertexBuffer }, { 0 });
        cmdList->cmdBindIndexBuffer(drawingContext.cmdBuffer, idxBuffer);
        cmdList->cmdBindDescriptorsSets(drawingContext.cmdBuffer, pipelineContext, imguiTransformParams.reference());

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
                QuantizedBox2D scissor(
                    /*.minBound = */ Int2D(
                        int32((drawCmd.ClipRect.x - drawData->DisplayPos.x) * uiToFbDispScale.x()),
                        int32((drawCmd.ClipRect.y - drawData->DisplayPos.y) * uiToFbDispScale.y())
                    ),
                    /*.maxBound = */ Int2D(
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

    ImGui::NewFrame();
    for (std::pair<const int32, std::vector<IImGuiLayer *>> &imGuiLayers : drawLayers)
    {
        std::sort(
            imGuiLayers.second.begin(), imGuiLayers.second.end(),
            [](IImGuiLayer *lhs, IImGuiLayer *rhs) -> bool
            {
                return lhs->sublayerDepth() > rhs->sublayerDepth();
            }
        );

        for (IImGuiLayer *layer : imGuiLayers.second)
        {
            layer->draw(&drawInterface);
        }
    }
    ImGui::Render();

    updateTextureParameters();
    setShaderData();
}

void ImGuiManager::setDisplaySize(Short2D newSize)
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

void ImGuiManager::addLayer(IImGuiLayer *layer)
{
    std::vector<IImGuiLayer *> &layers = drawLayers[layer->layerDepth()];
    auto itr = std::find(layers.cbegin(), layers.cend(), layer);

    if (itr == layers.cend())
    {
        layers.emplace_back(layer);
    }
}

void ImGuiManager::removeLayer(IImGuiLayer *layer)
{
    std::vector<IImGuiLayer *> &layers = drawLayers[layer->layerDepth()];
    auto itr = std::find(layers.cbegin(), layers.cend(), layer);
    if (itr != layers.cend())
    {
        layers.erase(itr);
    }
}

bool ImGuiManager::inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem)
{
    setCurrentContext();
    ImGuiIO &io = GetIO();

    if (Keys::isMouseKey(key->keyCode))
    {
        io.MouseDown[key->keyCode - Keys::LMB.keyCode] = state.isPressed;
    }
    else
    {
        io.KeysDown[key->keyCode] = state.isPressed;

        Utf32 keyChar = inputSystem->keyChar(*key);
        if (state.keyWentDown && keyChar != 0)
        {
            io.AddInputCharacter(keyChar);
        }

        switch (key->keyCode)
        {
        case EKeyCode::KEY_LCTRL:
        case EKeyCode::KEY_RCTRL:
            io.KeyCtrl = state.isPressed;
            break;
        case EKeyCode::KEY_LSHIFT:
        case EKeyCode::KEY_RSHIFT:
            io.KeyShift = state.isPressed;
            break;
        case EKeyCode::KEY_LALT:
        case EKeyCode::KEY_RALT:
            io.KeyAlt = state.isPressed;
            break;
        case EKeyCode::KEY_LWIN:
        case EKeyCode::KEY_RWIN:
            io.KeySuper = state.isPressed;
            break;
        default:
            break;
        };
    }
    return bCaptureInput;
}

void ImGuiManager::updateMouse(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem)
{
    setCurrentContext();
    ImGuiIO &io = GetIO();

    io.MousePos = ImVec2(widgetRelPos.x, widgetRelPos.y);
    io.MouseWheel = inputSystem->analogState(AnalogStates::ScrollWheelY)->currentValue;
    io.MouseWheelH = inputSystem->analogState(AnalogStates::ScrollWheelX)->currentValue;
}

void ImGuiManager::mouseEnter(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem)
{
    updateMouse(absPos, widgetRelPos, inputSystem);
}

void ImGuiManager::mouseMoved(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem)
{
    updateMouse(absPos, widgetRelPos, inputSystem);
}

void ImGuiManager::mouseLeave(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem)
{
    updateMouse(absPos, widgetRelPos, inputSystem);
}
