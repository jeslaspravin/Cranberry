#pragma once
#include "../../../RenderInterface/Rendering/IRenderCommandList.h"
#include "VulkanCommandBufferManager.h"

class IGraphicsInstance;
class VulkanDevice;

class VulkanCommandList : public IRenderCommandList
{
private:
    IGraphicsInstance* gInstance;
    VulkanDevice* vDevice;

    VulkanCmdBufferManager cmdBufferManager;
    VulkanResourcesTracker resourcesTracker;
    // command buffer in which swapchain frame buffers are written to
    std::vector<const GraphicsResource*> swapchainFrameWrites;

    FORCE_INLINE VkImageAspectFlags determineImageAspect(const ImageResource* image) const;
    // Determines mask that has info on how image can be access in pipelines
    FORCE_INLINE VkAccessFlags determineImageAccessMask(const ImageResource* image) const;
    // Determines the image layout if layout is yet to be defined
    FORCE_INLINE VkImageLayout determineImageLayout(const ImageResource* image) const;
    FORCE_INLINE VkImageLayout getImageLayout(const ImageResource* image) const;
    FORCE_INLINE VkPipelineBindPoint getPipelineBindPoint(const PipelineBase* pipeline) const;
    // Shader stage in which buffer/image maybe possibly written to/Read from in shader
    FORCE_INLINE VkPipelineStageFlags resourceShaderStageFlags() const;

    FORCE_INLINE void fillClearValue(EPixelDataFormat::Type format, VkClearColorValue& clearValue, const LinearColor& color) const;

    void copyToImage_Internal(ImageResource* dst, const BufferResource* pixelData, CopyPixelsToImageInfo copyInfo);
    void copyToBuffer_Internal(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size, bool bFlushMemory = false);
public:

    VulkanCommandList(IGraphicsInstance* graphicsInstance, VulkanDevice* vulkanDevice);
    ~VulkanCommandList() = default;

    void newFrame() final;

    void copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size) final;
    void copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo) final;
    void copyToBuffer(const std::vector<BatchCopyBufferData>& batchCopies) final;

    void copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyPixelsToImageInfo& copyInfo) final;
    void copyToImage(ImageResource* dst, const std::vector<class LinearColor>& pixelData, const CopyPixelsToImageInfo& copyInfo) final;
    void copyToImageLinearMapped(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyPixelsToImageInfo& copyInfo) final;
    void copyOrResolveImage(ImageResource* src, ImageResource* dst, const CopyImageInfo& srcInfo, const CopyImageInfo& dstInfo) final;

    void clearImage(ImageResource* image, const LinearColor& clearColor, const std::vector<ImageSubresource>& subresources) final;
    void clearDepth(ImageResource* image, float depth, uint32 stencil, const std::vector<ImageSubresource>& subresources) final;

    void setupInitialLayout(ImageResource* image) final;

    void presentImage(const std::vector<class GenericWindowCanvas*>& canvases,
        const std::vector<uint32>& imageIndices, const std::vector<SharedPtr<class GraphicsSemaphore>>& waitOnSemaphores) final;

    void cmdCopyOrResolveImage(const GraphicsResource* cmdBuffer, ImageResource* src, ImageResource* dst, const CopyImageInfo& srcInfo, const CopyImageInfo& dstInfo) final;

    void cmdTransitionLayouts(const GraphicsResource* cmdBuffer, const std::vector<ImageResource*>& images) final;

    void cmdClearImage(const GraphicsResource* cmdBuffer, ImageResource* image, const LinearColor& clearColor, const std::vector<ImageSubresource>& subresources) final;
    void cmdClearDepth(const GraphicsResource* cmdBuffer, ImageResource* image, float depth, uint32 stencil, const std::vector<ImageSubresource>& subresources) final;

    void cmdBarrierResources(const GraphicsResource* cmdBuffer, const std::set<const ShaderParameters*>& descriptorsSets) final;

    void cmdBeginRenderPass(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const QuantizedBox2D& renderArea, const RenderPassAdditionalProps& renderpassAdditionalProps, const RenderPassClearValue& clearColor) final;
    void cmdEndRenderPass(const GraphicsResource* cmdBuffer) final;

    void cmdBindComputePipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline) const final;
    void cmdBindGraphicsPipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const GraphicsPipelineState& state) const final;
    void cmdPushConstants(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, uint32 stagesUsed, const uint8* data, const std::vector<CopyBufferInfo>& pushConsts) const final;
    void cmdBindDescriptorsSetInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::map<uint32, const ShaderParameters*>& descriptorsSets) const final;
    void cmdBindDescriptorsSetsInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::vector<const ShaderParameters*>& descriptorsSets) const final;
    void cmdBindVertexBuffers(const GraphicsResource* cmdBuffer, uint32 firstBinding, const std::vector<const BufferResource*>& vertexBuffers, const std::vector<uint64>& offsets) const final;
    void cmdBindIndexBuffer(const GraphicsResource* cmdBuffer, const BufferResource* indexBuffer, uint64 offset = 0) const final;

    void cmdDispatch(const GraphicsResource* cmdBuffer, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ = 1) const final;
    void cmdDrawIndexed(const GraphicsResource* cmdBuffer, uint32 firstIndex, uint32 indexCount, uint32 firstInstance = 0, uint32 instanceCount = 1, int32 vertexOffset = 0) const final;
    void cmdDrawVertices(const GraphicsResource* cmdBuffer, uint32 firstVertex, uint32 vertexCount, uint32 firstInstance = 0, uint32 instanceCount = 1) const final;

    void cmdSetViewportAndScissors(const GraphicsResource* cmdBuffer, const std::vector<std::pair<QuantizedBox2D, QuantizedBox2D>>& viewportAndScissors, uint32 firstViewport = 0) const final;
    void cmdSetViewportAndScissor(const GraphicsResource* cmdBuffer, const QuantizedBox2D& viewport, const QuantizedBox2D& scissor, uint32 atViewport = 0) const final;
    void cmdSetLineWidth(const GraphicsResource* cmdBuffer, float lineWidth) const final;

    void cmdBeginBufferMarker(const GraphicsResource* commandBuffer, const String& name, const LinearColor& color = LinearColorConst::WHITE) const final;
    void cmdInsertBufferMarker(const GraphicsResource* commandBuffer, const String& name, const LinearColor& color = LinearColorConst::WHITE) const final;
    void cmdEndBufferMarker(const GraphicsResource* commandBuffer) const final;

    // Reusable here mean rerecord able command buffer
    const GraphicsResource* startCmd(const String& uniqueName, EQueueFunction queue, bool bIsReusable) final;
    void endCmd(const GraphicsResource* cmdBuffer) final;
    void freeCmd(const GraphicsResource* cmdBuffer) final;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& submitInfo
        , const SharedPtr<GraphicsFence>& fence) final; 
    void finishCmd(const GraphicsResource* cmdBuffer) final;
    void submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2& submitInfo) final;
    void submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo2>& commands) final;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2& command) final;
    void finishCmd(const String& uniqueName) final;
    const GraphicsResource* getCmdBuffer(const String& uniqueName) const final;
    void waitIdle() final;
    void flushAllcommands() final;

};