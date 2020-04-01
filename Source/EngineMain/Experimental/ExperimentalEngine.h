#pragma once
#include "../Core/Engine/GameEngine.h"
#include "../RenderInterface/Resources/QueueResource.h"
#include "../RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanSyncResource.h"

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
    class ImageResource* image = nullptr;
    VkImageView imageView = nullptr;
};

class ExperimentalEngine : public GameEngine
{
    class VulkanDevice* vDevice;
    VkDevice device;
    std::vector<QueueResourceBase*>* deviceQueues;
    const class VulkanDebugGraphics* graphicsDbg;

    std::map<EQueueFunction, QueueCommandPool> pools;
    VkCommandBuffer swapchainCmdBuffer;
    SharedPtr<GraphicsFence> vFence;
    void createPools();
    void destroyPools();

    BufferData normalBuffer;
    BufferData texelBuffer;
    void createBuffers();
    void destroyBuffers();

    ImageData texture;
    ImageData rtTexture;
    SharedPtr<class SamplerInterface> commonSampler = nullptr;
    void createImages();
    void destroyImages();

    // Test shader pipeline resources
    VkDescriptorSetLayout descriptorsSetLayout;
    VkDescriptorSet descriptorsSet;

    VkDescriptorPool descriptorsPool;
    void createPipelineResources();
    void destroyPipelineResources();
protected:
    void onStartUp() override;
    void onQuit() override;
    void tickEngine() override;

};