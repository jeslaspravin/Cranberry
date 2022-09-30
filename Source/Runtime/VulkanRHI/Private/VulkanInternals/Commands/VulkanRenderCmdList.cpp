/*!
 * \file VulkanRenderCmdList.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include <array>
#include <optional>

#include "Math/Box.h"
#include "Math/Math.h"
#include "IRenderInterfaceModule.h"
#include "RenderApi/RenderManager.h"
#include "RenderInterface/Rendering/RenderInterfaceContexts.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Resources/DeferredDeleter.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanGraphicsHelper.h"
#include "VulkanInternals/Commands/VulkanRenderCmdList.h"
#include "VulkanInternals/Rendering/VulkanRenderingContexts.h"
#include "VulkanInternals/Resources/VulkanImageResources.h"
#include "VulkanInternals/Resources/VulkanPipelines.h"
#include "VulkanInternals/ShaderCore/VulkanShaderParamResources.h"
#include "VulkanInternals/VulkanDescriptorAllocator.h"
#include "VulkanInternals/VulkanDevice.h"
#include "VulkanInternals/VulkanGraphicsTypes.h"
#include "VulkanRHIModule.h"

FORCE_INLINE VkImageAspectFlags VulkanCommandList::determineImageAspect(const ImageResourceRef &image) const
{
    return (
        EPixelDataFormat::isDepthFormat(image->imageFormat())
            ? (VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT
               | (EPixelDataFormat::isStencilFormat(image->imageFormat()) ? VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT : 0))
            : VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT
    );
}

FORCE_INLINE VkAccessFlags2 VulkanCommandList::determineImageAccessMask(const ImageResourceRef &image) const
{
    VkAccessFlags accessMask = 0;

    accessMask |= image->isShaderRead() ? VK_ACCESS_2_SHADER_READ_BIT : 0;
    accessMask |= image->isShaderWrite() ? VK_ACCESS_2_SHADER_WRITE_BIT : 0;
    if (image->getType()->isChildOf(VulkanRenderTargetResource::staticType()))
    {
        accessMask |= VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT;
        accessMask |= EPixelDataFormat::isDepthFormat(image->imageFormat()) ? VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
                                                                            : VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    }
    return accessMask;
}

FORCE_INLINE VkImageLayout VulkanCommandList::determineImageLayout(const ImageResourceRef &image) const
{
    VkImageLayout imgLayout = getImageLayout(image);
    if (imgLayout == VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED)
    {
        imgLayout = EPixelDataFormat::isDepthFormat(image->imageFormat()) ? VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                                                                          : VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imgLayout = image->getType()->isChildOf(VulkanRenderTargetResource::staticType()) ? imgLayout
                    : image->isShaderWrite()                                              ? VkImageLayout::VK_IMAGE_LAYOUT_GENERAL
                                             : VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    return imgLayout;
}

FORCE_INLINE VkImageLayout VulkanCommandList::getImageLayout(const ImageResourceRef &image) const
{
    // TODO(Jeslas) : change this to get final layout from some resource tracked layout
    VkImageLayout imgLayout = EPixelDataFormat::isDepthFormat(image->imageFormat())
                                  ? VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                                  : VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    imgLayout = image->getType()->isChildOf(VulkanRenderTargetResource::staticType()) ? imgLayout
                : image->isShaderWrite()                                              ? VkImageLayout::VK_IMAGE_LAYOUT_GENERAL
                                                                                      : VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return imgLayout;
}

FORCE_INLINE VkPipelineBindPoint VulkanCommandList::getPipelineBindPoint(const PipelineBase *pipeline) const
{
    if (pipeline->getType()->isChildOf<GraphicsPipelineBase>())
    {
        return VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
    }
    else if (pipeline->getType()->isChildOf<ComputePipelineBase>())
    {
        return VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;
    }

    LOG_ERROR("VulkanPipeline", "Invalid pipeline %s", pipeline->getResourceName().getChar());
    return VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_MAX_ENUM;
}

FORCE_INLINE VkPipelineStageFlags2 VulkanCommandList::resourceShaderStageFlags() const
{
    return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
}

FORCE_INLINE void
    VulkanCommandList::fillClearValue(EPixelDataFormat::Type format, VkClearColorValue &clearValue, const LinearColor &color) const
{
    const EPixelDataFormat::PixelFormatInfo *formatInfo = EPixelDataFormat::getFormatInfo(format);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#formats-numericformat Normalized and scaled values are
    // considered float
    if (EPixelDataFormat::isFloatingFormat(format) || EPixelDataFormat::isNormalizedFormat(format) || EPixelDataFormat::isScaledFormat(format))
    {
        clearValue.float32[0] = color.r();
        clearValue.float32[1] = color.g();
        clearValue.float32[2] = color.b();
        clearValue.float32[3] = color.a();
    }
    else
    {
        LinearColor clamped = LinearColor(Math::clamp(Vector4D(color), Vector4D::ZERO, Vector4D::ONE));
        uint32 uMaxVal = Math::pow(2, formatInfo->componentSize[0]) - 1;
        clearValue.uint32[0] = uint32(uMaxVal * clamped[0]);
        clearValue.uint32[1] = uint32(uMaxVal * clamped[1]);
        clearValue.uint32[2] = uint32(uMaxVal * clamped[2]);
        clearValue.uint32[3] = uint32(uMaxVal * clamped[3]);

        if (EPixelDataFormat::isSignedFormat(format))
        {
            clamped = LinearColor(Math::clamp(Vector4D(color), Vector4D(-1), Vector4D::ONE));
            int32 signedDelta = Math::pow(2, formatInfo->componentSize[0] - 1);
            clearValue.int32[0] = clearValue.uint32[0] - signedDelta;
            clearValue.int32[1] = clearValue.uint32[1] - signedDelta;
            clearValue.int32[2] = clearValue.uint32[2] - signedDelta;
            clearValue.int32[3] = clearValue.uint32[3] - signedDelta;
        }
    }
}

#if DEFER_DELETION
DeferredDeleter *VulkanGraphicsHelper::getDeferredDeleter(class IGraphicsInstance *graphicsInstance)
{
    VulkanGlobalRenderingContext *renderingCntxt
        = static_cast<VulkanGlobalRenderingContext *>(IRenderInterfaceModule::get()->getRenderManager()->getGlobalRenderingContext());
    return renderingCntxt->getDeferredDeleter();
}
#endif

FORCE_INLINE void cmdPipelineBarrier(
    VulkanDevice *vDevice, VkCommandBuffer cmdBuffer, ArrayView<const VkImageMemoryBarrier2> imageBarriers,
    ArrayView<const VkBufferMemoryBarrier2> bufferBarriers
)
{
    if (vDevice->vkCmdPipelineBarrier2KHR)
    {
        BARRIER_DEPENDENCY_INFO(dependencyInfo);
        dependencyInfo.dependencyFlags = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT;
        dependencyInfo.pImageMemoryBarriers = imageBarriers.data();
        dependencyInfo.imageMemoryBarrierCount = uint32(imageBarriers.size());
        dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();
        dependencyInfo.bufferMemoryBarrierCount = uint32(bufferBarriers.size());
        vDevice->vkCmdPipelineBarrier2KHR(cmdBuffer, &dependencyInfo);
    }
    else
    {
        struct Barriers
        {
            std::vector<VkImageMemoryBarrier> imgs;
            std::vector<VkBufferMemoryBarrier> buffers;
        };
        std::map<std::pair<VkPipelineStageFlags, VkPipelineStageFlags>, Barriers> stageToBarriers;

        for (const VkImageMemoryBarrier2 &imgBarrier2 : imageBarriers)
        {
            Barriers &barrier
                = stageToBarriers[{ VkPipelineStageFlags(imgBarrier2.srcStageMask), VkPipelineStageFlags(imgBarrier2.dstStageMask) }];

            IMAGE_MEMORY_BARRIER(imgBarrier);
            imgBarrier.image = imgBarrier2.image;
            imgBarrier.subresourceRange = imgBarrier2.subresourceRange;
            imgBarrier.oldLayout = imgBarrier2.oldLayout;
            imgBarrier.newLayout = imgBarrier2.newLayout;
            imgBarrier.srcAccessMask = VkAccessFlags(imgBarrier2.srcAccessMask);
            imgBarrier.dstAccessMask = VkAccessFlags(imgBarrier2.dstAccessMask);
            imgBarrier.srcQueueFamilyIndex = imgBarrier2.srcQueueFamilyIndex;
            imgBarrier.dstQueueFamilyIndex = imgBarrier2.dstQueueFamilyIndex;
            barrier.imgs.emplace_back(imgBarrier);
        }

        for (const VkBufferMemoryBarrier2 &bufBarrier2 : bufferBarriers)
        {
            Barriers &barrier
                = stageToBarriers[{ VkPipelineStageFlags(bufBarrier2.srcStageMask), VkPipelineStageFlags(bufBarrier2.dstStageMask) }];

            BUFFER_MEMORY_BARRIER(bufBarrier);
            bufBarrier.size = bufBarrier2.size;
            bufBarrier.buffer = bufBarrier2.buffer;
            bufBarrier.offset = bufBarrier2.offset;
            bufBarrier.srcAccessMask = VkAccessFlags(bufBarrier2.srcAccessMask);
            bufBarrier.dstAccessMask = VkAccessFlags(bufBarrier2.dstAccessMask);
            bufBarrier.srcQueueFamilyIndex = bufBarrier2.srcQueueFamilyIndex;
            bufBarrier.dstQueueFamilyIndex = bufBarrier2.dstQueueFamilyIndex;
            barrier.buffers.emplace_back(bufBarrier);
        }

        for (const auto &barriers : stageToBarriers)
        {
            vDevice->vkCmdPipelineBarrier(
                cmdBuffer, barriers.first.first, barriers.first.second, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr,
                uint32(barriers.second.buffers.size()), barriers.second.buffers.data(), uint32(barriers.second.imgs.size()),
                barriers.second.imgs.data()
            );
        }
    }
}

VulkanCommandList::VulkanCommandList(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper, VulkanDevice *vulkanDevice)
    : graphicsInstanceCache(graphicsInstance)
    , graphicsHelperCache(graphicsHelper)
    , vDevice(vulkanDevice)
    , cmdBufferManager(vulkanDevice)
{}

void VulkanCommandList::copyBuffer(BufferResourceRef src, BufferResourceRef dst, ArrayView<const CopyBufferInfo> copies)
{
    FenceRef tempFence = IVulkanRHIModule::get()->getGraphicsHelper()->createFence(graphicsInstanceCache, TCHAR("CopyBufferTemp"), false);
    tempFence->init();

    const GraphicsResource *commandBuffer = cmdBufferManager.beginTempCmdBuffer(TCHAR("Copy buffer"), EQueueFunction::Transfer);
    cmdCopyBuffer_Internal(commandBuffer, src, dst, copies);
    cmdBufferManager.endCmdBuffer(commandBuffer);

    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.push_back(commandBuffer);
    cmdBufferManager.submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence);

    tempFence->waitForSignal();

    cmdBufferManager.freeCmdBuffer(commandBuffer);
    tempFence->release();
}

void VulkanCommandList::copyBuffer(ArrayView<const BatchCopyBufferInfo> batchCopies)
{
    FenceRef tempFence = graphicsHelperCache->createFence(graphicsInstanceCache, TCHAR("BatchCopyBufferTemp"), false);
    tempFence->init();

    const GraphicsResource *commandBuffer = cmdBufferManager.beginTempCmdBuffer(TCHAR("Batch Copy buffer"), EQueueFunction::Transfer);

    std::map<std::pair<BufferResourceRef, BufferResourceRef>, std::vector<CopyBufferInfo>> srcDstToCopies;
    for (const BatchCopyBufferInfo &aCopy : batchCopies)
    {
        srcDstToCopies[{ aCopy.src, aCopy.dst }].emplace_back(aCopy.copyInfo);
    }
    for (const auto &copySrcToDst : srcDstToCopies)
    {
        cmdCopyBuffer_Internal(commandBuffer, copySrcToDst.first.first, copySrcToDst.first.second, copySrcToDst.second);
    }

    cmdBufferManager.endCmdBuffer(commandBuffer);

    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.push_back(commandBuffer);
    cmdBufferManager.submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence);

    tempFence->waitForSignal();

    cmdBufferManager.freeCmdBuffer(commandBuffer);
    tempFence->release();
}

void VulkanCommandList::newFrame(float timeDelta)
{
#if DEFER_DELETION
    VulkanGraphicsHelper::getDeferredDeleter(graphicsInstanceCache)->update();
#endif
    resourcesTracker.clearUnwanted();
    VulkanGraphicsHelper::getDescriptorsSetAllocator(graphicsInstanceCache)->tick(timeDelta);
}

void VulkanCommandList::copyToBuffer(BufferResourceRef dst, uint32 dstOffset, const void *dataToCopy, uint32 size)
{
    copyToBuffer_Internal(dst, dstOffset, dataToCopy, size, true);
}

void VulkanCommandList::copyToBuffer(ArrayView<const BatchCopyBufferData> batchCopies)
{
    std::vector<BatchCopyBufferInfo> allCopyInfo;
    BufferResourceRef stagingBuffer = copyToBuffer_GenCopyBufferInfo(allCopyInfo, batchCopies);
    if (stagingBuffer.isValid() && stagingBuffer->isValid())
    {
        stagingBuffer->setDeferredDelete(false);
        copyBuffer(allCopyInfo);
    }

#if 0 // Old impl creates separate staging buffer for each resource

    // For each buffer there will be bunch of copies associated to it
    std::map<BufferResourceRef, std::pair<BufferResourceRef, std::vector<const BatchCopyBufferData *>>> dstToStagingBufferMap;

    // Filling per buffer copy region data and staging data
    for (const BatchCopyBufferData &copyData : batchCopies)
    {
        auto *vulkanDst = static_cast<VulkanBufferResource *>(copyData.dst.reference());
        if (vulkanDst->isStagingResource())
        {
            copyToBuffer_Internal(vulkanDst, copyData.dstOffset, copyData.dataToCopy, copyData.size, false);
            flushBuffers.emplace_back(copyData.dst);
        }
        else
        {
            BufferResourceRef stagingBuffer;
            auto stagingBufferItr = dstToStagingBufferMap.find(vulkanDst);
            if (stagingBufferItr == dstToStagingBufferMap.end())
            {
                if (graphicsHelperCache->isTexelBuffer(vulkanDst))
                {
                    // In case of buffer larger than 4GB using UINT32 will create issue
                    stagingBuffer = graphicsHelperCache->createReadOnlyTexels(
                        graphicsInstanceCache, vulkanDst->texelFormat(),
                        uint32(vulkanDst->getResourceSize() / EPixelDataFormat::getFormatInfo(vulkanDst->texelFormat())->pixelDataSize)
                    );
                }
                else
                {
                    // In case of buffer larger than 4GB using UINT32 will create issue
                    stagingBuffer = graphicsHelperCache->createReadOnlyBuffer(graphicsInstanceCache, uint32(vulkanDst->getResourceSize()), 1);
                }
                dstToStagingBufferMap[vulkanDst] = { stagingBuffer, { &copyData } };
                stagingBuffer->setAsStagingResource(true);
                stagingBuffer->setResourceName(vulkanDst->getResourceName() + TCHAR("_Staging"));
                stagingBuffer->setDeferredDelete(false);
                stagingBuffer->init();

                // We don't want to flush same buffer again
                flushBuffers.emplace_back(stagingBuffer);
            }
            else
            {
                stagingBuffer = stagingBufferItr->second.first;
                stagingBufferItr->second.second.push_back(&copyData);
            }
            copyToBuffer_Internal(stagingBuffer, copyData.dstOffset, copyData.dataToCopy, copyData.size, false);
        }
    }
    graphicsHelperCache->flushMappedPtr(graphicsInstanceCache, flushBuffers);
    for (BufferResourceRef &buffer : flushBuffers)
    {
        graphicsHelperCache->returnMappedPtr(graphicsInstanceCache, buffer);
    }

    // Going to copy from staging to GPU buffers if any such copy exists
    if (dstToStagingBufferMap.empty())
    {
        return;
    }

    // Copying between buffers
    FenceRef tempFence = graphicsHelperCache->createFence(graphicsInstanceCache, TCHAR("BatchCopyBufferTemp"), false);
    tempFence->init();

    const GraphicsResource *commandBuffer = cmdBufferManager.beginTempCmdBuffer(TCHAR("Batch copy buffers"), EQueueFunction::Transfer);
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(commandBuffer);

    for (const auto &dstToStagingPair : dstToStagingBufferMap)
    {
        std::vector<VkBufferCopy> copyRegions;
        for (const BatchCopyBufferData *const &copyData : dstToStagingPair.second.second)
        {
            copyRegions.push_back({ copyData->dstOffset, copyData->dstOffset, copyData->size });
        }
        vDevice->vkCmdCopyBuffer(
            rawCmdBuffer, dstToStagingPair.second.first.reference<VulkanBufferResource>()->buffer,
            dstToStagingPair.first.reference<VulkanBufferResource>()->buffer, uint32(copyRegions.size()), copyRegions.data()
        );
    }

    cmdBufferManager.endCmdBuffer(commandBuffer);
    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.push_back(commandBuffer);
    cmdBufferManager.submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence);
    tempFence->waitForSignal();
    cmdBufferManager.freeCmdBuffer(commandBuffer);
    tempFence->release();

    for (const auto &dstToStagingPair : dstToStagingBufferMap)
    {
        dstToStagingPair.second.first->release();
    }
    dstToStagingBufferMap.clear();

#endif // Old impl creates separate staging buffer for each resource
}

void VulkanCommandList::
    copyToBuffer_Internal(BufferResourceRef dst, uint32 dstOffset, const void *dataToCopy, uint32 size, bool bFlushMemory /*= false*/)
{
    if (dst->getType()->isChildOf(graphicsHelperCache->writeOnlyBufferType())
        || dst->getType()->isChildOf(graphicsHelperCache->writeOnlyTexelsType()))
    {
        LOG_ERROR("VulkanCommandList", "Copy to buffer(%s) that is write only is not allowed", dst->getResourceName().getChar());
        return;
    }
    debugAssert((dst->getResourceSize() - dstOffset) >= size);

    if (dst->isStagingResource())
    {
        auto *vulkanDst = dst.reference<VulkanBufferResource>();
        void *stagingPtr = reinterpret_cast<uint8 *>(graphicsHelperCache->borrowMappedPtr(graphicsInstanceCache, dst)) + dstOffset;
        memcpy(stagingPtr, dataToCopy, size);
        if (bFlushMemory)
        {
            graphicsHelperCache->flushMappedPtr(graphicsInstanceCache, std::vector<BufferResourceRef>{ dst });
            graphicsHelperCache->returnMappedPtr(graphicsInstanceCache, dst);
        }
    }
    else
    {
        uint64 stagingSize = dst->getResourceSize() - dstOffset;
        CopyBufferInfo copyInfo{ 0, dstOffset, size };

        auto copyFromStaging = [&](BufferResourceRef &stagingBuffer)
        {
            stagingBuffer->setAsStagingResource(true);
            stagingBuffer->setDeferredDelete(false);
            stagingBuffer->setResourceName(dst->getResourceName() + TCHAR("_Staging"));
            stagingBuffer->init();

            fatalAssertf(stagingBuffer->isValid(), "Initializing staging buffer failed");
            copyToBuffer_Internal(stagingBuffer, 0, dataToCopy, size, true);
            copyBuffer(stagingBuffer, dst, { &copyInfo, 1 });

            stagingBuffer->release();
        };

        if (graphicsHelperCache->isTexelBuffer(dst))
        {
            // In case of buffer larger than 4GB using UINT32 will create issue
            auto stagingBuffer = graphicsHelperCache->createReadOnlyTexels(
                graphicsInstanceCache, dst->texelFormat(),
                uint32(stagingSize / EPixelDataFormat::getFormatInfo(dst->texelFormat())->pixelDataSize)
            );
            copyFromStaging(stagingBuffer);
        }
        else
        {
            // In case of buffer larger than 4GB using UINT32 will create issue
            auto stagingBuffer = graphicsHelperCache->createReadOnlyBuffer(graphicsInstanceCache, uint32(stagingSize), 1);
            copyFromStaging(stagingBuffer);
        }
    }
}

