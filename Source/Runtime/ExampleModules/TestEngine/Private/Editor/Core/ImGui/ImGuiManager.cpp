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

#include "Editor/Core/ImGui/ImGuiManager.h"
#include "Core/Types/Textures/RenderTargetTextures.h"
#include "Editor/Core/ImGui/IImGuiLayer.h"
#include "Editor/Core/ImGui/ImGuiFontTextureAtlas.h"
#include "Editor/Core/ImGui/ImGuiLib/imgui.h"
#include "Editor/Core/ImGui/ImGuiLib/implot.h"
#include "GenericAppWindow.h"
#include "IApplicationModule.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Keys.h"
#include "Math/Vector2D.h"
#include "Modules/ModuleManager.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/Rendering/RenderingContexts.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "String/String.h"
#include "Types/Platform/PlatformFunctions.h"
#include "WindowManager.h"
#include "ApplicationSettings.h"
#include "ApplicationInstance.h"

using namespace ImGui;

const String ImGuiManager::TEXTURE_PARAM_NAME{ TCHAR("textureAtlas") };
const String ImGuiManager::IMGUI_SHADER_NAME{ TCHAR("DrawImGui") };

ImGuiManager::ImGuiManager(ImGuiManager *parent)
    : parentGuiManager(parent)
{}

void ImGuiManager::initialize()
{
    IMGUI_CHECKVERSION();
    if (parentGuiManager)
    {
        parentGuiManager->setCurrentContexts();
        context = CreateContext(GetIO().Fonts);
    }
    else
    {
        context = CreateContext();
    }
    implotContext = ImPlot::CreateContext();
    setCurrentContexts();

    ImGuiIO &io = GetIO();
    io.BackendPlatformName = "CranberryEngine";
    io.LogFilename = nullptr;
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

void ImGuiManager::setCurrentContexts()
{
    SetCurrentContext(context);
    ImPlot::SetCurrentContext(implotContext);
}

TextureBase *ImGuiManager::getFontTextureAtlas() const { return parentGuiManager ? parentGuiManager->getFontTextureAtlas() : textureAtlas; }

SamplerRef ImGuiManager::getTextureSampler() const { return parentGuiManager ? parentGuiManager->getTextureSampler() : textureSampler; }

ShaderParametersRef ImGuiManager::getFontAtlasParam() const
{
    return parentGuiManager ? parentGuiManager->getFontAtlasParam() : imguiFontAtlasParams;
}

ShaderParametersRef ImGuiManager::getTextureParam(const TextureBase *textureUsed)
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
    const TextureBase *texture, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper,
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
        params->setTextureParam(TEXTURE_PARAM_NAME, texture->getTextureResource(), getTextureSampler());
        params->setResourceName(TCHAR("ShaderParams_") + texture->getTextureName());
        params->init();

        textureParams[texture] = params;

        return params;
    }
}

ShaderParametersRef ImGuiManager::findFreeTextureParam(const TextureBase *textureUsed)
{
    if (!freeTextureParams.empty())
    {
        ShaderParametersRef retVal = freeTextureParams.front();
        textureParams[textureUsed] = retVal;
        freeTextureParams.pop();
        retVal->setTextureParam(TEXTURE_PARAM_NAME, textureUsed->getTextureResource(), getTextureSampler());
        return retVal;
    }
    return nullptr;
}

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
    inputSystem = IApplicationModule::get()->getApplication()->inputSystem;
}

