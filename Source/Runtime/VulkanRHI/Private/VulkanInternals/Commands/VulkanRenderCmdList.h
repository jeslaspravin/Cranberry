/*!
 * \file VulkanRenderCmdList.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "VulkanInternals/Commands/VulkanCommandBufferManager.h"

class IGraphicsInstance;
class GraphicsHelperAPI;
class VulkanDevice;

class VulkanCommandList : public IRenderCommandList
{
private:
    IGraphicsInstance *graphicsInstanceCache;
    const GraphicsHelperAPI *graphicsHelperCache;

    VulkanDevice *vDevice;

    VulkanCmdBufferManager cmdBufferManager;
    VulkanResourcesTracker resourcesTracker;
    // command buffer in which swapchain frame buffers are written to
    std::vector<const GraphicsResource *> swapchainFrameWrites;

    FORCE_INLINE VkImageAspectFlags determineImageAspect(const ImageResourceRef &image) const;
    // Determines mask that has info on how image can be access in pipelines
    FORCE_INLINE VkAccessFlags2 determineImageAccessMask(const ImageResourceRef &image) const;
    // Determines the image layout if layout is yet to be defined
    FORCE_INLINE VkImageLayout determineImageLayout(const ImageResourceRef &image) const;
    FORCE_INLINE VkImageLayout getImageLayout(const ImageResourceRef &image) const;
    FORCE_INLINE VkPipelineBindPoint getPipelineBindPoint(const PipelineBase *pipeline) const;
    // Shader stage in which buffer/image maybe possibly written to/Read from in shader
    FORCE_INLINE VkPipelineStageFlags2 resourceShaderStageFlags() const;

    FORCE_INLINE void fillClearValue(EPixelDataFormat::Type format, VkClearColorValue &clearValue, const LinearColor &color) const;

    void copyToImage_Internal(ImageResourceRef dst, const BufferResourceRef &pixelData, CopyPixelsToImageInfo copyInfo);
    void copyToBuffer_Internal(BufferResourceRef dst, uint32 dstOffset, const void *dataToCopy, uint32 size, bool bFlushMemory = false);
    void cmdCopyBuffer_Internal(
        const GraphicsResource *cmdBuffer, BufferResourceRef src, BufferResourceRef dst, ArrayView<const CopyBufferInfo> copies
    );
    // Copies all staging dst inline. Creates a staging buffer for device only buffers and sets it up ready to be batch copied in caller choice
    // of command buffer. Returns the created staging buffer
    BufferResourceRef
        copyToBuffer_GenCopyBufferInfo(std::vector<BatchCopyBufferInfo> &outBatchCopies, ArrayView<const BatchCopyBufferData> batchCopies);
    void cmdCopyBuffer_GenBarriers(
        std::vector<VkBufferMemoryBarrier2> &outBarriers, const GraphicsResource *cmdBuffer, BufferResourceRef src, BufferResourceRef dst,
        ArrayView<const CopyBufferInfo> copies
    );

public:
    VulkanCommandList(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper, VulkanDevice *vulkanDevice);
    ~VulkanCommandList() = default;

    void newFrame(float timeDelta);

    void copyToBuffer(BufferResourceRef dst, uint32 dstOffset, const void *dataToCopy, uint32 size) final;
    void copyBuffer(BufferResourceRef src, BufferResourceRef dst, ArrayView<const CopyBufferInfo> copies) final;
    void copyToBuffer(ArrayView<const BatchCopyBufferData> batchCopies) final;
    void copyBuffer(ArrayView<const BatchCopyBufferInfo> batchCopies) final;

    void copyToImage(ImageResourceRef dst, ArrayView<const class Color> pixelData, const CopyPixelsToImageInfo &copyInfo) final;
    void copyToImage(ImageResourceRef dst, ArrayView<const class LinearColor> pixelData, const CopyPixelsToImageInfo &copyInfo) final;
    void copyToImageLinearMapped(ImageResourceRef dst, ArrayView<const class Color> pixelData, const CopyPixelsToImageInfo &copyInfo) final;
    void copyOrResolveImage(ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo, const CopyImageInfo &dstInfo) final;

    void clearImage(ImageResourceRef image, const LinearColor &clearColor, ArrayView<const ImageSubresource> subresources) final;
    void clearDepth(ImageResourceRef image, float depth, uint32 stencil, ArrayView<const ImageSubresource> subresources) final;

    void setupInitialLayout(ImageResourceRef image) final;

    void presentImage(
        ArrayView<const WindowCanvasRef> canvases, ArrayView<const uint32> imageIndices, ArrayView<const SemaphoreRef> waitOnSemaphores
    ) final;

    void cmdCopyBuffer(const GraphicsResource *cmdBuffer, BufferResourceRef src, BufferResourceRef dst, ArrayView<const CopyBufferInfo> copies)
        final;
    void cmdCopyBuffer(const GraphicsResource *cmdBuffer, ArrayView<const BatchCopyBufferInfo> copies) final;
    void cmdCopyToBuffer(const GraphicsResource *cmdBuffer, ArrayView<const BatchCopyBufferData> batchCopies) final;
    void cmdCopyOrResolveImage(
        const GraphicsResource *cmdBuffer, ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo,
        const CopyImageInfo &dstInfo
    ) final;

    void cmdTransitionLayouts(const GraphicsResource *cmdBuffer, ArrayView<const ImageResourceRef> images) final;

    void cmdClearImage(
        const GraphicsResource *cmdBuffer, ImageResourceRef image, const LinearColor &clearColor, ArrayView<const ImageSubresource> subresources
    ) final;
    void cmdClearDepth(
        const GraphicsResource *cmdBuffer, ImageResourceRef image, float depth, uint32 stencil, ArrayView<const ImageSubresource> subresources
    ) final;

    void cmdBarrierResources(const GraphicsResource *cmdBuffer, ArrayView<const ShaderParametersRef> descriptorsSets) final;
    void cmdReleaseQueueResources(const GraphicsResource *cmdBuffer, EQueueFunction releaseToQueue) final;
    void cmdReleaseQueueResources(
        const GraphicsResource *cmdBuffer, EQueueFunction releaseToQueue,
        const std::unordered_map<MemoryResourceRef, EQueueFunction> &perResourceRelease
    ) final;

    void cmdBeginRenderPass(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const QuantizedBox2D &renderArea,
        const RenderPassAdditionalProps &renderpassAdditionalProps, const RenderPassClearValue &clearColor
    ) final;
    void cmdEndRenderPass(const GraphicsResource *cmdBuffer) final;

    void cmdBindComputePipeline(const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline) const final;
    void cmdBindGraphicsPipeline(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const GraphicsPipelineState &state
    ) const final;
    void cmdPushConstants(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, uint32 stagesUsed, const uint8 *data,
        ArrayView<const CopyBufferInfo> pushConsts
    ) const final;
    void cmdBindDescriptorsSetInternal(
        const GraphicsResource *cmdBuffer, const PipelineBase *contextPipeline, const std::map<uint32, ShaderParametersRef> &descriptorsSets
    ) const final;
    void cmdBindDescriptorsSetsInternal(
        const GraphicsResource *cmdBuffer, const PipelineBase *contextPipeline, ArrayView<const ShaderParametersRef> descriptorsSets
    ) const final;
    void cmdBindVertexBuffer(const GraphicsResource *cmdBuffer, uint32 firstBinding, BufferResourceRef vertexBuffer, uint64 offset) final;
    void cmdBindVertexBuffers(
        const GraphicsResource *cmdBuffer, uint32 firstBinding, ArrayView<const BufferResourceRef> vertexBuffers,
        ArrayView<const uint64> offsets
    ) final;
    void cmdBindIndexBuffer(const GraphicsResource *cmdBuffer, const BufferResourceRef &indexBuffer, uint64 offset = 0) final;

    void cmdDispatch(const GraphicsResource *cmdBuffer, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ = 1) const final;
    void cmdDrawIndexed(
        const GraphicsResource *cmdBuffer, uint32 firstIndex, uint32 indexCount, uint32 firstInstance = 0, uint32 instanceCount = 1,
        int32 vertexOffset = 0
    ) const final;
    void cmdDrawVertices(
        const GraphicsResource *cmdBuffer, uint32 firstVertex, uint32 vertexCount, uint32 firstInstance = 0, uint32 instanceCount = 1
    ) const final;
    void cmdDrawIndexedIndirect(
        const GraphicsResource *cmdBuffer, const BufferResourceRef &drawCmdsBuffer, uint32 bufferOffset, uint32 drawCount, uint32 stride
    ) final;
    void cmdDrawIndirect(
        const GraphicsResource *cmdBuffer, const BufferResourceRef &drawCmdsBuffer, uint32 bufferOffset, uint32 drawCount, uint32 stride
    ) final;

    void cmdSetViewportAndScissors(
        const GraphicsResource *cmdBuffer, ArrayView<const std::pair<QuantizedBox2D, QuantizedBox2D>> viewportAndScissors,
        uint32 firstViewport = 0
    ) const final;
    void cmdSetViewportAndScissor(
        const GraphicsResource *cmdBuffer, const QuantizedBox2D &viewport, const QuantizedBox2D &scissor, uint32 atViewport = 0
    ) const final;
    void cmdSetScissor(const GraphicsResource *cmdBuffer, const QuantizedBox2D &scissor, uint32 atViewport = 0) const final;
    void cmdSetLineWidth(const GraphicsResource *cmdBuffer, float lineWidth) const final;
    void cmdSetDepthBias(const GraphicsResource *cmdBuffer, float constantBias, float slopeFactor, float clampValue) const final;

    void cmdBeginBufferMarker(const GraphicsResource *commandBuffer, const String &name, const LinearColor &color = LinearColorConst::WHITE)
        const final;
    void cmdInsertBufferMarker(const GraphicsResource *commandBuffer, const String &name, const LinearColor &color = LinearColorConst::WHITE)
        const final;
    void cmdEndBufferMarker(const GraphicsResource *commandBuffer) const final;

    // Reusable here mean rerecord able command buffer
    const GraphicsResource *startCmd(const String &uniqueName, EQueueFunction queue, bool bIsReusable) final;
    void endCmd(const GraphicsResource *cmdBuffer) final;
    void freeCmd(const GraphicsResource *cmdBuffer) final;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo &submitInfo, FenceRef fence) final;
    void submitCmds(EQueuePriority::Enum priority, ArrayView<const CommandSubmitInfo> submitInfos, FenceRef fence) final;
    void submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &submitInfo) final;
    void submitCmds(EQueuePriority::Enum priority, ArrayView<const CommandSubmitInfo2> commands) final;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &command) final;
    void finishCmd(const GraphicsResource *cmdBuffer) final;
    void finishCmd(const String &uniqueName) final;
    const GraphicsResource *getCmdBuffer(const String &uniqueName) const final;
    TimelineSemaphoreRef getCmdSignalSemaphore(const String &uniqueName) const final;
    TimelineSemaphoreRef getCmdSignalSemaphore(const GraphicsResource *cmdBuffer) const final;
    void waitIdle() final;
    void waitOnResDepCmds(const MemoryResourceRef &resource) final;
    void flushAllcommands() final;
    bool hasCmdsUsingResource(const MemoryResourceRef &resource) final;
};