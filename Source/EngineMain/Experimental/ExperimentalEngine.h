#pragma once
#include "../Core/Engine/GameEngine.h"
#include "../RenderInterface/Resources/QueueResource.h"
#include "../RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanSyncResource.h"
#include "../RenderInterface/Resources/MemoryResources.h"
#include "../Core/Platform/LFS/PlatformLFS.h"

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

struct BasicPipeline
{
    VkPipeline pipeline;
    VkPipelineCache cache;
    VkPipelineLayout layout;
};

struct FrameResource
{
    std::vector<SharedPtr<GraphicsSemaphore>> usageWaitSemaphore;
    VkCommandBuffer perFrameCommands;
    ImageData rtTexture;
    VkFramebuffer frameBuffer;
    VkDescriptorSet iAttachSetSubpass1;
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

    BufferData normalBuffer;
    BufferData texelBuffer;
    void createBuffers();
    void writeBuffers();
    void destroyBuffers();

    ImageData texture;
    SharedPtr<class SamplerInterface> commonSampler = nullptr;
    void createImages();
    void writeImages();
    void destroyImages();

    // Test shader pipeline resources
    // Pipeline specific stuffs
    VkDescriptorSetLayout descriptorsSetLayout;
    VkDescriptorSet descriptorsSet;
    void createShaderResDescriptors();
    void destroyShaderResDescriptors();

    VkRenderPass renderPass;
    std::vector<VkClearValue> attachmentsClearColors;// For render pass
    VkDescriptorSetLayout subpass1DescLayout;
    void createInputAttachmentDescriptors();
    void destroyInputAttachmentDescriptors();
    void createFrameResources();
    void destroyFrameResources();
    void createRenderpass();
    void destroyRenderpass();

    PlatformFile pipelineCacheFile;
    BasicPipeline drawTriPipeline;
    class BufferResource* quadVertexBuffer = nullptr;
    class BufferResource* quadIndexBuffer = nullptr;
    BasicPipeline drawQuadPipeline;
    void createPipelineCache();
    void writeAndDestroyPipelineCache();
    void createPipelineForSubpass();
    void destroySubpassPipelines();
    void createTriDrawPipeline();
    void createQuadDrawPipeline();

    // Common to many pipeline stuffs
    VkDescriptorPool descriptorsPool;
    // Since frame buffers with same FrameBufferFormat can be used with different render passes
    // std::map<FramebufferFormat, VkFramebuffer> framebuffers;
    std::vector<FrameResource> frameResources;

    void createPipelineResources();
    void destroyPipelineResources();

    // End shader pipeline resources

    float rotationOffset = 0;
protected:
    void onStartUp() override;
    void onQuit() override;
    void tickEngine() override;

    void tempTest();
    void tempTestPerFrame();
};