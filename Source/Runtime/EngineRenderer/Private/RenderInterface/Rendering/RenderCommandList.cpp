/*!
 * \file RenderCommandList.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/CoreMathTypes.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderInterface/Resources/ShaderResources.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "ShaderReflected.h"
#include "Types/Colors.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/PlatformFunctions.h"

#include <utility>

ScopedCommandMarker::ScopedCommandMarker(
    const class IRenderCommandList *commandList, const GraphicsResource *commandBuffer, const String &name,
    const LinearColor &color /*= LinearColorConst::WHITE*/
)
    : cmdList(commandList)
    , cmdBuffer(commandBuffer)
{
    cmdList->cmdBeginBufferMarker(cmdBuffer, name, color);
}

ScopedCommandMarker::~ScopedCommandMarker() { cmdList->cmdEndBufferMarker(cmdBuffer); }

// This must be modified to be a thread safe call when from other threads
class RenderCommandList final : public IRenderCommandList
{
private:
    IRenderCommandList *cmdList;

public:
    void setup(IRenderCommandList *commandList) final;
    void newFrame(float timeDelta) final;

    void copyToBuffer(BufferResourceRef dst, uint32 dstOffset, const void *dataToCopy, uint32 size) final;
    void copyToBuffer(const std::vector<BatchCopyBufferData> &batchCopies) final;
    void copyBuffer(BufferResourceRef src, BufferResourceRef dst, const CopyBufferInfo &copyInfo) final;
    void copyBuffer(const std::vector<BatchCopyBufferInfo> &batchCopies) final;

    void copyToImage(ImageResourceRef dst, const std::vector<class Color> &pixelData, const CopyPixelsToImageInfo &copyInfo) final;
    void copyToImage(ImageResourceRef dst, const std::vector<class LinearColor> &pixelData, const CopyPixelsToImageInfo &copyInfo) final;
    void copyToImageLinearMapped(ImageResourceRef dst, const std::vector<class Color> &pixelData, const CopyPixelsToImageInfo &copyInfo) final;

    void copyOrResolveImage(ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo, const CopyImageInfo &dstInfo) final;

    void clearImage(ImageResourceRef image, const LinearColor &clearColor, const std::vector<ImageSubresource> &subresources) final;
    void clearDepth(ImageResourceRef image, float depth, uint32 stencil, const std::vector<ImageSubresource> &subresources) final;

    void setupInitialLayout(ImageResourceRef image) final;

    void presentImage(
        const std::vector<WindowCanvasRef> &canvases, const std::vector<uint32> &imageIndices, const std::vector<SemaphoreRef> &waitOnSemaphores
    );

    void cmdCopyOrResolveImage(
        const GraphicsResource *cmdBuffer, ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo,
        const CopyImageInfo &dstInfo
    ) final;

    void cmdTransitionLayouts(const GraphicsResource *cmdBuffer, const std::vector<ImageResourceRef> &images) final;

    void cmdClearImage(
        const GraphicsResource *cmdBuffer, ImageResourceRef image, const LinearColor &clearColor,
        const std::vector<ImageSubresource> &subresources
    ) final;
    void cmdClearDepth(
        const GraphicsResource *cmdBuffer, ImageResourceRef image, float depth, uint32 stencil,
        const std::vector<ImageSubresource> &subresources
    ) final;

    void cmdBarrierResources(const GraphicsResource *cmdBuffer, const std::set<ShaderParametersRef> &descriptorsSets) final;

    void cmdBeginRenderPass(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const QuantizedBox2D &renderArea,
        const RenderPassAdditionalProps &renderpassAdditionalProps, const RenderPassClearValue &clearColor
    ) final;
    void cmdEndRenderPass(const GraphicsResource *cmdBuffer) final;