void VulkanCommandList::cmdCopyBuffer_Internal(
    const GraphicsResource *cmdBuffer, BufferResourceRef src, BufferResourceRef dst, ArrayView<const CopyBufferInfo> copies
)
{
    std::vector<VkBufferCopy2> bufferCopies;
    bufferCopies.reserve(copies.size());
    for (const CopyBufferInfo &copyInfo : copies)
    {
        BUFFER_COPY2(vulkanCopyInfo);
        vulkanCopyInfo.srcOffset = copyInfo.srcOffset;
        vulkanCopyInfo.dstOffset = copyInfo.dstOffset;
        vulkanCopyInfo.size = copyInfo.copySize;
        bufferCopies.emplace_back(std::move(vulkanCopyInfo));
    }

    COPY_BUFFER_INFO2(copyBufferInfo);
    copyBufferInfo.srcBuffer = src.reference<VulkanBufferResource>()->buffer;
    copyBufferInfo.dstBuffer = dst.reference<VulkanBufferResource>()->buffer;
    copyBufferInfo.regionCount = uint32(bufferCopies.size());
    copyBufferInfo.pRegions = bufferCopies.data();

    vDevice->vkCmdCopyBuffer2KHR(cmdBufferManager.getRawBuffer(cmdBuffer), &copyBufferInfo);
}

BufferResourceRef VulkanCommandList::copyToBuffer_GenCopyBufferInfo(
    std::vector<BatchCopyBufferInfo> &outBatchCopies, ArrayView<const BatchCopyBufferData> batchCopies
)
{
    std::vector<const void *> srcDataPtrs;
    outBatchCopies.clear();
    outBatchCopies.reserve(batchCopies.size());
    srcDataPtrs.reserve(batchCopies.size());

    std::vector<BufferResourceRef> flushBuffers;
    uint64 stagingBufferOffset = 0;

    // Filling per buffer copy region data and staging data
    for (const BatchCopyBufferData &copyData : batchCopies)
    {
        BufferResourceRef vulkanDst = copyData.dst;
        if (vulkanDst->isStagingResource())
        {
            copyToBuffer_Internal(vulkanDst, copyData.dstOffset, copyData.dataToCopy, copyData.size, false);
            flushBuffers.emplace_back(copyData.dst);
        }
        else
        {
            BatchCopyBufferInfo copyInfo{
                nullptr, vulkanDst, {stagingBufferOffset, copyData.dstOffset, copyData.size}
            };
            outBatchCopies.emplace_back(copyInfo);
            srcDataPtrs.emplace_back(copyData.dataToCopy);
            stagingBufferOffset += copyData.size;
        }
    }

    if (!flushBuffers.empty())
    {
        graphicsHelperCache->flushMappedPtr(graphicsInstanceCache, flushBuffers);
        for (BufferResourceRef &buffer : flushBuffers)
        {
            graphicsHelperCache->returnMappedPtr(graphicsInstanceCache, buffer);
        }
    }

    // Going to copy from staging to GPU buffers if any such copy exists
    if (outBatchCopies.empty())
    {
        return nullptr;
    }

    debugAssert(stagingBufferOffset > 0 && outBatchCopies.size() == srcDataPtrs.size());
    // In case of buffer larger than 4GB using UINT32 will create issue
    BufferResourceRef stagingBuffer = graphicsHelperCache->createReadOnlyBuffer(graphicsInstanceCache, uint32(stagingBufferOffset), 1);
    stagingBuffer->setAsStagingResource(true);
    stagingBuffer->setResourceName(TCHAR("BatchedCopy_Staging"));
    stagingBuffer->init();

    for (uint32 i = 0; i != outBatchCopies.size(); ++i)
    {
        BatchCopyBufferInfo &copyBufferInfo = outBatchCopies[i];
        const void *srcData = srcDataPtrs[i];

        copyBufferInfo.src = stagingBuffer;
        copyToBuffer_Internal(stagingBuffer, copyBufferInfo.copyInfo.srcOffset, srcData, copyBufferInfo.copyInfo.copySize, false);
    }

    graphicsHelperCache->flushMappedPtr(graphicsInstanceCache, std::vector<BufferResourceRef>{ stagingBuffer });
    graphicsHelperCache->returnMappedPtr(graphicsInstanceCache, stagingBuffer);

    return stagingBuffer;
}

void VulkanCommandList::cmdCopyBuffer_GenBarriers(
    std::vector<VkBufferMemoryBarrier2> &outBarriers, const GraphicsResource *cmdBuffer, BufferResourceRef src, BufferResourceRef dst,
    ArrayView<const CopyBufferInfo> copies
)
{
    VkBufferMemoryBarrier2 bufferBarriers[2];
    bool barrierSet[2] = { false, false };

    const VkPipelineStageFlags2 stagesUsed = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    uint32 qFamilyIdx = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);
    VkPipelineStageFlagBits2 cmdBufferSupportedStages = EngineToVulkanAPI::pipelinesSupportedPerQueue(vDevice->getQueueFlags(qFamilyIdx));
    VkAccessFlags2 cmdBufferSupportedAccess = EngineToVulkanAPI::accessMaskPerQueue(vDevice->getQueueFlags(qFamilyIdx));

    BUFFER_MEMORY_BARRIER2(memBarrier);
    memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = qFamilyIdx;
    memBarrier.dstStageMask = stagesUsed;
    // Src buffer
    {
        bool bIsWriteBuffer = graphicsHelperCache->isRWBuffer(src) || graphicsHelperCache->isWriteOnlyBuffer(src);
        bool bIsTexelBuffer = graphicsHelperCache->isTexelBuffer(src);

        std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo;
        if (bIsTexelBuffer)
        {
            barrierInfo = bIsWriteBuffer ? resourcesTracker.readFromWriteTexels(cmdBuffer, { src, stagesUsed })
                                         : resourcesTracker.readOnlyTexels(cmdBuffer, { src, stagesUsed });
        }
        else
        {
            barrierInfo = bIsWriteBuffer ? resourcesTracker.readFromWriteBuffers(cmdBuffer, { src, stagesUsed })
                                         : resourcesTracker.readOnlyBuffers(cmdBuffer, { src, stagesUsed });
        }

        memBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        if (barrierInfo && barrierInfo->accessors.lastWrite)
        {
            memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
            memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;

            if (cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastWrite)
                || BIT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
            {
                memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            }
            else
            {
                memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
            }
            memBarrier.srcStageMask &= cmdBufferSupportedStages;
            memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;
            // else only read so no issues
            barrierSet[0] = true;
        }
        bufferBarriers[0] = memBarrier;
    }

    // Dst buffer
    {
        bool bIsWriteBuffer = graphicsHelperCache->isRWBuffer(dst) || graphicsHelperCache->isWriteOnlyBuffer(dst);
        bool bIsTexelBuffer = graphicsHelperCache->isTexelBuffer(dst);

        std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo;
        if (bIsTexelBuffer)
        {
            barrierInfo = bIsWriteBuffer ? resourcesTracker.writeTexels(cmdBuffer, { dst, stagesUsed })
                                         : resourcesTracker.writeReadOnlyTexels(cmdBuffer, { dst, stagesUsed });
        }
        else
        {
            barrierInfo = bIsWriteBuffer ? resourcesTracker.writeBuffers(cmdBuffer, { dst, stagesUsed })
                                         : resourcesTracker.writeReadOnlyBuffers(cmdBuffer, { dst, stagesUsed });
        }

        memBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        if (barrierInfo)
        {
            barrierSet[1] = true;
            // If written last, and written in transfer or others
            if (barrierInfo->accessors.lastWrite)
            {
                memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
                memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;

                if (cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastWrite)
                    || BIT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
                {
                    memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                }
                else
                {
                    memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                }
            }
            else if (!barrierInfo->accessors.lastReadsIn.empty())
            {
                // If read in any command buffer
                memBarrier.srcStageMask = barrierInfo->accessors.allReadStages;
                memBarrier.srcAccessMask = 0;
                // If transfer read and shader read in same command
                if (BIT_SET(barrierInfo->accessors.lastReadStages, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
                {
                    memBarrier.srcAccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT;
                }
                else
                {
                    memBarrier.srcAccessMask |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT;
                }
            }
            else
            {
                // No barrier needed for no read/write
                barrierSet[1] = false;
            }
            memBarrier.srcStageMask &= cmdBufferSupportedStages;
            memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;
        }
        bufferBarriers[1] = memBarrier;
    }

    // Always add destination to be released from this queue
    resourcesTracker.addResourceToQTransfer(
        cmdBufferManager.getCmdBufferQueue(cmdBuffer), dst, bufferBarriers[1].dstStageMask, bufferBarriers[1].dstAccessMask, false
    );
    // For read buffer if queue family transfer happened, Then add it to release list of this queue.
    // As next acquire in other queue will need a release
    if (barrierSet[0] && bufferBarriers[0].srcQueueFamilyIndex != bufferBarriers[0].dstQueueFamilyIndex)
    {
        resourcesTracker.addResourceToQTransfer(
            cmdBufferManager.getCmdBufferQueue(cmdBuffer), src, bufferBarriers[0].dstStageMask, bufferBarriers[0].dstAccessMask, false
        );
    }

#if 0 // We are not ready for buffer memory range level barriers
    outBarriers.reserve(outBarriers.size() + (barrierSet[0] ? copies.size() : 0) + (barrierSet[1] ? copies.size() : 0));
    if (barrierSet[0])
    {
        for (const CopyBufferInfo &copyInfo : copies)
        {
            VkBufferMemoryBarrier2 &barrier = outBarriers.emplace_back(bufferBarriers[0]);
            barrier.buffer = src.reference<VulkanBufferResource>()->buffer;
            barrier.offset = copyInfo.srcOffset;
            barrier.size = copyInfo.copySize;
        }
    }
    if (barrierSet[1])
    {
        for (const CopyBufferInfo &copyInfo : copies)
        {
            VkBufferMemoryBarrier2 &barrier = outBarriers.emplace_back(bufferBarriers[1]);
            barrier.buffer = dst.reference<VulkanBufferResource>()->buffer;
            barrier.offset = copyInfo.dstOffset;
            barrier.size = copyInfo.copySize;
        }
    }
#else
    outBarriers.reserve(outBarriers.size() + (barrierSet[0] ? 1 : 0) + (barrierSet[1] ? 1 : 0));
    if (barrierSet[0])
    {
        VkBufferMemoryBarrier2 &barrier = outBarriers.emplace_back(bufferBarriers[0]);
        barrier.buffer = src.reference<VulkanBufferResource>()->buffer;
        barrier.offset = 0;
        barrier.size = src->getResourceSize();
    }
    if (barrierSet[1])
    {
        VkBufferMemoryBarrier2 &barrier = outBarriers.emplace_back(bufferBarriers[1]);
        barrier.buffer = dst.reference<VulkanBufferResource>()->buffer;
        barrier.offset = 0;
        barrier.size = dst->getResourceSize();
    }
#endif
}

const GraphicsResource *VulkanCommandList::startCmd(const String &uniqueName, EQueueFunction queue, bool bIsReusable)
{
    if (bIsReusable)
    {
        return cmdBufferManager.beginReuseCmdBuffer(uniqueName, queue);
    }
    else
    {
        return cmdBufferManager.beginRecordOnceCmdBuffer(uniqueName, queue);
    }
}

void VulkanCommandList::endCmd(const GraphicsResource *cmdBuffer) { cmdBufferManager.endCmdBuffer(cmdBuffer); }

void VulkanCommandList::freeCmd(const GraphicsResource *cmdBuffer) { cmdBufferManager.freeCmdBuffer(cmdBuffer); }

void VulkanCommandList::submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo &submitInfo, FenceRef fence)
{
    cmdBufferManager.submitCmd(priority, submitInfo, fence);
}

void VulkanCommandList::submitWaitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &submitInfo)
{
    cmdBufferManager.submitCmd(priority, submitInfo, &resourcesTracker);
    for (const GraphicsResource *cmdBuffer : submitInfo.cmdBuffers)
    {
        cmdBufferManager.cmdFinished(cmdBuffer, &resourcesTracker);
    }
}

void VulkanCommandList::submitCmds(EQueuePriority::Enum priority, ArrayView<const CommandSubmitInfo2> commands)
{
    cmdBufferManager.submitCmds(priority, commands, &resourcesTracker);
}

void VulkanCommandList::submitCmds(EQueuePriority::Enum priority, ArrayView<const CommandSubmitInfo> submitInfos, FenceRef fence)
{
    cmdBufferManager.submitCmds(priority, submitInfos, fence);
}

void VulkanCommandList::submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2 &command)
{
    cmdBufferManager.submitCmd(priority, command, &resourcesTracker);
}

void VulkanCommandList::finishCmd(const GraphicsResource *cmdBuffer) { cmdBufferManager.cmdFinished(cmdBuffer, &resourcesTracker); }

void VulkanCommandList::finishCmd(const String &uniqueName) { cmdBufferManager.cmdFinished(uniqueName, &resourcesTracker); }

const GraphicsResource *VulkanCommandList::getCmdBuffer(const String &uniqueName) const { return cmdBufferManager.getCmdBuffer(uniqueName); }

TimelineSemaphoreRef VulkanCommandList::getCmdSignalSemaphore(const String &uniqueName) const
{
    return getCmdSignalSemaphore(cmdBufferManager.getCmdBuffer(uniqueName));
}

TimelineSemaphoreRef VulkanCommandList::getCmdSignalSemaphore(const GraphicsResource *cmdBuffer) const
{
    return cmdBufferManager.cmdSignalSemaphore(cmdBuffer);
}

void VulkanCommandList::waitIdle() { vDevice->vkDeviceWaitIdle(VulkanGraphicsHelper::getDevice(vDevice)); }

