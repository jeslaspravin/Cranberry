/*!
 * \file VulkanGraphicsHelper.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "RenderInterface/CoreGraphicsTypes.h"
#include "RenderInterface/GraphicsHelper.h"
#include "VulkanRHIExports.h"

#include <vulkan_core.h>

class GenericAppWindow;
class GenericWindowCanvas;
class DeferredDeleter;

class VulkanGraphicsHelper : public GraphicsHelperAPI
{
public:
#if EXPERIMENTAL
    // Only in experimental branch
    VULKANRHI_EXPORT static class VulkanDevice *getVulkanDevice(class IGraphicsInstance *graphicsInstance);
    VULKANRHI_EXPORT static VkCommandBuffer getRawCmdBuffer(class IGraphicsInstance *graphicsInstance, const GraphicsResource *cmdBuffer);
#endif

    VULKANRHI_EXPORT static VkInstance getInstance(class IGraphicsInstance *graphicsInstance);
    VULKANRHI_EXPORT static VkDevice getDevice(const class VulkanDevice *vulkanDevice);
    static const class VulkanDebugGraphics *debugGraphics(class IGraphicsInstance *graphicsInstance);
    static class VulkanDescriptorsSetAllocator *getDescriptorsSetAllocator(class IGraphicsInstance *graphicsInstance);
#if DEFER_DELETION
    static DeferredDeleter *getDeferredDeleter(class IGraphicsInstance *graphicsInstance);
#endif

    static VkSwapchainKHR createSwapchain(
        class IGraphicsInstance *graphicsInstance, const GenericWindowCanvas *windowCanvas, struct SwapchainInfo *swapchainInfo
    );
    static void fillSwapchainImages(
        class IGraphicsInstance *graphicsInstance, VkSwapchainKHR swapchain, std::vector<VkImage> *images, std::vector<VkImageView> *imageViews
    );
    static void destroySwapchain(class IGraphicsInstance *graphicsInstance, VkSwapchainKHR swapchain);
    static uint32 getNextSwapchainImage(
        class IGraphicsInstance *graphicsInstance, VkSwapchainKHR swapchain, SemaphoreRef *waitOnSemaphore, FenceRef *waitOnFence = nullptr
    );
    static void presentImage(
        class IGraphicsInstance *graphicsInstance, const std::vector<WindowCanvasRef> *canvases, const std::vector<uint32> *imageIndex,
        const std::vector<SemaphoreRef> *waitOnSemaphores
    );
    WindowCanvasRef createWindowCanvas(class IGraphicsInstance *graphicsInstance, GenericAppWindow *fromWindow) const final;
    void cacheSurfaceProperties(class IGraphicsInstance *graphicsInstance, const WindowCanvasRef &windowCanvas) const final;

    SemaphoreRef createSemaphore(class IGraphicsInstance *graphicsInstance, const TChar *semaphoreName) const;
    TimelineSemaphoreRef createTimelineSemaphore(class IGraphicsInstance *graphicsInstance, const TChar *semaphoreName) const;
    void waitTimelineSemaphores(
        class IGraphicsInstance *graphicsInstance, std::vector<TimelineSemaphoreRef> *semaphores, std::vector<uint64> *waitForValues
    ) const final;

    FenceRef createFence(class IGraphicsInstance *graphicsInstance, const TChar *fenceName, bool bIsSignaled = false) const;
    void waitFences(class IGraphicsInstance *graphicsInstance, std::vector<FenceRef> *fences, bool waitAll) const final;

    SamplerRef createSampler(class IGraphicsInstance *graphicsInstance, SamplerCreateInfo createInfo) const final;
    ESamplerFiltering::Type clampFiltering(
        class IGraphicsInstance *graphicsInstance, ESamplerFiltering::Type sampleFiltering, EPixelDataFormat::Type imageFormat
    ) const final;

    static VkBuffer createBuffer(
        class IGraphicsInstance *graphicsInstance, const VkBufferCreateInfo &bufferCreateInfo, EPixelDataFormat::Type bufferDataFormat
    );
    static void destroyBuffer(class IGraphicsInstance *graphicsInstance, VkBuffer buffer);
    static bool
        allocateBufferResource(class IGraphicsInstance *graphicsInstance, class IVulkanMemoryResources *memoryResource, bool cpuAccessible);
    static void deallocateBufferResource(class IGraphicsInstance *graphicsInstance, class IVulkanMemoryResources *memoryResource);
    static VkBufferView createBufferView(class IGraphicsInstance *graphicsInstance, const VkBufferViewCreateInfo &viewCreateInfo);
    static void destroyBufferView(class IGraphicsInstance *graphicsInstance, VkBufferView view);

    // Normal data buffers
    BufferResourceRef createReadOnlyBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;
    // Cannot be used as uniform
    BufferResourceRef createWriteOnlyBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;
    // Can be used as both uniform and storage
    BufferResourceRef createReadWriteBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;

    // Texels buffers
    BufferResourceRef
        createReadOnlyTexels(class IGraphicsInstance *graphicsInstance, EPixelDataFormat::Type texelFormat, uint32 bufferCount = 1) const final;
    // Cannot be used as uniform sampled
    BufferResourceRef createWriteOnlyTexels(
        class IGraphicsInstance *graphicsInstance, EPixelDataFormat::Type texelFormat, uint32 bufferCount = 1
    ) const final;
    BufferResourceRef createReadWriteTexels(
        class IGraphicsInstance *graphicsInstance, EPixelDataFormat::Type texelFormat, uint32 bufferCount = 1
    ) const final;

    // Other utility buffers
    BufferResourceRef
        createReadOnlyIndexBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;
    BufferResourceRef
        createReadOnlyVertexBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;

    BufferResourceRef
        createReadOnlyIndirectBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;
    BufferResourceRef
        createWriteOnlyIndirectBuffer(class IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;

    static VkImage
        createImage(class IGraphicsInstance *graphicsInstance, VkImageCreateInfo &createInfo, VkFormatFeatureFlags &requiredFeatures);
    static void destroyImage(class IGraphicsInstance *graphicsInstance, VkImage image);
    static bool
        allocateImageResource(class IGraphicsInstance *graphicsInstance, class IVulkanMemoryResources *memoryResource, bool cpuAccessible);
    static void deallocateImageResource(class IGraphicsInstance *graphicsInstance, class IVulkanMemoryResources *memoryResource);
    static VkImageView createImageView(class IGraphicsInstance *graphicsInstance, const VkImageViewCreateInfo &viewCreateInfo);
    static void destroyImageView(class IGraphicsInstance *graphicsInstance, VkImageView view);

    // Images
    ImageResourceRef
        createImage(class IGraphicsInstance *graphicsInstance, ImageResourceCreateInfo createInfo, bool bIsStaging = false) const final;
    ImageResourceRef
        createCubeImage(class IGraphicsInstance *graphicsInstance, ImageResourceCreateInfo createInfo, bool bIsStaging = false) const final;
    ImageResourceRef createRTImage(
        class IGraphicsInstance *graphicsInstance, ImageResourceCreateInfo createInfo,
        EPixelSampleCount::Type sampleCount = EPixelSampleCount::SampleCount1
    ) const final;
    ImageResourceRef createCubeRTImage(
        class IGraphicsInstance *graphicsInstance, ImageResourceCreateInfo createInfo,
        EPixelSampleCount::Type sampleCount = EPixelSampleCount::SampleCount1
    ) const final;

    void mapResource(class IGraphicsInstance *graphicsInstance, BufferResourceRef &buffer) const final;
    void unmapResource(class IGraphicsInstance *graphicsInstance, BufferResourceRef &buffer) const final;
    void mapResource(class IGraphicsInstance *graphicsInstance, ImageResourceRef &image) const final;
    void unmapResource(class IGraphicsInstance *graphicsInstance, ImageResourceRef &image) const final;
    void *borrowMappedPtr(class IGraphicsInstance *graphicsInstance, ImageResourceRef &resource) const final;
    void returnMappedPtr(class IGraphicsInstance *graphicsInstance, ImageResourceRef &resource) const final;
    void flushMappedPtr(class IGraphicsInstance *graphicsInstance, const std::vector<ImageResourceRef> &resources) const final;
    void *borrowMappedPtr(class IGraphicsInstance *graphicsInstance, BufferResourceRef &resource) const final;
    void returnMappedPtr(class IGraphicsInstance *graphicsInstance, BufferResourceRef &resource) const final;
    void flushMappedPtr(class IGraphicsInstance *graphicsInstance, const std::vector<BufferResourceRef> &resources) const final;

    void markForDeletion(
        class IGraphicsInstance *graphicsInstance, GraphicsResource *resource, EDeferredDelStrategy deleteStrategy, TickRep duration = 1
    ) const final;
    void markForDeletion(
        class IGraphicsInstance *graphicsInstance, SimpleSingleCastDelegate deleter, EDeferredDelStrategy deleteStrategy, TickRep duration = 1
    ) const final;

    // Size in bytes not 4bytes
    static VkShaderModule createShaderModule(class IGraphicsInstance *graphicsInstance, const uint8 *code, uint32 size);
    static void destroyShaderModule(class IGraphicsInstance *graphicsInstance, VkShaderModule shaderModule);
    static VkRenderPass createDummyRenderPass(class IGraphicsInstance *graphicsInstance, const struct Framebuffer *framebuffer);
    static VkRenderPass createRenderPass(
        class IGraphicsInstance *graphicsInstance, const struct GenericRenderPassProperties &renderpassProps,
        const struct RenderPassAdditionalProps &additionalProps
    );
    static void destroyRenderPass(class IGraphicsInstance *graphicsInstance, VkRenderPass renderPass);

    static void createFramebuffer(class IGraphicsInstance *graphicsInstance, VkFramebufferCreateInfo &fbCreateInfo, VkFramebuffer *framebuffer);
    static void destroyFramebuffer(class IGraphicsInstance *graphicsInstance, VkFramebuffer framebuffer);
    static VkFramebuffer getFramebuffer(const struct Framebuffer *appFrameBuffer);

    static VkDescriptorSetLayout
        createDescriptorsSetLayout(class IGraphicsInstance *graphicsInstance, const VkDescriptorSetLayoutCreateInfo &layoutCreateInfo);
    static VkDescriptorSetLayout getEmptyDescriptorsSetLayout(class IGraphicsInstance *graphicsInstance);
    static void destroyDescriptorsSetLayout(class IGraphicsInstance *graphicsInstance, VkDescriptorSetLayout descriptorsSetLayout);
    static void updateDescriptorsSet(
        class IGraphicsInstance *graphicsInstance, const std::vector<VkWriteDescriptorSet> &writingDescriptors,
        const std::vector<VkCopyDescriptorSet> &copyingDescsSets
    );

    static VkPipelineLayout createPipelineLayout(class IGraphicsInstance *graphicsInstance, const class PipelineBase *pipeline);
    static void destroyPipelineLayout(class IGraphicsInstance *graphicsInstance, const VkPipelineLayout pipelineLayout);

    static VkPipelineCache createPipelineCache(class IGraphicsInstance *graphicsInstance, const std::vector<uint8> &cacheData);
    static VkPipelineCache createPipelineCache(class IGraphicsInstance *graphicsInstance);
    static void destroyPipelineCache(class IGraphicsInstance *graphicsInstance, VkPipelineCache pipelineCache);
    static void
        mergePipelineCaches(class IGraphicsInstance *graphicsInstance, VkPipelineCache dstCache, const std::vector<VkPipelineCache> &srcCaches);
    static void getPipelineCacheData(class IGraphicsInstance *graphicsInstance, VkPipelineCache pipelineCache, std::vector<uint8> &cacheData);
    static void getMergedCacheData(
        class IGraphicsInstance *graphicsInstance, std::vector<uint8> &cacheData, const std::vector<const class PipelineBase *> &pipelines
    );

    // Pipelines
    PipelineBase *createGraphicsPipeline(class IGraphicsInstance *graphicsInstance, const PipelineBase *parent) const final;
    PipelineBase *createGraphicsPipeline(class IGraphicsInstance *graphicsInstance, const GraphicsPipelineConfig &config) const final;

    PipelineBase *createComputePipeline(class IGraphicsInstance *graphicsInstance, const PipelineBase *parent) const final;
    PipelineBase *createComputePipeline(class IGraphicsInstance *graphicsInstance) const final;

    // Both Shader stage flags and Pipeline stage flags are in Vulkan types
    static VkPipelineStageFlags2KHR shaderToPipelineStageFlags(uint32 shaderStageFlags);
    static VkShaderStageFlags pipelineToShaderStageFlags(uint32 pipelineStageFlags);

    static std::vector<VkPipeline> createGraphicsPipeline(
        class IGraphicsInstance *graphicsInstance, const std::vector<VkGraphicsPipelineCreateInfo> &graphicsPipelineCI,
        VkPipelineCache pipelineCache
    );
    static std::vector<VkPipeline> createComputePipeline(
        class IGraphicsInstance *graphicsInstance, const std::vector<VkComputePipelineCreateInfo> &computePipelineCI,
        VkPipelineCache pipelineCache
    );

    static void destroyPipeline(class IGraphicsInstance *graphicsInstance, VkPipeline pipeline);

    // Application specific
    GlobalRenderingContextBase *createGlobalRenderingContext() const final;
    ShaderResource *createShaderResource(const ShaderConfigCollector *inConfig) const final;
    ShaderParametersRef createShaderParameters(
        class IGraphicsInstance *graphicsInstance, const class GraphicsResource *paramLayout, const std::set<uint32> &ignoredSetIds = {}
    ) const final;

    Framebuffer *createFbInstance() const final;
    void initializeFb(class IGraphicsInstance *graphicsInstance, Framebuffer *fb, const Size2D &frameSize) const final;
    void initializeSwapchainFb(class IGraphicsInstance *graphicsInstance, Framebuffer *fb, WindowCanvasRef canvas, uint32 swapchainIdx)
        const final;

    const GraphicsResourceType *readOnlyBufferType() const final;
    const GraphicsResourceType *writeOnlyBufferType() const final;
    const GraphicsResourceType *readWriteBufferType() const final;
    const GraphicsResourceType *readOnlyTexelsType() const final;
    const GraphicsResourceType *writeOnlyTexelsType() const final;
    const GraphicsResourceType *readWriteTexelsType() const final;
    const GraphicsResourceType *readOnlyIndexBufferType() const final;
    const GraphicsResourceType *readOnlyVertexBufferType() const final;
    const GraphicsResourceType *readOnlyIndirectBufferType() const final;
    const GraphicsResourceType *writeOnlyIndirectBufferType() const final;

    const GraphicsResourceType *imageType() const final;
    const GraphicsResourceType *cubeImageType() const final;
    const GraphicsResourceType *rtImageType() const final;
    const GraphicsResourceType *cubeRTImageType() const final;
};
