/*!
 * \file RenderCommandList.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2024
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/CoreMathTypes.h"
#include "RenderInterface/Rendering/CommandBuffer.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderInterface/Resources/ShaderResources.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "ShaderReflected.h"
#include "Types/Colors.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"
#include "Memory/Memory.h"

#include <utility>

// This must be modified to be a thread safe call when from other threads
class RenderCommandList final : public IRenderCommandList
{
private:
    IRenderCommandList *cmdList;

public:
    void setup(IRenderCommandList *commandList) final;
    void newFrame(float timeDelta) final;

    void copyToBuffer(BufferResourceRef dst, uint32 dstOffset, const void *dataToCopy, uint32 size) final;
    void copyToBuffer(ArrayView<BatchCopyBufferData> batchCopies) final;
    void copyBuffer(BufferResourceRef src, BufferResourceRef dst, ArrayView<CopyBufferInfo> copies) final;
    void copyBuffer(ArrayView<BatchCopyBufferInfo> batchCopies) final;

    void copyToImage(ImageResourceRef dst, ArrayView<Color> pixelData, const CopyPixelsToImageInfo &copyInfo) final;
    void copyToImage(ImageResourceRef dst, ArrayView<LinearColor> pixelData, const CopyPixelsToImageInfo &copyInfo) final;
    void copyToImageLinearMapped(ImageResourceRef dst, ArrayView<Color> pixelData, const CopyPixelsToImageInfo &copyInfo) final;

    void copyOrResolveImage(ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo, const CopyImageInfo &dstInfo) final;

    void clearImage(ImageResourceRef image, const LinearColor &clearColor, ArrayView<ImageSubresource> subresources) final;
    void clearDepth(ImageResourceRef image, float depth, uint32 stencil, ArrayView<ImageSubresource> subresources) final;

    void setupInitialLayout(ImageResourceRef image) final;

    void presentImage(ArrayView<WindowCanvasRef> canvases, ArrayView<uint32> imageIndices, ArrayView<SemaphoreRef> waitOnSemaphores) final;

    void cmdCopyBuffer(const GraphicsResource *cmdBuffer, BufferResourceRef src, BufferResourceRef dst, ArrayView<CopyBufferInfo> copies) final;
    void cmdCopyBuffer(const GraphicsResource *cmdBuffer, ArrayView<BatchCopyBufferInfo> copies) final;
    void cmdCopyToBuffer(const GraphicsResource *cmdBuffer, ArrayView<BatchCopyBufferData> batchCopies) final;
    void cmdCopyOrResolveImage(
        const GraphicsResource *cmdBuffer, ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo,
        const CopyImageInfo &dstInfo
    ) final;

    void cmdTransitionLayouts(const GraphicsResource *cmdBuffer, ArrayView<ImageResourceRef> images) final;

    void cmdClearImage(
        const GraphicsResource *cmdBuffer, ImageResourceRef image, const LinearColor &clearColor, ArrayView<ImageSubresource> subresources
    ) final;
    void cmdClearDepth(
        const GraphicsResource *cmdBuffer, ImageResourceRef image, float depth, uint32 stencil, ArrayView<ImageSubresource> subresources
    ) final;

    void cmdBarrierResources(const GraphicsResource *cmdBuffer, ArrayView<ShaderParametersRef> descriptorsSets) final;
    void cmdBarrierVertices(const GraphicsResource *cmdBuffer, ArrayView<BufferResourceRef> vertexBuffers) final;
    void cmdBarrierIndices(const GraphicsResource *cmdBuffer, ArrayView<BufferResourceRef> indexBuffers) final;
    void cmdBarrierIndirectDraws(const GraphicsResource *cmdBuffer, ArrayView<BufferResourceRef> indirectDrawBuffers) final;
    void cmdReleaseQueueResources(const GraphicsResource *cmdBuffer, EQueueFunction releaseToQueue) final;
    void cmdReleaseQueueResources(
        const GraphicsResource *cmdBuffer, EQueueFunction releaseToQueue,
        const std::unordered_map<MemoryResourceRef, EQueueFunction> &perResourceRelease
    ) final;

    void cmdBeginRenderPass(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const IRect &renderArea,
        const RenderPassAdditionalProps &renderpassAdditionalProps, const RenderPassClearValue &clearColor
    ) final;
    void cmdEndRenderPass(const GraphicsResource *cmdBuffer) final;

    void cmdBindGraphicsPipeline(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const GraphicsPipelineState &state
    ) const final;
    void cmdBindComputePipeline(const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline) const final;
    void cmdPushConstants(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, uint32 stagesUsed, const uint8 *data,
        ArrayView<CopyBufferInfo> pushConsts
    ) const final;
    void cmdBindDescriptorsSetInternal(
        const GraphicsResource *cmdBuffer, const PipelineBase *contextPipeline, const std::map<uint32, ShaderParametersRef> &descriptorsSets
    ) const final;
    void cmdBindDescriptorsSetsInternal(
        const GraphicsResource *cmdBuffer, const PipelineBase *contextPipeline, ArrayView<ShaderParametersRef> descriptorsSets
    ) const final;
    void cmdBindVertexBuffer(const GraphicsResource *cmdBuffer, uint32 firstBinding, BufferResourceRef vertexBuffer, uint64 offset) final;
    void cmdBindVertexBuffers(
        const GraphicsResource *cmdBuffer, uint32 firstBinding, ArrayView<BufferResourceRef> vertexBuffers, ArrayView<uint64> offsets
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
        const GraphicsResource *cmdBuffer, ArrayView<std::pair<IRect, IRect>> viewportAndScissors, uint32 firstViewport = 0
    ) const final;
    void
    cmdSetViewportAndScissor(const GraphicsResource *cmdBuffer, const IRect &viewport, const IRect &scissor, uint32 atViewport = 0) const final;
    void cmdSetScissor(const GraphicsResource *cmdBuffer, const IRect &scissor, uint32 atViewport = 0) const final;
    void cmdSetLineWidth(const GraphicsResource *cmdBuffer, float lineWidth) const final;
    void cmdSetDepthBias(const GraphicsResource *cmdBuffer, float constantBias, float slopeFactor, float clampValue) const final;

    void cmdBeginBufferMarker(const GraphicsResource *commandBuffer, const String &name, const LinearColor &color = LinearColorConst::WHITE)
        const final;
    void cmdInsertBufferMarker(const GraphicsResource *commandBuffer, const String &name, const LinearColor &color = LinearColorConst::WHITE)
        const final;
    void cmdEndBufferMarker(const GraphicsResource *commandBuffer) const final;

    const GraphicsResource *startCmd(const String &uniqueName, EQueueFunction queue, bool bIsReusable) final;
    void endCmd(const GraphicsResource *cmdBuffer) final;
    void freeCmd(const GraphicsResource *cmdBuffer) final;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo &submitInfo, FenceRef fence) final;
    void submitCmds(EQueuePriority::Enum priority, ArrayView<CommandSubmitInfo> submitInfos, FenceRef fence) final;
    void submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &submitInfo);
    void submitCmds(EQueuePriority::Enum priority, ArrayView<CommandSubmitInfo2> submitInfos) final;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &command) final;
    void finishCmd(const GraphicsResource *cmdBuffer) final;
    void finishCmd(const String &uniqueName) final;
    const GraphicsResource *getCmdBuffer(const String &uniqueName) const final;
    TimelineSemaphoreRef getCmdSignalSemaphore(const String &uniqueName) const final;
    TimelineSemaphoreRef getCmdSignalSemaphore(const GraphicsResource *cmdBuffer) const final;
    void waitIdle() final;
    void waitOnResDepCmds(const MemoryResourceRef &resource) final;
    void flushAllcommands() final;
    bool hasCmdsUsingResource(const MemoryResourceRef &resource, bool bFinishCmds) final;
};

void RenderCommandList::cmdPushConstants(
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, uint32 stagesUsed, const uint8 *data,
    ArrayView<CopyBufferInfo> pushConsts
) const
{
    cmdList->cmdPushConstants(cmdBuffer, contextPipeline, stagesUsed, data, pushConsts);
}

void RenderCommandList::cmdBindDescriptorsSetInternal(
    const GraphicsResource *cmdBuffer, const PipelineBase *contextPipeline, const std::map<uint32, ShaderParametersRef> &descriptorsSets
) const
{
    cmdList->cmdBindDescriptorsSetInternal(cmdBuffer, contextPipeline, descriptorsSets);
}

void RenderCommandList::cmdBindDescriptorsSetsInternal(
    const GraphicsResource *cmdBuffer, const PipelineBase *contextPipeline, ArrayView<ShaderParametersRef> descriptorsSets
) const
{
    cmdList->cmdBindDescriptorsSetsInternal(cmdBuffer, contextPipeline, descriptorsSets);
}

void RenderCommandList::cmdBindVertexBuffer(
    const GraphicsResource *cmdBuffer, uint32 firstBinding, BufferResourceRef vertexBuffer, uint64 offset
)
{
    cmdList->cmdBindVertexBuffer(cmdBuffer, firstBinding, vertexBuffer, offset);
}

void RenderCommandList::cmdBindVertexBuffers(
    const GraphicsResource *cmdBuffer, uint32 firstBinding, ArrayView<BufferResourceRef> vertexBuffers, ArrayView<uint64> offsets
)
{
    cmdList->cmdBindVertexBuffers(cmdBuffer, firstBinding, vertexBuffers, offsets);
}

void RenderCommandList::cmdBindIndexBuffer(const GraphicsResource *cmdBuffer, const BufferResourceRef &indexBuffer, uint64 offset /*= 0*/)
{
    cmdList->cmdBindIndexBuffer(cmdBuffer, indexBuffer, offset);
}

