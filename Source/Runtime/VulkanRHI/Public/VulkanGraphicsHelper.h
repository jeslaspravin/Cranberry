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
class VulkanDevice;
struct SwapchainInfo;
class VulkanDebugGraphics;
class IVulkanMemoryResources;
class VulkanDescriptorsSetAllocator;

class VulkanGraphicsHelper : public GraphicsHelperAPI
{
public:
#if EXPERIMENTAL
    // Only in experimental branch
    VULKANRHI_EXPORT static VulkanDevice *getVulkanDevice(IGraphicsInstance *graphicsInstance);
    VULKANRHI_EXPORT static VkCommandBuffer getRawCmdBuffer(IGraphicsInstance *graphicsInstance, const GraphicsResource *cmdBuffer);
#endif

    VULKANRHI_EXPORT static VkInstance getInstance(IGraphicsInstance *graphicsInstance);
    VULKANRHI_EXPORT static VkDevice getDevice(const VulkanDevice *vulkanDevice);
    static const VulkanDebugGraphics *debugGraphics(IGraphicsInstance *graphicsInstance);
    static VulkanDescriptorsSetAllocator *getDescriptorsSetAllocator(IGraphicsInstance *graphicsInstance);
#if DEFER_DELETION
    static DeferredDeleter *getDeferredDeleter(IGraphicsInstance *graphicsInstance);
#endif

    static VkSwapchainKHR createSwapchain(
        IGraphicsInstance *graphicsInstance, const GenericWindowCanvas *windowCanvas, SwapchainInfo *swapchainInfo
    );
    static void fillSwapchainImages(
        IGraphicsInstance *graphicsInstance, VkSwapchainKHR swapchain, std::vector<VkImage> *images, std::vector<VkImageView> *imageViews
    );
    static void destroySwapchain(IGraphicsInstance *graphicsInstance, VkSwapchainKHR swapchain);
    static int32 getNextSwapchainImage(
        IGraphicsInstance *graphicsInstance, VkSwapchainKHR swapchain, SemaphoreRef *waitOnSemaphore, FenceRef *waitOnFence = nullptr
    );
    static void presentImage(
        IGraphicsInstance *graphicsInstance, ArrayView<const WindowCanvasRef> canvases, ArrayView<const uint32> imageIndex,
        ArrayView<const SemaphoreRef> waitOnSemaphores
    );
    WindowCanvasRef createWindowCanvas(IGraphicsInstance *graphicsInstance, GenericAppWindow *fromWindow) const final;
    void cacheSurfaceProperties(IGraphicsInstance *graphicsInstance, const WindowCanvasRef &windowCanvas) const final;

    SemaphoreRef createSemaphore(IGraphicsInstance *graphicsInstance, const TChar *semaphoreName) const;
    TimelineSemaphoreRef createTimelineSemaphore(IGraphicsInstance *graphicsInstance, const TChar *semaphoreName) const;
    void waitTimelineSemaphores(
        IGraphicsInstance *graphicsInstance, std::vector<TimelineSemaphoreRef> *semaphores, std::vector<uint64> *waitForValues
    ) const final;

    FenceRef createFence(IGraphicsInstance *graphicsInstance, const TChar *fenceName, bool bIsSignaled = false) const;
    void waitFences(IGraphicsInstance *graphicsInstance, std::vector<FenceRef> *fences, bool waitAll) const final;

    SamplerRef createSampler(IGraphicsInstance *graphicsInstance, SamplerCreateInfo createInfo) const final;
    ESamplerFiltering::Type clampFiltering(
        IGraphicsInstance *graphicsInstance, ESamplerFiltering::Type sampleFiltering, EPixelDataFormat::Type imageFormat
    ) const final;

    static VkBuffer createBuffer(
        IGraphicsInstance *graphicsInstance, const VkBufferCreateInfo &bufferCreateInfo, EPixelDataFormat::Type bufferDataFormat
    );
    static void destroyBuffer(IGraphicsInstance *graphicsInstance, VkBuffer buffer);
    static bool
        allocateBufferResource(IGraphicsInstance *graphicsInstance, IVulkanMemoryResources *memoryResource, bool cpuAccessible);
    static void deallocateBufferResource(IGraphicsInstance *graphicsInstance, IVulkanMemoryResources *memoryResource);
    static VkBufferView createBufferView(IGraphicsInstance *graphicsInstance, const VkBufferViewCreateInfo &viewCreateInfo);
    static void destroyBufferView(IGraphicsInstance *graphicsInstance, VkBufferView view);