void ImGuiManager::updateInputs()
{
    ImGuiIO &io = GetIO();

    if (!inputSystem)
    {
        inputSystem = IApplicationModule::get()->getApplication()->inputSystem;
    }

    for (const Key *key : Keys::Range())
    {
        if (Keys::isMouseKey(key->keyCode))
        {
            io.MouseDown[key->keyCode - Keys::LMB.keyCode] = inputSystem->isKeyPressed(*key);
        }
        else
        {
            const KeyState *state = inputSystem->keyState(*key);
            io.KeysDown[key->keyCode] = state->isPressed;

            Utf32 keyChar = inputSystem->keyChar(*key);
            if (state->keyWentDown && keyChar != 0)
            {
                io.AddInputCharacter(keyChar);
            }
        }
    }

    io.KeyCtrl = inputSystem->isKeyPressed(Keys::RCTRL) || inputSystem->isKeyPressed(Keys::LCTRL);
    io.KeyShift = inputSystem->isKeyPressed(Keys::RSHIFT) || inputSystem->isKeyPressed(Keys::LSHIFT);
    io.KeyAlt = inputSystem->isKeyPressed(Keys::RALT) || inputSystem->isKeyPressed(Keys::LALT);
    io.KeySuper = inputSystem->isKeyPressed(Keys::RWIN) || inputSystem->isKeyPressed(Keys::LWIN);
    io.MouseWheel = inputSystem->analogState(AnalogStates::ScrollWheelY)->currentValue;
    io.MouseWheelH = inputSystem->analogState(AnalogStates::ScrollWheelX)->currentValue;

    // TODO(Jeslas) : If we are supporting multi-window then this has to be reworked.
    Rect windowArea = IApplicationModule::get()->getApplication()->windowManager->getMainWindow()->windowClientRect();
    float dpiScaleFactor = IApplicationModule::get()->getApplication()->windowManager->getMainWindow()->dpiScale();
    io.MousePos
        = (Vector2D(
               inputSystem->analogState(AnalogStates::AbsMouseX)->currentValue, inputSystem->analogState(AnalogStates::AbsMouseY)->currentValue
           )
           - windowArea.minBound)
          * dpiScaleFactor;
    // Resize to screen render size, If using screen size
    // const Vector2D pos = Vector2D(inputSystem->analogState(AnalogStates::AbsMouseX)->currentValue,
    // inputSystem->analogState(AnalogStates::AbsMouseY)->currentValue) - windowArea.minBound;
    // io.MousePos = pos * (Vector2D(ApplicationSettings::screenSize.get()) /
    // Vector2D(ApplicationSettings::surfaceSize.get()));

    bCaptureInput = io.WantCaptureKeyboard || io.WantCaptureMouse;
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
                    ShaderParametersRef perDrawTexture = getTextureParam(static_cast<const TextureBase *>(drawCmd.TextureId));
                    if (perDrawTexture.isValid())
                    {
                        texturesUsed.insert(perDrawTexture.reference());
                    }
                    else
                    {
                        perDrawTexture = findFreeTextureParam(static_cast<const TextureBase *>(drawCmd.TextureId));
                        if (perDrawTexture.isValid())
                        {
                            texturesUsed.insert(perDrawTexture.reference());
                        }
                        else
                        {
                            texturesToCreate.insert(static_cast<const TextureBase *>(drawCmd.TextureId));
                        }
                    }
                }
            }
        }
    }
}