void RenderCommandList::cmdDispatch(const GraphicsResource *cmdBuffer, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ /*= 1*/) const
{
    cmdList->cmdDispatch(cmdBuffer, groupSizeX, groupSizeY, groupSizeZ);
}

void RenderCommandList::cmdDrawIndexed(
    const GraphicsResource *cmdBuffer, uint32 firstIndex, uint32 indexCount, uint32 firstInstance /*= 0*/, uint32 instanceCount /*= 1*/,
    int32 vertexOffset /*= 0*/
) const
{
    cmdList->cmdDrawIndexed(cmdBuffer, firstIndex, indexCount, firstInstance, instanceCount, vertexOffset);
}

void RenderCommandList::cmdDrawVertices(
    const GraphicsResource *cmdBuffer, uint32 firstVertex, uint32 vertexCount, uint32 firstInstance /*= 0*/, uint32 instanceCount /*= 1*/
) const
{
    cmdList->cmdDrawVertices(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

void RenderCommandList::cmdDrawIndexedIndirect(
    const GraphicsResource *cmdBuffer, const BufferResourceRef &drawCmdsBuffer, uint32 bufferOffset, uint32 drawCount, uint32 stride
)
{
    cmdList->cmdDrawIndexedIndirect(cmdBuffer, drawCmdsBuffer, bufferOffset, drawCount, stride);
}

void RenderCommandList::cmdDrawIndirect(
    const GraphicsResource *cmdBuffer, const BufferResourceRef &drawCmdsBuffer, uint32 bufferOffset, uint32 drawCount, uint32 stride
)
{
    cmdList->cmdDrawIndirect(cmdBuffer, drawCmdsBuffer, bufferOffset, drawCount, stride);
}

void RenderCommandList::setup(IRenderCommandList *commandList)
{
    if (commandList != cmdList)
    {
        cmdList = commandList;
    }
}

void RenderCommandList::newFrame(float timeDelta) { cmdList->newFrame(timeDelta); }

void RenderCommandList::copyBuffer(BufferResourceRef src, BufferResourceRef dst, ArrayView<CopyBufferInfo> copies)
{
    cmdList->copyBuffer(src, dst, copies);
}

void RenderCommandList::copyBuffer(ArrayView<BatchCopyBufferInfo> batchCopies) { cmdList->copyBuffer(batchCopies); }

void RenderCommandList::copyToBuffer(BufferResourceRef dst, uint32 dstOffset, const void *dataToCopy, uint32 size)
{
    cmdList->copyToBuffer(dst, dstOffset, dataToCopy, size);
}

void RenderCommandList::copyToBuffer(ArrayView<BatchCopyBufferData> batchCopies) { cmdList->copyToBuffer(batchCopies); }

const GraphicsResource *RenderCommandList::startCmd(const String &uniqueName, EQueueFunction queue, bool bIsReusable)
{
    return cmdList->startCmd(uniqueName, queue, bIsReusable);
}

void RenderCommandList::endCmd(const GraphicsResource *cmdBuffer) { cmdList->endCmd(cmdBuffer); }

void RenderCommandList::freeCmd(const GraphicsResource *cmdBuffer) { cmdList->freeCmd(cmdBuffer); }

void RenderCommandList::submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo &submitInfo, FenceRef fence)
{
    cmdList->submitCmd(priority, submitInfo, fence);
}

void RenderCommandList::submitCmds(EQueuePriority::Enum priority, ArrayView<CommandSubmitInfo> submitInfos, FenceRef fence)
{
    cmdList->submitCmds(priority, submitInfos, fence);
}

void RenderCommandList::submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &submitInfo)
{
    cmdList->submitWaitCmd(priority, submitInfo);
}

