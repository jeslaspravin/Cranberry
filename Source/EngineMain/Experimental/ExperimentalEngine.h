#pragma once
#include "../Core/Engine/GameEngine.h"
#include "../RenderInterface/Resources/QueueResource.h"
#include "../RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanSyncResource.h"
#include "../RenderInterface/Resources/MemoryResources.h"

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

struct FramebufferFormat
{
    struct FramebufferAttachmentFormat
    {
        EPixelDataFormat::Type pixelFormat;
        EPixelSampleCount::Type sampleCount;

        bool operator==(const FramebufferAttachmentFormat& otherFormat) const
        {
            return pixelFormat == otherFormat.pixelFormat && sampleCount == otherFormat.sampleCount;
        }
    };

    std::vector<FramebufferAttachmentFormat> attachments;

    bool operator==(const FramebufferFormat& otherFormat) const
    {
        bool isEqual = true;
        if(otherFormat.attachments.size() != attachments.size())
        {
            isEqual = false;
        }
        else
        {
            for (int32 index = 0; index < (int32)attachments.size(); ++index)
            {
                isEqual = isEqual && (attachments[index] == otherFormat.attachments[index]);
            }
        }

        return isEqual;
    }
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
    // Pipeline specific stuffs
    VkDescriptorSetLayout descriptorsSetLayout;
    VkDescriptorSet descriptorsSet;
    void createShaderResDescriptors();
    void destroyShaderResDescriptors();

    VkRenderPass renderPass;
    //void createRenderpass();
    //void destroyRenderpass();

    // Common to many pipeline stuffs
    VkDescriptorPool descriptorsPool;
    // Since framebuffers with same FrameBufferFormat can be used with different render passes
    std::map<FramebufferFormat, VkFramebuffer> framebuffers;

    void createPipelineResources();
    void destroyPipelineResources();

    // End shader pipeline resources
protected:
    void onStartUp() override;
    void onQuit() override;
    void tickEngine() override;

};