void VulkanCommandList::waitOnResDepCmds(const MemoryResourceRef &resource)
{
    std::vector<const GraphicsResource *> cmdBuffers = resourcesTracker.getCmdBufferResourceDeps(resource);
    resourcesTracker.clearResource(resource);
    for (const GraphicsResource *cmdBuffer : cmdBuffers)
    {
        finishCmd(cmdBuffer);
        resourcesTracker.clearFinishedCmd(cmdBuffer);
    }
}

void VulkanCommandList::flushAllcommands() { cmdBufferManager.finishAllSubmited(&resourcesTracker); }

bool VulkanCommandList::hasCmdsUsingResource(const MemoryResourceRef &resource)
{
    std::vector<const GraphicsResource *> cmdBuffers = resourcesTracker.getCmdBufferResourceDeps(resource);
    bool bAllCmdBuffersFinished = true;
    for (const GraphicsResource *cmdBuffer : cmdBuffers)
    {
        if (!cmdBufferManager.isCmdFinished(cmdBuffer))
        {
            bAllCmdBuffersFinished = false;
        }
    }
    if (bAllCmdBuffersFinished)
    {
        for (const GraphicsResource *cmdBuffer : cmdBuffers)
        {
            finishCmd(cmdBuffer);
            resourcesTracker.clearFinishedCmd(cmdBuffer);
        }
        resourcesTracker.clearResource(resource);
    }
    return !bAllCmdBuffersFinished;
}

void VulkanCommandList::setupInitialLayout(ImageResourceRef image)
{
    const EPixelDataFormat::PixelFormatInfo *formatInfo = EPixelDataFormat::getFormatInfo(image->imageFormat());

    const GraphicsResource *cmdBuffer
        = cmdBufferManager.beginTempCmdBuffer(TCHAR("LayoutTransition_") + image->getResourceName(), EQueueFunction::Graphics);
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    IMAGE_MEMORY_BARRIER(layoutTransition);
    layoutTransition.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    layoutTransition.newLayout = determineImageLayout(image);
    layoutTransition.srcQueueFamilyIndex = layoutTransition.dstQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);
    layoutTransition.srcAccessMask = layoutTransition.dstAccessMask = determineImageAccessMask(image);
    layoutTransition.image = image.reference<VulkanImageResource>()->image;
    layoutTransition.subresourceRange = { determineImageAspect(image), 0, image->getNumOfMips(), 0, image->getLayerCount() };

    vDevice->vkCmdPipelineBarrier(
        rawCmdBuffer, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
        VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &layoutTransition
    );

    cmdBufferManager.endCmdBuffer(cmdBuffer);

    FenceRef tempFence = graphicsHelperCache->createFence(graphicsInstanceCache, TCHAR("TempLayoutTransitionFence"));
    tempFence->init();

    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdBufferManager.submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence);
    tempFence->waitForSignal();

    cmdBufferManager.freeCmdBuffer(cmdBuffer);
    tempFence->release();
}

void VulkanCommandList::presentImage(
    ArrayView<const WindowCanvasRef> canvases, ArrayView<const uint32> imageIndices, ArrayView<const SemaphoreRef> waitOnSemaphores
)
{
    // TODO(Jeslas) : Right now vkQueuePresentKHR does not support timeline semaphore, Include below once that is supported
    // std::vector<SemaphoreRef> waitSemaphores{ waitOnSemaphores.begin(), waitOnSemaphores.end() };
    // for (const GraphicsResource *cmdBuffer : swapchainFrameWrites)
    //{
    //    SemaphoreRef cmdSignal = cmdBufferManager.cmdSignalSemaphore(cmdBuffer);
    //    fatalAssertf(cmdSignal.isValid(), "Invalid signalling semaphore for cmd buffer %s!", cmdBuffer->getResourceName());
    //    waitSemaphores.emplace_back(cmdSignal);
    //}

    VulkanGraphicsHelper::presentImage(graphicsInstanceCache, canvases, imageIndices, waitOnSemaphores);
    swapchainFrameWrites.clear();
}

void VulkanCommandList::cmdCopyBuffer(
    const GraphicsResource *cmdBuffer, BufferResourceRef src, BufferResourceRef dst, ArrayView<const CopyBufferInfo> copies
)
{
    debugAssert(src.isValid() && src->isValid() && dst.isValid() && dst->isValid());

    std::vector<VkBufferMemoryBarrier2> allBarriers;
    cmdCopyBuffer_GenBarriers(allBarriers, cmdBuffer, src, dst, copies);

    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    if (!allBarriers.empty())
    {
        cmdPipelineBarrier(vDevice, rawCmdBuffer, {}, allBarriers);
    }

    cmdCopyBuffer_Internal(cmdBuffer, src, dst, copies);
}

void VulkanCommandList::cmdCopyBuffer(const GraphicsResource *cmdBuffer, ArrayView<const BatchCopyBufferInfo> copies)
{
    std::map<std::pair<BufferResourceRef, BufferResourceRef>, std::vector<CopyBufferInfo>> srcDstToCopies;
    for (const BatchCopyBufferInfo &aCopy : copies)
    {
        srcDstToCopies[{ aCopy.src, aCopy.dst }].emplace_back(aCopy.copyInfo);
    }

    // Barrier each copied resources
    std::vector<VkBufferMemoryBarrier2> allBarriers;
    for (const auto &copySrcToDst : srcDstToCopies)
    {
        cmdCopyBuffer_GenBarriers(allBarriers, cmdBuffer, copySrcToDst.first.first, copySrcToDst.first.second, copySrcToDst.second);
    }
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    if (!allBarriers.empty())
    {
        cmdPipelineBarrier(vDevice, rawCmdBuffer, {}, allBarriers);
    }

    // Finally copy all src-dst combination
    for (const auto &copySrcToDst : srcDstToCopies)
    {
        cmdCopyBuffer_Internal(cmdBuffer, copySrcToDst.first.first, copySrcToDst.first.second, copySrcToDst.second);
    }
}

void VulkanCommandList::cmdCopyToBuffer(const GraphicsResource *cmdBuffer, ArrayView<const BatchCopyBufferData> batchCopies)
{
    std::vector<BatchCopyBufferInfo> allCopyInfo;
    BufferResourceRef stagingBuffer = copyToBuffer_GenCopyBufferInfo(allCopyInfo, batchCopies);
    if (stagingBuffer.isValid() && stagingBuffer->isValid())
    {
        std::map<BufferResourceRef, std::vector<CopyBufferInfo>> dstToCopies;
        for (const BatchCopyBufferInfo &bufferCopyInfo : allCopyInfo)
        {
            debugAssert(stagingBuffer == bufferCopyInfo.src);
            dstToCopies[bufferCopyInfo.dst].emplace_back(bufferCopyInfo.copyInfo);
        }

        // Barrier each copied resources
        std::vector<VkBufferMemoryBarrier2> allBarriers;
        for (const std::pair<const BufferResourceRef, std::vector<CopyBufferInfo>> &copyToDst : dstToCopies)
        {
            cmdCopyBuffer_GenBarriers(allBarriers, cmdBuffer, stagingBuffer, copyToDst.first, copyToDst.second);
        }
        VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
        if (!allBarriers.empty())
        {
            cmdPipelineBarrier(vDevice, rawCmdBuffer, {}, allBarriers);
        }

        // Finally copy all src-dst combination
        for (const std::pair<const BufferResourceRef, std::vector<CopyBufferInfo>> &copyToDst : dstToCopies)
        {
            cmdCopyBuffer_Internal(cmdBuffer, stagingBuffer, copyToDst.first, copyToDst.second);
        }
    }
}