void RenderCommandList::submitCmds(EQueuePriority::Enum priority, ArrayView<CommandSubmitInfo2> submitInfos)
{
    cmdList->submitCmds(priority, submitInfos);
}

void RenderCommandList::submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &command) { cmdList->submitCmd(priority, command); }

void RenderCommandList::finishCmd(const GraphicsResource *cmdBuffer) { cmdList->finishCmd(cmdBuffer); }

void RenderCommandList::finishCmd(const String &uniqueName) { cmdList->finishCmd(uniqueName); }

const GraphicsResource *RenderCommandList::getCmdBuffer(const String &uniqueName) const { return cmdList->getCmdBuffer(uniqueName); }

TimelineSemaphoreRef RenderCommandList::getCmdSignalSemaphore(const String &uniqueName) const
{
    return cmdList->getCmdSignalSemaphore(uniqueName);
}

TimelineSemaphoreRef RenderCommandList::getCmdSignalSemaphore(const GraphicsResource *cmdBuffer) const
{
    return cmdList->getCmdSignalSemaphore(cmdBuffer);
}

void RenderCommandList::waitIdle() { cmdList->waitIdle(); }

void RenderCommandList::waitOnResDepCmds(const MemoryResourceRef &resource) { cmdList->waitOnResDepCmds(resource); }

void RenderCommandList::flushAllcommands() { cmdList->flushAllcommands(); }

bool RenderCommandList::hasCmdsUsingResource(const MemoryResourceRef &resource, bool bFinishCmds)
{
    return cmdList->hasCmdsUsingResource(resource, bFinishCmds);
}

void RenderCommandList::copyToImage(ImageResourceRef dst, ArrayView<Color> pixelData, const CopyPixelsToImageInfo &copyInfo)
{
    cmdList->copyToImage(dst, pixelData, copyInfo);
}

void RenderCommandList::copyToImage(ImageResourceRef dst, ArrayView<LinearColor> pixelData, const CopyPixelsToImageInfo &copyInfo)
{
    cmdList->copyToImage(dst, pixelData, copyInfo);
}

void RenderCommandList::copyToImageLinearMapped(ImageResourceRef dst, ArrayView<Color> pixelData, const CopyPixelsToImageInfo &copyInfo)
{
    cmdList->copyToImageLinearMapped(dst, pixelData, copyInfo);
}

void RenderCommandList::copyOrResolveImage(
    ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo, const CopyImageInfo &dstInfo
)
{
    cmdList->copyOrResolveImage(src, dst, srcInfo, dstInfo);
}

void RenderCommandList::clearImage(ImageResourceRef image, const LinearColor &clearColor, ArrayView<ImageSubresource> subresources)
{
    cmdList->clearImage(image, clearColor, subresources);
}

void RenderCommandList::clearDepth(ImageResourceRef image, float depth, uint32 stencil, ArrayView<ImageSubresource> subresources)
{
    cmdList->clearDepth(image, depth, stencil, subresources);
}

void RenderCommandList::setupInitialLayout(ImageResourceRef image) { cmdList->setupInitialLayout(image); }

void RenderCommandList::presentImage(
    ArrayView<WindowCanvasRef> canvases, ArrayView<uint32> imageIndices, ArrayView<SemaphoreRef> waitOnSemaphores
)
{
    cmdList->presentImage(canvases, imageIndices, waitOnSemaphores);
}

void RenderCommandList::cmdCopyBuffer(
    const GraphicsResource *cmdBuffer, BufferResourceRef src, BufferResourceRef dst, ArrayView<CopyBufferInfo> copies
)
{
    cmdList->cmdCopyBuffer(cmdBuffer, src, dst, copies);
}

void RenderCommandList::cmdCopyBuffer(const GraphicsResource *cmdBuffer, ArrayView<BatchCopyBufferInfo> copies)
{
    cmdList->cmdCopyBuffer(cmdBuffer, copies);
}

void RenderCommandList::cmdCopyToBuffer(const GraphicsResource *cmdBuffer, ArrayView<BatchCopyBufferData> batchCopies)
{
    cmdList->cmdCopyToBuffer(cmdBuffer, batchCopies);
}

void RenderCommandList::cmdCopyOrResolveImage(
    const GraphicsResource *cmdBuffer, ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo, const CopyImageInfo &dstInfo
)
{
    cmdList->cmdCopyOrResolveImage(cmdBuffer, src, dst, srcInfo, dstInfo);
}