    void cmdBindGraphicsPipeline(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const GraphicsPipelineState &state
    ) const final;
    void cmdBindComputePipeline(const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline) const final;
    void cmdPushConstants(
        const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, uint32 stagesUsed, const uint8 *data,
        const std::vector<CopyBufferInfo> &pushConsts
    ) const final;
    void cmdBindDescriptorsSetInternal(
        const GraphicsResource *cmdBuffer, const PipelineBase *contextPipeline, const std::map<uint32, ShaderParametersRef> &descriptorsSets
    ) const final;
    void cmdBindDescriptorsSetsInternal(
        const GraphicsResource *cmdBuffer, const PipelineBase *contextPipeline, const std::vector<ShaderParametersRef> &descriptorsSets
    ) const final;
    void cmdBindVertexBuffers(
        const GraphicsResource *cmdBuffer, uint32 firstBinding, const std::vector<BufferResourceRef> &vertexBuffers,
        const std::vector<uint64> &offsets
    ) const final;
    void cmdBindIndexBuffer(const GraphicsResource *cmdBuffer, const BufferResourceRef &indexBuffer, uint64 offset = 0) const final;

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
    ) const final;
    void cmdDrawIndirect(
        const GraphicsResource *cmdBuffer, const BufferResourceRef &drawCmdsBuffer, uint32 bufferOffset, uint32 drawCount, uint32 stride
    ) const final;

    void cmdSetViewportAndScissors(
        const GraphicsResource *cmdBuffer, const std::vector<std::pair<QuantizedBox2D, QuantizedBox2D>> &viewportAndScissors,
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

    const GraphicsResource *startCmd(const String &uniqueName, EQueueFunction queue, bool bIsReusable) final;
    void endCmd(const GraphicsResource *cmdBuffer) final;
    void freeCmd(const GraphicsResource *cmdBuffer) final;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo &submitInfo, FenceRef fence) final;
    void submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo> &submitInfos, FenceRef fence) final;
    void submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &submitInfo);
    void submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo2> &submitInfos) final;
    void submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &command) final;
    void finishCmd(const GraphicsResource *cmdBuffer) final;
    void finishCmd(const String &uniqueName) final;
    const GraphicsResource *getCmdBuffer(const String &uniqueName) const final;
    SemaphoreRef getCmdSignalSemaphore(const String &uniqueName) const final;
    SemaphoreRef getCmdSignalSemaphore(const GraphicsResource *cmdBuffer) const final;
    void waitIdle() final;
    void waitOnResDepCmds(const MemoryResourceRef &resource) final;
    void flushAllcommands() final;
};

void RenderCommandList::cmdPushConstants(
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, uint32 stagesUsed, const uint8 *data,
    const std::vector<CopyBufferInfo> &pushConsts
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
    const GraphicsResource *cmdBuffer, const PipelineBase *contextPipeline, const std::vector<ShaderParametersRef> &descriptorsSets
) const
{
    cmdList->cmdBindDescriptorsSetsInternal(cmdBuffer, contextPipeline, descriptorsSets);
}

void RenderCommandList::cmdBindVertexBuffers(
    const GraphicsResource *cmdBuffer, uint32 firstBinding, const std::vector<BufferResourceRef> &vertexBuffers,
    const std::vector<uint64> &offsets
) const
{
    cmdList->cmdBindVertexBuffers(cmdBuffer, firstBinding, vertexBuffers, offsets);
}

void RenderCommandList::cmdBindIndexBuffer(const GraphicsResource *cmdBuffer, const BufferResourceRef &indexBuffer, uint64 offset /*= 0*/) const
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
) const
{
    cmdList->cmdDrawIndexedIndirect(cmdBuffer, drawCmdsBuffer, bufferOffset, drawCount, stride);
}

void RenderCommandList::cmdDrawIndirect(
    const GraphicsResource *cmdBuffer, const BufferResourceRef &drawCmdsBuffer, uint32 bufferOffset, uint32 drawCount, uint32 stride
) const
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

void RenderCommandList::copyBuffer(BufferResourceRef src, BufferResourceRef dst, const CopyBufferInfo &copyInfo)
{
    cmdList->copyBuffer(src, dst, copyInfo);
}

void RenderCommandList::copyBuffer(const std::vector<BatchCopyBufferInfo> &batchCopies) { cmdList->copyBuffer(batchCopies); }

void RenderCommandList::copyToBuffer(BufferResourceRef dst, uint32 dstOffset, const void *dataToCopy, uint32 size)
{
    cmdList->copyToBuffer(dst, dstOffset, dataToCopy, size);
}

void RenderCommandList::copyToBuffer(const std::vector<BatchCopyBufferData> &batchCopies) { cmdList->copyToBuffer(batchCopies); }

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