void ImGuiManager::updateRenderResources(
    class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper,
    const ImGuiDrawingContext &drawingContext, const class LocalPipelineContext &pipelineContext
)
{
    ImDrawData *drawData = ImGui::GetDrawData();
    // Setting up vertex and index buffers
    {
        if (!vertexBuffer.isValid() || !idxBuffer.isValid())
        {
            // TODO(Jeslas) : If we are supporting multi-window then this has to be reworked.
            WindowCanvasRef windowCanvas = IApplicationModule::get()->getApplication()->windowManager->getWindowCanvas(
                IApplicationModule::get()->getApplication()->windowManager->getMainWindow()
            );
            vertexBuffer.setNewSwapchain(windowCanvas);
            idxBuffer.setNewSwapchain(windowCanvas);

            for (uint32 i = 0; i < windowCanvas->imagesCount(); ++i)
            {
                BufferResourceRef vertexBuf = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, int32(sizeof(ImDrawVert)), 1);
                vertexBuf->setAsStagingResource(true);
                vertexBuf->setResourceName(TCHAR("ImGuiVertices_") + String::toString(i));
                BufferResourceRef idxBuf = graphicsHelper->createReadOnlyIndexBuffer(graphicsInstance, int32(sizeof(ImDrawIdx)), 1);
                idxBuf->setAsStagingResource(true);
                idxBuf->setResourceName(TCHAR("ImGuiIndices_") + String::toString(i));

                vertexBuffer.set(vertexBuf, i);
                idxBuffer.set(idxBuf, i);
            }
        }

        if (vertexBuffer->bufferCount() < uint32(drawData->TotalVtxCount))
        {
            vertexBuffer->setBufferCount(drawData->TotalVtxCount);
            if (vertexBuffer->isValid())
            {
                vertexBuffer->reinitResources();
            }
            else
            {
                vertexBuffer->init();
            }
        }
        if (idxBuffer->bufferCount() < uint32(drawData->TotalIdxCount))
        {
            idxBuffer->setBufferCount(drawData->TotalIdxCount);
            if (idxBuffer->isValid())
            {
                idxBuffer->reinitResources();
            }
            else
            {
                idxBuffer->init();
            }
        }
        std::vector<BatchCopyBufferData> bufferCopies;
        uint32 vertOffset = 0;
        uint32 idxOffset = 0;
        for (int32 n = 0; n < drawData->CmdListsCount; ++n)
        {
            const ImDrawList *cmdList = drawData->CmdLists[n];
            BatchCopyBufferData &vertCpy = bufferCopies.emplace_back();
            vertCpy.dst = *vertexBuffer;
            vertCpy.dstOffset = vertOffset;
            vertCpy.dataToCopy = cmdList->VtxBuffer.Data;
            vertCpy.size = cmdList->VtxBuffer.Size * vertexBuffer->bufferStride();
            vertOffset += vertCpy.size;

            BatchCopyBufferData &idxCpy = bufferCopies.emplace_back();
            idxCpy.dst = *idxBuffer;
            idxCpy.dstOffset = idxOffset;
            idxCpy.dataToCopy = cmdList->IdxBuffer.Data;
            idxCpy.size = cmdList->IdxBuffer.Size * idxBuffer->bufferStride();
            idxOffset += idxCpy.size;
        }
        cmdList->copyToBuffer(bufferCopies);
    }

    // only in parent GUI
    if (!getFontAtlasParam().isValid() && parentGuiManager == nullptr)
    {
        // As 0 contains all sets in utility shader, ignore 0 as it is unique to each GUI manager
        imguiFontAtlasParams
            = graphicsHelper->createShaderParameters(graphicsInstance, pipelineContext.getPipeline()->getParamLayoutAtSet(0), { 0 });
        imguiFontAtlasParams->setTextureParam(TEXTURE_PARAM_NAME, getFontTextureAtlas()->getTextureResource(), getTextureSampler());
        imguiFontAtlasParams->setResourceName(TCHAR("ShaderParams_") + getFontTextureAtlas()->getTextureName());
        imguiFontAtlasParams->init();
    }
    if (!imguiTransformParams.isValid())
    {
        imguiTransformParams
            = graphicsHelper->createShaderParameters(graphicsInstance, pipelineContext.getPipeline()->getParamLayoutAtSet(0), { 1 });
        imguiTransformParams->setResourceName(TCHAR("ShaderParams_IMGUI_TX"));
        setShaderData();
        imguiTransformParams->init();
    }
    // Create necessary texture parameters
    for (const TextureBase *texture : texturesToCreate)
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

    // TODO(Jeslas) : If we are supporting multi-window then this has to be reworked.
    float dpiScaleFactor = IApplicationModule::get()->getApplication()->windowManager->getMainWindow()->dpiScale();
    // Using surface size
    io.DisplaySize
        = Vector2D(float(ApplicationSettings::surfaceSize.get().x), float(ApplicationSettings::surfaceSize.get().y)) * dpiScaleFactor;
    textureResizedHnd = ApplicationSettings::surfaceSize.onConfigChanged().bindLambda(
        [&io](Size2D oldSize, Size2D newSize)
        {
            float dpiScaleFactor = IApplicationModule::get()->getApplication()->windowManager->getMainWindow()->dpiScale();
            io.DisplaySize = Vector2D(float(newSize.x), float(newSize.y)) * dpiScaleFactor;
        }
    );
    // Using screen size
    // io.DisplaySize = Vector2D(float(ApplicationSettings::screenSize.get().x),
    // float(ApplicationSettings::screenSize.get().y)); textureResizedHnd =
    // ApplicationSettings::screenSize.onConfigChanged().bindLambda(LambdaFunction<void, Size2D,
    // Size2D>([&io](Size2D oldSize, Size2D newSize)
    //    {
    //        io.DisplaySize = Vector2D(float(newSize.x), float(newSize.y));
    //    }));

    // texture atlas can be used from parent
    if (parentGuiManager)
    {
        textureAtlas = nullptr;
        textureSampler = nullptr;
    }
    else
    {
        ImGuiFontTextureParams textureParams;
        textureParams.textureName = TCHAR("ImGuiTextureAtlas");
        textureParams.filtering = ESamplerFiltering::Linear;
        textureParams.owningContext = context;
        textureAtlas = TextureBase::createTexture<ImGuiFontTextureAtlas>(textureParams);

        ENQUEUE_COMMAND(CreateSampler)
        (
            [this](class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
            {
                SamplerCreateInfo createInfo{
                    .filtering = ESamplerFiltering::Linear,
                    .mipFiltering = ESamplerFiltering::Linear,
                    .tilingMode = {ESamplerTilingMode::EdgeClamp, ESamplerTilingMode::EdgeClamp, ESamplerTilingMode::EdgeClamp },
                    .mipLodRange = {                            0,                             0                             },
                    .resourceName = TCHAR("ImGuiFontAtlasSampler")
                };
                textureSampler = graphicsHelper->createSampler(graphicsInstance, createInfo);
                textureSampler->init();
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
                TextureBase::destroyTexture<ImGuiFontTextureAtlas>(textureAtlas);
                textureAtlas = nullptr;
            }
            if (textureSampler.isValid())
            {
                textureSampler->release();
                textureSampler.reset();
            }
            if (imguiFontAtlasParams.isValid())
            {
                imguiFontAtlasParams->release();
                imguiFontAtlasParams.reset();
            }
            if (imguiTransformParams.isValid())
            {
                imguiTransformParams->release();
                imguiTransformParams.reset();
            }
            vertexBuffer.reset();
            idxBuffer.reset();

            if (!parentGuiManager)
            {
                for (std::pair<TextureBase const *const, ShaderParametersRef> &textureParam : textureParams)
                {
                    textureParam.second->release();
                    textureParam.second.reset();
                }
                while (!freeTextureParams.empty())
                {
                    freeTextureParams.front()->release();
                    freeTextureParams.front().reset();
                    freeTextureParams.pop();
                }
                activeTextureParams.clear();
            }
        }
    );

    ApplicationSettings::surfaceSize.onConfigChanged().unbind(textureResizedHnd);
    // ApplicationSettings::screenSize.onConfigChanged().unbind(textureResizedHnd);
}

