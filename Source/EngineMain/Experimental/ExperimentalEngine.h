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

#include <map>
#include <vulkan_core.h>

struct QueueCommandPool
{
    VkCommandPool tempCommandsPool;
    VkCommandPool resetableCommandPool;
    VkCommandPool oneTimeRecordPool;
};

struct BufferData
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


struct BasicPipeline
{
    VkPipeline pipeline;
    VkPipelineCache cache;
    VkPipelineLayout layout;
};

struct DescSetInfo
{
    std::map<String, uint32> descBindingNames;
    std::vector<VkDescriptorPoolSize> descLayoutInfo;
    VkDescriptorSetLayout descLayout;
    VkDescriptorSet descSet;
};

struct FrameResource
{
    std::vector<SharedPtr<GraphicsSemaphore>> usageWaitSemaphore;
    VkCommandBuffer perFrameCommands;
    SharedPtr<GraphicsFence> recordingFence;
};

class ExperimentalEngine : public GameEngine
{
    class VulkanDevice* vDevice;
    VkDevice device;
    std::vector<QueueResourceBase*>* deviceQueues;
    const class VulkanDebugGraphics* graphicsDbg;

    std::map<EQueueFunction, QueueCommandPool> pools;
    SharedPtr<GraphicsFence> cmdSubmitFence;
    void createPools();
    void destroyPools();

    BufferData viewBuffer;
    BufferData instanceBuffer;
    void createBuffers();
    void writeBuffers();
    void destroyBuffers();

    ImageData texture;
    ImageData normalTexture;
    SharedPtr<class SamplerInterface> commonSampler = nullptr;
    void createImages();
    void destroyImages();

    // Test shader pipeline resources
    // Pipeline specific stuffs
    std::map<String, ImageData*> smTextureBinding;
    std::map<String, ShaderBufferParamInfo*> smUniformBinding;
    std::vector<DescSetInfo> staticMeshDescs;

    std::vector<std::vector<DescSetInfo>> drawQuadTextureDescs;
    std::vector<std::vector<DescSetInfo>> drawQuadNormalDescs;
    std::vector<std::vector<DescSetInfo>> drawQuadDepthDescs;
    void fillBindings();

    void createShaderResDescriptors();
    void writeUnlitBuffToQuadDrawDescs();
    void destroyShaderResDescriptors();

    VkRenderPass smRenderPass;
    std::vector<VkClearValue> smAttachmentsClearColors;

    VkRenderPass swapchainRenderPass;
    VkClearValue swapchainClearColor;
    void createFrameResources();
    void destroyFrameResources();
    void createRenderpass();
    void destroyRenderpass();

    BasicPipeline drawSmPipeline;

    class BufferResource* quadVertexBuffer = nullptr;
    class BufferResource* quadIndexBuffer = nullptr;
    BasicPipeline drawQuadPipeline;
    PlatformFile pipelineCacheFile;
    void createPipelineCache();
    void writeAndDestroyPipelineCache();
    void createPipelineForSubpass();
    void destroySubpassPipelines();
    void createSmPipeline();
    void createQuadDrawPipeline();

    std::vector<FrameResource> frameResources;

    void createPipelineResources();
    void destroyPipelineResources();

    // End shader pipeline resources

    // Camera parameters
    Camera camera;
    void updateCameraParams();

    float distanceOffset = 0;
    float rotationOffset = 0;
    uint32 frameVisualizeId = 0;// 0 color 1 normal 2 depth
    bool toggleRes;
    bool useSuzanne;
    float useVertexColor = 0.0f;

    class StaticMeshAsset* meshAsset;
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