void RenderCommandList::submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo> &submitInfos, FenceRef fence)
{
    cmdList->submitCmds(priority, submitInfos, fence);
}

void RenderCommandList::submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &submitInfo)
{
    cmdList->submitWaitCmd(priority, submitInfo);
}

void RenderCommandList::submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo2> &submitInfos)
{
    cmdList->submitCmds(priority, submitInfos);
}

void RenderCommandList::submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &command) { cmdList->submitCmd(priority, command); }

void RenderCommandList::finishCmd(const GraphicsResource *cmdBuffer) { cmdList->finishCmd(cmdBuffer); }

void RenderCommandList::finishCmd(const String &uniqueName) { cmdList->finishCmd(uniqueName); }

const GraphicsResource *RenderCommandList::getCmdBuffer(const String &uniqueName) const { return cmdList->getCmdBuffer(uniqueName); }

SemaphoreRef RenderCommandList::getCmdSignalSemaphore(const String &uniqueName) const { return cmdList->getCmdSignalSemaphore(uniqueName); }

SemaphoreRef RenderCommandList::getCmdSignalSemaphore(const GraphicsResource *cmdBuffer) const
{
    return cmdList->getCmdSignalSemaphore(cmdBuffer);
}

void RenderCommandList::waitIdle() { cmdList->waitIdle(); }

void RenderCommandList::waitOnResDepCmds(const MemoryResourceRef &resource) { cmdList->waitOnResDepCmds(resource); }

void RenderCommandList::flushAllcommands() { cmdList->flushAllcommands(); }

void RenderCommandList::copyToImage(ImageResourceRef dst, const std::vector<class Color> &pixelData, const CopyPixelsToImageInfo &copyInfo)
{
    cmdList->copyToImage(dst, pixelData, copyInfo);
}

void RenderCommandList::copyToImage(
    ImageResourceRef dst, const std::vector<class LinearColor> &pixelData, const CopyPixelsToImageInfo &copyInfo
)
{
    cmdList->copyToImage(dst, pixelData, copyInfo);
}

void RenderCommandList::copyToImageLinearMapped(
    ImageResourceRef dst, const std::vector<class Color> &pixelData, const CopyPixelsToImageInfo &copyInfo
)
{
    cmdList->copyToImageLinearMapped(dst, pixelData, copyInfo);
}

void RenderCommandList::copyOrResolveImage(
    ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo, const CopyImageInfo &dstInfo
)
{
    cmdList->copyOrResolveImage(src, dst, srcInfo, dstInfo);
}

void RenderCommandList::clearImage(ImageResourceRef image, const LinearColor &clearColor, const std::vector<ImageSubresource> &subresources)
{
    cmdList->clearImage(image, clearColor, subresources);
}

void RenderCommandList::clearDepth(ImageResourceRef image, float depth, uint32 stencil, const std::vector<ImageSubresource> &subresources)
{
    cmdList->clearDepth(image, depth, stencil, subresources);
}

void RenderCommandList::setupInitialLayout(ImageResourceRef image) { cmdList->setupInitialLayout(image); }

void RenderCommandList::presentImage(
    const std::vector<WindowCanvasRef> &canvases, const std::vector<uint32> &imageIndices, const std::vector<SemaphoreRef> &waitOnSemaphores
)
{
    cmdList->presentImage(canvases, imageIndices, waitOnSemaphores);
}

void RenderCommandList::cmdCopyOrResolveImage(
    const GraphicsResource *cmdBuffer, ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo, const CopyImageInfo &dstInfo
)
{
    cmdList->cmdCopyOrResolveImage(cmdBuffer, src, dst, srcInfo, dstInfo);
}

void RenderCommandList::cmdTransitionLayouts(const GraphicsResource *cmdBuffer, const std::vector<ImageResourceRef> &images)
{
    cmdList->cmdTransitionLayouts(cmdBuffer, images);
}

void RenderCommandList::cmdClearImage(
    const GraphicsResource *cmdBuffer, ImageResourceRef image, const LinearColor &clearColor, const std::vector<ImageSubresource> &subresources
)
{
    cmdList->cmdClearImage(cmdBuffer, image, clearColor, subresources);
}