    // Normal data buffers
    BufferResourceRef createReadOnlyBuffer(IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;
    // Cannot be used as uniform
    BufferResourceRef createWriteOnlyBuffer(IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;
    // Can be used as both uniform and storage
    BufferResourceRef createReadWriteBuffer(IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;

    // Texels buffers
    BufferResourceRef
        createReadOnlyTexels(IGraphicsInstance *graphicsInstance, EPixelDataFormat::Type texelFormat, uint32 bufferCount = 1) const final;
    // Cannot be used as uniform sampled
    BufferResourceRef createWriteOnlyTexels(
        IGraphicsInstance *graphicsInstance, EPixelDataFormat::Type texelFormat, uint32 bufferCount = 1
    ) const final;
    BufferResourceRef createReadWriteTexels(
        IGraphicsInstance *graphicsInstance, EPixelDataFormat::Type texelFormat, uint32 bufferCount = 1
    ) const final;

    // Other utility buffers
    BufferResourceRef
        createReadOnlyIndexBuffer(IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;
    BufferResourceRef
        createReadOnlyVertexBuffer(IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;

    BufferResourceRef
        createReadOnlyIndirectBuffer(IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;
    BufferResourceRef
        createWriteOnlyIndirectBuffer(IGraphicsInstance *graphicsInstance, uint32 bufferStride, uint32 bufferCount = 1) const final;

    static VkImage
        createImage(IGraphicsInstance *graphicsInstance, VkImageCreateInfo &createInfo, VkFormatFeatureFlags &requiredFeatures);
    static void destroyImage(IGraphicsInstance *graphicsInstance, VkImage image);
    static bool
        allocateImageResource(IGraphicsInstance *graphicsInstance, IVulkanMemoryResources *memoryResource, bool cpuAccessible);
    static void deallocateImageResource(IGraphicsInstance *graphicsInstance, IVulkanMemoryResources *memoryResource);
    static VkImageView createImageView(IGraphicsInstance *graphicsInstance, const VkImageViewCreateInfo &viewCreateInfo);
    static void destroyImageView(IGraphicsInstance *graphicsInstance, VkImageView view);

    // Images
    ImageResourceRef
        createImage(IGraphicsInstance *graphicsInstance, ImageResourceCreateInfo createInfo, bool bIsStaging = false) const final;
    ImageResourceRef
        createCubeImage(IGraphicsInstance *graphicsInstance, ImageResourceCreateInfo createInfo, bool bIsStaging = false) const final;
    ImageResourceRef createRTImage(
        IGraphicsInstance *graphicsInstance, ImageResourceCreateInfo createInfo,
        EPixelSampleCount::Type sampleCount = EPixelSampleCount::SampleCount1
    ) const final;
    ImageResourceRef createCubeRTImage(
        IGraphicsInstance *graphicsInstance, ImageResourceCreateInfo createInfo,
        EPixelSampleCount::Type sampleCount = EPixelSampleCount::SampleCount1
    ) const final;

    void mapResource(IGraphicsInstance *graphicsInstance, BufferResourceRef &buffer) const final;
    void unmapResource(IGraphicsInstance *graphicsInstance, BufferResourceRef &buffer) const final;
    void mapResource(IGraphicsInstance *graphicsInstance, ImageResourceRef &image) const final;
    void unmapResource(IGraphicsInstance *graphicsInstance, ImageResourceRef &image) const final;
    void *borrowMappedPtr(IGraphicsInstance *graphicsInstance, ImageResourceRef &resource) const final;
    void returnMappedPtr(IGraphicsInstance *graphicsInstance, ImageResourceRef &resource) const final;
    void flushMappedPtr(IGraphicsInstance *graphicsInstance, const std::vector<ImageResourceRef> &resources) const final;
    void *borrowMappedPtr(IGraphicsInstance *graphicsInstance, BufferResourceRef &resource) const final;
    void returnMappedPtr(IGraphicsInstance *graphicsInstance, BufferResourceRef &resource) const final;
    void flushMappedPtr(IGraphicsInstance *graphicsInstance, const std::vector<BufferResourceRef> &resources) const final;

    void markForDeletion(
        IGraphicsInstance *graphicsInstance, GraphicsResource *resource, EDeferredDelStrategy deleteStrategy, TickRep duration = 1
    ) const final;
    void markForDeletion(
        IGraphicsInstance *graphicsInstance, SimpleSingleCastDelegate deleter, EDeferredDelStrategy deleteStrategy, TickRep duration = 1
    ) const final;

    // Size in bytes not 4bytes
    static VkShaderModule createShaderModule(IGraphicsInstance *graphicsInstance, const uint8 *code, uint32 size);
    static void destroyShaderModule(IGraphicsInstance *graphicsInstance, VkShaderModule shaderModule);
    static VkRenderPass createDummyRenderPass(IGraphicsInstance *graphicsInstance, const struct Framebuffer *framebuffer);
    static VkRenderPass createRenderPass(
        IGraphicsInstance *graphicsInstance, const struct GenericRenderPassProperties &renderpassProps,
        const struct RenderPassAdditionalProps &additionalProps
    );
    static void destroyRenderPass(IGraphicsInstance *graphicsInstance, VkRenderPass renderPass);

    static void createFramebuffer(IGraphicsInstance *graphicsInstance, VkFramebufferCreateInfo &fbCreateInfo, VkFramebuffer *framebuffer);
    static void destroyFramebuffer(IGraphicsInstance *graphicsInstance, VkFramebuffer framebuffer);
    static VkFramebuffer getFramebuffer(const struct Framebuffer *appFrameBuffer);

    static VkDescriptorSetLayout
        createDescriptorsSetLayout(IGraphicsInstance *graphicsInstance, const VkDescriptorSetLayoutCreateInfo &layoutCreateInfo);
    static VkDescriptorSetLayout getEmptyDescriptorsSetLayout(IGraphicsInstance *graphicsInstance);
    static void destroyDescriptorsSetLayout(IGraphicsInstance *graphicsInstance, VkDescriptorSetLayout descriptorsSetLayout);
    static void updateDescriptorsSet(
        IGraphicsInstance *graphicsInstance, const std::vector<VkWriteDescriptorSet> &writingDescriptors,
        const std::vector<VkCopyDescriptorSet> &copyingDescsSets
    );

    static VkPipelineLayout createPipelineLayout(IGraphicsInstance *graphicsInstance, const PipelineBase *pipeline);
    static void destroyPipelineLayout(IGraphicsInstance *graphicsInstance, const VkPipelineLayout pipelineLayout);

    static VkPipelineCache createPipelineCache(IGraphicsInstance *graphicsInstance, const std::vector<uint8> &cacheData);
    static VkPipelineCache createPipelineCache(IGraphicsInstance *graphicsInstance);
    static void destroyPipelineCache(IGraphicsInstance *graphicsInstance, VkPipelineCache pipelineCache);
    static void
        mergePipelineCaches(IGraphicsInstance *graphicsInstance, VkPipelineCache dstCache, const std::vector<VkPipelineCache> &srcCaches);
    static void getPipelineCacheData(IGraphicsInstance *graphicsInstance, VkPipelineCache pipelineCache, std::vector<uint8> &cacheData);
    static void getMergedCacheData(
        IGraphicsInstance *graphicsInstance, std::vector<uint8> &cacheData, const std::vector<const PipelineBase *> &pipelines
    );

    // Pipelines
    PipelineBase *createGraphicsPipeline(IGraphicsInstance *graphicsInstance, const PipelineBase *parent) const final;
    PipelineBase *createGraphicsPipeline(IGraphicsInstance *graphicsInstance, const GraphicsPipelineConfig &config) const final;

    PipelineBase *createComputePipeline(IGraphicsInstance *graphicsInstance, const PipelineBase *parent) const final;
    PipelineBase *createComputePipeline(IGraphicsInstance *graphicsInstance) const final;

    // Both Shader stage flags and Pipeline stage flags are in Vulkan types
    static VkPipelineStageFlags2 shaderToPipelineStageFlags(uint32 shaderStageFlags);
    static VkShaderStageFlags pipelineToShaderStageFlags(uint64 pipelineStageFlags);

    static std::vector<VkPipeline> createGraphicsPipeline(
        IGraphicsInstance *graphicsInstance, const std::vector<VkGraphicsPipelineCreateInfo> &graphicsPipelineCI,
        VkPipelineCache pipelineCache
    );
    static std::vector<VkPipeline> createComputePipeline(
        IGraphicsInstance *graphicsInstance, const std::vector<VkComputePipelineCreateInfo> &computePipelineCI,
        VkPipelineCache pipelineCache
    );

    static void destroyPipeline(IGraphicsInstance *graphicsInstance, VkPipeline pipeline);

    // Application specific
    GlobalRenderingContextBase *createGlobalRenderingContext() const final;
    ShaderResource *createShaderResource(const ShaderConfigCollector *inConfig) const final;
    ShaderParametersRef createShaderParameters(
        IGraphicsInstance *graphicsInstance, const GraphicsResource *paramLayout, const std::set<uint32> &ignoredSetIds = {}
    ) const final;

    Framebuffer *createFbInstance() const final;
    void initializeFb(IGraphicsInstance *graphicsInstance, Framebuffer *fb, const Size2D &frameSize) const final;
    void initializeSwapchainFb(IGraphicsInstance *graphicsInstance, Framebuffer *fb, WindowCanvasRef canvas, uint32 swapchainIdx)
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
