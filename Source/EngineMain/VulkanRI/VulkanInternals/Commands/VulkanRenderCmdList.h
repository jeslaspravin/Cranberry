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
public:

    VulkanCommandList(IGraphicsInstance* graphicsInstance, VulkanDevice* vulkanDevice);
    ~VulkanCommandList() = default;

    void newFrame() override;

    void copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size) override;
    void copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo) override;
    void copyToBuffer(const std::vector<BatchCopyBufferData>& batchCopies) override;

    void copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyPixelsToImageInfo& copyInfo) override;
    void copyOrResolveImage(ImageResource* src, ImageResource* dst, const CopyImageInfo& srcInfo, const CopyImageInfo& dstInfo) override;

    void setupInitialLayout(ImageResource* image) override;

    void presentImage(const std::vector<class GenericWindowCanvas*>& canvases,
        const std::vector<uint32>& imageIndices, const std::vector<SharedPtr<class GraphicsSemaphore>>& waitOnSemaphores) override;

    void cmdBarrierResources(const GraphicsResource* cmdBuffer, const std::set<const ShaderParameters*>& descriptorsSets) override;

    void cmdBeginRenderPass(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const QuantizedBox2D& renderArea, const RenderPassAdditionalProps& renderpassAdditionalProps, const RenderPassClearValue& clearColor) override;
    void cmdEndRenderPass(const GraphicsResource* cmdBuffer) override;

    void cmdBindComputePipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline) const override;
    void cmdBindGraphicsPipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const GraphicsPipelineState& state) const override;
    void cmdPushConstants(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, uint32 stagesUsed, const uint8* data, const std::vector<CopyBufferInfo>& pushConsts) const override;
    void cmdBindDescriptorsSetInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::map<uint32, const ShaderParameters*>& descriptorsSets) const override;
    void cmdBindDescriptorsSetsInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::vector<const ShaderParameters*>& descriptorsSets) const override;
    void cmdBindVertexBuffers(const GraphicsResource* cmdBuffer, uint32 firstBinding, const std::vector<const BufferResource*>& vertexBuffers, const std::vector<uint64>& offsets) const override;
    void cmdBindIndexBuffer(const GraphicsResource* cmdBuffer, const BufferResource* indexBuffer, uint64 offset = 0) const override;

    void cmdDispatch(const GraphicsResource* cmdBuffer, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ = 1) const override;
    void cmdDrawIndexed(const GraphicsResource* cmdBuffer, uint32 firstIndex, uint32 indexCount, uint32 firstInstance = 0, uint32 instanceCount = 1, int32 vertexOffset = 0) const override;

    void cmdSetViewportAndScissors(const GraphicsResource* cmdBuffer, const std::vector<std::pair<QuantizedBox2D, QuantizedBox2D>>& viewportAndScissors, uint32 firstViewport = 0) const override;
    void cmdSetViewportAndScissor(const GraphicsResource* cmdBuffer, const QuantizedBox2D& viewport, const QuantizedBox2D& scissor, uint32 atViewport = 0) const override;

    void cmdBeginBufferMarker(const GraphicsResource* commandBuffer, const String& name, const LinearColor& color = LinearColorConst::WHITE) const override;
    void cmdInsertBufferMarker(const GraphicsResource* commandBuffer, const String& name, const LinearColor& color = LinearColorConst::WHITE) const override;
    void cmdEndBufferMarker(const GraphicsResource* commandBuffer) const override;

    // Reusable here mean rerecord able command buffer
    const GraphicsResource* startCmd(const String& uniqueName, EQueueFunction queue, bool bIsReusable) override;
    void endCmd(const GraphicsResource* cmdBuffer) override;
    void freeCmd(const GraphicsResource* cmdBuffer) override;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& submitInfo
        , const SharedPtr<GraphicsFence>& fence) override;
    void submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo& submitInfo) override;
    void finishCmd(const GraphicsResource* cmdBuffer) override;
    void submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo2>& commands) override;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2& command) override;
    void finishCmd(const String& uniqueName) override;
    const GraphicsResource* getCmdBuffer(const String& uniqueName) const override;
    void waitIdle() override;
};