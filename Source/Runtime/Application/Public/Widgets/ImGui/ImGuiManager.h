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

#include "ApplicationExports.h"
#include "InputSystem/Keys.h"
#include "ImGuiDrawInterface.h"
#include "Memory/SmartPointers.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "String/String.h"
#include "Types/CoreTypes.h"

#include <map>
#include <queue>
#include <set>

class IImGuiLayer;

class IRenderTargetTexture;
class IGraphicsInstance;
struct ImGuiContext;
struct ImPlotContext;
class GraphicsHelperAPI;
class InputSystem;
class LocalPipelineContext;
class WidgetBase;
class WgWindow;

struct ImGuiDrawingContext
{
    const GraphicsResource *cmdBuffer;
    IRenderTargetTexture *rtTexture;
    QuantizedBox2D viewport;
};

class APPLICATION_EXPORT ImGuiManager
{
private:
    static const StringID TEXTURE_PARAM_NAME;
    static const NameString IMGUI_SHADER_NAME;

    // Only parent GUI manager data
    ImageResourceRef textureAtlas;
    ShaderParametersRef imguiFontAtlasParams;
    SharedPtr<WgWindow> wgWindow;

    std::map<ImageResourceRef, ShaderParametersRef> textureParams;
    // Inactive free texture params
    std::queue<ShaderParametersRef> freeTextureParams;
    /**
     * Texture params accessed last frame, if any from texture params that are not here it goes to
     * inactive free params
     */ 
    std::set<ShaderParametersRef> activeTextureParams;

    // Unique per GUI manager
    std::string clipboard;
    bool bCaptureInput;

    std::string name;
    SharedPtr<WidgetBase> widget;

    ImGuiManager *parentGuiManager;
    ImGuiContext *context;
    ImPlotContext *implotContext;
    ImGuiDrawInterface drawInterface;
    // Per display size
    ShaderParametersRef imguiTransformParams;
    // No need for per swapchain resource as vertices will be deleted and not mutated
    BufferResourceRef vertexBuffer;
    BufferResourceRef idxBuffer;

    std::map<int32, std::vector<IImGuiLayer *>, std::greater<int32>> drawLayers;

    // Per frame data

    // Texture parameters to be used this frame in this GUI manager(Unsafe to use outside frame
    // draw/graphics thread)
    std::set<ImageResourceRef> texturesToCreate;
    std::set<ShaderParametersRef> texturesUsed;

private:
    static void setClipboard(void *userData, const char *text);
    static const char *getClipboard(void *userData);

    // Main thread functions
    void setupInputs();
    void updateMouse(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem);
    void updateTextureParameters();
    void setCurrentContext();

    void setShaderData();
    void recreateFontAtlas(class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    void updateRenderResources(
        class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper,
        const LocalPipelineContext &pipelineContext
    );

    void setupRendering();
    void releaseRendering();

protected:
    ImageResourceRef getFontTextureAtlas() const;
    ShaderParametersRef getFontAtlasParam() const;
    ShaderParametersRef getTextureParam(ImageResourceRef textureUsed);
    ShaderParametersRef createTextureParam(
        ImageResourceRef texture, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper,
        const LocalPipelineContext &pipelineContext
    );
    ShaderParametersRef findFreeTextureParam(ImageResourceRef textureUsed);
    SharedPtr<WgWindow> getWindowWidget() const;

public:
    ImGuiManager() = delete;
    ImGuiManager(const TChar *managerName, ImGuiManager *parent, SharedPtr<WidgetBase> inWidget);
    ImGuiManager(const TChar *managerName, SharedPtr<WidgetBase> inWidget);

    void initialize();
    void release();

    void draw(
        class IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper,
        const ImGuiDrawingContext &drawingContext
    );
    void updateFrame(float deltaTime);
    void setDisplaySize(Short2D newSize);
    bool inputKey(Keys::StateKeyType key, Keys::StateInfoType state, const InputSystem *inputSystem);
    void mouseEnter(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem);
    void mouseMoved(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem);
    void mouseLeave(Short2D absPos, Short2D widgetRelPos, const InputSystem *inputSystem);

    void addFont(const String &fontAssetPath, float fontSize);
    void addLayer(IImGuiLayer *layer);
    void removeLayer(IImGuiLayer *layer);

    FORCE_INLINE bool capturedInputs() const { return bCaptureInput; }
};