void VulkanCommandList::cmdCopyOrResolveImage(
    const GraphicsResource *cmdBuffer, ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo, const CopyImageInfo &dstInfo
)
{
    CopyImageInfo srcInfoCpy = srcInfo;
    CopyImageInfo dstInfoCpy = dstInfo;
    // Make sure mips and layers never exceeds above max
    srcInfoCpy.subres.mipCount = Math::min(srcInfoCpy.subres.mipCount, src->getNumOfMips());
    srcInfoCpy.subres.layersCount = Math::min(srcInfoCpy.subres.layersCount, src->getLayerCount());
    dstInfoCpy.subres.mipCount = Math::min(dstInfoCpy.subres.mipCount, dst->getNumOfMips());
    dstInfoCpy.subres.layersCount = Math::min(dstInfoCpy.subres.layersCount, dst->getLayerCount());

    bool bCanSimpleCopy
        = src->getImageSize() == dst->getImageSize() && src->imageFormat() == dst->imageFormat() && srcInfoCpy.isCopyCompatible(dstInfoCpy);
    if (srcInfoCpy.subres.mipCount != dstInfoCpy.subres.mipCount || srcInfoCpy.extent != dstInfoCpy.extent)
    {
        LOG_ERROR("VulkanCommandList", "MIP counts && extent must be same between source and destination regions");
        return;
    }
    {
        SizeBox3D srcBound(srcInfoCpy.offset, Size3D(srcInfoCpy.offset + srcInfoCpy.extent));
        SizeBox3D dstBound(dstInfoCpy.offset, Size3D(dstInfoCpy.offset + dstInfoCpy.extent));
        if (src == dst && srcBound.intersect(dstBound))
        {
            LOG_ERROR("VulkanCommandList", "Cannot copy to same image with intersecting region");
            return;
        }
    }
    if (cmdBufferManager.isTransferCmdBuffer(cmdBuffer)
        && (EPixelDataFormat::isDepthFormat(src->imageFormat()) || EPixelDataFormat::isDepthFormat(dst->imageFormat())))
    {
        LOG_ERROR("VulkanCommandList", "Cannot copy of resolve depth/stensil textures in transfer queue allocated command buffers!");
        return;
    }

    std::vector<VkImageMemoryBarrier2> imageBarriers;
    imageBarriers.reserve(2);
    // TODO(Jeslas) : Is right?
    const VkPipelineStageFlags2 stagesUsed = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    uint32 qFamilyIdx = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);
    VkPipelineStageFlagBits2 cmdBufferSupportedStages = EngineToVulkanAPI::pipelinesSupportedPerQueue(vDevice->getQueueFlags(qFamilyIdx));
    VkAccessFlags2 cmdBufferSupportedAccess = EngineToVulkanAPI::accessMaskPerQueue(vDevice->getQueueFlags(qFamilyIdx));

    VkImageAspectFlags srcImageAspect = determineImageAspect(src);
    VkImageAspectFlags dstImageAspect = determineImageAspect(dst);

    const VkAccessFlags2 srcAccessFlags = VK_ACCESS_2_TRANSFER_READ_BIT;
    const VkAccessFlags2 dstAccessFlags = VK_ACCESS_2_TRANSFER_WRITE_BIT;

    VkImageLayout srcOriginalLayout = getImageLayout(src);
    VkImageLayout dstOriginalLayout = getImageLayout(dst);

    const VkImageLayout srcNewLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    const VkImageLayout dstNewLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    IMAGE_MEMORY_BARRIER2(memBarrier);
    // Source barrier
    memBarrier.image = src.reference<VulkanImageResource>()->image;
    memBarrier.subresourceRange = { srcImageAspect, 0, src->getNumOfMips(), 0, src->getLayerCount() };

    memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = qFamilyIdx;
    memBarrier.dstStageMask = stagesUsed;

    memBarrier.newLayout = memBarrier.oldLayout = srcNewLayout;
    memBarrier.dstAccessMask = srcAccessFlags;
    memBarrier.srcAccessMask = determineImageAccessMask(src);
    // Source barriers
    {
        bool bIsRtSrc = src->getType()->isChildOf(graphicsHelperCache->rtImageType());
        std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo
            = (src->isShaderWrite() || bIsRtSrc) ? resourcesTracker.readFromWriteImages(cmdBuffer, { src, stagesUsed })
                                                 : resourcesTracker.readOnlyImages(cmdBuffer, { src, stagesUsed });

        // If write texture
        // If written last, and written in transfer or others
        // If read only
        // There is no write in graphics
        if (barrierInfo && barrierInfo->accessors.lastWrite)
        {
            memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
            memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;

            if (cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastWrite)
                || BIT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
            {
                memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                memBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            }
            else
            {
                memBarrier.srcAccessMask = bIsRtSrc ? VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT : VK_ACCESS_2_SHADER_WRITE_BIT;
                memBarrier.oldLayout = srcOriginalLayout;
            }
            memBarrier.srcStageMask &= cmdBufferSupportedStages;
            memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;
            imageBarriers.emplace_back(memBarrier);
            // else only read so no issues
        }
    }
    memBarrier.image = dst.reference<VulkanImageResource>()->image;
    memBarrier.subresourceRange = { dstImageAspect, 0, dst->getNumOfMips(), 0, dst->getLayerCount() };

    memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = qFamilyIdx;
    memBarrier.dstStageMask = stagesUsed;

    memBarrier.newLayout = memBarrier.oldLayout = dstNewLayout;
    memBarrier.dstAccessMask = dstAccessFlags;
    memBarrier.srcAccessMask = determineImageAccessMask(dst);
    // Dst barriers
    {
        if (dst->getType()->isChildOf(graphicsHelperCache->rtImageType()))
        {
            // TODO(Jeslas) : Not handled
            debugAssertf(false, "Why resolve/copy to render target?");
        }
        else
        {
            std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo
                = dst->isShaderWrite() ? resourcesTracker.writeImages(cmdBuffer, { dst, stagesUsed })
                                       : resourcesTracker.writeReadOnlyImages(cmdBuffer, { dst, stagesUsed });

            if (barrierInfo)
            {
                // If written last, and written in transfer or others
                if (barrierInfo->accessors.lastWrite)
                {
                    memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
                    memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;

                    if (cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastWrite)
                        || BIT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
                    {
                        memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        memBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    }
                    else
                    {
                        memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                        memBarrier.oldLayout = dstOriginalLayout;
                    }
                }
                else if (barrierInfo->accessors.lastReadsIn.empty())
                {
                    // No read write happened so far
                    memBarrier.srcStageMask = resourceShaderStageFlags();
                    memBarrier.oldLayout = dstOriginalLayout;
                }
                // only reads happened
                else
                {
                    // If read in any command buffer
                    memBarrier.srcStageMask = barrierInfo->accessors.allReadStages;
                    memBarrier.srcAccessMask = 0;
                    // If transfer read and shader read in same command
                    if (BIT_SET(barrierInfo->accessors.lastReadStages, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
                    {
                        memBarrier.srcAccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT;
                        memBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    }
                    else
                    {
                        memBarrier.srcAccessMask |= VK_ACCESS_2_SHADER_READ_BIT;
                        memBarrier.oldLayout = dstOriginalLayout;
                    }
                }
            }
            memBarrier.srcStageMask &= cmdBufferSupportedStages;
            memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;
            imageBarriers.emplace_back(memBarrier);
        }
    }

    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    cmdPipelineBarrier(vDevice, rawCmdBuffer, imageBarriers, {});
    // For images anyway we have to do layout transfer anyways so add both src and dst for queue transfer
    resourcesTracker.addResourceToQTransfer(cmdBufferManager.getCmdBufferQueue(cmdBuffer), src, stagesUsed, srcAccessFlags, srcNewLayout, true);
    resourcesTracker.addResourceToQTransfer(cmdBufferManager.getCmdBufferQueue(cmdBuffer), dst, stagesUsed, dstAccessFlags, dstNewLayout, true);

    if (bCanSimpleCopy)
    {
        std::vector<VkImageCopy> imageCopyRegions(srcInfoCpy.subres.mipCount);

        Size3D mipSize = srcInfoCpy.extent;
        Size3D srcMipSizeOffset = srcInfoCpy.offset;
        Size3D dstMipSizeOffset = dstInfoCpy.offset;
        for (uint32 mipLevel = 0; mipLevel < srcInfoCpy.subres.mipCount; ++mipLevel)
        {
            imageCopyRegions[mipLevel].srcOffset = { int32(srcMipSizeOffset.x), int32(srcMipSizeOffset.y), int32(srcMipSizeOffset.z) };
            imageCopyRegions[mipLevel].srcSubresource
                = { srcImageAspect, srcInfoCpy.subres.baseMip + mipLevel, srcInfoCpy.subres.baseLayer, srcInfoCpy.subres.layersCount };
            imageCopyRegions[mipLevel].dstOffset = { int32(dstMipSizeOffset.x), int32(dstMipSizeOffset.y), int32(dstMipSizeOffset.z) };
            imageCopyRegions[mipLevel].dstSubresource
                = { dstImageAspect, dstInfoCpy.subres.baseMip + mipLevel, dstInfoCpy.subres.baseLayer, dstInfoCpy.subres.layersCount };
            imageCopyRegions[mipLevel].extent = { mipSize.x, mipSize.y, mipSize.z };

            srcMipSizeOffset /= 2u;
            dstMipSizeOffset /= 2u;
            mipSize = Math::max(mipSize / 2u, Size3D{ 1, 1, 1 });
        }

        vDevice->vkCmdCopyImage(
            rawCmdBuffer, src.reference<VulkanImageResource>()->image, srcNewLayout, dst.reference<VulkanImageResource>()->image, dstNewLayout,
            uint32(imageCopyRegions.size()), imageCopyRegions.data()
        );
    }
    else
    {
        std::vector<VkImageResolve> imageResolveRegions;
        imageResolveRegions.reserve(srcInfoCpy.subres.mipCount);

        Size3D mipSize = srcInfoCpy.extent;
        Size3D srcMipSizeOffset = srcInfoCpy.offset;
        Size3D dstMipSizeOffset = dstInfoCpy.offset;
        for (uint32 mipLevel = 0; mipLevel < srcInfoCpy.subres.mipCount; ++mipLevel)
        {
            imageResolveRegions[mipLevel].srcOffset = { int32(srcMipSizeOffset.x), int32(srcMipSizeOffset.y), int32(srcMipSizeOffset.z) };
            imageResolveRegions[mipLevel].srcSubresource
                = { srcImageAspect, srcInfoCpy.subres.baseMip + mipLevel, srcInfoCpy.subres.baseLayer, srcInfoCpy.subres.layersCount };
            imageResolveRegions[mipLevel].dstOffset = { int32(dstMipSizeOffset.x), int32(dstMipSizeOffset.y), int32(dstMipSizeOffset.z) };
            imageResolveRegions[mipLevel].dstSubresource
                = { dstImageAspect, dstInfoCpy.subres.baseMip + mipLevel, dstInfoCpy.subres.baseLayer, dstInfoCpy.subres.layersCount };
            imageResolveRegions[mipLevel].extent = { mipSize.x, mipSize.y, mipSize.z };

            srcMipSizeOffset /= 2u;
            dstMipSizeOffset /= 2u;
            mipSize = Math::max(mipSize / 2u, Size3D{ 1, 1, 1 });
        }

        vDevice->vkCmdResolveImage(
            rawCmdBuffer, src.reference<VulkanImageResource>()->image, srcNewLayout, dst.reference<VulkanImageResource>()->image, dstNewLayout,
            uint32(imageResolveRegions.size()), imageResolveRegions.data()
        );
    }
}

void VulkanCommandList::cmdTransitionLayouts(const GraphicsResource *cmdBuffer, ArrayView<const ImageResourceRef> images)
{
    std::vector<VkImageMemoryBarrier2> imageBarriers;
    imageBarriers.reserve(images.size());

    uint32 qFamilyIdx = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);
    VkPipelineStageFlagBits2 cmdBufferSupportedStages = EngineToVulkanAPI::pipelinesSupportedPerQueue(vDevice->getQueueFlags(qFamilyIdx));
    VkAccessFlags2 cmdBufferSupportedAccess = EngineToVulkanAPI::accessMaskPerQueue(vDevice->getQueueFlags(qFamilyIdx));

    for (ImageResourceRef image : images)
    {
        IMAGE_MEMORY_BARRIER2(memBarrier);
        memBarrier.srcStageMask = memBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        memBarrier.srcAccessMask = memBarrier.dstAccessMask = determineImageAccessMask(image);
        memBarrier.oldLayout = memBarrier.newLayout = determineImageLayout(image);
        memBarrier.srcQueueFamilyIndex = memBarrier.dstQueueFamilyIndex = qFamilyIdx;
        memBarrier.subresourceRange = { determineImageAspect(image), 0, image->getNumOfMips(), 0, image->getLayerCount() };
        memBarrier.image = image.reference<VulkanImageResource>()->image;

        if (cmdBufferManager.isTransferCmdBuffer(cmdBuffer))
        {
            memBarrier.srcStageMask = memBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            memBarrier.srcAccessMask = memBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
        }

        if (image->getType()->isChildOf(graphicsHelperCache->rtImageType()))
        {
            // No need to transition to attachment optimal layout as they are handled in render
            // pass, So just transition to shader read if used in transfer
            memBarrier.oldLayout = memBarrier.newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            memBarrier.srcAccessMask = memBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
        }

        std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo = resourcesTracker.imageToGeneralLayout(cmdBuffer, image);
        if (!barrierInfo)
        {
            continue;
        }

        if (barrierInfo->accessors.lastWrite && barrierInfo->accessors.lastReadsIn.empty())
        {
            memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
            memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;

            // If shader read only then it can be written only in transfer
            if (!image->isShaderWrite() || BIT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
            {
                memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                memBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            }
            else
            {
                memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                // imageBarrier.oldLayout = determineImageLayout(image);
            }
        }
        // Read in is not empty as if both last write and reads are empty optional barrier info will
        // be empty as well
        else
        {
            memBarrier.srcStageMask = barrierInfo->accessors.allReadStages;

            if (BIT_SET(barrierInfo->accessors.lastReadStages, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
            {
                memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastReadsIn.back());
                memBarrier.srcAccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT;
                memBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            }
            else
            {
                LOG_ERROR("VulkanCommandList", "Barrier is applied on image(%s) that is only read so far", image->getResourceName().getChar());
            }
        }

        memBarrier.srcStageMask &= cmdBufferSupportedStages;
        memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;
        imageBarriers.emplace_back(memBarrier);
        // Need to add to transfer only if we are actually changing queue or changing layout
        if (memBarrier.srcQueueFamilyIndex != memBarrier.dstQueueFamilyIndex || memBarrier.oldLayout != memBarrier.newLayout)
        {
            resourcesTracker.addResourceToQTransfer(
                cmdBufferManager.getCmdBufferQueue(cmdBuffer), image, memBarrier.dstStageMask, memBarrier.dstAccessMask, memBarrier.newLayout,
                true
            );
        }
    }

    if (!imageBarriers.empty())
    {
        cmdPipelineBarrier(vDevice, cmdBufferManager.getRawBuffer(cmdBuffer), imageBarriers, {});
    }
}

void VulkanCommandList::cmdClearImage(
    const GraphicsResource *cmdBuffer, ImageResourceRef image, const LinearColor &clearColor, ArrayView<const ImageSubresource> subresources
)
{
    if (EPixelDataFormat::isDepthFormat(image->imageFormat()))
    {
        LOG_ERROR("VulkanCommandList", "Depth image clear cannot be done in color clear");
        return;
    }

    LOG_WARN("VulkanCommandList", "Synchronization not handled");

    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    std::vector<VkImageSubresourceRange> ranges;
    for (const ImageSubresource &subres : subresources)
    {
        VkImageSubresourceRange range;
        range.aspectMask = determineImageAspect(image);
        range.baseMipLevel = subres.baseMip;
        range.levelCount = subres.mipCount;
        range.baseArrayLayer = subres.baseLayer;
        range.layerCount = subres.layersCount;

        ranges.emplace_back(range);
    }

    VkClearColorValue clearVals;
    fillClearValue(image->imageFormat(), clearVals, clearColor);
    vDevice->vkCmdClearColorImage(
        rawCmdBuffer, image.reference<VulkanImageResource>()->image, determineImageLayout(image), &clearVals, uint32(ranges.size()),
        ranges.data()
    );
}

void VulkanCommandList::cmdClearDepth(
    const GraphicsResource *cmdBuffer, ImageResourceRef image, float depth, uint32 stencil, ArrayView<const ImageSubresource> subresources
)
{
    if (!EPixelDataFormat::isDepthFormat(image->imageFormat()))
    {
        LOG_ERROR("VulkanCommandList", "Color image clear cannot be done in depth clear");
        return;
    }

    LOG_WARN("VulkanCommandList", "Synchronization not handled");

    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    std::vector<VkImageSubresourceRange> ranges;
    for (const ImageSubresource &subres : subresources)
    {
        VkImageSubresourceRange range;
        range.aspectMask = determineImageAspect(image);
        range.baseMipLevel = subres.baseMip;
        range.levelCount = subres.mipCount;
        range.baseArrayLayer = subres.baseLayer;
        range.layerCount = subres.layersCount;

        ranges.emplace_back(range);
    }

    VkClearDepthStencilValue clearVals{ depth, stencil };
    vDevice->vkCmdClearDepthStencilImage(
        rawCmdBuffer, image.reference<VulkanImageResource>()->image, determineImageLayout(image), &clearVals, uint32(ranges.size()),
        ranges.data()
    );
}

void VulkanCommandList::cmdBarrierResources(const GraphicsResource *cmdBuffer, ArrayView<const ShaderParametersRef> descriptorsSets)
{
    fatalAssertf(
        !cmdBufferManager.isInRenderPass(cmdBuffer), "%s cmd buffer is inside render pass, it is not supported",
        cmdBuffer->getResourceName().getChar()
    );

    EQueueFunction cmdBufferQ = cmdBufferManager.getCmdBufferQueue(cmdBuffer);
    uint32 qFamilyIdx = cmdBufferManager.getQueueFamilyIdx(cmdBufferQ);
    VkPipelineStageFlagBits2 cmdBufferSupportedStages = EngineToVulkanAPI::pipelinesSupportedPerQueue(vDevice->getQueueFlags(qFamilyIdx));
    VkAccessFlags2 cmdBufferSupportedAccess = EngineToVulkanAPI::accessMaskPerQueue(vDevice->getQueueFlags(qFamilyIdx));

    std::vector<VkImageMemoryBarrier2> imageBarriers;
    std::vector<VkBufferMemoryBarrier2> bufferBarriers;

    for (const ShaderParametersRef descriptorsSet : descriptorsSets)
    {
        // READ only buffers and texels ( might be copied to in transfer queue )
        {
            std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> resources = descriptorsSet->getAllReadOnlyBuffers();
            {
                std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> tempTexels
                    = descriptorsSet->getAllReadOnlyTexels();
                resources.insert(resources.end(), tempTexels.cbegin(), tempTexels.cend());
            }
            for (const auto &resource : resources)
            {
                VkPipelineStageFlags2 stagesUsed
                    = VulkanGraphicsHelper::shaderToPipelineStageFlags(resource.second->bufferEntryPtr->data.stagesUsed);
                std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo
                    = resourcesTracker.readOnlyBuffers(cmdBuffer, { resource.first, stagesUsed });
                if (barrierInfo)
                {
                    BUFFER_MEMORY_BARRIER2(memBarrier);
                    memBarrier.buffer = resource.first.reference<VulkanBufferResource>()->buffer;
                    memBarrier.offset = 0;
                    memBarrier.size = resource.first->getResourceSize();

                    memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = qFamilyIdx;
                    memBarrier.dstStageMask = memBarrier.srcStageMask = stagesUsed;
                    // Since shader binding and read only
                    memBarrier.dstAccessMask = memBarrier.srcAccessMask = VK_ACCESS_2_UNIFORM_READ_BIT;

                    if (barrierInfo->accessors.lastWrite)
                    {
                        memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
                        memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;

                        // If Resource is write usable but read in transfer(Resource is
                        // read only usable then only option is transfer write) Or
                        // Resource is written in transfer last then transition from
                        // transfer
                        if (!(resource.second->bIsStorage || ANY_BIT_SET(barrierInfo->accessors.lastWriteStage, resourceShaderStageFlags()))
                            || BIT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_TRANSFER_BIT)
                            || cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastWrite))
                        {
                            memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        }
                        else
                        {
                            memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                        }
                        memBarrier.srcStageMask &= cmdBufferSupportedStages;
                        memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;
                        bufferBarriers.emplace_back(memBarrier);
                        // If changing queue on read buffer, We need to release the queue
                        if (memBarrier.srcQueueFamilyIndex != memBarrier.dstQueueFamilyIndex)
                        {
                            resourcesTracker.addResourceToQTransfer(cmdBufferQ, resource.first, stagesUsed, memBarrier.dstAccessMask, false);
                        }
                    }
                }
            }
        }
        // READ only textures ( might be copied to in transfer queue )
        {
            // TODO(Jeslas) : Handle attachment images
            std::vector<std::pair<ImageResourceRef, const ShaderTextureDescriptorType *>> resources = descriptorsSet->getAllReadOnlyTextures();
            for (const auto &resource : resources)
            {
                VkPipelineStageFlags2 stagesUsed
                    = VulkanGraphicsHelper::shaderToPipelineStageFlags(resource.second->textureEntryPtr->data.stagesUsed);
                std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo
                    = resourcesTracker.readOnlyImages(cmdBuffer, { resource.first, stagesUsed });
                if (barrierInfo)
                {
                    IMAGE_MEMORY_BARRIER2(memBarrier);
                    memBarrier.image = resource.first.reference<VulkanImageResource>()->image;
                    memBarrier.subresourceRange
                        = { determineImageAspect(resource.first), 0, resource.first->getNumOfMips(), 0, resource.first->getLayerCount() };

                    memBarrier.newLayout = memBarrier.oldLayout = determineImageLayout(resource.first);
                    memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = qFamilyIdx;
                    memBarrier.dstStageMask = memBarrier.srcStageMask = stagesUsed;
                    // Since shader binding and read only
                    memBarrier.dstAccessMask = memBarrier.srcAccessMask = determineImageAccessMask(resource.first);

                    // If last write is a color attachment then we have nothing to barrier as render pass takes care of it
                    if (barrierInfo->accessors.lastWrite
                        && BIT_NOT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT))
                    {
                        memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
                        memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;

                        // If Resource is write usable but read in transfer(Resource is
                        // read only usable then only option is transfer write) Or
                        // Resource is written in transfer last then transition from
                        // transfer
                        if (!(resource.second->imageUsageFlags == EImageShaderUsage::Writing
                              || ANY_BIT_SET(barrierInfo->accessors.lastWriteStage, resourceShaderStageFlags()))
                            || BIT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_TRANSFER_BIT)
                            || cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastWrite))
                        {
                            memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                            memBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                        }
                        else
                        {
                            memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                            memBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
                        }
                        memBarrier.srcStageMask &= cmdBufferSupportedStages;
                        memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;
                        imageBarriers.emplace_back(memBarrier);
                        resourcesTracker.addResourceToQTransfer(
                            cmdBufferQ, resource.first, stagesUsed, memBarrier.dstAccessMask, memBarrier.newLayout, false
                        );
                    }
                    // We do not handle transfer read here as it is unlikely that a read only
                    // texture needs to be copied without finished
                }
            }
        }
        // Write able buffers and texels
        {
            std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> resources = descriptorsSet->getAllWriteBuffers();
            {
                std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> tempTexels = descriptorsSet->getAllWriteTexels();
                resources.insert(resources.end(), tempTexels.cbegin(), tempTexels.cend());
            }
            for (const auto &resource : resources)
            {
                VkPipelineStageFlags2 stagesUsed
                    = VulkanGraphicsHelper::shaderToPipelineStageFlags(resource.second->bufferEntryPtr->data.stagesUsed);
                VkAccessFlags2 accessMask;
                std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo;
                if (resource.second->bIsStorage)
                {
                    barrierInfo = resourcesTracker.writeBuffers(cmdBuffer, { resource.first, stagesUsed });
                    accessMask = VK_ACCESS_2_SHADER_WRITE_BIT;

                    // If storing then always we need Q transfers
                    resourcesTracker.addResourceToQTransfer(cmdBufferQ, resource.first, stagesUsed, accessMask, false);
                }
                else
                {
                    barrierInfo = resourcesTracker.readFromWriteBuffers(cmdBuffer, { resource.first, stagesUsed });
                    accessMask = VK_ACCESS_2_UNIFORM_READ_BIT;
                }

                if (barrierInfo)
                {
                    BUFFER_MEMORY_BARRIER2(memBarrier);
                    memBarrier.buffer = resource.first.reference<VulkanBufferResource>()->buffer;
                    memBarrier.offset = 0;
                    memBarrier.size = resource.first->getResourceSize();

                    memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = qFamilyIdx;
                    memBarrier.dstStageMask = memBarrier.srcStageMask = stagesUsed;
                    // Since shader binding and read only
                    memBarrier.dstAccessMask = accessMask;

                    // If there is last write but no read so far then wait for write
                    if (barrierInfo->accessors.lastWrite)
                    {
                        if (cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastWrite)
                            || BIT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
                        {
                            // If last write, wait for transfer write as read only
                            memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                            memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
                            memBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                        }
                        // Written in Shader
                        else
                        {
                            memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                            memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
                            memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;
                        }
                        memBarrier.srcStageMask &= cmdBufferSupportedStages;
                        memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;
                        bufferBarriers.emplace_back(memBarrier);
                        // If changing queue on read buffer, We need to release the queue
                        if (memBarrier.srcQueueFamilyIndex != memBarrier.dstQueueFamilyIndex)
                        {
                            resourcesTracker.addResourceToQTransfer(cmdBufferQ, resource.first, stagesUsed, accessMask, false);
                        }
                    }
                    // If not written but read last in same command buffer then wait, This
                    // will not be empty if writing/storage buffer
                    // Queue change can also trigger this, but in that case lastReadsIn will not be same
                    else if (barrierInfo->accessors.lastReadsIn.size() == 1)
                    {
                        memBarrier.srcStageMask = barrierInfo->accessors.allReadStages;
                        memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastReadsIn.front());
                        if (cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastReadsIn.front())
                            || BIT_SET(barrierInfo->accessors.allReadStages, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
                        {
                            memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                        }
                        else
                        {
                            memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT;
                        }
                        memBarrier.srcStageMask &= cmdBufferSupportedStages;
                        memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;
                        bufferBarriers.emplace_back(memBarrier);

                        if (barrierInfo->accessors.lastReadsIn.front() != cmdBuffer)
                        {
                            resourcesTracker.addResourceToQTransfer(cmdBufferQ, resource.first, stagesUsed, accessMask, false);
                        }
                    }
                }
            }
        }
        // WRITE textures
        {
            std::vector<std::pair<ImageResourceRef, const ShaderTextureDescriptorType *>> resources = descriptorsSet->getAllWriteTextures();
            for (const auto &resource : resources)
            {
                // TODO(Jeslas) : Handle attachment images
                VkPipelineStageFlags2 stagesUsed
                    = VulkanGraphicsHelper::shaderToPipelineStageFlags(resource.second->textureEntryPtr->data.stagesUsed);
                VkAccessFlags2 accessMask;
                VkImageLayout imgLayout = determineImageLayout(resource.first);
                std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo;
                if (resource.second->imageUsageFlags == EImageShaderUsage::Writing)
                {
                    barrierInfo = resourcesTracker.writeImages(cmdBuffer, { resource.first, stagesUsed });
                    accessMask = VK_ACCESS_2_SHADER_WRITE_BIT;

                    // If storing then always we need Q transfers
                    resourcesTracker.addResourceToQTransfer(cmdBufferQ, resource.first, stagesUsed, accessMask, imgLayout, false);
                }
                else
                {
                    barrierInfo = resourcesTracker.readFromWriteImages(cmdBuffer, { resource.first, stagesUsed });
                    accessMask = VK_ACCESS_2_UNIFORM_READ_BIT;
                }

                if (barrierInfo)
                {
                    IMAGE_MEMORY_BARRIER2(memBarrier);
                    memBarrier.image = resource.first.reference<VulkanImageResource>()->image;
                    memBarrier.subresourceRange
                        = { determineImageAspect(resource.first), 0, resource.first->getNumOfMips(), 0, resource.first->getLayerCount() };

                    memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = qFamilyIdx;
                    memBarrier.dstStageMask = memBarrier.srcStageMask = stagesUsed;

                    memBarrier.newLayout = memBarrier.oldLayout = imgLayout;
                    memBarrier.dstAccessMask = memBarrier.srcAccessMask = resource.second->imageUsageFlags == EImageShaderUsage::Writing
                                                                              ? VK_ACCESS_2_SHADER_WRITE_BIT
                                                                              : VK_ACCESS_2_SHADER_READ_BIT;

                    // If there is last write but no read so far then wait for write within
                    // same cmd buffer then just barrier no layout switch
                    if (barrierInfo->accessors.lastWrite)
                    {
                        // if written in render pass then we get implicit barrier
                        if (BIT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT))
                        {
                            continue;
                        }

                        memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
                        // If written in transfer before
                        if (cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastWrite)
                            || BIT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
                        {
                            memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                            memBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                            memBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                        }
                        else if (resource.second->imageUsageFlags != EImageShaderUsage::Writing) // We are not writing
                        {
                            memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;
                            memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                        }

                        // If access is across queue family or if layout changes
                        if (memBarrier.srcQueueFamilyIndex != memBarrier.dstQueueFamilyIndex || memBarrier.oldLayout != memBarrier.newLayout)
                        {
                            resourcesTracker.addResourceToQTransfer(cmdBufferQ, resource.first, stagesUsed, accessMask, imgLayout, false);
                        }
                        memBarrier.srcStageMask &= cmdBufferSupportedStages;
                        memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;
                        imageBarriers.emplace_back(memBarrier);
                    }
                    // At this point there is no read or write in this resource so if read
                    // write resource and we are in incorrect layout then change it
                    else if (barrierInfo->accessors.lastReadsIn.empty())
                    {
                        memBarrier.oldLayout = determineImageLayout(resource.first);
                        memBarrier.srcAccessMask = determineImageAccessMask(resource.first);
                        // We Will not be in incorrect layout in write image
                        // imageBarriers.emplace_back(memBarrier);
                    }
                    // If not written but read last in same command buffer then wait.
                    // Below barrier is if current usage is write. Read current usage will not reach this point
                    else
                    {
                        // If transfer read at last then use transfer src layout
                        if (BIT_SET(barrierInfo->accessors.lastReadStages, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
                        {
                            memBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                            memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                        }
                        else
                        {
                            memBarrier.oldLayout = determineImageLayout(resource.first);
                            memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                        }

                        memBarrier.srcStageMask = barrierInfo->accessors.allReadStages;
                        for (const GraphicsResource *readInCmd : barrierInfo->accessors.lastReadsIn)
                        {
                            if (cmdBufferManager.isTransferCmdBuffer(readInCmd))
                            {
                                memBarrier.srcAccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT;
                                memBarrier.srcStageMask |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                            }
                            else
                            {
                                memBarrier.srcAccessMask |= VK_ACCESS_2_SHADER_READ_BIT;
                            }
                        }
                        memBarrier.srcStageMask &= cmdBufferSupportedStages;
                        memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;
                        imageBarriers.emplace_back(memBarrier);
                        // No need to qTransfer here as write will always do the transfer
                    }
                }
            }
        }
    }

    if (!imageBarriers.empty() || !bufferBarriers.empty())
    {
        cmdPipelineBarrier(vDevice, cmdBufferManager.getRawBuffer(cmdBuffer), imageBarriers, bufferBarriers);
    }
}