void RenderCommandList::cmdTransitionLayouts(const GraphicsResource *cmdBuffer, ArrayView<ImageResourceRef> images)
{
    cmdList->cmdTransitionLayouts(cmdBuffer, images);
}

void RenderCommandList::cmdClearImage(
    const GraphicsResource *cmdBuffer, ImageResourceRef image, const LinearColor &clearColor, ArrayView<ImageSubresource> subresources
)
{
    cmdList->cmdClearImage(cmdBuffer, image, clearColor, subresources);
}

void RenderCommandList::cmdClearDepth(
    const GraphicsResource *cmdBuffer, ImageResourceRef image, float depth, uint32 stencil, ArrayView<ImageSubresource> subresources
)
{
    cmdList->cmdClearDepth(cmdBuffer, image, depth, stencil, subresources);
}

void RenderCommandList::cmdBarrierResources(const GraphicsResource *cmdBuffer, ArrayView<ShaderParametersRef> descriptorsSets)
{
    cmdList->cmdBarrierResources(cmdBuffer, descriptorsSets);
}

void RenderCommandList::cmdBarrierVertices(const GraphicsResource *cmdBuffer, ArrayView<BufferResourceRef> vertexBuffers)
{
    cmdList->cmdBarrierVertices(cmdBuffer, vertexBuffers);
}

void RenderCommandList::cmdBarrierIndices(const GraphicsResource *cmdBuffer, ArrayView<BufferResourceRef> indexBuffers)
{
    cmdList->cmdBarrierIndices(cmdBuffer, indexBuffers);
}

void RenderCommandList::cmdBarrierIndirectDraws(const GraphicsResource *cmdBuffer, ArrayView<BufferResourceRef> indirectDrawBuffers)
{
    cmdList->cmdBarrierIndirectDraws(cmdBuffer, indirectDrawBuffers);
}

void RenderCommandList::cmdReleaseQueueResources(const GraphicsResource *cmdBuffer, EQueueFunction releaseToQueue)
{
    cmdList->cmdReleaseQueueResources(cmdBuffer, releaseToQueue);
}

void RenderCommandList::cmdReleaseQueueResources(
    const GraphicsResource *cmdBuffer, EQueueFunction releaseToQueue,
    const std::unordered_map<MemoryResourceRef, EQueueFunction> &perResourceRelease
)
{
    cmdList->cmdReleaseQueueResources(cmdBuffer, releaseToQueue, perResourceRelease);
}

void RenderCommandList::cmdBeginRenderPass(
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const IRect &renderArea,
    const RenderPassAdditionalProps &renderpassAdditionalProps, const RenderPassClearValue &clearColor
)
{
    cmdList->cmdBeginRenderPass(cmdBuffer, contextPipeline, renderArea, renderpassAdditionalProps, clearColor);
}

void RenderCommandList::cmdEndRenderPass(const GraphicsResource *cmdBuffer) { cmdList->cmdEndRenderPass(cmdBuffer); }

void RenderCommandList::cmdBindGraphicsPipeline(
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const GraphicsPipelineState &state
) const
{
    cmdList->cmdBindGraphicsPipeline(cmdBuffer, contextPipeline, state);
}

void RenderCommandList::cmdBindComputePipeline(const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline) const
{
    cmdList->cmdBindComputePipeline(cmdBuffer, contextPipeline);
}

void RenderCommandList::cmdSetViewportAndScissors(
    const GraphicsResource *cmdBuffer, ArrayView<std::pair<IRect, IRect>> viewportAndScissors, uint32 firstViewport /*= 0*/
) const
{
    cmdList->cmdSetViewportAndScissors(cmdBuffer, viewportAndScissors, firstViewport);
}

void RenderCommandList::cmdSetViewportAndScissor(
    const GraphicsResource *cmdBuffer, const IRect &viewport, const IRect &scissor, uint32 atViewport /*= 0*/
) const
{
    cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor, atViewport);
}

void RenderCommandList::cmdSetScissor(const GraphicsResource *cmdBuffer, const IRect &scissor, uint32 atViewport /*= 0*/) const
{
    cmdList->cmdSetScissor(cmdBuffer, scissor, atViewport);
}

void RenderCommandList::cmdSetLineWidth(const GraphicsResource *cmdBuffer, float lineWidth) const
{
    cmdList->cmdSetLineWidth(cmdBuffer, lineWidth);
}

void RenderCommandList::cmdSetDepthBias(const GraphicsResource *cmdBuffer, float constantBias, float slopeFactor, float clampValue) const
{
    cmdList->cmdSetDepthBias(cmdBuffer, constantBias, slopeFactor, clampValue);
}

void RenderCommandList::
    cmdBeginBufferMarker(const GraphicsResource *commandBuffer, const String &name, const LinearColor &color /*= LinearColorConst::WHITE*/)
        const
{
    cmdList->cmdBeginBufferMarker(commandBuffer, name, color);
}

void RenderCommandList::
    cmdInsertBufferMarker(const GraphicsResource *commandBuffer, const String &name, const LinearColor &color /*= LinearColorConst::WHITE*/)
        const
{
    cmdList->cmdInsertBufferMarker(commandBuffer, name, color);
}

void RenderCommandList::cmdEndBufferMarker(const GraphicsResource *commandBuffer) const { cmdList->cmdEndBufferMarker(commandBuffer); }

