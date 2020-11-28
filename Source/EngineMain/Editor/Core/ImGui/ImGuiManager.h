#pragma once
#include "ImGuiDrawInterface.h"
#include "../../../Core/Platform/PlatformTypes.h"
#include "../../../Core/String/String.h"
#include "../../../Core/Memory/SmartPointers.h"
#include "../../../Core/Types/Delegates/Delegate.h"
#include "../../../RenderInterface/PlatformIndependentGraphicsTypes.h"
#include "../../../RenderInterface/Resources/BufferedResources.h"

#include <map>
#include <vector>

class TextureBase;
class IImGuiLayer;
class BufferResource;


class ImGuiManager
{
private:

// In Seconds
    String clipboard;

    ImGuiManager* parentGuiManager;
    ImGuiContext* context;
    ImGuiDrawInterface drawInterface;
    SharedPtr<class SamplerInterface> textureSampler;
    TextureBase* textureAtlas;
    SwapchainBufferedResource<GraphicsVertexBuffer> vertexBuffer;
    SwapchainBufferedResource<GraphicsIndexBuffer> idxBuffer;

    DelegateHandle textureResizedHnd;
    SharedPtr<class ShaderParameters> imguiShaderParams;

    std::map<int32, std::vector<IImGuiLayer*>, std::greater<int32>> drawLayers;
private:

    static void setClipboard(void* userData, const char* text);
    static const char* getClipboard(void* userData);
    void setupInputs();
    void updateInputs();

    void setupRendering();
    void releaseRendering();
    void setShaderData();
protected:
    TextureBase* getTextureAtlas() const;
    SharedPtr<class SamplerInterface> getTextureSampler() const;
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
};