void VulkanCommandList::cmdBarrierVertices(const GraphicsResource *cmdBuffer, ArrayView<const BufferResourceRef> vertexBuffers)
{
    const VkPipelineStageFlags2 stagesUsed = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
    const EQueueFunction cmdBufferQ = cmdBufferManager.getCmdBufferQueue(cmdBuffer);
    uint32 qFamilyIdx = cmdBufferManager.getQueueFamilyIdx(cmdBufferQ);
    VkPipelineStageFlagBits2 cmdBufferSupportedStages = EngineToVulkanAPI::pipelinesSupportedPerQueue(vDevice->getQueueFlags(qFamilyIdx));
    VkAccessFlags2 cmdBufferSupportedAccess = EngineToVulkanAPI::accessMaskPerQueue(vDevice->getQueueFlags(qFamilyIdx));

    std::vector<VkBufferMemoryBarrier2> barriers;
    barriers.reserve(vertexBuffers.size());

    BUFFER_MEMORY_BARRIER2(memBarrier);
    memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = qFamilyIdx;
    memBarrier.dstStageMask = stagesUsed;
    memBarrier.dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
    for (uint32 i = 0; i != vertexBuffers.size(); ++i)
    {
        const BufferResourceRef &vertBuffer = vertexBuffers[i];
        bool bIsWriteBuffer = graphicsHelperCache->isRWBuffer(vertBuffer) || graphicsHelperCache->isWriteOnlyBuffer(vertBuffer);

        std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo
            = bIsWriteBuffer ? resourcesTracker.readFromWriteBuffers(cmdBuffer, { vertBuffer, stagesUsed })
                             : resourcesTracker.readOnlyBuffers(cmdBuffer, { vertBuffer, stagesUsed });

        if (barrierInfo && barrierInfo->accessors.lastWrite)
        {
            memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
            memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;

            if (cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastWrite)
                || BIT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
            {
                memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            }
            else
            {
                memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
            }
            // else only read so no issues

            memBarrier.buffer = vertBuffer.reference<VulkanBufferResource>()->buffer;
            memBarrier.offset = 0;
            memBarrier.size = vertBuffer->getResourceSize();

            memBarrier.srcStageMask &= cmdBufferSupportedStages;
            memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;
            barriers.emplace_back(memBarrier);
            // If access is across queue family
            if (memBarrier.srcQueueFamilyIndex != memBarrier.dstQueueFamilyIndex)
            {
                resourcesTracker.addResourceToQTransfer(cmdBufferQ, vertBuffer, stagesUsed, memBarrier.dstAccessMask, false);
            }
        }
    }

    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    if (!barriers.empty())
    {
        cmdPipelineBarrier(vDevice, rawCmdBuffer, {}, barriers);
    }
}

void VulkanCommandList::cmdBarrierIndices(const GraphicsResource *cmdBuffer, ArrayView<const BufferResourceRef> indexBuffers)
{
    const VkPipelineStageFlags2 stagesUsed = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
    const EQueueFunction cmdBufferQ = cmdBufferManager.getCmdBufferQueue(cmdBuffer);
    uint32 qFamilyIdx = cmdBufferManager.getQueueFamilyIdx(cmdBufferQ);
    VkPipelineStageFlagBits2 cmdBufferSupportedStages = EngineToVulkanAPI::pipelinesSupportedPerQueue(vDevice->getQueueFlags(qFamilyIdx));
    VkAccessFlags2 cmdBufferSupportedAccess = EngineToVulkanAPI::accessMaskPerQueue(vDevice->getQueueFlags(qFamilyIdx));

    std::vector<VkBufferMemoryBarrier2> barriers;
    barriers.reserve(indexBuffers.size());

    for (const BufferResourceRef &indexBuffer : indexBuffers)
    {
        BUFFER_MEMORY_BARRIER2(memBarrier);
        memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = qFamilyIdx;
        memBarrier.dstStageMask = stagesUsed;
        memBarrier.dstAccessMask = VK_ACCESS_2_INDEX_READ_BIT;

        bool bIsWriteBuffer = graphicsHelperCache->isRWBuffer(indexBuffer) || graphicsHelperCache->isWriteOnlyBuffer(indexBuffer);
        std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo
            = bIsWriteBuffer ? resourcesTracker.readFromWriteBuffers(cmdBuffer, { indexBuffer, stagesUsed })
                             : resourcesTracker.readOnlyBuffers(cmdBuffer, { indexBuffer, stagesUsed });

        if (barrierInfo && barrierInfo->accessors.lastWrite)
        {
            memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
            memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;

            if (cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastWrite)
                || BIT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
            {
                memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            }
            else
            {
                memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
            }
            // else only read so no issues

            memBarrier.buffer = indexBuffer.reference<VulkanBufferResource>()->buffer;
            memBarrier.offset = 0;
            memBarrier.size = indexBuffer->getResourceSize();

            memBarrier.srcStageMask &= cmdBufferSupportedStages;
            memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;
            // If access is across queue family
            if (memBarrier.srcQueueFamilyIndex != memBarrier.dstQueueFamilyIndex)
            {
                resourcesTracker.addResourceToQTransfer(cmdBufferQ, indexBuffer, stagesUsed, memBarrier.dstAccessMask, false);
            }

            barriers.emplace_back(std::move(memBarrier));
        }
    }

    if (!barriers.empty())
    {
        VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
        cmdPipelineBarrier(vDevice, rawCmdBuffer, {}, barriers);
    }
}

void VulkanCommandList::cmdBarrierIndirectDraws(const GraphicsResource *cmdBuffer, ArrayView<const BufferResourceRef> indirectDrawBuffers)
{
    const VkPipelineStageFlags2 stagesUsed = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
    const EQueueFunction cmdBufferQ = cmdBufferManager.getCmdBufferQueue(cmdBuffer);
    uint32 qFamilyIdx = cmdBufferManager.getQueueFamilyIdx(cmdBufferQ);
    VkPipelineStageFlagBits2 cmdBufferSupportedStages = EngineToVulkanAPI::pipelinesSupportedPerQueue(vDevice->getQueueFlags(qFamilyIdx));
    VkAccessFlags2 cmdBufferSupportedAccess = EngineToVulkanAPI::accessMaskPerQueue(vDevice->getQueueFlags(qFamilyIdx));

    std::vector<VkBufferMemoryBarrier2> barriers;
    barriers.reserve(indirectDrawBuffers.size());

    for (const BufferResourceRef &drawCmdsBuffer : indirectDrawBuffers)
    {
        BUFFER_MEMORY_BARRIER2(memBarrier);
        memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = qFamilyIdx;
        memBarrier.dstStageMask = stagesUsed;
        memBarrier.dstAccessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;

        bool bIsWriteBuffer = graphicsHelperCache->isRWBuffer(drawCmdsBuffer) || graphicsHelperCache->isWriteOnlyBuffer(drawCmdsBuffer);
        std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo
            = bIsWriteBuffer ? resourcesTracker.readFromWriteBuffers(cmdBuffer, { drawCmdsBuffer, stagesUsed })
                             : resourcesTracker.readOnlyBuffers(cmdBuffer, { drawCmdsBuffer, stagesUsed });

        if (barrierInfo && barrierInfo->accessors.lastWrite)
        {
            memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
            memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;

            if (cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastWrite)
                || BIT_SET(barrierInfo->accessors.lastWriteStage, VK_PIPELINE_STAGE_2_TRANSFER_BIT))
            {
                memBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            }
            else
            {
                memBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
            }
            // else only read so no issues

            memBarrier.buffer = drawCmdsBuffer.reference<VulkanBufferResource>()->buffer;
            memBarrier.offset = 0;
            memBarrier.size = drawCmdsBuffer->getResourceSize();

            memBarrier.srcStageMask &= cmdBufferSupportedStages;
            memBarrier.srcAccessMask &= EngineToVulkanAPI::accessMaskForStages(memBarrier.srcStageMask) & cmdBufferSupportedAccess;

            // If access is across queue family
            if (memBarrier.srcQueueFamilyIndex != memBarrier.dstQueueFamilyIndex)
            {
                resourcesTracker.addResourceToQTransfer(cmdBufferQ, drawCmdsBuffer, stagesUsed, memBarrier.dstAccessMask, false);
            }

            barriers.emplace_back(std::move(memBarrier));
        }
    }

    if (!barriers.empty())
    {
        VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
        cmdPipelineBarrier(vDevice, rawCmdBuffer, {}, barriers);
    }
}

void VulkanCommandList::cmdReleaseQueueResources(const GraphicsResource *cmdBuffer, EQueueFunction releaseToQueue)
{
    cmdReleaseQueueResources(cmdBuffer, releaseToQueue, {});
}

void VulkanCommandList::cmdReleaseQueueResources(
    const GraphicsResource *cmdBuffer, EQueueFunction releaseToQueue,
    const std::unordered_map<MemoryResourceRef, EQueueFunction> &perResourceRelease
)
{
    const EQueueFunction currentQueue = cmdBufferManager.getCmdBufferQueue(cmdBuffer);
    const uint32 qFamilyIdx = cmdBufferManager.getQueueFamilyIdx(currentQueue);

    const uint32 defaultReleaseToIdx = cmdBufferManager.getQueueFamilyIdx(releaseToQueue);

    std::map<MemoryResource *, VulkanResourcesTracker::ResourceQueueTransferInfo> resToQRelease
        = resourcesTracker.getReleasesFromQueue(currentQueue);

    std::vector<VkImageMemoryBarrier2> imageBarriers;
    std::vector<VkBufferMemoryBarrier2> bufferBarriers;

    for (const std::pair<MemoryResource *const, VulkanResourcesTracker::ResourceQueueTransferInfo> &res : resToQRelease)
    {
        if (res.first->refCount() == 0)
        {
            continue;
        }
        MemoryResourceRef resourceRef = res.first;

        auto resQReleaseOverride = perResourceRelease.find(resourceRef);
        const uint32 dstQFamilyIdx = resQReleaseOverride != perResourceRelease.cend()
                                         ? cmdBufferManager.getQueueFamilyIdx(resQReleaseOverride->second)
                                         : defaultReleaseToIdx;

        if (resourceRef->getType()->isChildOf<ImageResource>())
        {
            IMAGE_MEMORY_BARRIER2(imgBarrier);
            imgBarrier.srcAccessMask = res.second.srcAccessMask;
            imgBarrier.srcStageMask = res.second.srcStages;
            imgBarrier.oldLayout = res.second.srcLayout;

            imgBarrier.srcQueueFamilyIndex = qFamilyIdx;

            imgBarrier.dstQueueFamilyIndex = dstQFamilyIdx;
            // imgBarrier.dstStageMask = // What will be here?
            // imgBarrier.dstAccessMask // can be skipped
            imgBarrier.newLayout = determineImageLayout(resourceRef);

            imgBarrier.image = resourceRef.reference<VulkanImageResource>()->image;
            imgBarrier.subresourceRange = { determineImageAspect(resourceRef), 0, resourceRef.reference<ImageResource>()->getNumOfMips(), 0,
                                            resourceRef.reference<ImageResource>()->getLayerCount() };
            if (dstQFamilyIdx != qFamilyIdx)
            {
                imageBarriers.emplace_back(std::move(imgBarrier));
            }
            else
            {
                // Add back to queue if we are not changing the queue family index
                resourcesTracker.addResourceToQTransfer(
                    currentQueue, resourceRef, res.second.srcStages, res.second.srcAccessMask, res.second.srcLayout, true
                );
            }
        }
        else
        {
            BUFFER_MEMORY_BARRIER2(memBarrier);
            memBarrier.srcAccessMask = res.second.srcAccessMask;
            memBarrier.srcStageMask = res.second.srcStages;

            memBarrier.srcQueueFamilyIndex = qFamilyIdx;

            memBarrier.dstQueueFamilyIndex = dstQFamilyIdx;
            // memBarrier.dstStageMask = // What will be here?
            // memBarrier.dstAccessMask // can be skipped

            memBarrier.buffer = resourceRef.reference<VulkanBufferResource>()->buffer;
            memBarrier.offset = 0;
            memBarrier.size = resourceRef->getResourceSize();

            if (dstQFamilyIdx != qFamilyIdx)
            {
                bufferBarriers.emplace_back(std::move(memBarrier));
            }
            else
            {
                // Add back to queue if we are not changing the queue family index
                resourcesTracker.addResourceToQTransfer(currentQueue, resourceRef, res.second.srcStages, res.second.srcAccessMask, true);
            }
        }
    }

    if (!imageBarriers.empty() || !bufferBarriers.empty())
    {
        cmdPipelineBarrier(vDevice, cmdBufferManager.getRawBuffer(cmdBuffer), imageBarriers, bufferBarriers);
    }
}

