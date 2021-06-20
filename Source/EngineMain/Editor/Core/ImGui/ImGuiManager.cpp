#include "ImGuiManager.h"
#include "ImGuiLib/imgui.h"
#include "ImGuiFontTextureAtlas.h"
#include "IImGuiLayer.h"
#include "../../../Core/String/String.h"
#include "../../../RenderInterface/PlatformIndependentHeaders.h"
#include "../../../Core/Platform/PlatformFunctions.h"
#include "../../../Core/Input/Keys.h"
#include "../../../Core/Input/InputSystem.h"
#include "../../../Core/Engine/Config/EngineGlobalConfigs.h"
#include "../../../RenderInterface/Rendering/IRenderCommandList.h"
#include "../../../RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "../../../RenderInterface/PlatformIndependentHelper.h"
#include "../../../RenderInterface/Rendering/RenderingContexts.h"
#include "../../../RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "../../../Core/Types/Textures/RenderTargetTextures.h"
#include "../../../Core/Math/Vector2D.h"
#include "../../../Core/Engine/WindowManager.h"
#include "../../../Core/Platform/GenericAppWindow.h"

using namespace ImGui;

const String ImGuiManager::TEXTURE_PARAM_NAME{ "textureAtlas" };
const String ImGuiManager::IMGUI_SHADER_NAME{ "DrawImGui" };

ImGuiManager::ImGuiManager(ImGuiManager* parent)
    : parentGuiManager(parent)
{}

void ImGuiManager::initialize()
{
    IMGUI_CHECKVERSION();
    if (parentGuiManager)
    {
        SetCurrentContext(parentGuiManager->context);
        context = CreateContext(GetIO().Fonts);
    }
    else
    {
        context = CreateContext();
    }
    SetCurrentContext(context);

    ImGuiIO& io = GetIO();
    io.BackendPlatformName = "CranberryEngine";
    io.LogFilename = nullptr;
    io.IniFilename = nullptr;
    io.Fonts->AddFontDefault();

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
    DestroyContext(context);
}

void ImGuiManager::setClipboard(void* userData, const char* text)
{
    PlatformFunctions::setClipboard(text);
}

const char* ImGuiManager::getClipboard(void* userData)
{
    reinterpret_cast<ImGuiManager*>(userData)->clipboard = PlatformFunctions::getClipboard();
    return reinterpret_cast<ImGuiManager*>(userData)->clipboard.getChar();
}

void ImGuiManager::setShaderData()
{
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData && drawData->Valid && imguiTransformParams)
    {
        Vector2D scale = 2.0f / Vector2D(drawData->DisplaySize);
        Vector2D translate = -1.0f - Vector2D(drawData->DisplayPos) * scale;
        imguiTransformParams->setVector2Param("scale", scale);
        imguiTransformParams->setVector2Param("translate", translate);
    }
}

TextureBase* ImGuiManager::getFontTextureAtlas() const
{
    return parentGuiManager ? parentGuiManager->getFontTextureAtlas() : textureAtlas;
}

SharedPtr<class SamplerInterface> ImGuiManager::getTextureSampler() const
{
    return parentGuiManager ? parentGuiManager->getTextureSampler() : textureSampler;
}

SharedPtr<class ShaderParameters> ImGuiManager::getFontAtlasParam() const
{
    return parentGuiManager ? parentGuiManager->getFontAtlasParam() : imguiFontAtlasParams;
}

SharedPtr<class ShaderParameters> ImGuiManager::getTextureParam(const TextureBase* textureUsed)
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

SharedPtr<ShaderParameters> ImGuiManager::createTextureParam(const TextureBase* texture, IGraphicsInstance* graphicsInstance, const class LocalPipelineContext& pipelineContext)
{
    if (parentGuiManager)
    {
        return parentGuiManager->createTextureParam(texture, graphicsInstance, pipelineContext);
    }
    else
    {
        SharedPtr<ShaderParameters> params = GraphicsHelper::createShaderParameters(graphicsInstance, pipelineContext.getPipeline()->getParamLayoutAtSet(0), { 0 });
        params->setTextureParam(TEXTURE_PARAM_NAME, texture->getTextureResource(), getTextureSampler());
        params->setResourceName("ShaderParams_" + texture->getTextureName());
        params->init();
        
        textureParams[texture] = params;

        return params;
    }
}