void ImGuiManager::draw(
    class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper,
    const ImGuiDrawingContext &drawingContext
)
{
    setCurrentContexts();
    ImDrawData *drawData = ImGui::GetDrawData();
    if (!drawData || drawingContext.rtTextures.empty() || drawData->CmdListsCount == 0 || drawData->DisplaySize.x <= 0.0f
        || drawData->DisplaySize.y <= 0.0f)
    {
        return;
    }

    SCOPED_CMD_MARKER(cmdList, drawingContext.cmdBuffer, DrawImGui);
    LocalPipelineContext pipelineContext;
    pipelineContext.materialName = IMGUI_SHADER_NAME;
    pipelineContext.forVertexType = EVertexType::UI;
    // pipelineContext.frameAttachments = drawingContext.rtTextures;
    pipelineContext.swapchainIdx = drawingContext.swapchainIdx;
    IRenderInterfaceModule::get()->getRenderManager()->preparePipelineContext(&pipelineContext, drawingContext.rtTextures);

    updateRenderResources(cmdList, graphicsInstance, graphicsHelper, drawingContext, pipelineContext);

    //////////////////////////////////////////////////////////////////////////
    /// Drawing
    //////////////////////////////////////////////////////////////////////////

    QuantizedBox2D viewport;
    viewport.minBound = Int2D(0, 0);
    // doing like this because even if ImGui size is different from framebuffer we can still draw
    viewport.maxBound = static_cast<RenderTargetTexture *>(drawingContext.rtTextures[0])->getTextureSize();

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
        cmdList->cmdBindVertexBuffers(drawingContext.cmdBuffer, 0, { *vertexBuffer }, { 0 });
        cmdList->cmdBindIndexBuffer(drawingContext.cmdBuffer, *idxBuffer);
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
                    LOG_WARN("ImGui", "%s() : Commands with callback is not supported", __func__);
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
                    ShaderParametersRef perDrawTexture = getFontAtlasParam();
                    if (drawCmd.TextureId)
                    {
                        perDrawTexture = getTextureParam(static_cast<const TextureBase *>(drawCmd.TextureId));
                        fatalAssert(perDrawTexture.isValid(), "%s(): Failed getting texture parameters for imgui", __func__);
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

void ImGuiManager::updateFrame(const float &deltaTime)
{
    setCurrentContexts();
    GetIO().DeltaTime = deltaTime;
    updateInputs();

    ImGui::NewFrame();
    for (std::pair<const int32, std::vector<IImGuiLayer *>> &imGuiLayers : drawLayers)
    {
        std::sort(
            imGuiLayers.second.begin(), imGuiLayers.second.end(),
            [](IImGuiLayer *lhs, IImGuiLayer *rhs) -> bool { return lhs->sublayerDepth() > rhs->sublayerDepth(); }
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

void ImGuiManager::addFont(const String &fontAssetPath, float fontSize)
{
    if (parentGuiManager)
    {
        parentGuiManager->addFont(fontAssetPath, fontSize);
    }
    else
    {
        setCurrentContexts();

        // TODO(Jeslas) : Load from asset manager
        std::vector<uint8> fontData;
        GetIO().Fonts->AddFontFromMemoryTTF(fontData.data(), int32(fontData.size()), fontSize);
        getFontTextureAtlas()->markResourceDirty();

        if (imguiFontAtlasParams.isValid())
        {
            imguiFontAtlasParams->setTextureParam(TCHAR("textureAtlas"), getFontTextureAtlas()->getTextureResource(), getTextureSampler());
        }
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