void VulkanCommandList::cmdBeginRenderPass(
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const QuantizedBox2D &renderArea,
    const RenderPassAdditionalProps &renderpassAdditionalProps, const RenderPassClearValue &clearColor
)
{
    if (!renderArea.isValidAABB())
    {
        LOG_ERROR("VulkanCommandList", "Incorrect render area");
        debugAssert(false);
        return;
    }
    if (cmdBuffer == nullptr || contextPipeline.getPipeline() == nullptr || contextPipeline.getFb() == nullptr)
    {
        debugAssert(false);
        return;
    }
    VulkanGlobalRenderingContext *renderingContext
        = static_cast<VulkanGlobalRenderingContext *>(IRenderInterfaceModule::get()->getRenderManager()->getGlobalRenderingContext());
    const VulkanGraphicsPipeline *graphicsPipeline = static_cast<const VulkanGraphicsPipeline *>(contextPipeline.getPipeline());

    Size2D extent = renderArea.size();
    std::vector<VkClearValue> clearValues;

    VkClearColorValue lastClearColor;
    lastClearColor.float32[0] = LinearColorConst::BLACK.r();
    lastClearColor.float32[1] = LinearColorConst::BLACK.g();
    lastClearColor.float32[2] = LinearColorConst::BLACK.b();
    lastClearColor.float32[3] = LinearColorConst::BLACK.a();
    // If swapchain there will be only one attachment as we are using it for drawing before present
    if (contextPipeline.windowCanvas.isValid())
    {
        if (!clearColor.colors.empty())
        {
            if (static_cast<const GraphicsPipelineBase *>(contextPipeline.getPipeline())
                    ->getRenderpassProperties()
                    .renderpassAttachmentFormat.attachments.empty())
            {
                fillClearValue(
                    static_cast<const GraphicsPipelineBase *>(contextPipeline.getPipeline())
                        ->getRenderpassProperties()
                        .renderpassAttachmentFormat.attachments[0],
                    lastClearColor, clearColor.colors[0]
                );
            }
            else
            {
                lastClearColor.float32[0] = clearColor.colors[0].r();
                lastClearColor.float32[1] = clearColor.colors[0].g();
                lastClearColor.float32[2] = clearColor.colors[0].b();
                lastClearColor.float32[3] = clearColor.colors[0].a();
            }
        }
        VkClearValue clearVal;
        clearVal.color = lastClearColor;
        clearValues.emplace_back(clearVal);

        swapchainFrameWrites.emplace_back(cmdBuffer);
    }
    else
    {
        uint32 colorIdx = 0;
        for (const ImageResourceRef &frameTexture : contextPipeline.getFb()->textures)
        {
            // no need to barrier as render pass load/clear both will have implicit barriers
            resourcesTracker.colorAttachmentWrite(cmdBuffer, frameTexture);

            if (EPixelDataFormat::isDepthFormat(frameTexture->imageFormat()))
            {
                VkClearValue clearVal;
                clearVal.depthStencil = { clearColor.depth, clearColor.stencil };
                clearValues.emplace_back(clearVal);
            }
            else
            {
                if (colorIdx < clearColor.colors.size())
                {
                    fillClearValue(frameTexture->imageFormat(), lastClearColor, clearColor.colors[colorIdx]);
                }
                VkClearValue clearVal;
                clearVal.color = lastClearColor;
                clearValues.emplace_back(clearVal);
                colorIdx++;
            }
        }
    }

    RENDERPASS_BEGIN_INFO(beginInfo);
    beginInfo.clearValueCount = uint32(clearValues.size());
    beginInfo.pClearValues = clearValues.data();
    beginInfo.framebuffer = VulkanGraphicsHelper::getFramebuffer(contextPipeline.getFb());
    beginInfo.renderPass = renderingContext->getRenderPass(graphicsPipeline->getRenderpassProperties(), renderpassAdditionalProps);
    beginInfo.renderArea = {
        {renderArea.minBound.x, renderArea.minBound.y}, // offset
        {             extent.x,              extent.y}
    };

    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    vDevice->vkCmdBeginRenderPass(rawCmdBuffer, &beginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
    cmdBufferManager.startRenderPass(cmdBuffer);
}

void VulkanCommandList::cmdEndRenderPass(const GraphicsResource *cmdBuffer)
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    vDevice->vkCmdEndRenderPass(rawCmdBuffer);
    cmdBufferManager.endRenderPass(cmdBuffer);
}

void VulkanCommandList::cmdBindComputePipeline(const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    const VulkanComputePipeline *computePipeline = static_cast<const VulkanComputePipeline *>(contextPipeline.getPipeline());

    vDevice->vkCmdBindPipeline(rawCmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline->getPipeline());
}

void VulkanCommandList::cmdBindGraphicsPipeline(
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, const GraphicsPipelineState &state
) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    const VulkanGraphicsPipeline *graphicsPipeline = static_cast<const VulkanGraphicsPipeline *>(contextPipeline.getPipeline());
    VkPipeline pipeline = graphicsPipeline->getPipeline(state.pipelineQuery);

    if (pipeline == VK_NULL_HANDLE)
    {
        LOG_ERROR("VulkanCommandList", "Pipeline is invalid");
        debugAssert(false);
        return;
    }
    vDevice->vkCmdBindPipeline(rawCmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    if (state.blendConstant)
    {
        float blendConst[] = { state.blendConstant->r(), state.blendConstant->g(), state.blendConstant->b(), state.blendConstant->a() };
        vDevice->vkCmdSetBlendConstants(rawCmdBuffer, blendConst);
    }
    if (state.lineWidth)
    {
        vDevice->vkCmdSetLineWidth(rawCmdBuffer, *state.lineWidth);
    }
    for (const std::pair<EStencilFaceMode, uint32> &stencilRef : state.stencilReferences)
    {
        vDevice->vkCmdSetStencilReference(rawCmdBuffer, VkStencilFaceFlagBits(stencilRef.first), stencilRef.second);
    }
}

void VulkanCommandList::cmdPushConstants(
    const GraphicsResource *cmdBuffer, const LocalPipelineContext &contextPipeline, uint32 stagesUsed, const uint8 *data,
    ArrayView<const CopyBufferInfo> pushConsts
) const
{
    VkPipelineLayout pipelineLayout = nullptr;
    if (contextPipeline.getPipeline()->getType()->isChildOf<GraphicsPipelineBase>())
    {
        pipelineLayout = static_cast<const VulkanGraphicsPipeline *>(contextPipeline.getPipeline())->pipelineLayout;
    }
    else if (contextPipeline.getPipeline()->getType()->isChildOf<ComputePipelineBase>())
    {
        pipelineLayout = static_cast<const VulkanComputePipeline *>(contextPipeline.getPipeline())->pipelineLayout;
    }
    else
    {
        LOG_ERROR("VulkanPipeline", "Invalid pipeline %s", contextPipeline.getPipeline()->getResourceName().getChar());
        debugAssert(false);
        return;
    }
    for (const CopyBufferInfo &copyInfo : pushConsts)
    {
        vDevice->vkCmdPushConstants(
            cmdBufferManager.getRawBuffer(cmdBuffer), pipelineLayout, stagesUsed, uint32(copyInfo.dstOffset), copyInfo.copySize,
            data + copyInfo.srcOffset
        );
    }
}

void VulkanCommandList::cmdBindDescriptorsSetInternal(
    const GraphicsResource *cmdBuffer, const PipelineBase *contextPipeline, const std::map<uint32, ShaderParametersRef> &descriptorsSets
) const
{
    std::map<uint32, std::vector<VkDescriptorSet>> descsSets;

    for (const std::pair<const uint32, const ShaderParametersRef> &descsSet : descriptorsSets)
    {
        // If first element or next expected sequential set ID is not equal to current ID
        if (descsSets.empty() || descsSet.first != (--descsSets.end())->first + (--descsSets.end())->second.size())
        {
            descsSets[descsSet.first].emplace_back(descsSet.second.reference<VulkanShaderSetParameters>()->descriptorsSet);
        }
        else
        {
            (--descsSets.end())->second.emplace_back(descsSet.second.reference<VulkanShaderSetParameters>()->descriptorsSet);
        }
    }

    VkPipelineBindPoint pipelineBindPt = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_MAX_ENUM;
    VkPipelineLayout pipelineLayout = nullptr;
    if (contextPipeline->getType()->isChildOf<GraphicsPipelineBase>())
    {
        pipelineBindPt = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
        pipelineLayout = static_cast<const VulkanGraphicsPipeline *>(contextPipeline)->pipelineLayout;
    }
    else if (contextPipeline->getType()->isChildOf<ComputePipelineBase>())
    {
        pipelineBindPt = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;
        pipelineLayout = static_cast<const VulkanComputePipeline *>(contextPipeline)->pipelineLayout;
    }
    else
    {
        LOG_ERROR("VulkanPipeline", "Invalid pipeline %s", contextPipeline->getResourceName().getChar());
        debugAssert(false);
        return;
    }
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    for (const std::pair<const uint32, std::vector<VkDescriptorSet>> &descsSet : descsSets)
    {
        vDevice->vkCmdBindDescriptorSets(
            rawCmdBuffer, pipelineBindPt, pipelineLayout, descsSet.first, uint32(descsSet.second.size()), descsSet.second.data(), 0, nullptr
        );
    }
}

void VulkanCommandList::cmdBindDescriptorsSetsInternal(
    const GraphicsResource *cmdBuffer, const PipelineBase *contextPipeline, ArrayView<const ShaderParametersRef> descriptorsSets
) const
{
    std::map<uint32, std::vector<VkDescriptorSet>> descsSets;
    {
        std::map<uint32, VkDescriptorSet> tempDescsSets;
        for (const ShaderParametersRef shaderParams : descriptorsSets)
        {
            const VulkanShaderParameters *vulkanShaderParams = shaderParams.reference<VulkanShaderParameters>();
            tempDescsSets.insert(vulkanShaderParams->descriptorsSets.cbegin(), vulkanShaderParams->descriptorsSets.cend());
        }

        for (const std::pair<const uint32, VkDescriptorSet> &descsSet : tempDescsSets)
        {
            // If first element or next expected sequential set ID is not equal to current ID
            if (descsSets.empty() || descsSet.first != (--descsSets.end())->first + (--descsSets.end())->second.size())
            {
                descsSets[descsSet.first].emplace_back(descsSet.second);
            }
            else
            {
                (--descsSets.end())->second.emplace_back(descsSet.second);
            }
        }
    }

    VkPipelineBindPoint pipelineBindPt = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_MAX_ENUM;
    VkPipelineLayout pipelineLayout = nullptr;
    if (contextPipeline->getType()->isChildOf<GraphicsPipelineBase>())
    {
        pipelineBindPt = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
        pipelineLayout = static_cast<const VulkanGraphicsPipeline *>(contextPipeline)->pipelineLayout;
    }
    else if (contextPipeline->getType()->isChildOf<ComputePipelineBase>())
    {
        pipelineBindPt = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;
        pipelineLayout = static_cast<const VulkanComputePipeline *>(contextPipeline)->pipelineLayout;
    }
    else
    {
        LOG_ERROR("VulkanPipeline", "Invalid pipeline %s", contextPipeline->getResourceName().getChar());
        debugAssert(false);
        return;
    }
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    for (const std::pair<const uint32, std::vector<VkDescriptorSet>> &descsSet : descsSets)
    {
        vDevice->vkCmdBindDescriptorSets(
            rawCmdBuffer, pipelineBindPt, pipelineLayout, descsSet.first, uint32(descsSet.second.size()), descsSet.second.data(), 0, nullptr
        );
    }
}

void VulkanCommandList::cmdBindVertexBuffer(
    const GraphicsResource *cmdBuffer, uint32 firstBinding, BufferResourceRef vertexBuffer, uint64 offset
)
{
    cmdBindVertexBuffers(cmdBuffer, firstBinding, { &vertexBuffer, 1 }, { &offset, 1 });
}

void VulkanCommandList::cmdBindVertexBuffers(
    const GraphicsResource *cmdBuffer, uint32 firstBinding, ArrayView<const BufferResourceRef> vertexBuffers, ArrayView<const uint64> offsets
)
{
    fatalAssertf(vertexBuffers.size() == offsets.size(), "Offsets must be equivalent to vertex buffers");

    std::vector<VkBuffer> vertBuffers(vertexBuffers.size());
    for (int32 i = 0; i < vertexBuffers.size(); ++i)
    {
        vertBuffers[i] = vertexBuffers[i].reference<VulkanBufferResource>()->buffer;
    }

    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    vDevice->vkCmdBindVertexBuffers(rawCmdBuffer, firstBinding, uint32(vertexBuffers.size()), vertBuffers.data(), offsets.data());
}

void VulkanCommandList::cmdBindIndexBuffer(const GraphicsResource *cmdBuffer, const BufferResourceRef &indexBuffer, uint64 offset /*= 0*/)
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    vDevice->vkCmdBindIndexBuffer(
        rawCmdBuffer, indexBuffer.reference<VulkanBufferResource>()->buffer, offset, VkIndexType::VK_INDEX_TYPE_UINT32
    );
}

void VulkanCommandList::cmdDispatch(const GraphicsResource *cmdBuffer, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ /*= 1*/) const
{
    vDevice->vkCmdDispatch(cmdBufferManager.getRawBuffer(cmdBuffer), groupSizeX, groupSizeY, groupSizeZ);
}

void VulkanCommandList::cmdDrawIndexed(
    const GraphicsResource *cmdBuffer, uint32 firstIndex, uint32 indexCount, uint32 firstInstance /*= 0*/, uint32 instanceCount /*= 1*/,
    int32 vertexOffset /*= 0*/
) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    vDevice->vkCmdDrawIndexed(rawCmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandList::cmdDrawVertices(
    const GraphicsResource *cmdBuffer, uint32 firstVertex, uint32 vertexCount, uint32 firstInstance /*= 0*/, uint32 instanceCount /*= 1*/
) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    vDevice->vkCmdDraw(rawCmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandList::cmdDrawIndexedIndirect(
    const GraphicsResource *cmdBuffer, const BufferResourceRef &drawCmdsBuffer, uint32 bufferOffset, uint32 drawCount, uint32 stride
)
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    for (uint32 drawnCount = 0; drawnCount < drawCount;)
    {
        uint32 currDrawCount = Math::min(GlobalRenderVariables::MAX_INDIRECT_DRAW_COUNT.get(), drawCount - drawnCount);
        vDevice->vkCmdDrawIndexedIndirect(
            rawCmdBuffer, drawCmdsBuffer.reference<VulkanBufferResource>()->buffer, bufferOffset + drawnCount * stride, drawCount, stride
        );
        drawnCount += currDrawCount;
    }
}

void VulkanCommandList::cmdDrawIndirect(
    const GraphicsResource *cmdBuffer, const BufferResourceRef &drawCmdsBuffer, uint32 bufferOffset, uint32 drawCount, uint32 stride
)
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    for (uint32 drawnCount = 0; drawnCount < drawCount;)
    {
        uint32 currDrawCount = Math::min(GlobalRenderVariables::MAX_INDIRECT_DRAW_COUNT.get(), drawCount - drawnCount);
        vDevice->vkCmdDrawIndirect(
            rawCmdBuffer, drawCmdsBuffer.reference<VulkanBufferResource>()->buffer, bufferOffset + drawnCount * stride, drawCount, stride
        );
        drawnCount += currDrawCount;
    }
}