bool IRenderCommandList::simpleCopyPixelsTo(
    BufferResourceRef stagingBuffer, uint8 *stagingPtr, ArrayView<Color> pixelData, EPixelDataFormat::Type dataFormat,
    const EPixelDataFormat::PixelFormatInfo *formatInfo
) const
{
    if (dataFormat == EPixelDataFormat::BGRA_U8_Norm || dataFormat == EPixelDataFormat::BGRA_U8_SRGB
        || dataFormat == EPixelDataFormat::BGRA_U8_Scaled)
    {
        // Do I need memset here?
        // memset(stagingPtr, 0, stagingBuffer->getResourceSize());

        // Copying data
        for (uint32 i = 0; i < pixelData.size(); ++i)
        {
            uint8 *pixelStagingPtr = stagingPtr + (i * formatInfo->pixelDataSize);
            pixelStagingPtr[0] = pixelData[i].b();
            pixelStagingPtr[1] = pixelData[i].g();
            pixelStagingPtr[2] = pixelData[i].r();
            pixelStagingPtr[3] = pixelData[i].a();
        }
        return true;
    }

    if (dataFormat == EPixelDataFormat::RGBA_U8_Norm || dataFormat == EPixelDataFormat::RGBA_U8_SRGB
        || dataFormat == EPixelDataFormat::RGBA_U8_Scaled)
    {
        memcpy(stagingPtr, pixelData.data(), pixelData.size() * formatInfo->pixelDataSize);
        return true;
    }

    // If components are in order(R,G,B) and 8bit per component the do memcpy
    bool bInOrder = true;
    bool bAllByteSized = true;
    for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
    {
        bInOrder = bInOrder && EPixelComponent(idx) == formatInfo->componentOrder[idx]        // Is in same order
                   && formatInfo->componentSize[uint8(formatInfo->componentOrder[idx])] == 8; // 8 bit
        bAllByteSized = bAllByteSized && formatInfo->componentSize[uint8(formatInfo->componentOrder[idx])] == 8;
    }
    if (bInOrder)
    {
        for (uint32 i = 0; i < pixelData.size(); ++i)
        {
            uint8 *pixelStagingPtr = stagingPtr + (i * formatInfo->pixelDataSize);
            memcpy(pixelStagingPtr, &pixelData[i], formatInfo->pixelDataSize);
        }
        return true;
    }
    // if all components are byte sized, then copy each component to corresponding other component
    if (bAllByteSized)
    {
        for (uint32 i = 0; i < pixelData.size(); ++i)
        {
            uint8 *pixelStagingPtr = stagingPtr + (i * formatInfo->pixelDataSize);
            for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
            {
                const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);
                pixelStagingPtr[idx] = pixelData[i].getColorValue()[compIdx];
            }
        }
        return true;
    }
    return false;
}

void IRenderCommandList::copyPixelsTo(
    BufferResourceRef stagingBuffer, uint8 *stagingPtr, ArrayView<Color> pixelData, const EPixelDataFormat::PixelFormatInfo *formatInfo
) const
{
    constexpr uint32 colorCompBits = sizeof(decltype(std::declval<Color>().r())) * 8;
    static_assert(colorCompBits == 8, "Unsupported color format");

    // Mask from component's first byte
    uint32 perCompMask[MAX_PIXEL_COMP_COUNT];
    for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
    {
        const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

        uint8 offset = formatInfo->getOffset(EPixelComponent(compIdx)) % 8;
        fatalAssertf(
            ((sizeof(uint32) * 8) - offset) >= formatInfo->componentSize[compIdx],
            "Component {} of pixel format {} is going beyond 32bits mask after offset", compIdx, formatInfo->formatName
        );

        uint32 end = 1 << (formatInfo->componentSize[compIdx] - 1);
        uint32 mask = 1;
        perCompMask[compIdx] = 1;
        while ((mask & end) != end)
        {
            mask <<= 1;
            mask |= 1;
        }

        mask <<= offset;
        perCompMask[compIdx] = mask;
    }

    memset(stagingPtr, 0, stagingBuffer->getResourceSize());

#if LITTLE_ENDIAN
    // Copying data
    for (uint32 i = 0; i < pixelData.size(); ++i)
    {
        uint8 *pixelStagingPtr = stagingPtr + (i * formatInfo->pixelDataSize);
        for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
        {
            const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

            const uint32 compOffset = formatInfo->getOffset(EPixelComponent(compIdx));

            // We are never going to go above 32 bits per channel
            const uint32 compValue = pixelData[i].getColorValue()[compIdx];

            uint8 *offsetStagingPtr = pixelStagingPtr + (compOffset / 8);

            // Left shift
            const uint32 byte1Shift = compOffset % 8;

            (*reinterpret_cast<uint32 *>(offsetStagingPtr)) |= (perCompMask[compIdx] & (compValue << byte1Shift));
        }
    }
#else
#error Big endian platform not supported!
#endif
}

void IRenderCommandList::copyPixelsTo(
    BufferResourceRef stagingBuffer, uint8 *stagingPtr, ArrayView<LinearColor> pixelData, const EPixelDataFormat::PixelFormatInfo *formatInfo,
    bool bIsFloatingFormat
) const
{
    constexpr uint32 colorCompBits = sizeof(decltype(std::declval<LinearColor>().r())) * 8;
    static_assert(colorCompBits == 32, "Unsupported LinearColor format");

    memset(stagingPtr, 0, stagingBuffer->getResourceSize());
    if (bIsFloatingFormat)
    {
        for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
        {
            debugAssert(formatInfo->componentSize[uint32(formatInfo->componentOrder[idx])] == colorCompBits);
        }

        // Copying data
        for (uint32 i = 0; i < pixelData.size(); ++i)
        {
            uint8 *pixelStagingPtr = stagingPtr + (i * formatInfo->pixelDataSize);
            for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
            {
                const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);
                const uint32 compOffset = formatInfo->getOffset(EPixelComponent(compIdx));

                uint8 *offsetStagingPtr = pixelStagingPtr + (compOffset / 8);

                memcpy(offsetStagingPtr, &pixelData[i].getColorValue()[compIdx], sizeof(float));
            }
        }
    }
    else
    {
        // Mask from component's first byte
        uint32 perCompMask[MAX_PIXEL_COMP_COUNT];
        for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
        {
            const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

            uint8 offset = formatInfo->getOffset(EPixelComponent(compIdx)) % 8;
            fatalAssertf(
                ((sizeof(uint32) * 8) - offset) >= formatInfo->componentSize[compIdx],
                "Component {} of pixel format {} is going beyond 32bits mask after offset", compIdx, formatInfo->formatName
            );

            uint32 end = 1 << (formatInfo->componentSize[compIdx] - 1);
            uint32 mask = 1;
            perCompMask[compIdx] = 1;
            while ((mask & end) != end)
            {
                mask <<= 1;
                mask |= 1;
            }

            mask <<= offset;
            perCompMask[compIdx] = mask;
        }

#if LITTLE_ENDIAN
        // Copying data
        for (uint32 i = 0; i < pixelData.size(); ++i)
        {
            uint8 *pixelStagingPtr = stagingPtr + (i * formatInfo->pixelDataSize);
            for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
            {
                const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

                const uint32 compOffset = formatInfo->getOffset(EPixelComponent(compIdx));
                const uint8 compSizeBits = formatInfo->componentSize[compIdx];

                // We are never going to go above 32 bits per channel
                const uint32 maxVal = (Math::pow(2u, compSizeBits) - 1);
                const uint32 compValue = uint32(pixelData[i].getColorValue()[compIdx] * maxVal);

                uint8 *offsetStagingPtr = pixelStagingPtr + (compOffset / 8);

                // Left shift
                const uint32 byte1Shift = compOffset % 8;

                (*reinterpret_cast<uint32 *>(offsetStagingPtr)) |= (perCompMask[compIdx] & (compValue << byte1Shift));
            }
        }
#else
#error Big endian platform not supported!
#endif
    }
}