SharedPtr<ShaderParameters> ImGuiManager::findFreeTextureParam(const TextureBase* textureUsed)
{
    if (!freeTextureParams.empty())
    {
        SharedPtr<ShaderParameters> retVal = freeTextureParams.front();
        textureParams[textureUsed] = retVal;
        freeTextureParams.pop();
        retVal->setTextureParam(TEXTURE_PARAM_NAME, textureUsed->getTextureResource(), getTextureSampler());
        return retVal;
    }
    return nullptr;
}

void ImGuiManager::setupInputs()
{
    ImGuiIO& io = GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls 

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
}

void ImGuiManager::updateInputs()
{
    ImGuiIO& io = GetIO();

    const InputSystem* inputSystem = gEngine->appInstance().inputSystem();
    for (const Key* key : Keys::Range())
    {
        if (Keys::isMouseKey(key->keyCode))
        {
            io.MouseDown[key->keyCode - Keys::LMB.keyCode] = inputSystem->isKeyPressed(*key);
        }
        else
        {
            const KeyState* state = inputSystem->keyState(*key);
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
    Rect windowArea = gEngine->getApplicationInstance()->appWindowManager.getMainWindow()->windowClientRect();
    io.MousePos = Vector2D(inputSystem->analogState(AnalogStates::AbsMouseX)->currentValue, inputSystem->analogState(AnalogStates::AbsMouseY)->currentValue) - windowArea.minBound;
    // Resize to screen render size, If using screen size
    //const Vector2D pos = Vector2D(inputSystem->analogState(AnalogStates::AbsMouseX)->currentValue, inputSystem->analogState(AnalogStates::AbsMouseY)->currentValue) - windowArea.minBound;
    //io.MousePos = pos * (Vector2D(EngineSettings::screenSize.get()) / Vector2D(EngineSettings::surfaceSize.get()));
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

    ImDrawData* drawData = ImGui::GetDrawData();

    // Update used texture resources
    if (drawData && drawData->Valid)
    {
        texturesUsed.clear();
        for (int32 cmdListIdx = 0; cmdListIdx < drawData->CmdListsCount; ++cmdListIdx)
        {
            const ImDrawList* uiCmdList = drawData->CmdLists[cmdListIdx];
            for (int32 cmdIdx = 0; cmdIdx < uiCmdList->CmdBuffer.Size; ++cmdIdx)
            {
                const ImDrawCmd& drawCmd = uiCmdList->CmdBuffer[cmdIdx];
                if (drawCmd.TextureId)
                {
                    SharedPtr<ShaderParameters> perDrawTexture = getTextureParam(static_cast<const TextureBase*>(drawCmd.TextureId));
                    if (perDrawTexture)
                    {
                        texturesUsed.insert(perDrawTexture.get());
                    }
                    else if (perDrawTexture = findFreeTextureParam(static_cast<const TextureBase*>(drawCmd.TextureId)))
                    {
                        texturesUsed.insert(perDrawTexture.get());
                    }
                    else
                    {
                        texturesToCreate.insert(static_cast<const TextureBase*>(drawCmd.TextureId));
                    }
                }
            }
        }
    }
}

void ImGuiManager::updateRenderResources(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const TinyDrawingContext& drawingContext, const class LocalPipelineContext& pipelineContext)
{
    ImDrawData* drawData = ImGui::GetDrawData();
    // Setting up vertex and index buffers
    {
        if (!vertexBuffer.isValid() || !idxBuffer.isValid())
        {
            // TODO(Jeslas) : If we are supporting multi-window then this has to be reworked.
            vertexBuffer.setNewSwapchain(gEngine->getApplicationInstance()->appWindowManager.getWindowCanvas(gEngine->getApplicationInstance()->appWindowManager.getMainWindow()),
                int32(sizeof(ImDrawVert)), 0);
            idxBuffer.setNewSwapchain(gEngine->getApplicationInstance()->appWindowManager.getWindowCanvas(gEngine->getApplicationInstance()->appWindowManager.getMainWindow()),
                int32(sizeof(ImDrawIdx)), 0);

            for (uint32 i = 0; i < vertexBuffer.getResources().size(); ++i)
            {
                vertexBuffer.getResources()[i]->setAsStagingResource(true);
                vertexBuffer.getResources()[i]->setResourceName("ImGuiVertices_" + std::to_string(i));
                idxBuffer.getResources()[i]->setAsStagingResource(true);
                idxBuffer.getResources()[i]->setResourceName("ImGuiIndices_" + std::to_string(i));
            }
        }

        if (vertexBuffer->bufferCount() < drawData->TotalVtxCount)
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
        if (idxBuffer->bufferCount() < drawData->TotalIdxCount)
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
            const ImDrawList* cmdList = drawData->CmdLists[n];
            BatchCopyBufferData& vertCpy = bufferCopies.emplace_back();
            vertCpy.dst = *vertexBuffer;
            vertCpy.dstOffset = vertOffset;
            vertCpy.dataToCopy = cmdList->VtxBuffer.Data;
            vertCpy.size = cmdList->VtxBuffer.Size * vertexBuffer->bufferStride();
            vertOffset += vertCpy.size;

            BatchCopyBufferData& idxCpy = bufferCopies.emplace_back();
            idxCpy.dst = *idxBuffer;
            idxCpy.dstOffset = idxOffset;
            idxCpy.dataToCopy = cmdList->IdxBuffer.Data;
            idxCpy.size = cmdList->IdxBuffer.Size * idxBuffer->bufferStride();
            idxOffset += idxCpy.size;
        }
        cmdList->copyToBuffer(bufferCopies);
    }

    // only in parent GUI
    if (!getFontAtlasParam() && parentGuiManager == nullptr)
    {
        // As 0 contains all sets in utility shader, ignore 0 as it is unique to each GUI manager
        imguiFontAtlasParams = GraphicsHelper::createShaderParameters(graphicsInstance, pipelineContext.getPipeline()->getParamLayoutAtSet(0), { 0 });
        imguiFontAtlasParams->setTextureParam(TEXTURE_PARAM_NAME, getFontTextureAtlas()->getTextureResource(), getTextureSampler());
        imguiFontAtlasParams->setResourceName("ShaderParams_" + getFontTextureAtlas()->getTextureName());
        imguiFontAtlasParams->init();
    }
    if (!imguiTransformParams)
    {
        imguiTransformParams = GraphicsHelper::createShaderParameters(graphicsInstance, pipelineContext.getPipeline()->getParamLayoutAtSet(0), { 1 });
        imguiTransformParams->setResourceName("ShaderParams_IMGUI_TX");
        setShaderData();
        imguiTransformParams->init();
    }
    // Create necessary texture parameters
    for (const TextureBase* texture : texturesToCreate)
    {
        texturesUsed.insert(createTextureParam(texture, graphicsInstance, pipelineContext).get());
    }
    texturesToCreate.clear();
}

void ImGuiManager::setupRendering()
{
    ImGuiIO& io = GetIO();
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    // Using surface size
    io.DisplaySize = Vector2D(float(EngineSettings::surfaceSize.get().x), float(EngineSettings::surfaceSize.get().y));
    textureResizedHnd = EngineSettings::surfaceSize.onConfigChanged().bindLambda(LambdaFunction<void, Size2D, Size2D>([&io](Size2D oldSize, Size2D newSize)
        {
            io.DisplaySize = Vector2D(float(newSize.x), float(newSize.y));
        }));
    // Using screen size
    //io.DisplaySize = Vector2D(float(EngineSettings::screenSize.get().x), float(EngineSettings::screenSize.get().y));
    //textureResizedHnd = EngineSettings::screenSize.onConfigChanged().bindLambda(LambdaFunction<void, Size2D, Size2D>([&io](Size2D oldSize, Size2D newSize)
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
        textureParams.textureName = "ImGuiTextureAtlas";
        textureParams.filtering = ESamplerFiltering::Linear;
        textureParams.owningContext = context;
        textureAtlas = TextureBase::createTexture<ImGuiFontTextureAtlas>(textureParams);

        ENQUEUE_COMMAND(CreateSampler, LAMBDA_BODY(
            textureSampler = GraphicsHelper::createSampler(graphicsInstance, "ImGuiFontAtlasSampler", ESamplerTilingMode::EdgeClamp, ESamplerFiltering::Linear);
        ), this);
    }
}

