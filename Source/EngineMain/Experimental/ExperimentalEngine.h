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
    class ImageResource* image = nullptr;
    EPixelDataFormat::Type format = EPixelDataFormat::Undefined;
    EPixelSampleCount::Type sampleCount = EPixelSampleCount::SampleCount1;
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
};

class ExperimentalEngine : public GameEngine
{
    class VulkanDevice* vDevice;
    VkDevice device;
    std::vector<QueueResourceBase*>* deviceQueues;
    const class VulkanDebugGraphics* graphicsDbg;

    std::map<EQueueFunction, QueueCommandPool> pools;
    VkCommandBuffer renderPassCmdBuffer;
    SharedPtr<GraphicsFence> cmdSubmitFence;
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
    std::vector<VkClearValue> attachmentsClearColors;// For render pass
    SharedPtr<GraphicsSemaphore> renderpassSemaphore;
    std::vector<SharedPtr<GraphicsSemaphore>> presentWaitOn;
    void createRenderpass();
    void destroyRenderpass();

    VkPipelineLayout pipelineLayout;// Right now not using any descriptors set so same layout for both tri and quad draw pipelines
    PlatformFile pipelineCacheFile;

    BasicPipeline drawTriPipeline;
    BasicPipeline drawQuadPipeline;
    void createPipelineCache();
    void writeAndDestroyPipelineCache();
    void createPipelineForSubpass();
    void destroySubpassPipelines();
    void createTriDrawPipeline();
    void createQuadDrawPipeline();

    // Common to many pipeline stuffs
    VkDescriptorPool descriptorsPool;
    // Since framebuffers with same FrameBufferFormat can be used with different render passes
    // std::map<FramebufferFormat, VkFramebuffer> framebuffers;
    std::vector<VkFramebuffer> framebuffers;

    void createPipelineResources();
    void destroyPipelineResources();

    // End shader pipeline resources
protected:
    void onStartUp() override;
    void onQuit() override;
    void tickEngine() override;

};