void IRenderCommandList::copyPixelsLinearMappedTo(
    BufferResourceRef stagingBuffer, uint8 *stagingPtr, ArrayView<Color> pixelData, const EPixelDataFormat::PixelFormatInfo *formatInfo
) const
{
    constexpr uint32 colorCompBits = sizeof(decltype(std::declval<Color>().r())) * 8;
    static_assert(colorCompBits == 8, "Unsupported color format");

    // Mask from component's first byte
    uint32 perCompMask[MAX_PIXEL_COMP_COUNT];
    for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
    {
        const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

        uint8 offset = formatInfo->getOffset(EPixelComponent(compIdx)) % 8;
        fatalAssertf(
            ((sizeof(uint32) * 8) - offset) >= formatInfo->componentSize[compIdx],
            "Component {} of pixel format {} is going beyond 32bits mask after offset", compIdx, formatInfo->formatName
        );

        uint32 end = 1 << (formatInfo->componentSize[compIdx] - 1);
        uint32 mask = 1;
        perCompMask[compIdx] = 1;
        while ((mask & end) != end)
        {
            mask <<= 1;
            mask |= 1;
        }

        mask <<= offset;
        perCompMask[compIdx] = mask;
    }

    memset(stagingPtr, 0, stagingBuffer->getResourceSize());

#if LITTLE_ENDIAN
    // Copying data
    for (uint32 i = 0; i < pixelData.size(); ++i)
    {
        uint8 *pixelStagingPtr = stagingPtr + (i * formatInfo->pixelDataSize);
        for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
        {
            const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

            const uint32 compOffset = formatInfo->getOffset(EPixelComponent(compIdx));
            const uint8 compSizeBits = formatInfo->componentSize[compIdx];

            // We are never going to go above 32 bits per channel
            const uint32 maxVal = (Math::pow(2u, compSizeBits) - 1);
            const uint32 compValue = uint32((pixelData[i].getColorValue()[compIdx] / 255.0f) * maxVal);

            uint8 *offsetStagingPtr = pixelStagingPtr + (compOffset / 8);

            // Left shift
            const uint32 byte1Shift = compOffset % 8;

            (*reinterpret_cast<uint32 *>(offsetStagingPtr)) |= (perCompMask[compIdx] & (compValue << byte1Shift));
        }
    }
#else
#error Big endian platform not supported!
#endif
}

void IRenderCommandList::copyToImage(ImageResourceRef dst, ArrayView<Color> pixelData)
{
    if (pixelData.size() < (dst->getImageSize().z * dst->getImageSize().y * dst->getImageSize().x) * dst->getLayerCount())
    {
        LOG_ERROR("RenderCommandList", "Texel data count is not sufficient to fill all texels of {}", dst->getResourceName().getChar());
        return;
    }
    CopyPixelsToImageInfo copyInfo;
    copyInfo.dstOffset = { 0, 0, 0 };
    copyInfo.extent = dst->getImageSize();
    copyInfo.subres.baseLayer = 0;
    copyInfo.subres.layersCount = dst->getLayerCount();
    copyInfo.subres.baseMip = 0;
    copyInfo.subres.mipCount = dst->getNumOfMips();
    copyInfo.bGenerateMips = true;
    copyInfo.mipFiltering = ESamplerFiltering::Nearest;
    copyToImage(dst, pixelData, copyInfo);
}

void IRenderCommandList::copyToImageLinearMapped(ImageResourceRef dst, ArrayView<Color> pixelData)
{
    if (pixelData.size() < (dst->getImageSize().z * dst->getImageSize().y * dst->getImageSize().x) * dst->getLayerCount())
    {
        LOG_ERROR("RenderCommandList", "Texel data count is not sufficient to fill all texels of {}", dst->getResourceName().getChar());
        return;
    }
    CopyPixelsToImageInfo copyInfo;
    copyInfo.dstOffset = { 0, 0, 0 };
    copyInfo.extent = dst->getImageSize();
    copyInfo.subres.baseLayer = 0;
    copyInfo.subres.layersCount = dst->getLayerCount();
    copyInfo.subres.baseMip = 0;
    copyInfo.subres.mipCount = dst->getNumOfMips();
    copyInfo.bGenerateMips = true;
    copyInfo.mipFiltering = ESamplerFiltering::Nearest;
    copyToImageLinearMapped(dst, pixelData, copyInfo);
}

template <typename T>
struct PushConstCopier
{
    std::vector<uint8> &d;
    PushConstCopier(std::vector<uint8> &inData)
        : d(inData)
    {}