void ImGuiManager::releaseRendering()
{
    ENQUEUE_COMMAND(ReleaseImGui, LAMBDA_BODY(
        if (textureAtlas)
        {
            TextureBase::destroyTexture<ImGuiFontTextureAtlas>(textureAtlas);
            textureAtlas = nullptr;
        }
        if (textureSampler)
        {
            textureSampler->release();
            textureSampler.reset();
        }
        if (imguiFontAtlasParams)
        {
            imguiFontAtlasParams->release();
            imguiFontAtlasParams.reset();
        }
        if (imguiTransformParams)
        {
            imguiTransformParams->release();
            imguiTransformParams.reset();
        }
        vertexBuffer.reset();
        idxBuffer.reset();

        if (!parentGuiManager)
        {
            for (std::pair<TextureBase const* const, SharedPtr<ShaderParameters>>& textureParam : textureParams)
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
        }
    ), this);

    EngineSettings::surfaceSize.onConfigChanged().unbind(textureResizedHnd);
    //EngineSettings::screenSize.onConfigChanged().unbind(textureResizedHnd);
}

void ImGuiManager::draw(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const TinyDrawingContext& drawingContext)
{
    SetCurrentContext(context);
    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData || drawingContext.rtTextures.empty() || drawData->CmdListsCount == 0 || drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
    {
        return;
    }

    SCOPED_CMD_MARKER(cmdList, drawingContext.cmdBuffer, DrawImGui);
    LocalPipelineContext pipelineContext;
    pipelineContext.materialName = IMGUI_SHADER_NAME;
    pipelineContext.forVertexType = EVertexType::UI;
    pipelineContext.rtTextures = drawingContext.rtTextures;
    pipelineContext.swapchainIdx = drawingContext.swapchainIdx;
    gEngine->getRenderApi()->getGlobalRenderingContext()->preparePipelineContext(&pipelineContext);

    updateRenderResources(cmdList, graphicsInstance, drawingContext, pipelineContext);

    //////////////////////////////////////////////////////////////////////////
    /// Drawing
    //////////////////////////////////////////////////////////////////////////

    QuantizedBox2D viewport;
    viewport.minBound = Int2D(0, 0);
    viewport.maxBound = drawingContext.rtTextures[0]->getTextureSize();// doing like this because even if ImGui size is different from framebuffer we can still draw    

    Vector2D uiToFbDispScale = Vector2D(float(viewport.maxBound.x), float(viewport.maxBound.y))/Vector2D(drawData->DisplaySize);
    
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
        cmdList->cmdBindDescriptorsSets(drawingContext.cmdBuffer, pipelineContext, imguiTransformParams.get());

        int32 vertOffset = 0;
        uint32 idxOffset = 0;
        for (int32 cmdListIdx = 0; cmdListIdx < drawData->CmdListsCount; ++cmdListIdx)
        {
            const ImDrawList *uiCmdList = drawData->CmdLists[cmdListIdx];
            for (int32 cmdIdx = 0; cmdIdx < uiCmdList->CmdBuffer.Size; ++cmdIdx)
            {
                const ImDrawCmd& drawCmd = uiCmdList->CmdBuffer[cmdIdx];
                if (drawCmd.UserCallback != nullptr)
                {
                    Logger::warn("ImGui", "%s() : Commands with callback is not supported", __func__);
                    debugAssert(drawCmd.UserCallback != nullptr);
                    continue;
                }
                // All vertex, clip data is in display texel coordinates + Display pos(in case of multi monitor setup)
                QuantizedBox2D scissor(
                    /*.minBound = */Int2D(
                        int32((drawCmd.ClipRect.x - drawData->DisplayPos.x)* uiToFbDispScale.x())
                        , int32((drawCmd.ClipRect.y - drawData->DisplayPos.y)* uiToFbDispScale.y()))
                    ,/*.maxBound = */Int2D(
                        int32((drawCmd.ClipRect.z - drawData->DisplayPos.x)* uiToFbDispScale.x())
                        , int32((drawCmd.ClipRect.w - drawData->DisplayPos.y)* uiToFbDispScale.y()))
                );
                if (scissor.intersect(viewport))
                {
                    SharedPtr<ShaderParameters> perDrawTexture = getFontAtlasParam();
                    if(drawCmd.TextureId)
                    {
                        perDrawTexture = getTextureParam(static_cast<const TextureBase*>(drawCmd.TextureId));
                        fatalAssert(perDrawTexture, "%s(): Failed getting texture parameters for imgui", __func__);
                    }
                    cmdList->cmdBindDescriptorsSets(drawingContext.cmdBuffer, pipelineContext, perDrawTexture.get());
                    cmdList->cmdSetViewportAndScissor(drawingContext.cmdBuffer, viewport, scissor);
                    cmdList->cmdDrawIndexed(drawingContext.cmdBuffer, idxOffset + drawCmd.IdxOffset, drawCmd.ElemCount, 0, 1, vertOffset + drawCmd.VtxOffset);
                }
            }
            vertOffset += uiCmdList->VtxBuffer.Size;
            idxOffset += uiCmdList->IdxBuffer.Size;
        }
    }
    cmdList->cmdEndRenderPass(drawingContext.cmdBuffer);
}