void RenderCommandList::cmdClearDepth(
    const GraphicsResource *cmdBuffer, ImageResourceRef image, float depth, uint32 stencil, const std::vector<ImageSubresource> &subresources
)
{
    cmdList->cmdClearDepth(cmdBuffer, image, depth, stencil, subresources);
}

void RenderCommandList::cmdBarrierResources(const GraphicsResource *cmdBuffer, const std::set<ShaderParametersRef> &descriptorsSets)
{
    cmdList->cmdBarrierResources(cmdBuffer, descriptorsSets);
}

void RenderCommandList::cmdBeginRenderPass(
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const QuantizedBox2D &renderArea,
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
    const GraphicsResource *cmdBuffer, const std::vector<std::pair<QuantizedBox2D, QuantizedBox2D>> &viewportAndScissors,
    uint32 firstViewport /*= 0*/
) const
{
    cmdList->cmdSetViewportAndScissors(cmdBuffer, viewportAndScissors, firstViewport);
}

void RenderCommandList::cmdSetViewportAndScissor(
    const GraphicsResource *cmdBuffer, const QuantizedBox2D &viewport, const QuantizedBox2D &scissor, uint32 atViewport /*= 0*/
) const
{
    cmdList->cmdSetViewportAndScissor(cmdBuffer, viewport, scissor, atViewport);
}

void RenderCommandList::cmdSetScissor(const GraphicsResource *cmdBuffer, const QuantizedBox2D &scissor, uint32 atViewport /*= 0*/) const
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
    BufferResourceRef stagingBuffer, uint8 *stagingPtr, const std::vector<class Color> &pixelData, EPixelDataFormat::Type dataFormat,
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
    BufferResourceRef stagingBuffer, uint8 *stagingPtr, const std::vector<Color> &pixelData, const EPixelDataFormat::PixelFormatInfo *formatInfo
) const
{
    constexpr uint32 colorCompBits = sizeof(decltype(std::declval<Color>().r())) * 8;
    debugAssert(colorCompBits == 8);

    // Mask from component's first byte
    uint32 perCompMask[MAX_PIXEL_COMP_COUNT];
    for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
    {
        const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

        uint8 offset = formatInfo->getOffset(EPixelComponent(compIdx)) % 8;
        fatalAssertf(
            ((sizeof(uint32) * 8) - offset) >= formatInfo->componentSize[compIdx],
            "Component %d of pixel format %s is going beyond 32bits mask after offset", compIdx, formatInfo->formatName
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

    if (GPlatformConfigs::PLATFORM_ENDIAN.isLittleEndian())
    {
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
                const uint32 compValue = pixelData[i].getColorValue()[compIdx];

                uint8 *offsetStagingPtr = pixelStagingPtr + (compOffset / 8);

                // Left shift
                const uint32 byte1Shift = compOffset % 8;

                (*reinterpret_cast<uint32 *>(offsetStagingPtr)) |= (perCompMask[compIdx] & (compValue << byte1Shift));
            }
        }
    }
    else
    {
        fatalAssertf(false, "Big endian platform not supported yet");
    }
}

void IRenderCommandList::copyPixelsTo(
    BufferResourceRef stagingBuffer, uint8 *stagingPtr, const std::vector<class LinearColor> &pixelData,
    const EPixelDataFormat::PixelFormatInfo *formatInfo, bool bIsFloatingFormat
) const
{
    constexpr uint32 colorCompBits = sizeof(decltype(std::declval<LinearColor>().r())) * 8;
    debugAssert(colorCompBits == 32);

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
                "Component %d of pixel format %s is going beyond 32bits mask after offset", compIdx, formatInfo->formatName
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

        if (GPlatformConfigs::PLATFORM_ENDIAN.isLittleEndian())
        {
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
        }
        else
        {
            fatalAssertf(false, "Big endian platform not supported yet");
        }
    }
}