    void operator() (CopyBufferInfo &copyInfo, const std::any &data, const ReflectBufferEntry *field)
    {
        const T *value = std::any_cast<T>(&data);
        if (value)
        {
            copyInfo.copySize = sizeof(T);
            copyInfo.srcOffset = uint32(d.size());
            copyInfo.dstOffset = field->data.offset;

            d.resize(copyInfo.srcOffset + copyInfo.copySize);
            memcpy(&d[copyInfo.srcOffset], value, copyInfo.copySize);
        }
        else
        {
            LOG_ERROR("RenderCommandList", "Cannot cast pushable constant {}", field->attributeName.c_str());
        }
    }
};

void IRenderCommandList::cmdPushConstants(
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, ArrayView<std::pair<String, std::any>> pushData
) const
{
    const ReflectPushConstant &entry = contextPipeline.getPipeline()->getShaderResource()->getReflection()->pushConstants;

    if (!entry.data.pushConstantField.bufferStructFields.empty())
    {
        LOG_WARN(
            "RenderCommandList", "[Shader: {}, Attribute: {}]Using SoS in push constant in not recommended",
            contextPipeline.getPipeline()->getShaderResource()->getResourceName(), entry.attributeName.c_str()
        );
    }

    if (entry.data.pushConstantField.bufferFields.empty() && entry.data.pushConstantField.bufferStructFields.empty())
    {
        return;
    }

    std::unordered_map<String, const ReflectBufferEntry *> nameToEntry;
    {
        std::vector<const ReflectBufferShaderField *> tree{ &entry.data.pushConstantField };

        for (uint32 i = 0; i < tree.size(); ++i)
        {
            const ReflectBufferShaderField *current = tree[i];
            for (const ReflectBufferEntry &field : current->bufferFields)
            {
                String fieldAttribName{ UTF8_TO_TCHAR(field.attributeName.c_str()) };
                if (field.data.arraySize.size() != 1 || field.data.arraySize[0].isSpecializationConst || field.data.arraySize[0].dimension != 1)
                {
                    LOG_WARN(
                        "RenderCommandList",
                        "[Shader: {}, Attribute: {}] Array data is not supported in "
                        "push constants",
                        contextPipeline.getPipeline()->getShaderResource()->getResourceName(), fieldAttribName
                    );
                }
                else
                {
                    nameToEntry[fieldAttribName] = &field;
                }
            }

            for (const ReflectBufferStructEntry &structField : current->bufferStructFields)
            {
                tree.emplace_back(&structField.data.data);
            }
        }
    }

    std::vector<uint8> data;
    std::vector<CopyBufferInfo> copies;
    for (const std::pair<String, std::any> &pushConst : pushData)
    {
        auto itr = nameToEntry.find(pushConst.first);
        if (itr == nameToEntry.end())
        {
            LOG_ERROR("RenderCommandList", "Cannot find {} in pushable constants", pushConst.first);
            continue;
        }

        EShaderInputAttribFormat::Type format = EShaderInputAttribFormat::getInputFormat(itr->second->data.data.type);
        switch (format)
        {
        case EShaderInputAttribFormat::Float:
            PushConstCopier<float>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Float2:
            PushConstCopier<Vector2>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Float3:
            PushConstCopier<Vector3>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Float4:
            PushConstCopier<Vector4>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Int:
            PushConstCopier<int32>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Int2:
            PushConstCopier<Int2>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Int3:
            PushConstCopier<Int3>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Int4:
            PushConstCopier<Int4>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UInt:
            PushConstCopier<uint32>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UInt2:
            PushConstCopier<UInt2>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UInt3:
            PushConstCopier<UInt3>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UInt4:
            PushConstCopier<UInt4>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UByte:
            PushConstCopier<uint8>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UByte2:
            PushConstCopier<Byte2>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UByte3:
            PushConstCopier<Byte3>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UByte4:
            PushConstCopier<Byte4>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Matrix2x2:
            PushConstCopier<Matrix2>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Matrix3x3:
            PushConstCopier<Matrix3>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Matrix4x4:
            PushConstCopier<Matrix4>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Double:
        case EShaderInputAttribFormat::Double2:
        case EShaderInputAttribFormat::Double3:
        case EShaderInputAttribFormat::Double4:
        case EShaderInputAttribFormat::Byte:
        case EShaderInputAttribFormat::Byte2:
        case EShaderInputAttribFormat::Byte3:
        case EShaderInputAttribFormat::Byte4:
        case EShaderInputAttribFormat::ShortInt:
        case EShaderInputAttribFormat::ShortInt2:
        case EShaderInputAttribFormat::ShortInt3:
        case EShaderInputAttribFormat::ShortInt4:
        case EShaderInputAttribFormat::UShortInt:
        case EShaderInputAttribFormat::UShortInt2:
        case EShaderInputAttribFormat::UShortInt3:
        case EShaderInputAttribFormat::UShortInt4:
        case EShaderInputAttribFormat::UInt4Norm:
        case EShaderInputAttribFormat::Undefined:
        default:
            LOG_ERROR(
                "RenderCommandList", "[Shader: {}, Attribute: {}] Unsupported format {} in push constants",
                contextPipeline.getPipeline()->getShaderResource()->getResourceName().getChar(), itr->second->attributeName.c_str(),
                pushConst.second.type().name()
            );
            break;
        }
    }

    cmdPushConstants(
        cmdBuffer, contextPipeline, contextPipeline.getPipeline()->getShaderResource()->getReflection()->pushConstants.data.stagesUsed,
        data.data(), copies
    );
}

void IRenderCommandList::cmdBindDescriptorsSets(
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, ShaderParametersRef descriptorsSets
) const
{
    if (descriptorsSets->getParamLayout()->getType()->isChildOf<ShaderParametersLayout>())
    {
        cmdBindDescriptorsSetsInternal(cmdBuffer, contextPipeline.getPipeline(), { &descriptorsSets, 1 });
    }
    else if (descriptorsSets->getParamLayout()->getType()->isChildOf<ShaderSetParametersLayout>())
    {
        std::pair<uint32, const ShaderParametersRef> setIdToDescsSet{
            static_cast<const ShaderSetParametersLayout *>(descriptorsSets->getParamLayout())->getSetID(), descriptorsSets
        };
        cmdBindDescriptorsSetInternal(cmdBuffer, contextPipeline.getPipeline(), { setIdToDescsSet });
    }
}

