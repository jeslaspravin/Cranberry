/*!
 * \file ImGuiManager.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "ImGuiDrawInterface.h"
#include "Types/CoreTypes.h"
#include "String/String.h"
#include "Memory/SmartPointers.h"
#include "Types/Delegates/Delegate.h"
#include "RenderInterface/Resources/BufferedResources.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderInterface/Resources/Samplers/SamplerInterface.h"

#include <map>
#include <queue>
#include <set>

class TextureBase;
class IImGuiLayer;

class IRenderTargetTexture;
class IGraphicsInstance;
struct ImGuiContext;
struct ImPlotContext;
class GraphicsHelperAPI;
class InputSystem;

struct ImGuiDrawingContext
{
    const GraphicsResource* cmdBuffer;

    std::vector<IRenderTargetTexture*> rtTextures;
    uint32 swapchainIdx = ~(0u);
};

class ImGuiManager
{
private:
    static const String TEXTURE_PARAM_NAME;
    static const String IMGUI_SHADER_NAME;
    // Only parent GUI manager data
    SamplerRef textureSampler;
    TextureBase* textureAtlas;
    ShaderParametersRef imguiFontAtlasParams;

    std::map<const TextureBase*, ShaderParametersRef> textureParams;
    // Inactive free texture params
    std::queue<ShaderParametersRef> freeTextureParams;
    // Texture params accessed last frame, if any from texture params that are not here it goes to inactive free params
    std::set<ShaderParametersRef> activeTextureParams;

    // Unique per GUI manager
    String clipboard;
    const InputSystem* inputSystem;
    bool bCaptureInput;

    ImGuiManager* parentGuiManager;
    ImGuiContext* context;
    ImPlotContext* implotContext;
    ImGuiDrawInterface drawInterface;
    // Per display size
    ShaderParametersRef imguiTransformParams;
    SwapchainBufferedResource<BufferResourceRef> vertexBuffer;
    SwapchainBufferedResource<BufferResourceRef> idxBuffer;

    DelegateHandle textureResizedHnd;

    std::map<int32, std::vector<IImGuiLayer*>, std::greater<int32>> drawLayers;

    // Per frame data

    // Texture parameters to be used this frame in this GUI manager(Unsafe to use outside frame draw/graphics thread)
    std::set<const TextureBase*> texturesToCreate;
    std::set<ShaderParametersRef> texturesUsed;
private:

    static void setClipboard(void* userData, const char* text);
    static const char* getClipboard(void* userData);

    void setupInputs();
    void updateInputs();
    void updateTextureParameters();

    void updateRenderResources(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance
        , const GraphicsHelperAPI* graphicsHelper, const ImGuiDrawingContext& drawingContext, const class LocalPipelineContext& pipelineContext);

    void setupRendering();
    void releaseRendering();
    void setShaderData();
    void setCurrentContexts();
protected:
    TextureBase* getFontTextureAtlas() const;
    SamplerRef getTextureSampler() const;
    ShaderParametersRef getFontAtlasParam() const;
    ShaderParametersRef getTextureParam(const TextureBase* textureUsed);
    ShaderParametersRef createTextureParam(const TextureBase* texture, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper,
        const class LocalPipelineContext& pipelineContext);
    ShaderParametersRef findFreeTextureParam(const TextureBase* textureUsed);

public:
    ImGuiManager() = default;
    ImGuiManager(ImGuiManager* parent);

    void initialize();
    void updateFrame(const float& deltaTime);
    void release();
    void draw(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper, const ImGuiDrawingContext& drawingContext);

    void addFont(const String& fontAssetPath, float fontSize);
    void addLayer(IImGuiLayer* layer);
    void removeLayer(IImGuiLayer* layer);

    bool capturedInputs() const { return bCaptureInput; }
};