void VulkanCommandList::cmdSetViewportAndScissors(
    const GraphicsResource *cmdBuffer, ArrayView<const std::pair<QuantizedBox2D, QuantizedBox2D>> viewportAndScissors,
    uint32 firstViewport /*= 0*/
) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    std::vector<VkViewport> viewports;
    viewports.reserve(viewportAndScissors.size());
    std::vector<VkRect2D> scissors;
    scissors.reserve(viewportAndScissors.size());
    for (std::pair<QuantizedBox2D, QuantizedBox2D> viewportAndScis : viewportAndScissors)
    {
        Int2D viewportSize = viewportAndScis.first.size();
        viewports.emplace_back(VkViewport{ float(viewportAndScis.first.minBound.x), float(viewportAndScis.first.minBound.y),
                                           float(viewportSize.x), float(viewportSize.y), 0.f /* Min depth */, 1.f /* Max depth */ });

        viewportAndScis.second.fixAABB();
        Size2D scissorSize = viewportAndScis.second.size();
        scissors.emplace_back(VkRect2D{
            {viewportAndScis.second.minBound.x, viewportAndScis.second.minBound.y},
            {                    scissorSize.x,                     scissorSize.y}
        });
    }

    vDevice->vkCmdSetViewport(rawCmdBuffer, firstViewport, uint32(viewports.size()), viewports.data());
    vDevice->vkCmdSetScissor(rawCmdBuffer, firstViewport, uint32(scissors.size()), scissors.data());
}

void VulkanCommandList::cmdSetViewportAndScissor(
    const GraphicsResource *cmdBuffer, const QuantizedBox2D &viewport, const QuantizedBox2D &scissor, uint32 atViewport /*= 0*/
) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    Int2D viewportSize = viewport.size();
    VkViewport vulkanViewport{ float(viewport.minBound.x), float(viewport.minBound.y), float(viewportSize.x),
                               float(viewportSize.y),      0.f /* Min depth */,        1.f /* Max depth */ };
    vDevice->vkCmdSetViewport(rawCmdBuffer, atViewport, 1, &vulkanViewport);

    cmdSetScissor(cmdBuffer, scissor, atViewport);
}

void VulkanCommandList::cmdSetScissor(const GraphicsResource *cmdBuffer, const QuantizedBox2D &scissor, uint32 atViewport /*= 0*/) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    if (scissor.isValidAABB())
    {
        Size2D scissorSize = scissor.size();
        VkRect2D vulkanScissor{
            {scissor.minBound.x, scissor.minBound.y},
            {     scissorSize.x,      scissorSize.y}
        };
        vDevice->vkCmdSetScissor(rawCmdBuffer, atViewport, 1, &vulkanScissor);
    }
    else
    {
        QuantizedBox2D tempScissor(scissor);
        tempScissor.fixAABB();

        Size2D scissorSize = tempScissor.size();
        VkRect2D vulkanScissor{
            {tempScissor.minBound.x, tempScissor.minBound.y},
            {         scissorSize.x,          scissorSize.y}
        };
        vDevice->vkCmdSetScissor(rawCmdBuffer, atViewport, 1, &vulkanScissor);
    }
}

void VulkanCommandList::cmdSetLineWidth(const GraphicsResource *cmdBuffer, float lineWidth) const
{
    if (GlobalRenderVariables::ENABLE_WIDE_LINES)
    {
        VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
        vDevice->vkCmdSetLineWidth(rawCmdBuffer, lineWidth);
    }
}

void VulkanCommandList::cmdSetDepthBias(const GraphicsResource *cmdBuffer, float constantBias, float slopeFactor, float clampValue) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    vDevice->vkCmdSetDepthBias(rawCmdBuffer, constantBias, clampValue, slopeFactor);
}

void VulkanCommandList::
    cmdBeginBufferMarker(const GraphicsResource *commandBuffer, const String &name, const LinearColor &color /*= LinearColorConst::WHITE*/)
        const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(commandBuffer);
    VulkanGraphicsHelper::debugGraphics(graphicsInstanceCache)->beginCmdBufferMarker(rawCmdBuffer, name, color);
}

void VulkanCommandList::
    cmdInsertBufferMarker(const GraphicsResource *commandBuffer, const String &name, const LinearColor &color /*= LinearColorConst::WHITE*/)
        const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(commandBuffer);
    VulkanGraphicsHelper::debugGraphics(graphicsInstanceCache)->insertCmdBufferMarker(rawCmdBuffer, name, color);
}

void VulkanCommandList::cmdEndBufferMarker(const GraphicsResource *commandBuffer) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(commandBuffer);
    VulkanGraphicsHelper::debugGraphics(graphicsInstanceCache)->endCmdBufferMarker(rawCmdBuffer);
}

void VulkanCommandList::copyToImage(ImageResourceRef dst, ArrayView<const class Color> pixelData, const CopyPixelsToImageInfo &copyInfo)
{
    fatalAssertf(dst->isValid(), "Invalid image resource %s", dst->getResourceName().getChar());
    if (EPixelDataFormat::isDepthFormat(dst->imageFormat()) || EPixelDataFormat::isFloatingFormat(dst->imageFormat()))
    {
        LOG_ERROR("VulkanCommandList", "Depth/Float format is not supported for copying from Color data");
        return;
    }
    const EPixelDataFormat::PixelFormatInfo *formatInfo = EPixelDataFormat::getFormatInfo(dst->imageFormat());

    // Add 32 bit extra space to staging to compensate 32 mask out of range when copying data
    uint32 dataMargin = uint32(Math::ceil(float(sizeof(uint32)) / formatInfo->pixelDataSize));
    BufferResourceRef stagingBuffer
        = graphicsHelperCache->createReadOnlyBuffer(graphicsInstanceCache, formatInfo->pixelDataSize, uint32(pixelData.size()) + dataMargin);
    stagingBuffer->setAsStagingResource(true);
    stagingBuffer->setDeferredDelete(false);
    stagingBuffer->setResourceName(dst->getResourceName() + TCHAR("_Staging"));
    stagingBuffer->init();

    uint8 *stagingPtr = reinterpret_cast<uint8 *>(graphicsHelperCache->borrowMappedPtr(graphicsInstanceCache, stagingBuffer));
    if (!simpleCopyPixelsTo(stagingBuffer, stagingPtr, pixelData, dst->imageFormat(), formatInfo))
    {
        copyPixelsTo(stagingBuffer, stagingPtr, pixelData, formatInfo);
    }
    graphicsHelperCache->returnMappedPtr(graphicsInstanceCache, stagingBuffer);

    copyToImage_Internal(dst, stagingBuffer, copyInfo);
    stagingBuffer->release();
    stagingBuffer.reset();
}

void VulkanCommandList::copyToImage(ImageResourceRef dst, ArrayView<const class LinearColor> pixelData, const CopyPixelsToImageInfo &copyInfo)
{
    fatalAssertf(dst->isValid(), "Invalid image resource %s", dst->getResourceName().getChar());
    const EPixelDataFormat::PixelFormatInfo *formatInfo = EPixelDataFormat::getFormatInfo(dst->imageFormat());
    if (EPixelDataFormat::isDepthFormat(dst->imageFormat())
        && (formatInfo->componentSize[0] != 32 || EPixelDataFormat::isStencilFormat(dst->imageFormat())))
    {
        LOG_ERROR(
            "VulkanCommandList", "Depth/Float format with size other than 32bit is not supported for copying from "
                                 "Color data"
        );
        return;
    }

    // Add 32 bit extra space to staging to compensate 32 mask out of range when copying data
    uint32 dataMargin = uint32(Math::ceil(float(sizeof(uint32)) / formatInfo->pixelDataSize));
    BufferResourceRef stagingBuffer
        = graphicsHelperCache->createReadOnlyBuffer(graphicsInstanceCache, formatInfo->pixelDataSize, uint32(pixelData.size()) + dataMargin);
    stagingBuffer->setAsStagingResource(true);
    stagingBuffer->setDeferredDelete(false);
    stagingBuffer->setResourceName(dst->getResourceName() + TCHAR("_Staging"));
    stagingBuffer->init();

    uint8 *stagingPtr = reinterpret_cast<uint8 *>(graphicsHelperCache->borrowMappedPtr(graphicsInstanceCache, stagingBuffer));
    copyPixelsTo(
        stagingBuffer, stagingPtr, pixelData, formatInfo,
        EPixelDataFormat::isDepthFormat(dst->imageFormat()) || EPixelDataFormat::isFloatingFormat(dst->imageFormat())
    );
    graphicsHelperCache->returnMappedPtr(graphicsInstanceCache, stagingBuffer);

    copyToImage_Internal(dst, stagingBuffer, copyInfo);
    stagingBuffer->release();
    stagingBuffer.reset();
}

void VulkanCommandList::copyToImageLinearMapped(
    ImageResourceRef dst, ArrayView<const class Color> pixelData, const CopyPixelsToImageInfo &copyInfo
)
{
    fatalAssertf(dst->isValid(), "Invalid image resource %s", dst->getResourceName().getChar());
    if (EPixelDataFormat::isDepthFormat(dst->imageFormat()) || EPixelDataFormat::isFloatingFormat(dst->imageFormat()))
    {
        LOG_ERROR("VulkanCommandList", "Depth/Float format is not supported for copying from Color data");
        return;
    }

    const EPixelDataFormat::PixelFormatInfo *formatInfo = EPixelDataFormat::getFormatInfo(dst->imageFormat());

    // Add 32 bit extra space to staging to compensate 32 mask out of range when copying data
    uint32 dataMargin = uint32(Math::ceil(float(sizeof(uint32)) / formatInfo->pixelDataSize));
    BufferResourceRef stagingBuffer
        = graphicsHelperCache->createReadOnlyBuffer(graphicsInstanceCache, formatInfo->pixelDataSize, uint32(pixelData.size()) + dataMargin);
    stagingBuffer->setAsStagingResource(true);
    stagingBuffer->setDeferredDelete(false);
    stagingBuffer->setResourceName(dst->getResourceName() + TCHAR("_Staging"));
    stagingBuffer->init();

    uint8 *stagingPtr = reinterpret_cast<uint8 *>(graphicsHelperCache->borrowMappedPtr(graphicsInstanceCache, stagingBuffer));
    if (!simpleCopyPixelsTo(stagingBuffer, stagingPtr, pixelData, dst->imageFormat(), formatInfo))
    {
        copyPixelsLinearMappedTo(stagingBuffer, stagingPtr, pixelData, formatInfo);
    }
    graphicsHelperCache->returnMappedPtr(graphicsInstanceCache, stagingBuffer);

    copyToImage_Internal(dst, stagingBuffer, copyInfo);
    stagingBuffer->release();
    stagingBuffer.reset();
}

void VulkanCommandList::copyToImage_Internal(ImageResourceRef dst, const BufferResourceRef &pixelData, CopyPixelsToImageInfo copyInfo)
{
    // Make sure mips and layers never exceeds above max
    copyInfo.subres.mipCount = Math::min(copyInfo.subres.mipCount, dst->getNumOfMips());
    copyInfo.subres.layersCount = Math::min(copyInfo.subres.layersCount, dst->getLayerCount());

    const VkFilter filtering
        = EngineToVulkanAPI::vulkanFilter(graphicsHelperCache->clampFiltering(graphicsInstanceCache, copyInfo.mipFiltering, dst->imageFormat())
        );

    const VkImageAspectFlags imageAspect = determineImageAspect(dst);

    // Layout that is acceptable for this image
    VkImageLayout postCopyLayout = determineImageLayout(dst);
    VkAccessFlags postCopyAccessMask = determineImageAccessMask(dst);
    VkPipelineStageFlags postCopyStages = resourceShaderStageFlags();

    // TODO(Jeslas) : change this to get current layout from some resource tracked layout
    VkImageLayout currentLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

    std::vector<VkBufferImageCopy> copies;
    if (copyInfo.bGenerateMips)
    {
        copies.resize(1);
        copies[0].imageExtent = { copyInfo.extent.x, copyInfo.extent.y, copyInfo.extent.z };
        copies[0].imageOffset = { int32(copyInfo.dstOffset.x), int32(copyInfo.dstOffset.y), int32(copyInfo.dstOffset.z) };
        copies[0].bufferOffset = copies[0].bufferRowLength = copies[0].bufferImageHeight = 0;
        copies[0].imageSubresource = { imageAspect, copyInfo.subres.baseMip, copyInfo.subres.baseLayer, copyInfo.subres.layersCount };
    }
    else
    {
        uint32 mipLinearOffset = 0;
        Size3D mipSize = copyInfo.extent;
        Size3D mipSizeOffset = copyInfo.dstOffset;

        for (uint32 mipLevel = 0; mipLevel < copyInfo.subres.mipCount; ++mipLevel)
        {
            VkBufferImageCopy vkCopyInfo;
            vkCopyInfo.imageExtent = { mipSize.x, mipSize.y, mipSize.z };
            vkCopyInfo.imageOffset = { int32(mipSizeOffset.x), int32(mipSizeOffset.y), int32(mipSizeOffset.z) };
            vkCopyInfo.bufferOffset = mipLinearOffset;
            vkCopyInfo.bufferRowLength = mipSize.x;
            vkCopyInfo.bufferImageHeight = mipSize.y;
            vkCopyInfo.imageSubresource
                = { imageAspect, copyInfo.subres.baseMip + mipLevel, copyInfo.subres.baseLayer, copyInfo.subres.layersCount };

            copies.emplace_back(vkCopyInfo);

            mipLinearOffset += mipSize.x * mipSize.y * mipSize.z * copyInfo.subres.layersCount;
            mipSize = Math::max(mipSize / 2u, Size3D{ 1, 1, 1 });
            mipSizeOffset /= 2u;
        }
    }

    const bool bRequiresGraphicsQ = copyInfo.bGenerateMips || EPixelDataFormat::isDepthFormat(dst->imageFormat());
    const GraphicsResource *cmdBuffer = cmdBufferManager.beginTempCmdBuffer(
        TCHAR("CopyPixelToImage_") + dst->getResourceName(), bRequiresGraphicsQ ? EQueueFunction::Graphics : EQueueFunction::Transfer
    );
    uint32 qFamilyIdx = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);

    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    if (cmdBufferManager.isTransferCmdBuffer(cmdBuffer))
    {
        postCopyStages = VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        postCopyAccessMask = VK_ACCESS_2_MEMORY_READ_BIT; // Do I need transfer write?
    }

    // Transitioning all MIPs to Transfer Destination layout
    {
        IMAGE_MEMORY_BARRIER(layoutTransition);
        layoutTransition.oldLayout = currentLayout;
        layoutTransition.newLayout = currentLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        layoutTransition.srcQueueFamilyIndex = layoutTransition.dstQueueFamilyIndex = qFamilyIdx;
        layoutTransition.srcAccessMask = postCopyAccessMask;
        layoutTransition.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        layoutTransition.image = dst.reference<VulkanImageResource>()->image;
        layoutTransition.subresourceRange
            = { imageAspect, copyInfo.subres.baseMip, copyInfo.subres.mipCount, copyInfo.subres.baseLayer, copyInfo.subres.layersCount };

        vDevice->vkCmdPipelineBarrier(
            rawCmdBuffer, postCopyStages, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0,
            nullptr, 1, &layoutTransition
        );
    }

    vDevice->vkCmdCopyBufferToImage(
        rawCmdBuffer, pixelData.reference<VulkanBufferResource>()->buffer, dst.reference<VulkanImageResource>()->image, currentLayout,
        uint32(copies.size()), copies.data()
    );

    FenceRef tempFence = graphicsHelperCache->createFence(graphicsInstanceCache, TCHAR("TempCpyImageFence"));
    tempFence->init();
    if (copyInfo.bGenerateMips && copyInfo.subres.mipCount > 1)
    {
        IMAGE_MEMORY_BARRIER(transitionToSrc);
        transitionToSrc.oldLayout = currentLayout;
        transitionToSrc.newLayout = currentLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        transitionToSrc.srcQueueFamilyIndex = transitionToSrc.dstQueueFamilyIndex
            = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Graphics);
        transitionToSrc.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        transitionToSrc.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        transitionToSrc.image = dst.reference<VulkanImageResource>()->image;
        transitionToSrc.subresourceRange = { imageAspect, copyInfo.subres.baseMip, 1, copyInfo.subres.baseLayer, copyInfo.subres.layersCount };

        Size3D srcMipSize = copyInfo.extent;
        Size3D srcMipSizeOffset = copyInfo.dstOffset;
        for (uint32 mipLevel = 1; mipLevel < copyInfo.subres.mipCount; ++mipLevel)
        {
            transitionToSrc.subresourceRange.baseMipLevel = copyInfo.subres.baseMip + mipLevel - 1;
            vDevice->vkCmdPipelineBarrier(
                rawCmdBuffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &transitionToSrc
            );

            Size3D dstMipSize = Math::max(srcMipSize / 2u, Size3D{ 1, 1, 1 });
            Size3D dstMipSizeOffset = srcMipSizeOffset / 2u;
            VkImageBlit blitRegion;
            blitRegion.srcOffsets[0] = { int32(srcMipSizeOffset.x), int32(srcMipSizeOffset.y), int32(srcMipSizeOffset.z) };
            ;
            blitRegion.srcOffsets[1] = { int32(srcMipSize.x), int32(srcMipSize.y), int32(srcMipSize.z) };
            blitRegion.dstOffsets[0] = { int32(dstMipSizeOffset.x), int32(dstMipSizeOffset.y), int32(dstMipSizeOffset.z) };
            blitRegion.dstOffsets[1] = { int32(dstMipSize.x), int32(dstMipSize.y), int32(dstMipSize.z) };
            blitRegion.dstSubresource = blitRegion.srcSubresource
                = { imageAspect, copyInfo.subres.baseMip + mipLevel, copyInfo.subres.baseLayer, copyInfo.subres.layersCount };
            blitRegion.srcSubresource.mipLevel = transitionToSrc.subresourceRange.baseMipLevel;

            vDevice->vkCmdBlitImage(
                rawCmdBuffer, transitionToSrc.image, currentLayout, transitionToSrc.image, transitionToSrc.oldLayout, 1, &blitRegion, filtering
            );

            srcMipSize = dstMipSize;
            srcMipSizeOffset = dstMipSizeOffset;
        }
        // 2 needed as lowest MIP will be in transfer dst layout while others will be in transfer src
        // layout
        std::array<VkImageMemoryBarrier, 2> toFinalLayout;

        // Lowest MIP from dst to post copy
        transitionToSrc.newLayout = postCopyLayout;
        transitionToSrc.dstAccessMask = postCopyAccessMask;
        transitionToSrc.subresourceRange.baseMipLevel = copyInfo.subres.baseMip + copyInfo.subres.mipCount - 1;
        toFinalLayout[0] = transitionToSrc;

        // base MIP to MIP count - 1 from src to post copy
        transitionToSrc.oldLayout = currentLayout;
        transitionToSrc.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        transitionToSrc.subresourceRange.baseMipLevel = copyInfo.subres.baseMip;
        transitionToSrc.subresourceRange.levelCount = copyInfo.subres.mipCount - 1;
        toFinalLayout[1] = transitionToSrc;

        currentLayout = transitionToSrc.newLayout;
        vDevice->vkCmdPipelineBarrier(
            rawCmdBuffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT, postCopyStages, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0,
            nullptr, uint32(toFinalLayout.size()), toFinalLayout.data()
        );
    }
    else
    {
        IMAGE_MEMORY_BARRIER(layoutTransition);
        layoutTransition.oldLayout = currentLayout;
        layoutTransition.newLayout = postCopyLayout;
        layoutTransition.srcQueueFamilyIndex = qFamilyIdx;
        layoutTransition.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        // We choose to not release ownership(which causes need to acquire in dst queue) but just to
        // transfer layout as we wait for this to finish making queue transfer unnecessary
        // layoutTransition.dstQueueFamilyIndex = qFamilyIdx;
        // Above validation error seems to be fixed/not showing so we now transfer resource to
        // graphics queue
        layoutTransition.dstQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Graphics);
        layoutTransition.dstAccessMask = postCopyAccessMask;
        layoutTransition.image = dst.reference<VulkanImageResource>()->image;
        layoutTransition.subresourceRange
            = { imageAspect, copyInfo.subres.baseMip, copyInfo.subres.mipCount, copyInfo.subres.baseLayer, copyInfo.subres.layersCount };

        vDevice->vkCmdPipelineBarrier(
            rawCmdBuffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT, postCopyStages, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0,
            nullptr, 1, &layoutTransition
        );
    }
    cmdBufferManager.endCmdBuffer(cmdBuffer);

    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdBufferManager.submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence);
    tempFence->waitForSignal();

    cmdBufferManager.freeCmdBuffer(cmdBuffer);
    tempFence->release();
}