void IRenderCommandList::cmdBindDescriptorsSets(
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, ArrayView<ShaderParametersRef> descriptorsSets
) const
{
    std::vector<ShaderParametersRef> shaderParamsSetsList;
    std::map<uint32, ShaderParametersRef> shaderParamsSetList;

    for (const ShaderParametersRef &shaderParams : descriptorsSets)
    {
        if (shaderParams->getParamLayout()->getType()->isChildOf<ShaderParametersLayout>())
        {
            shaderParamsSetsList.emplace_back(shaderParams);
        }
        else if (shaderParams->getParamLayout()->getType()->isChildOf<ShaderSetParametersLayout>())
        {
            shaderParamsSetList[static_cast<const ShaderSetParametersLayout *>(shaderParams->getParamLayout())->getSetID()] = shaderParams;
        }
    }

    if (!shaderParamsSetsList.empty())
    {
        cmdBindDescriptorsSetsInternal(cmdBuffer, contextPipeline.getPipeline(), shaderParamsSetsList);
    }

    if (!shaderParamsSetList.empty())
    {
        cmdBindDescriptorsSetInternal(cmdBuffer, contextPipeline.getPipeline(), shaderParamsSetList);
    }
}

void IRenderCommandList::recordCopyToBuffer(
    std::vector<BatchCopyBufferData> &recordTo, BufferResourceRef dst, uint32 dstOffset, const void *dataToCopy,
    const ShaderBufferParamInfo *bufferFields
)
{
    for (const ShaderBufferField *bufferField : *bufferFields)
    {
        BatchCopyBufferData copyData;
        copyData.dst = dst;
        copyData.dstOffset = dstOffset + bufferField->offset;
        copyData.dataToCopy = bufferField->fieldData(dataToCopy, &copyData.size, nullptr);
        recordTo.push_back(copyData);
    }
}

bool ShaderParameters::setBuffer(StringID paramName, const void *bufferValue, uint32 index /*= 0*/)
{
    bool bValueSet = false;
    auto bufferDataItr = shaderBuffers.find(paramName);

    if (bufferDataItr == shaderBuffers.end())
    {
        StringID bufferName;
        std::pair<const BufferParametersData *, const BufferParametersData::BufferParameter *> foundInfo
            = findBufferParam(bufferName, paramName);
        if (foundInfo.first && foundInfo.second && BIT_SET(foundInfo.second->bufferField->fieldDecorations, ShaderBufferField::IsStruct)
            && (!foundInfo.second->bufferField->isPointer() || foundInfo.first->runtimeArray->currentCount > index))
        {
            if (foundInfo.second->bufferField->isIndexAccessible())
            {
                bValueSet = foundInfo.second->bufferField->setFieldDataArray(foundInfo.second->outerPtr, bufferValue, index);
                if (bValueSet)
                {
                    genericUpdates.emplace_back(
                        [foundInfo, index](ParamUpdateLambdaOut &paramOut, IRenderCommandList *cmdList, IGraphicsInstance *)
                        {
                            const uint32 bufferNativeStride = foundInfo.second->bufferField->paramInfo->paramNativeStride();
                            void *bufferPtr = foundInfo.second->bufferField->fieldData(foundInfo.second->outerPtr, nullptr, nullptr);
                            cmdList->recordCopyToBuffer(
                                *paramOut.bufferUpdates, foundInfo.first->gpuBuffer,
                                foundInfo.second->bufferField->offset + (index * foundInfo.second->bufferField->stride),
                                (void *)(UPtrInt(bufferPtr) + (bufferNativeStride * index)), foundInfo.second->bufferField->paramInfo
                            );
                        }
                    );
                }
            }
            else
            {
                bValueSet = foundInfo.second->bufferField->setFieldData(foundInfo.second->outerPtr, bufferValue);
                if (bValueSet)
                {
                    genericUpdates.emplace_back(
                        [foundInfo](ParamUpdateLambdaOut &paramOut, IRenderCommandList *cmdList, IGraphicsInstance *)
                        {
                            cmdList->recordCopyToBuffer(
                                *paramOut.bufferUpdates, foundInfo.first->gpuBuffer, foundInfo.second->bufferField->offset,
                                foundInfo.second->bufferField->fieldData(foundInfo.second->outerPtr, nullptr, nullptr),
                                foundInfo.second->bufferField->paramInfo
                            );
                        }
                    );
                }
            }
        }
        else
        {
            LOG_ERROR("ShaderParameters", "Cannot set {}[{}] of {}", paramName, index, bufferName);
        }
    }
    else
    {
        BufferParametersData *bufferDataPtr = &bufferDataItr->second;
        bValueSet = !bufferDataPtr->runtimeArray.has_value();
        if (bValueSet)
        {
            const uint32 bufferNativeStride = bufferDataPtr->descriptorInfo->bufferParamInfo->paramNativeStride();
            CBEMemory::memCopy(bufferDataPtr->cpuBuffer, bufferValue, bufferNativeStride);
            genericUpdates.emplace_back(
                [bufferDataPtr](ParamUpdateLambdaOut &paramOut, IRenderCommandList *cmdList, IGraphicsInstance *)
                {
                    cmdList->recordCopyToBuffer(
                        *paramOut.bufferUpdates, bufferDataPtr->gpuBuffer, 0, bufferDataPtr->cpuBuffer,
                        bufferDataPtr->descriptorInfo->bufferParamInfo
                    );
                }
            );
        }
        else
        {
            LOG_ERROR(
                "ShaderParameters", "Cannot set buffer with runtime array as single struct, Set runtime array separately",
                bufferDataPtr->descriptorInfo->bufferParamInfo->paramNativeStride()
            );
        }
    }
    return bValueSet;
}

IRenderCommandList *IRenderCommandList::genericInstance() { return new RenderCommandList(); }