void IRenderCommandList::copyPixelsLinearMappedTo(
    BufferResourceRef stagingBuffer, uint8 *stagingPtr, const std::vector<class Color> &pixelData,
    const EPixelDataFormat::PixelFormatInfo *formatInfo
) const
{
    constexpr uint32 colorCompBits = sizeof(decltype(std::declval<Color>().r())) * 8;
    debugAssert(colorCompBits == 8);

    // Mask from component's first byte
    uint32 perCompMask[MAX_PIXEL_COMP_COUNT];
    for (uint8 idx = 0; idx < formatInfo->componentCount; ++idx)
    {
        const uint8 compIdx = uint8(formatInfo->componentOrder[idx]);

        uint8 offset = formatInfo->getOffset(EPixelComponent(compIdx)) % 8;
        fatalAssertf(
            ((sizeof(uint32) * 8) - offset) >= formatInfo->componentSize[compIdx],
            "Component %d of pixel format %s is going beyond 32bits mask after offset", compIdx, formatInfo->formatName
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

    if (GPlatformConfigs::PLATFORM_ENDIAN.isLittleEndian())
    {
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
    }
    else
    {
        fatalAssertf(false, "Big endian platform not supported yet");
    }
}

void IRenderCommandList::copyToImage(ImageResourceRef dst, const std::vector<class Color> &pixelData)
{
    if (pixelData.size() < (dst->getImageSize().z * dst->getImageSize().y * dst->getImageSize().x) * dst->getLayerCount())
    {
        LOG_ERROR("RenderCommandList", "Texel data count is not sufficient to fill all texels of %s", dst->getResourceName().getChar());
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

void IRenderCommandList::copyToImageLinearMapped(ImageResourceRef dst, const std::vector<class Color> &pixelData)
{
    if (pixelData.size() < (dst->getImageSize().z * dst->getImageSize().y * dst->getImageSize().x) * dst->getLayerCount())
    {
        LOG_ERROR("RenderCommandList", "Texel data count is not sufficient to fill all texels of %s", dst->getResourceName().getChar());
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

    void operator()(CopyBufferInfo &copyInfo, const std::any &data, const ReflectBufferEntry *field)
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
            LOG_ERROR("RenderCommandList", "Cannot cast pushable constant %s", field->attributeName.c_str());
        }
    }
};

void IRenderCommandList::cmdPushConstants(
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const std::vector<std::pair<String, std::any>> &pushData
) const
{
    const ReflectPushConstant &entry = contextPipeline.getPipeline()->getShaderResource()->getReflection()->pushConstants;

    if (!entry.data.pushConstantField.bufferStructFields.empty())
    {
        LOG_WARN(
            "RenderCommandList", "[Shader: %s, Attribute: %s]Using SoS in push constant in not recommended",
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
                        "[Shader: %s, Attribute: %s] Array data is not supported in "
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
            LOG_ERROR("RenderCommandList", "Cannot find %s in pushable constants", pushConst.first);
            continue;
        }

        EShaderInputAttribFormat::Type format = EShaderInputAttribFormat::getInputFormat(itr->second->data.data.type);
        switch (format)
        {
        case EShaderInputAttribFormat::Float:
            PushConstCopier<float>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Float2:
            PushConstCopier<Vector2D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Float3:
            PushConstCopier<Vector3D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Float4:
            PushConstCopier<Vector4D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Int:
            PushConstCopier<int32>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Int2:
            PushConstCopier<Int2D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Int3:
            PushConstCopier<Int3D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::Int4:
            PushConstCopier<Int4D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UInt:
            PushConstCopier<uint32>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UInt2:
            PushConstCopier<Size2D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UInt3:
            PushConstCopier<Size3D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UInt4:
            PushConstCopier<Size4D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UByte:
            PushConstCopier<uint8>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UByte2:
            PushConstCopier<Byte2D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UByte3:
            PushConstCopier<Byte3D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
            break;
        case EShaderInputAttribFormat::UByte4:
            PushConstCopier<Byte4D>{ data }(copies.emplace_back(), pushConst.second, itr->second);
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
                "RenderCommandList", "[Shader: %s, Attribute: %s] Unsupported format %s in push constants",
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
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const ShaderParametersRef &descriptorsSets
) const
{
    if (descriptorsSets->getParamLayout()->getType()->isChildOf<ShaderParametersLayout>())
    {
        cmdBindDescriptorsSetsInternal(cmdBuffer, contextPipeline.getPipeline(), { descriptorsSets });
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
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const std::vector<ShaderParametersRef> &descriptorsSets
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

IRenderCommandList *IRenderCommandList::genericInstance() { return new RenderCommandList(); }