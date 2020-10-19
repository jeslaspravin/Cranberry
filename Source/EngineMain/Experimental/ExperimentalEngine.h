#pragma once
#include "../Core/Engine/GameEngine.h"
#include "../RenderInterface/Resources/QueueResource.h"
#include "../RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanSyncResource.h"
#include "../RenderInterface/Resources/MemoryResources.h"
#include "../RenderInterface/ShaderCore/ShaderParameters.h"
#include "../Core/Platform/LFS/PlatformLFS.h"
#include "../Core/Types/Transform3D.h"
#include "../Core/Types/Camera/Camera.h"
#include "../Core/Types/Colors.h"
#include "../RenderInterface/Rendering/RenderingContexts.h"

#include <map>
#include <vulkan_core.h>

class ShaderParameters;
class IRenderCommandList;

struct QueueCommandPool
{
    VkCommandPool tempCommandsPool;
    VkCommandPool resetableCommandPool;
    VkCommandPool oneTimeRecordPool;
};

struct TexelBuffer
{
    class BufferResource* buffer = nullptr;
    // Only necessary for texel buffers
    VkBufferView bufferView = nullptr;
};

struct ImageData
{
    class TextureBase* image = nullptr;
    VkImageView imageView = nullptr;
};

struct SceneEntity
{
    Transform3D transform;
    class StaticMeshAsset* meshAsset;

    SharedPtr<ShaderParameters> instanceParameters;
    std::vector<LinearColor> meshBatchColors;
    std::vector<SharedPtr<ShaderParameters>> meshBatchParameters;
};

struct FrameResource
{
    std::vector<SharedPtr<GraphicsSemaphore>> usageWaitSemaphore;
    VkCommandBuffer perFrameCommands;
    RenderTargetTexture* lightingPassRt;
    SharedPtr<GraphicsFence> recordingFence;
};

class ExperimentalEngine : public GameEngine
{
    class VulkanDevice* vDevice;
    VkDevice device;
    const class VulkanDebugGraphics* graphicsDbg;

    std::map<EQueueFunction, QueueCommandPool> pools;
    void createPools();
    void destroyPools();

    ImageData texture;
    SharedPtr<class SamplerInterface> nearestFiltering = nullptr;
    SharedPtr<class SamplerInterface> linearFiltering = nullptr;
    // TODO(Jeslas) : Cubic filtering not working check new drivers or log bug in nvidia
    // SharedPtr<class SamplerInterface> cubicFiltering = nullptr;
    void createImages();
    void destroyImages();

    // Scene data
    std::vector<SceneEntity> sceneData;
    std::vector<std::pair<struct GoochModelLightData, SharedPtr<ShaderParameters>>> lightData;
    SharedPtr<ShaderParameters> lightCommon;
    std::vector<SharedPtr<ShaderParameters>> lightTextures;
    SharedPtr<ShaderParameters> viewParameters;
    void createScene();
    void destroyScene();

    // Camera parameters
    Camera camera;
    Vector3D cameraTranslation;
    Rotation cameraRotation;
    void updateCameraParams();

    std::vector<SharedPtr<ShaderParameters>> drawQuadTextureDescs;
    std::vector<SharedPtr<ShaderParameters>> drawQuadNormalDescs;
    std::vector<SharedPtr<ShaderParameters>> drawQuadDepthDescs;
    std::vector<SharedPtr<ShaderParameters>> drawLitColorsDescs;

    void createShaderParameters();
    void setupShaderParameterParams();
    void updateShaderParameters(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance);
    void destroyShaderParameters();

    void resizeLightingRts(const Size2D& size);
    void reupdateTextureParamsOnResize();

    // Shader pipeline resources
    std::vector<VkClearValue> smAttachmentsClearColors;

    VkClearValue swapchainClearColor;
    void createFrameResources();
    void destroyFrameResources();

    VkRenderPass drawSmRenderPass;
    LocalPipelineContext drawSmPipelineContext;

    VkRenderPass lightingRenderPass;
    LocalPipelineContext drawGoochPipelineContext;

    class BufferResource* quadVertexBuffer = nullptr;
    class BufferResource* quadIndexBuffer = nullptr;
    VkRenderPass drawQuadRenderPass;
    LocalPipelineContext drawQuadPipelineContext;

    SharedPtr<ShaderParameters> clearInfoParams;
    LocalPipelineContext clearQuadPipelineContext;
     
    void getPipelineForSubpass();

    std::vector<FrameResource> frameResources;
    void createPipelineResources();
    void destroyPipelineResources();

    // End shader pipeline resources

    uint32 frameVisualizeId = 0;// 0 color 1 normal 2 depth
    bool toggleRes;

protected:
    void onStartUp() override;
    void onQuit() override;
    void tickEngine() override;

    void startUpRenderInit();
    void renderQuit();
    void frameRender();

    void tempTest();
    void tempTestPerFrame();
};