#pragma once
#include "ImGuiDrawInterface.h"
#include "../../../Core/Platform/PlatformTypes.h"
#include "../../../Core/String/String.h"
#include "../../../Core/Memory/SmartPointers.h"
#include "../../../Core/Types/Delegates/Delegate.h"
#include "../../../RenderInterface/PlatformIndependentGraphicsTypes.h"
#include "../../../RenderInterface/Resources/BufferedResources.h"

#include <map>
#include <queue>
#include <set>

class TextureBase;
class IImGuiLayer;
class BufferResource;
class ShaderParameters;


class ImGuiManager
{
private:
    static const String TEXTURE_PARAM_NAME;
    static const String IMGUI_SHADER_NAME;
    // Only parent GUI manager data
    SharedPtr<class SamplerInterface> textureSampler;
    TextureBase* textureAtlas;
    SharedPtr<ShaderParameters> imguiFontAtlasParams;

    std::map<const TextureBase*, SharedPtr<ShaderParameters>> textureParams;
    // Inactive free texture params
    std::queue<SharedPtr<ShaderParameters>> freeTextureParams;
    // Texture params accessed last frame, if any from texture params that are not here it goes to inactive free params
    std::set<SharedPtr<ShaderParameters>> activeTextureParams;

    // Unique per GUI manager
    String clipboard;
    bool bCaptureInput;

    ImGuiManager* parentGuiManager;
    ImGuiContext* context;
    ImGuiDrawInterface drawInterface;
    // Per display size
    SharedPtr<class ShaderParameters> imguiTransformParams;
    SwapchainBufferedResource<GraphicsVertexBuffer> vertexBuffer;
    SwapchainBufferedResource<GraphicsIndexBuffer> idxBuffer;

    DelegateHandle textureResizedHnd;

    std::map<int32, std::vector<IImGuiLayer*>, std::greater<int32>> drawLayers;

    // Per frame data

    // Texture parameters to be used this frame in this GUI manager(Unsafe to use outside frame draw/graphics thread)
    std::set<const TextureBase*> texturesToCreate;
    std::set<const ShaderParameters*> texturesUsed;
private:

    static void setClipboard(void* userData, const char* text);
    static const char* getClipboard(void* userData);
    void setupInputs();
    void updateInputs();
    void updateTextureParameters();

    void updateRenderResources(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance
        , const TinyDrawingContext& drawingContext, const class LocalPipelineContext& pipelineContext);

    void setupRendering();
    void releaseRendering();
    void setShaderData();
protected:
    TextureBase* getFontTextureAtlas() const;
    SharedPtr<class SamplerInterface> getTextureSampler() const;
    SharedPtr<ShaderParameters> getFontAtlasParam() const;
    SharedPtr<ShaderParameters> getTextureParam(const TextureBase* textureUsed);
    SharedPtr<ShaderParameters> createTextureParam(const TextureBase* texture, IGraphicsInstance* graphicsInstance,
        const class LocalPipelineContext& pipelineContext);
    SharedPtr<ShaderParameters> findFreeTextureParam(const TextureBase* textureUsed);

public:
    ImGuiManager() = default;
    ImGuiManager(ImGuiManager* parent);

    void initialize();
    void updateFrame(const float& deltaTime);
    void release();
    void draw(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const TinyDrawingContext& drawingContext);

    void addFont(const String& fontAssetPath, float fontSize);
    void addLayer(IImGuiLayer* layer);
    void removeLayer(IImGuiLayer* layer);

    bool capturedInputs() const { return bCaptureInput; }
};