void ImGuiManager::updateFrame(const float& deltaTime)
{
    SetCurrentContext(context);
    GetIO().DeltaTime = deltaTime;
    updateInputs();

    ImGui::NewFrame();
    for (std::pair<const int32, std::vector<IImGuiLayer*>>& imGuiLayers : drawLayers)
    {
        std::sort(imGuiLayers.second.begin(), imGuiLayers.second.end()
            , [](IImGuiLayer* lhs, IImGuiLayer* rhs)->bool { return lhs->sublayerDepth() > rhs->sublayerDepth(); });

        for (IImGuiLayer* layer : imGuiLayers.second)
        {
            layer->draw(&drawInterface);
        }
    }
    ImGui::Render();

    updateTextureParameters();
    setShaderData();
}

void ImGuiManager::addFont(const String& fontAssetPath, float fontSize)
{
    if (parentGuiManager)
    {
        parentGuiManager->addFont(fontAssetPath, fontSize);
    }
    else
    {
        SetCurrentContext(context);

        // TODO(Jeslas) : Load from asset manager
        std::vector<uint8> fontData;
        GetIO().Fonts->AddFontFromMemoryTTF(fontData.data(), int32(fontData.size()), fontSize);
        getFontTextureAtlas()->markResourceDirty();

        if (imguiFontAtlasParams)
        {
            imguiFontAtlasParams->setTextureParam("textureAtlas", getFontTextureAtlas()->getTextureResource(), getTextureSampler());
        }
    }
}

void ImGuiManager::addLayer(IImGuiLayer* layer)
{
    std::vector<IImGuiLayer*>& layers = drawLayers[layer->layerDepth()];
    auto itr = std::find(layers.cbegin(), layers.cend(), layer);

    if (itr == layers.cend())
    {
        layers.emplace_back(layer);
    }
}

void ImGuiManager::removeLayer(IImGuiLayer* layer)
{
    std::vector<IImGuiLayer*>& layers = drawLayers[layer->layerDepth()];
    auto itr = std::find(layers.cbegin(), layers.cend(), layer);
    if (itr != layers.cend())
    {
        layers.erase(itr);
    }
}