void VulkanCommandList::copyOrResolveImage(
    ImageResourceRef src, ImageResourceRef dst, const CopyImageInfo &srcInfo, const CopyImageInfo &dstInfo
)
{
    CopyImageInfo srcInfoCpy = srcInfo;
    CopyImageInfo dstInfoCpy = dstInfo;
    // Make sure mips and layers never exceeds above max
    srcInfoCpy.subres.mipCount = Math::min(srcInfoCpy.subres.mipCount, src->getNumOfMips());
    srcInfoCpy.subres.layersCount = Math::min(srcInfoCpy.subres.layersCount, src->getLayerCount());
    dstInfoCpy.subres.mipCount = Math::min(dstInfoCpy.subres.mipCount, dst->getNumOfMips());
    dstInfoCpy.subres.layersCount = Math::min(dstInfoCpy.subres.layersCount, dst->getLayerCount());

    bool bCanSimpleCopy
        = src->getImageSize() == dst->getImageSize() && src->imageFormat() == dst->imageFormat() && srcInfoCpy.isCopyCompatible(dstInfo);
    if (srcInfoCpy.subres.mipCount != dstInfo.subres.mipCount || srcInfoCpy.extent != dstInfo.extent)
    {
        LOG_ERROR("VulkanCommandList", "MIP counts && extent must be same between source and destination regions");
        return;
    }
    {
        SizeBox3D srcBound(srcInfoCpy.offset, Size3D(srcInfoCpy.offset + srcInfoCpy.extent));
        SizeBox3D dstBound(dstInfo.offset, Size3D(dstInfo.offset + dstInfo.extent));
        if (src == dst && srcBound.intersect(dstBound))
        {
            LOG_ERROR("VulkanCommandList", "Cannot copy to same image with intersecting region");
            return;
        }
    }

    VkImageAspectFlags srcImageAspect = determineImageAspect(src);
    VkImageAspectFlags dstImageAspect = determineImageAspect(dst);

    VkAccessFlags srcAccessFlags = determineImageAccessMask(src);
    VkAccessFlags dstAccessFlags = determineImageAccessMask(dst);

    VkImageLayout srcOriginalLayout = getImageLayout(src);
    VkImageLayout dstOriginalLayout = getImageLayout(dst);

    // If copying from to same MIP within same image then subresource layout has to be both src and dst
    VkImageLayout copySrcLayout = src == dst && srcInfoCpy.subres.baseMip == dstInfoCpy.subres.baseMip
                                      ? VkImageLayout::VK_IMAGE_LAYOUT_GENERAL
                                      : VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkImageLayout copyDstLayout = src == dst && srcInfoCpy.subres.baseMip == dstInfoCpy.subres.baseMip
                                      ? VkImageLayout::VK_IMAGE_LAYOUT_GENERAL
                                      : VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    const bool bRequiresGraphicsQ = EPixelDataFormat::isDepthFormat(src->imageFormat()) || EPixelDataFormat::isDepthFormat(dst->imageFormat());
    const GraphicsResource *cmdBuffer = cmdBufferManager.beginTempCmdBuffer(
        (bCanSimpleCopy ? TCHAR("CopyImage_") : TCHAR("ResolveImage_")) + src->getResourceName()
         + TCHAR("_to_") + dst->getResourceName(), bRequiresGraphicsQ ? EQueueFunction::Graphics : EQueueFunction::Transfer);
    uint32 qFamilyIdx = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);

    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    // Transition to transferable layout one for src and dst
    std::vector<VkImageMemoryBarrier2KHR> transitionInfo(2);

    IMAGE_MEMORY_BARRIER2(tempTransition);
    tempTransition.oldLayout = srcOriginalLayout;
    tempTransition.srcAccessMask = srcAccessFlags;
    tempTransition.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Graphics);
    tempTransition.newLayout = copySrcLayout;
    tempTransition.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
    tempTransition.dstQueueFamilyIndex = qFamilyIdx;
    tempTransition.srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    tempTransition.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    tempTransition.subresourceRange
        = { srcImageAspect, srcInfoCpy.subres.baseMip, srcInfoCpy.subres.mipCount, srcInfoCpy.subres.baseLayer, srcInfoCpy.subres.layersCount };
    tempTransition.image = src.reference<VulkanImageResource>()->image;
    transitionInfo[0] = tempTransition;

    tempTransition.oldLayout = dstOriginalLayout;
    tempTransition.srcAccessMask = dstAccessFlags;
    tempTransition.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Graphics);
    tempTransition.newLayout = copyDstLayout;
    tempTransition.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    tempTransition.dstQueueFamilyIndex = qFamilyIdx;
    tempTransition.subresourceRange
        = { dstImageAspect, dstInfoCpy.subres.baseMip, dstInfoCpy.subres.mipCount, dstInfoCpy.subres.baseLayer, dstInfoCpy.subres.layersCount };
    tempTransition.image = dst.reference<VulkanImageResource>()->image;
    transitionInfo[1] = tempTransition;

    cmdPipelineBarrier(vDevice, rawCmdBuffer, transitionInfo, {});

    if (bCanSimpleCopy)
    {
        std::vector<VkImageCopy> imageCopyRegions(srcInfoCpy.subres.mipCount);

        Size3D mipSize = srcInfoCpy.extent;
        Size3D srcMipSizeOffset = srcInfoCpy.offset;
        Size3D dstMipSizeOffset = dstInfoCpy.offset;
        for (uint32 mipLevel = 0; mipLevel < srcInfoCpy.subres.mipCount; ++mipLevel)
        {
            imageCopyRegions[mipLevel].srcOffset = { int32(srcMipSizeOffset.x), int32(srcMipSizeOffset.y), int32(srcMipSizeOffset.z) };
            imageCopyRegions[mipLevel].srcSubresource
                = { srcImageAspect, srcInfoCpy.subres.baseMip + mipLevel, srcInfoCpy.subres.baseLayer, srcInfoCpy.subres.layersCount };
            imageCopyRegions[mipLevel].dstOffset = { int32(dstMipSizeOffset.x), int32(dstMipSizeOffset.y), int32(dstMipSizeOffset.z) };
            imageCopyRegions[mipLevel].dstSubresource
                = { dstImageAspect, dstInfoCpy.subres.baseMip + mipLevel, dstInfoCpy.subres.baseLayer, dstInfoCpy.subres.layersCount };
            imageCopyRegions[mipLevel].extent = { mipSize.x, mipSize.y, mipSize.z };

            srcMipSizeOffset /= 2u;
            dstMipSizeOffset /= 2u;
            mipSize = Math::max(mipSize / 2u, Size3D{ 1, 1, 1 });
        }

        vDevice->vkCmdCopyImage(
            rawCmdBuffer, transitionInfo[0].image, copySrcLayout, transitionInfo[1].image, copyDstLayout, uint32(imageCopyRegions.size()),
            imageCopyRegions.data()
        );
    }
    else
    {
        std::vector<VkImageResolve> imageResolveRegions;
        imageResolveRegions.reserve(srcInfoCpy.subres.mipCount);

        Size3D mipSize = srcInfoCpy.extent;
        Size3D srcMipSizeOffset = srcInfoCpy.offset;
        Size3D dstMipSizeOffset = dstInfoCpy.offset;
        for (uint32 mipLevel = 0; mipLevel < srcInfoCpy.subres.mipCount; ++mipLevel)
        {
            imageResolveRegions[mipLevel].srcOffset = { int32(srcMipSizeOffset.x), int32(srcMipSizeOffset.y), int32(srcMipSizeOffset.z) };
            imageResolveRegions[mipLevel].srcSubresource
                = { srcImageAspect, srcInfoCpy.subres.baseMip + mipLevel, srcInfoCpy.subres.baseLayer, srcInfoCpy.subres.layersCount };
            imageResolveRegions[mipLevel].dstOffset = { int32(dstMipSizeOffset.x), int32(dstMipSizeOffset.y), int32(dstMipSizeOffset.z) };
            imageResolveRegions[mipLevel].dstSubresource
                = { dstImageAspect, dstInfoCpy.subres.baseMip + mipLevel, dstInfoCpy.subres.baseLayer, dstInfoCpy.subres.layersCount };
            imageResolveRegions[mipLevel].extent = { mipSize.x, mipSize.y, mipSize.z };

            srcMipSizeOffset /= 2u;
            dstMipSizeOffset /= 2u;
            mipSize = Math::max(mipSize / 2u, Size3D{ 1, 1, 1 });
        }

        vDevice->vkCmdResolveImage(
            rawCmdBuffer, transitionInfo[0].image, copySrcLayout, transitionInfo[1].image, copyDstLayout, uint32(imageResolveRegions.size()),
            imageResolveRegions.data()
        );
    }

    // Transition back to original
    transitionInfo[0].oldLayout = copySrcLayout;
    transitionInfo[0].srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
    transitionInfo[0].srcQueueFamilyIndex = qFamilyIdx;
    transitionInfo[0].newLayout = srcOriginalLayout;
    transitionInfo[0].dstAccessMask = srcAccessFlags;
    // We choose to not release ownership(which causes need to acquire in dst queue) but just to transfer
    // layout as we wait for this to finish making queue transfer unnecessary
    // transitionInfo[0].dstQueueFamilyIndex = transitionInfo[0].srcQueueFamilyIndex;
    // Above validation error seems to be fixed/not showing so we now transfer resource to graphics queue
    transitionInfo[0].dstQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Graphics);

    transitionInfo[1].oldLayout = copyDstLayout;
    transitionInfo[1].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    transitionInfo[1].srcQueueFamilyIndex = qFamilyIdx;
    transitionInfo[1].newLayout = dstOriginalLayout;
    transitionInfo[1].dstAccessMask = dstAccessFlags;
    // We choose to not release ownership(which causes need to acquire in dst queue) but just to transfer
    // layout as we wait for this to finish making queue transfer unnecessary
    // transitionInfo[1].dstQueueFamilyIndex = transitionInfo[0].srcQueueFamilyIndex;
    // Above validation error seems to be fixed/not showing so we now transfer resource to graphics queue
    transitionInfo[1].dstQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Graphics);

    // Stages
    transitionInfo[0].dstStageMask = transitionInfo[1].dstStageMask = transitionInfo[0].srcStageMask;
    transitionInfo[0].srcStageMask = transitionInfo[1].srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    cmdPipelineBarrier(vDevice, rawCmdBuffer, transitionInfo, {});

    cmdBufferManager.endCmdBuffer(cmdBuffer);
    FenceRef tempFence = graphicsHelperCache->createFence(graphicsInstanceCache, TCHAR("CopyOrResolveImage"));
    tempFence->init();

    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdBufferManager.submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence);

    tempFence->waitForSignal();

    cmdBufferManager.freeCmdBuffer(cmdBuffer);
    tempFence->release();
}

void VulkanCommandList::clearImage(ImageResourceRef image, const LinearColor &clearColor, ArrayView<const ImageSubresource> subresources)
{
    if (EPixelDataFormat::isDepthFormat(image->imageFormat()))
    {
        LOG_ERROR("VulkanCommandList", "Depth image clear cannot be done in color clear");
        return;
    }

    const GraphicsResource *cmdBuffer
        = cmdBufferManager.beginTempCmdBuffer(TCHAR("ClearImage_") + image->getResourceName(), EQueueFunction::Graphics);
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    std::vector<VkImageSubresourceRange> ranges;
    for (const ImageSubresource &subres : subresources)
    {
        VkImageSubresourceRange range;
        range.aspectMask = determineImageAspect(image);
        range.baseMipLevel = subres.baseMip;
        range.levelCount = subres.mipCount;
        range.baseArrayLayer = subres.baseLayer;
        range.layerCount = subres.layersCount;

        ranges.emplace_back(range);
    }

    VkClearColorValue clearVals;
    fillClearValue(image->imageFormat(), clearVals, clearColor);
    vDevice->vkCmdClearColorImage(
        rawCmdBuffer, image.reference<VulkanImageResource>()->image, determineImageLayout(image), &clearVals, uint32(ranges.size()),
        ranges.data()
    );

    cmdBufferManager.endCmdBuffer(cmdBuffer);
    FenceRef tempFence = graphicsHelperCache->createFence(graphicsInstanceCache, TCHAR("ClearImageFence"));
    tempFence->init();

    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdBufferManager.submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence);

    tempFence->waitForSignal();

    cmdBufferManager.freeCmdBuffer(cmdBuffer);
    tempFence->release();
}

void VulkanCommandList::clearDepth(ImageResourceRef image, float depth, uint32 stencil, ArrayView<const ImageSubresource> subresources)
{
    if (!EPixelDataFormat::isDepthFormat(image->imageFormat()))
    {
        LOG_ERROR("VulkanCommandList", "Color image clear cannot be done in depth clear");
        return;
    }

    const GraphicsResource *cmdBuffer
        = cmdBufferManager.beginTempCmdBuffer(TCHAR("ClearDepth_") + image->getResourceName(), EQueueFunction::Graphics);
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    std::vector<VkImageSubresourceRange> ranges;
    for (const ImageSubresource &subres : subresources)
    {
        VkImageSubresourceRange range;
        range.aspectMask = determineImageAspect(image);
        range.baseMipLevel = subres.baseMip;
        range.levelCount = subres.mipCount;
        range.baseArrayLayer = subres.baseLayer;
        range.layerCount = subres.layersCount;

        ranges.emplace_back(range);
    }

    VkClearDepthStencilValue clearVals{ depth, stencil };
    vDevice->vkCmdClearDepthStencilImage(
        rawCmdBuffer, image.reference<VulkanImageResource>()->image, determineImageLayout(image), &clearVals, uint32(ranges.size()),
        ranges.data()
    );

    cmdBufferManager.endCmdBuffer(cmdBuffer);
    FenceRef tempFence = graphicsHelperCache->createFence(graphicsInstanceCache, TCHAR("ClearDepthFence"));
    tempFence->init();

    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdBufferManager.submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence);

    tempFence->waitForSignal();

    cmdBufferManager.freeCmdBuffer(cmdBuffer);
    tempFence->release();
}
