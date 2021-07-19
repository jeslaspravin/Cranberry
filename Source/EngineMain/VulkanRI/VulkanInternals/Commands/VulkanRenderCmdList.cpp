#include "VulkanRenderCmdList.h"
#include "../../../RenderInterface/Resources/GraphicsSyncResource.h"
#include "../../../RenderInterface/PlatformIndependentHeaders.h"
#include "../../../RenderInterface/PlatformIndependentHelper.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "VulkanCommandBufferManager.h"
#include "../ShaderCore/VulkanShaderParamResources.h"
#include "../../../Core/Math/Math.h"
#include "../../../Core/Math/Box.h"

#include <array>
#include <optional>

FORCE_INLINE VkImageAspectFlags VulkanCommandList::determineImageAspect(const ImageResource* image) const
{
    return (EPixelDataFormat::isDepthFormat(image->imageFormat())
        ? (VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT
            | (EPixelDataFormat::isStencilFormat(image->imageFormat()) ? VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT : 0))
        : VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT);
}

FORCE_INLINE VkAccessFlags VulkanCommandList::determineImageAccessMask(const ImageResource* image) const
{
    VkAccessFlags accessMask = 0;

    accessMask |= image->isShaderRead() ? VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT : 0;
    accessMask |= image->isShaderWrite() ? VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT : 0;
    if (image->getType()->isChildOf(GraphicsRenderTargetResource::staticType()))
    {
        accessMask |= VkAccessFlagBits::VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        accessMask |= EPixelDataFormat::isDepthFormat(image->imageFormat()) 
            ? VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    return accessMask;
}

FORCE_INLINE VkImageLayout VulkanCommandList::determineImageLayout(const ImageResource* image) const
{
    VkImageLayout imgLayout = getImageLayout(image);
    if (imgLayout == VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED)
    {
        imgLayout = EPixelDataFormat::isDepthFormat(image->imageFormat())
            ? VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imgLayout = image->getType()->isChildOf(GraphicsRenderTargetResource::staticType())
            ? imgLayout : image->isShaderWrite()
                ? VkImageLayout::VK_IMAGE_LAYOUT_GENERAL : VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    return imgLayout;
}

FORCE_INLINE VkImageLayout VulkanCommandList::getImageLayout(const ImageResource* image) const
{
    // TODO(Jeslas) : change this to get final layout from some resource tracked layout
    VkImageLayout imgLayout = EPixelDataFormat::isDepthFormat(image->imageFormat())
        ? VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    imgLayout = image->getType()->isChildOf(GraphicsRenderTargetResource::staticType())
        ? imgLayout : image->isShaderWrite()
            ? VkImageLayout::VK_IMAGE_LAYOUT_GENERAL : VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return imgLayout;
}

FORCE_INLINE VkPipelineBindPoint VulkanCommandList::getPipelineBindPoint(const PipelineBase* pipeline) const
{
    if (pipeline->getType()->isChildOf<GraphicsPipelineBase>())
    {
        return VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
    }
    else if (pipeline->getType()->isChildOf<ComputePipelineBase>())
    {
        return VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;
    }

    Logger::error("VulkanPipeline", "%s() : Invalid pipeline %s", __func__, pipeline->getResourceName().getChar());
    return VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_MAX_ENUM;
}

FORCE_INLINE void cmdPipelineBarrier(VulkanDevice* vDevice, VkCommandBuffer cmdBuffer, const std::vector<VkImageMemoryBarrier2KHR>& imageBarriers, const std::vector<VkBufferMemoryBarrier2KHR>& bufferBarriers)
{
    // #TODO(Jeslas) : check if this fixes BSOD
    //if (vDevice->vkCmdPipelineBarrier2KHR)
    //{
    //    BARRIER_DEPENDENCY_INFO_KHR(dependencyInfo);
    //    dependencyInfo.dependencyFlags = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT;
    //    dependencyInfo.pImageMemoryBarriers = imageBarriers.data();
    //    dependencyInfo.imageMemoryBarrierCount = uint32(imageBarriers.size());
    //    dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();
    //    dependencyInfo.bufferMemoryBarrierCount = uint32(bufferBarriers.size());
    //    vDevice->vkCmdPipelineBarrier2KHR(cmdBuffer, &dependencyInfo);
    //}
    //else
    {
        struct Barriers
        {
            std::vector<VkImageMemoryBarrier> imgs;
            std::vector<VkBufferMemoryBarrier> buffers;
        };
        std::map<std::pair<VkPipelineStageFlags, VkPipelineStageFlags>, Barriers> stageToBarriers;

        for (const VkImageMemoryBarrier2KHR& imgBarrier2 : imageBarriers)
        {
            Barriers& barrier = stageToBarriers[
            { 
                VkPipelineStageFlags(imgBarrier2.srcStageMask)
                , VkPipelineStageFlags(imgBarrier2.dstStageMask)
            }];

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

        for (const VkBufferMemoryBarrier2KHR& bufBarrier2 : bufferBarriers)
        {
            Barriers& barrier = stageToBarriers[
            {
                VkPipelineStageFlags(bufBarrier2.srcStageMask)
                , VkPipelineStageFlags(bufBarrier2.dstStageMask)
            }];

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

        for (const auto& barriers : stageToBarriers)
        {
            vDevice->vkCmdPipelineBarrier(cmdBuffer, barriers.first.first, barriers.first.second, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT
                , 0, nullptr
                , uint32(barriers.second.buffers.size()), barriers.second.buffers.data()
                , uint32(barriers.second.imgs.size()), barriers.second.imgs.data());
        }
    }
}

VulkanCommandList::VulkanCommandList(IGraphicsInstance* graphicsInstance, VulkanDevice* vulkanDevice)
    : gInstance(graphicsInstance)
    , vDevice(vulkanDevice)
    , cmdBufferManager(vulkanDevice)
{}

void VulkanCommandList::copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo)
{
    SharedPtr<GraphicsFence> tempFence = GraphicsHelper::createFence(gInstance, "CopyBufferTemp", false);

    const GraphicsResource* commandBuffer = cmdBufferManager.beginTempCmdBuffer("Copy buffer", EQueueFunction::Transfer);
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(commandBuffer);

    VkBufferCopy bufferCopyRegion{ copyInfo.srcOffset, copyInfo.dstOffset, copyInfo.copySize };
    vDevice->vkCmdCopyBuffer(rawCmdBuffer, static_cast<VulkanBufferResource*>(src)->buffer
        , static_cast<VulkanBufferResource*>(dst)->buffer, 1, &bufferCopyRegion);

    cmdBufferManager.endCmdBuffer(commandBuffer);

    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.push_back(commandBuffer);
    cmdBufferManager.submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence);

    tempFence->waitForSignal();

    cmdBufferManager.freeCmdBuffer(commandBuffer);
    tempFence->release();
}

void VulkanCommandList::newFrame()
{
    resourcesTracker.clearUnwanted();
}

void VulkanCommandList::copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size)
{
    copyToBuffer_Internal(dst, dstOffset, dataToCopy, size, true);
}

void VulkanCommandList::copyToBuffer(const std::vector<BatchCopyBufferData>& batchCopies)
{
    // For each buffer there will be bunch of copies associated to it
    std::map<VulkanBufferResource*, std::pair<VulkanBufferResource*, std::vector<const BatchCopyBufferData*>>> dstToStagingBufferMap;
    std::vector<GraphicsResource*> flushBuffers;

    // Filling per buffer copy region data and staging data
    for (const BatchCopyBufferData& copyData : batchCopies)
    {
        auto* vulkanDst = static_cast<VulkanBufferResource*>(copyData.dst);
        if (vulkanDst->isStagingResource())
        {
            copyToBuffer_Internal(vulkanDst, copyData.dstOffset, copyData.dataToCopy, copyData.size, false);
            flushBuffers.emplace_back(copyData.dst);
        }
        else
        {
            VulkanBufferResource* stagingBuffer = nullptr;
            auto stagingBufferItr = dstToStagingBufferMap.find(vulkanDst);
            if (stagingBufferItr == dstToStagingBufferMap.end())
            {
                if (vulkanDst->getType()->isChildOf<GraphicsRBuffer>() || vulkanDst->getType()->isChildOf<GraphicsRWBuffer>()
                    || vulkanDst->getType()->isChildOf<GraphicsVertexBuffer>() || vulkanDst->getType()->isChildOf<GraphicsIndexBuffer>())
                {
                    // In case of buffer larger than 4GB using UINT32 will create issue
                    stagingBuffer = new GraphicsRBuffer(uint32(vulkanDst->getResourceSize()));
                }
                else if (vulkanDst->getType()->isChildOf<GraphicsRTexelBuffer>() || vulkanDst->getType()->isChildOf<GraphicsRWTexelBuffer>())
                {
                    // In case of buffer larger than 4GB using UINT32 will create issue
                    stagingBuffer = new GraphicsRTexelBuffer(vulkanDst->texelFormat()
                        , uint32(vulkanDst->getResourceSize() / EPixelDataFormat::getFormatInfo(vulkanDst->texelFormat())->pixelDataSize));
                }
                else
                {
                    Logger::error("VulkanCommandList", "%s() : Copying buffer type is invalid", __func__);
                    continue;
                }
                dstToStagingBufferMap[vulkanDst] = { stagingBuffer, { &copyData } };
                stagingBuffer->setAsStagingResource(true);
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
    GraphicsHelper::flushMappedPtr(gInstance, flushBuffers);
    for (GraphicsResource* buffer : flushBuffers)
    {
        GraphicsHelper::returnMappedPtr(gInstance, buffer);
    }

    // Going to copy from staging to GPU buffers if any such copy exists
    if (dstToStagingBufferMap.empty())
    {
        return;
    }

    // Copying between buffers
    SharedPtr<GraphicsFence> tempFence = GraphicsHelper::createFence(gInstance, "BatchCopyBufferTemp", false);
    const GraphicsResource* commandBuffer = cmdBufferManager.beginTempCmdBuffer("Batch copy buffers", EQueueFunction::Transfer);
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(commandBuffer);

    for (const auto& dstToStagingPair : dstToStagingBufferMap)
    {
        std::vector<VkBufferCopy> copyRegions;
        for (const BatchCopyBufferData* const& copyData : dstToStagingPair.second.second)
        {
            copyRegions.push_back({ copyData->dstOffset, copyData->dstOffset, copyData->size });
        }
        vDevice->vkCmdCopyBuffer(rawCmdBuffer, dstToStagingPair.second.first->buffer
            , dstToStagingPair.first->buffer, uint32(copyRegions.size()), copyRegions.data());
    }

    cmdBufferManager.endCmdBuffer(commandBuffer);
    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.push_back(commandBuffer);
    cmdBufferManager.submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence);
    tempFence->waitForSignal();
    cmdBufferManager.freeCmdBuffer(commandBuffer);
    tempFence->release();

    for (const auto& dstToStagingPair : dstToStagingBufferMap)
    {
        dstToStagingPair.second.first->release();
        delete dstToStagingPair.second.first;
    }
    dstToStagingBufferMap.clear();
}

void VulkanCommandList::copyToBuffer_Internal(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size, bool bFlushMemory /*= false*/)
{
    if (dst->getType()->isChildOf<GraphicsWBuffer>() || dst->getType()->isChildOf<GraphicsWTexelBuffer>())
    {
        Logger::error("VulkanCommandList", "%s() : Copy to buffer(%s) that is write only is not allowed", __func__, dst->getResourceName().getChar());
        return;
    }
    debugAssert((dst->getResourceSize() - dstOffset) >= size);

    if (dst->isStagingResource())
    {
        auto* vulkanDst = static_cast<VulkanBufferResource*>(dst);
        void* stagingPtr = reinterpret_cast<uint8*>(GraphicsHelper::borrowMappedPtr(gInstance, dst)) + dstOffset;
        memcpy(stagingPtr, dataToCopy, size);
        if (bFlushMemory)
        {
            GraphicsHelper::flushMappedPtr(gInstance, { dst });
            GraphicsHelper::returnMappedPtr(gInstance, dst);
        }
    }
    else
    {
        uint64 stagingSize = dst->getResourceSize() - dstOffset;

        CopyBufferInfo copyInfo{ 0, dstOffset, size };

        if (dst->getType()->isChildOf<GraphicsRBuffer>() || dst->getType()->isChildOf<GraphicsRWBuffer>()
            || dst->getType()->isChildOf<GraphicsVertexBuffer>() || dst->getType()->isChildOf<GraphicsIndexBuffer>())
        {
            // In case of buffer larger than 4GB using UINT32 will create issue
            auto stagingBuffer = GraphicsRBuffer(uint32(stagingSize));
            stagingBuffer.setAsStagingResource(true);
            stagingBuffer.init();

            fatalAssert(stagingBuffer.isValid(), "Initializing staging buffer failed");
            copyToBuffer_Internal(&stagingBuffer, 0, dataToCopy, size, true);
            copyBuffer(&stagingBuffer, dst, copyInfo);

            stagingBuffer.release();
        }
        else if (dst->getType()->isChildOf<GraphicsRTexelBuffer>() || dst->getType()->isChildOf<GraphicsRWTexelBuffer>())
        {
            // In case of buffer larger than 4GB using UINT32 will create issue
            auto stagingBuffer = GraphicsRTexelBuffer(dst->texelFormat(), uint32(stagingSize / EPixelDataFormat::getFormatInfo(dst->texelFormat())->pixelDataSize));
            stagingBuffer.setAsStagingResource(true);
            stagingBuffer.init();

            fatalAssert(stagingBuffer.isValid(), "Initializing staging buffer failed");
            copyToBuffer_Internal(&stagingBuffer, 0, dataToCopy, size, true);
            copyBuffer(&stagingBuffer, dst, copyInfo);

            stagingBuffer.release();
        }
        else
        {
            Logger::error("VulkanCommandList", "%s() : Copying buffer type is invalid", __func__);
        }
    }
}

const GraphicsResource* VulkanCommandList::startCmd(const String& uniqueName, EQueueFunction queue, bool bIsReusable)
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

void VulkanCommandList::endCmd(const GraphicsResource* cmdBuffer)
{
    cmdBufferManager.endCmdBuffer(cmdBuffer);
}

void VulkanCommandList::freeCmd(const GraphicsResource* cmdBuffer)
{
    cmdBufferManager.freeCmdBuffer(cmdBuffer);
}

void VulkanCommandList::submitCmd(EQueuePriority::Enum priority
    , const CommandSubmitInfo& submitInfo, const SharedPtr<GraphicsFence>& fence)
{
    cmdBufferManager.submitCmd(priority, submitInfo, fence);
}

void VulkanCommandList::submitWaitCmd(EQueuePriority::Enum priority
    , const CommandSubmitInfo& submitInfo)
{
    SharedPtr<GraphicsFence> fence = GraphicsHelper::createFence(gInstance, "CommandSubmitFence");
    cmdBufferManager.submitCmd(priority, submitInfo, fence);
    fence->waitForSignal();
    for (const GraphicsResource* cmdBuffer : submitInfo.cmdBuffers)
    {
        cmdBufferManager.cmdFinished(cmdBuffer);
    }
    fence->release();
}

void VulkanCommandList::submitCmds(EQueuePriority::Enum priority, const std::vector<CommandSubmitInfo2>& commands)
{
    cmdBufferManager.submitCmds(priority, commands, &resourcesTracker);
}

void VulkanCommandList::submitCmd(EQueuePriority::Enum priority, const CommandSubmitInfo2& command)
{
    cmdBufferManager.submitCmd(priority, command, &resourcesTracker);
}

void VulkanCommandList::finishCmd(const GraphicsResource* cmdBuffer)
{
    cmdBufferManager.cmdFinished(cmdBuffer);
}

void VulkanCommandList::finishCmd(const String& uniqueName)
{
    cmdBufferManager.cmdFinished(uniqueName);
}

const GraphicsResource* VulkanCommandList::getCmdBuffer(const String& uniqueName) const
{
    return cmdBufferManager.getCmdBuffer(uniqueName);
}

void VulkanCommandList::waitIdle()
{
    vDevice->vkDeviceWaitIdle(VulkanGraphicsHelper::getDevice(vDevice));
}

void VulkanCommandList::setupInitialLayout(ImageResource* image)
{
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(image->imageFormat());

    const GraphicsResource* cmdBuffer = cmdBufferManager.beginTempCmdBuffer("LayoutTransition_" + image->getResourceName(), EQueueFunction::Graphics);
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    IMAGE_MEMORY_BARRIER(layoutTransition);
    layoutTransition.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    layoutTransition.newLayout = determineImageLayout(image);
    layoutTransition.srcQueueFamilyIndex = layoutTransition.dstQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);
    layoutTransition.srcAccessMask = layoutTransition.dstAccessMask = determineImageAccessMask(image);
    layoutTransition.image = static_cast<VulkanImageResource*>(image)->image;
    layoutTransition.subresourceRange = { determineImageAspect(image), 0, image->getNumOfMips(), 0, image->getLayerCount() };

    vDevice->vkCmdPipelineBarrier(rawCmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT
        , VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT
        , 0, nullptr, 0, nullptr, 1, &layoutTransition);

    cmdBufferManager.endCmdBuffer(cmdBuffer);

    SharedPtr<GraphicsFence> tempFence = GraphicsHelper::createFence(gInstance, "TempLayoutTransitionFence");
    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdBufferManager.submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence);
    tempFence->waitForSignal();

    cmdBufferManager.freeCmdBuffer(cmdBuffer);
    tempFence->release();
}

void VulkanCommandList::presentImage(const std::vector<class GenericWindowCanvas*>& canvases
    , const std::vector<uint32>& imageIndices, const std::vector<SharedPtr<class GraphicsSemaphore>>& waitOnSemaphores)
{
    std::vector<SharedPtr<GraphicsSemaphore>> waitSemaphores = waitOnSemaphores;
    for (const GraphicsResource* cmdBuffer : swapchainFrameWrites)
    {
        waitSemaphores.emplace_back(cmdBufferManager.cmdSignalSemaphore(cmdBuffer));
    }

    GraphicsHelper::presentImage(gInstance, &canvases, &imageIndices, &waitSemaphores);
    swapchainFrameWrites.clear();
}

void VulkanCommandList::cmdBarrierResources(const GraphicsResource* cmdBuffer, const std::set<const ShaderParameters*>& descriptorsSets)
{
    fatalAssert(!cmdBufferManager.isInRenderPass(cmdBuffer), "%s: %s cmd buffer is inside render pass, it is not supported", __func__, cmdBuffer->getResourceName().getChar());

    std::vector<VkImageMemoryBarrier2KHR> imageBarriers;
    std::vector<VkBufferMemoryBarrier2KHR> bufferBarriers;

    for (const ShaderParameters* descriptorsSet : descriptorsSets)
    {
        // READ only buffers and texels ( might be copied to in transfer queue )
        {
            std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> resources = descriptorsSet->getAllReadOnlyBuffers();
            {
                std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> tempTexels = descriptorsSet->getAllReadOnlyTexels();
                resources.insert(resources.end(), tempTexels.cbegin(), tempTexels.cend());
            }
            for (const auto& resource : resources)
            {
                VkPipelineStageFlags stagesUsed = VkPipelineStageFlags(GraphicsHelper::shaderToPipelineStageFlags(resource.second->bufferEntryPtr->data.stagesUsed));
                std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo
                    = resourcesTracker.readOnlyBuffers(cmdBuffer, { resource.first, stagesUsed });
                if (barrierInfo)
                {
                    BUFFER_MEMORY_BARRIER2_KHR(memBarrier);
                    memBarrier.buffer = static_cast<const VulkanBufferResource*>(resource.first)->buffer;
                    memBarrier.offset = 0;
                    memBarrier.size = resource.first->getResourceSize();

                    memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);
                    memBarrier.dstStageMask = memBarrier.srcStageMask = stagesUsed;
                    // Since shader binding and read only
                    memBarrier.dstAccessMask = memBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_UNIFORM_READ_BIT;

                    if (barrierInfo->accessors.lastWrite)
                    {
                        // If last write, wait for transfer write as read only
                        memBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
                        memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
                        memBarrier.srcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;
                        bufferBarriers.emplace_back(memBarrier);
                    }
                }
            }
        }
        // READ only textures ( might be copied to in transfer queue )
        {
            // #TODO(Jeslas) : Handle attachment images
            std::vector<std::pair<ImageResource*, const ShaderTextureDescriptorType*>> resources = descriptorsSet->getAllReadOnlyTextures();
            for (const auto& resource : resources)
            {
                VkPipelineStageFlags stagesUsed = VkPipelineStageFlags(GraphicsHelper::shaderToPipelineStageFlags(resource.second->textureEntryPtr->data.stagesUsed));
                std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo
                    = resourcesTracker.readOnlyImages(cmdBuffer, { resource.first, stagesUsed });
                if (barrierInfo)
                {
                    IMAGE_MEMORY_BARRIER2_KHR(memBarrier);
                    memBarrier.image = static_cast<const VulkanImageResource*>(resource.first)->image;
                    memBarrier.subresourceRange = { determineImageAspect(resource.first), 0, resource.first->getNumOfMips()
                        , 0, resource.first->getLayerCount() };

                    memBarrier.newLayout = memBarrier.oldLayout = determineImageLayout(resource.first);
                    memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);
                    memBarrier.dstStageMask = memBarrier.srcStageMask = stagesUsed;
                    // Since shader binding and read only
                    memBarrier.dstAccessMask = memBarrier.srcAccessMask = determineImageAccessMask(resource.first);

                    if (barrierInfo->accessors.lastWrite)
                    {
                        // If last write, wait for transfer write as read only
                        memBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                        memBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
                        memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
                        memBarrier.srcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;
                        imageBarriers.emplace_back(memBarrier);
                    }
                }
            }
        }
        // Write able buffers and texels
        {
            std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> resources = descriptorsSet->getAllWriteBuffers();
            {
                std::vector<std::pair<BufferResource*, const ShaderBufferDescriptorType*>> tempTexels = descriptorsSet->getAllWriteTexels();
                resources.insert(resources.end(), tempTexels.cbegin(), tempTexels.cend());
            }
            for (const auto& resource : resources)
            {
                VkPipelineStageFlags stagesUsed = VkPipelineStageFlags(GraphicsHelper::shaderToPipelineStageFlags(resource.second->bufferEntryPtr->data.stagesUsed));
                std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo
                    = resource.second->bIsStorage
                    ? resourcesTracker.writeBuffers(cmdBuffer, { resource.first, stagesUsed })
                    : resourcesTracker.readFromWriteBuffers(cmdBuffer, { resource.first, stagesUsed });
                if (barrierInfo)
                {
                    BUFFER_MEMORY_BARRIER2_KHR(memBarrier);
                    memBarrier.buffer = static_cast<const VulkanBufferResource*>(resource.first)->buffer;
                    memBarrier.offset = 0;
                    memBarrier.size = resource.first->getResourceSize();

                    memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);
                    memBarrier.dstStageMask = memBarrier.srcStageMask = stagesUsed;
                    // Since shader binding and read only
                    memBarrier.dstAccessMask = memBarrier.srcAccessMask = resource.second->bIsStorage
                        ? VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT
                        : VkAccessFlagBits::VK_ACCESS_UNIFORM_READ_BIT;

                    // If there is last write but no read so far then wait for write
                    if (barrierInfo->accessors.lastWrite)
                    {
                        if (cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastWrite))
                        {
                            // If last write, wait for transfer write as read only
                            memBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
                            memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
                            memBarrier.srcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;
                        }
                        // Written in Shader
                        else
                        {
                            memBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
                            memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(barrierInfo->accessors.lastWrite);
                            memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;
                        }
                        bufferBarriers.emplace_back(memBarrier);
                    }
                    // If not written but read last in same command buffer then wait, This will not be empty if writing
                    else if (!barrierInfo->accessors.lastReadsIn.empty())
                    {
                        memBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_UNIFORM_READ_BIT;
                        memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);
                        if (barrierInfo->accessors.allReadStages != 0)
                        {
                            memBarrier.srcStageMask = barrierInfo->accessors.allReadStages;
                        }
                        else
                        {
                            Logger::error("VulkanRenderCmdList", "%s(): Invalid all read pipeline stages %d when expected before writing to buffer"
                                , __func__, barrierInfo->accessors.allReadStages);
                            memBarrier.srcStageMask = cmdBufferManager.isGraphicsCmdBuffer(cmdBuffer)
                                ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                                : VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                        }

                        bufferBarriers.emplace_back(memBarrier);
                    }
                }
            }
        }
        // WRITE textures
        {
            std::vector<std::pair<ImageResource*, const ShaderTextureDescriptorType*>> resources = descriptorsSet->getAllWriteTextures();
            for (const auto& resource : resources)
            {
                // #TODO(Jeslas) : Handle attachment images
                VkPipelineStageFlags stagesUsed = VkPipelineStageFlags(GraphicsHelper::shaderToPipelineStageFlags(resource.second->textureEntryPtr->data.stagesUsed));
                std::optional<VulkanResourcesTracker::ResourceBarrierInfo> barrierInfo
                    = resource.second->imageUsageFlags == EImageShaderUsage::Writing
                    ? resourcesTracker.writeImages(cmdBuffer, { resource.first, stagesUsed })
                    : resourcesTracker.readFromWriteImages(cmdBuffer, { resource.first, stagesUsed });
                if (barrierInfo)
                {
                    IMAGE_MEMORY_BARRIER2_KHR(memBarrier);
                    memBarrier.image = static_cast<const VulkanImageResource*>(resource.first)->image;
                    memBarrier.subresourceRange = { determineImageAspect(resource.first), 0, resource.first->getNumOfMips()
                        , 0, resource.first->getLayerCount() };

                    memBarrier.dstQueueFamilyIndex = memBarrier.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);
                    memBarrier.dstStageMask = memBarrier.srcStageMask = stagesUsed;

                    memBarrier.newLayout = memBarrier.oldLayout = determineImageLayout(resource.first);
                    memBarrier.dstAccessMask = memBarrier.srcAccessMask = resource.second->imageUsageFlags == EImageShaderUsage::Writing
                        ? VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT : VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;

                    // If there is last write but no read so far then wait for write within same cmd buffer then just barrier no layout switch
                    if (barrierInfo->accessors.lastWrite)
                    {
                        // We are not writing
                        if (resource.second->imageUsageFlags != EImageShaderUsage::Writing)
                        {
                            memBarrier.srcStageMask = barrierInfo->accessors.lastWriteStage;
                            if (cmdBufferManager.isTransferCmdBuffer(barrierInfo->accessors.lastWrite))
                            {
                                memBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
                                memBarrier.srcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;
                            }
                            else
                            {
                                memBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
                            }
                        }
                        imageBarriers.emplace_back(memBarrier);
                    }
                    // At this point there is no read or write in this resource so if read write resource and we are in incorrect layout then change it
                    else if (barrierInfo->accessors.lastReadsIn.empty())
                    {
                        memBarrier.oldLayout = determineImageLayout(resource.first);
                        memBarrier.srcAccessMask = determineImageAccessMask(resource.first);
                        // We Will not be in incorrect layout in write image
                        // imageBarriers.emplace_back(memBarrier);
                    }
                    // If not written but read last in same command buffer then wait
                    else if (barrierInfo->accessors.lastReadsIn[0] == cmdBuffer)
                    {
                        memBarrier.oldLayout = determineImageLayout(resource.first);
                        memBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;

                        if (barrierInfo->accessors.allReadStages != 0)
                        {
                            memBarrier.srcStageMask = barrierInfo->accessors.allReadStages;
                        }
                        else
                        {
                            Logger::error("VulkanRenderCmdList", "%s(): Invalid all read pipeline stages %d when expected before writing to buffer"
                                , __func__, barrierInfo->accessors.allReadStages);
                            memBarrier.srcStageMask = cmdBufferManager.isGraphicsCmdBuffer(cmdBuffer)
                                ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                                : VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                        }
                        imageBarriers.emplace_back(memBarrier);
                    }
                    // Read after write in some other cmd buffer
                    else
                    {
                        memBarrier.oldLayout = determineImageLayout(resource.first);
                        memBarrier.srcAccessMask = 0;
                        memBarrier.srcStageMask = barrierInfo->accessors.allReadStages;
                        for (const GraphicsResource* readInCmd : barrierInfo->accessors.lastReadsIn)
                        {
                            if (cmdBufferManager.isTransferCmdBuffer(readInCmd))
                            {
                                memBarrier.srcAccessMask |= VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
                                memBarrier.srcStageMask |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;
                            }
                            else
                            {
                                memBarrier.srcAccessMask |= VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
                            }
                        }
                        imageBarriers.emplace_back(memBarrier);
                    }
                }
            }
        }
    }

    cmdPipelineBarrier(vDevice, cmdBufferManager.getRawBuffer(cmdBuffer), imageBarriers, bufferBarriers);
}

void VulkanCommandList::cmdBeginRenderPass(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const QuantizedBox2D& renderArea, const RenderPassAdditionalProps& renderpassAdditionalProps, const RenderPassClearValue& clearColor)
{
    if (!renderArea.isValidAABB())
    {
        Logger::error("VulkanCommandList", "%s() : Incorrect render area", __func__);
        debugAssert(false);
        return;
    }
    if (cmdBuffer == nullptr || contextPipeline.getPipeline() == nullptr || contextPipeline.getFb() == nullptr)
    {
        debugAssert(false);
        return;
    }
    VulkanGlobalRenderingContext* renderingContext = static_cast<VulkanGlobalRenderingContext*>(gEngine->getRenderApi()->getGlobalRenderingContext());
    const VulkanGraphicsPipeline* graphicsPipeline = static_cast<const VulkanGraphicsPipeline*>(contextPipeline.getPipeline());

    Size2D extent = renderArea.size();
    std::vector<VkClearValue> clearValues;

    VkClearColorValue lastClearColor;
    lastClearColor.float32[0] = LinearColorConst::BLACK.r();
    lastClearColor.float32[1] = LinearColorConst::BLACK.g();
    lastClearColor.float32[2] = LinearColorConst::BLACK.b();
    lastClearColor.float32[3] = LinearColorConst::BLACK.a();
    if (contextPipeline.bUseSwapchainFb)
    {
        for (const LinearColor& clearCol : clearColor.colors)
        {
            lastClearColor.float32[0] = clearCol.r();
            lastClearColor.float32[1] = clearCol.g();
            lastClearColor.float32[2] = clearCol.b();
            lastClearColor.float32[3] = clearCol.a();

            VkClearValue clearVal;
            clearVal.color = lastClearColor;
            clearValues.emplace_back(clearVal);
        }
        swapchainFrameWrites.emplace_back(cmdBuffer);
    }
    else
    {
        uint32 colorIdx = 0;
        for (const ImageResource* frameTexture : contextPipeline.getFb()->textures)
        {
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
                    lastClearColor.float32[0] = clearColor.colors[colorIdx].r();
                    lastClearColor.float32[1] = clearColor.colors[colorIdx].g();
                    lastClearColor.float32[2] = clearColor.colors[colorIdx].b();
                    lastClearColor.float32[3] = clearColor.colors[colorIdx].a();
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
        { renderArea.minBound.x, renderArea.minBound.y },// offset
        { extent.x, extent.y }
    };

    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    vDevice->vkCmdBeginRenderPass(rawCmdBuffer, &beginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
    cmdBufferManager.startRenderPass(cmdBuffer);
}

void VulkanCommandList::cmdEndRenderPass(const GraphicsResource* cmdBuffer)
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    vDevice->vkCmdEndRenderPass(rawCmdBuffer);
    cmdBufferManager.endRenderPass(cmdBuffer);
}

void VulkanCommandList::cmdBindComputePipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    const VulkanComputePipeline* computePipeline = static_cast<const VulkanComputePipeline*>(contextPipeline.getPipeline());

    vDevice->vkCmdBindPipeline(rawCmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline->getPipeline());
}

void VulkanCommandList::cmdBindGraphicsPipeline(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline, const GraphicsPipelineState& state) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    const VulkanGraphicsPipeline* graphicsPipeline = static_cast<const VulkanGraphicsPipeline*>(contextPipeline.getPipeline());
    VkPipeline pipeline = graphicsPipeline->getPipeline(state.pipelineQuery);

    if (pipeline == VK_NULL_HANDLE)
    {
        Logger::error("VulkanCommandList", "%s() : Pipeline is invalid", __func__);
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
    for(const std::pair<EStencilFaceMode, uint32>& stencilRef : state.stencilReferences)
    {
        vDevice->vkCmdSetStencilReference(rawCmdBuffer, VkStencilFaceFlagBits(stencilRef.first), stencilRef.second);
    }
}

void VulkanCommandList::cmdPushConstants(const GraphicsResource* cmdBuffer, const LocalPipelineContext& contextPipeline
    , uint32 stagesUsed, const uint8* data, const std::vector<CopyBufferInfo>& pushConsts) const
{
    VkPipelineLayout pipelineLayout = nullptr;
    if (contextPipeline.getPipeline()->getType()->isChildOf<GraphicsPipelineBase>())
    {
        pipelineLayout = static_cast<const VulkanGraphicsPipeline*>(contextPipeline.getPipeline())->pipelineLayout;
    }
    else if (contextPipeline.getPipeline()->getType()->isChildOf<ComputePipelineBase>())
    {
        pipelineLayout = static_cast<const VulkanComputePipeline*>(contextPipeline.getPipeline())->pipelineLayout;
    }
    else
    {
        Logger::error("VulkanPipeline", "%s() : Invalid pipeline %s", __func__, contextPipeline.getPipeline()->getResourceName().getChar());
        debugAssert(false);
        return;
    }
    for (const CopyBufferInfo& copyInfo : pushConsts)
    {
        vDevice->vkCmdPushConstants(cmdBufferManager.getRawBuffer(cmdBuffer), pipelineLayout, stagesUsed
            , uint32(copyInfo.dstOffset), copyInfo.copySize, data + copyInfo.srcOffset);
    }
}

void VulkanCommandList::cmdBindDescriptorsSetInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::map<uint32, const ShaderParameters*>& descriptorsSets) const
{
    std::map<uint32, std::vector<VkDescriptorSet>> descsSets;

    for (const std::pair<const uint32, const ShaderParameters*>& descsSet : descriptorsSets)
    {
        // If first element or next expected sequential set ID is not equal to current ID
        if (descsSets.empty() || descsSet.first != (--descsSets.end())->first + (--descsSets.end())->second.size())
        {
            descsSets[descsSet.first].emplace_back(static_cast<const VulkanShaderSetParameters*>(descsSet.second)->descriptorsSet);
        }
        else
        {
            (--descsSets.end())->second.emplace_back(static_cast<const VulkanShaderSetParameters*>(descsSet.second)->descriptorsSet);
        }
    }

    VkPipelineBindPoint pipelineBindPt = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_MAX_ENUM;
    VkPipelineLayout pipelineLayout = nullptr;
    if (contextPipeline->getType()->isChildOf<GraphicsPipelineBase>())
    {
        pipelineBindPt = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
        pipelineLayout = static_cast<const VulkanGraphicsPipeline*>(contextPipeline)->pipelineLayout;
    }
    else if (contextPipeline->getType()->isChildOf<ComputePipelineBase>())
    {
        pipelineBindPt = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;
        pipelineLayout = static_cast<const VulkanComputePipeline*>(contextPipeline)->pipelineLayout;
    }
    else
    {
        Logger::error("VulkanPipeline", "%s() : Invalid pipeline %s", __func__, contextPipeline->getResourceName().getChar());
        debugAssert(false);
        return;
    }
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    for (const std::pair<const uint32, std::vector<VkDescriptorSet>>& descsSet : descsSets)
    {
        vDevice->vkCmdBindDescriptorSets(rawCmdBuffer, pipelineBindPt, pipelineLayout, descsSet.first, uint32(descsSet.second.size()), descsSet.second.data(), 0, nullptr);
    }
}

void VulkanCommandList::cmdBindDescriptorsSetsInternal(const GraphicsResource* cmdBuffer, const PipelineBase* contextPipeline, const std::vector<const ShaderParameters*>& descriptorsSets) const
{
    std::map<uint32, std::vector<VkDescriptorSet>> descsSets;
    {
        std::map<uint32, VkDescriptorSet> tempDescsSets;
        for (const ShaderParameters* shaderParams : descriptorsSets)
        {
            const VulkanShaderParameters* vulkanShaderParams = static_cast<const VulkanShaderParameters*>(shaderParams);
            tempDescsSets.insert(vulkanShaderParams->descriptorsSets.cbegin(), vulkanShaderParams->descriptorsSets.cend());
        }

        for (const std::pair<const uint32, VkDescriptorSet>& descsSet : tempDescsSets)
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
        pipelineLayout = static_cast<const VulkanGraphicsPipeline*>(contextPipeline)->pipelineLayout;
    }
    else if (contextPipeline->getType()->isChildOf<ComputePipelineBase>())
    {
        pipelineBindPt = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;
        pipelineLayout = static_cast<const VulkanComputePipeline*>(contextPipeline)->pipelineLayout;
    }
    else
    {
        Logger::error("VulkanPipeline", "%s() : Invalid pipeline %s", __func__, contextPipeline->getResourceName().getChar());
        debugAssert(false);
        return;
    }
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    for (const std::pair<const uint32, std::vector<VkDescriptorSet>>& descsSet : descsSets)
    {
        vDevice->vkCmdBindDescriptorSets(rawCmdBuffer, pipelineBindPt, pipelineLayout, descsSet.first, uint32(descsSet.second.size()), descsSet.second.data(), 0, nullptr);
    }
}

void VulkanCommandList::cmdBindVertexBuffers(const GraphicsResource* cmdBuffer, uint32 firstBinding, const std::vector<const BufferResource*>& vertexBuffers, const std::vector<uint64>& offsets) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    fatalAssert(vertexBuffers.size() == offsets.size(), "Offsets must be equivalent to vertex buffers");
    std::vector<VkBuffer> vertBuffers(vertexBuffers.size());
    for (int32 i = 0; i < vertexBuffers.size(); ++i)
    {
        vertBuffers[i] = static_cast<const VulkanBufferResource*>(vertexBuffers[i])->buffer;
    }

    vDevice->vkCmdBindVertexBuffers(rawCmdBuffer, firstBinding, uint32(vertexBuffers.size()), vertBuffers.data(), offsets.data());
}

void VulkanCommandList::cmdBindIndexBuffer(const GraphicsResource* cmdBuffer, const BufferResource* indexBuffer, uint64 offset /*= 0*/) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    vDevice->vkCmdBindIndexBuffer(rawCmdBuffer, static_cast<const VulkanBufferResource*>(indexBuffer)->buffer, offset, VkIndexType::VK_INDEX_TYPE_UINT32);
}

void VulkanCommandList::cmdDispatch(const GraphicsResource* cmdBuffer, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ /*= 1*/) const
{
    vDevice->vkCmdDispatch(cmdBufferManager.getRawBuffer(cmdBuffer), groupSizeX, groupSizeY, groupSizeZ);
}

void VulkanCommandList::cmdDrawIndexed(const GraphicsResource* cmdBuffer, uint32 firstIndex, uint32 indexCount, uint32 firstInstance /*= 0*/, uint32 instanceCount /*= 1*/, int32 vertexOffset /*= 0*/) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    vDevice->vkCmdDrawIndexed(rawCmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandList::cmdDrawVertices(const GraphicsResource* cmdBuffer, uint32 firstVertex, uint32 vertexCount, uint32 firstInstance /*= 0*/, uint32 instanceCount /*= 1*/) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);
    vDevice->vkCmdDraw(rawCmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandList::cmdSetViewportAndScissors(const GraphicsResource* cmdBuffer, const std::vector<std::pair<QuantizedBox2D, QuantizedBox2D>>& viewportAndScissors, uint32 firstViewport /*= 0*/) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    std::vector<VkViewport> viewports;
    viewports.reserve(viewportAndScissors.size());
    std::vector<VkRect2D> scissors;
    scissors.reserve(viewportAndScissors.size());
    for (std::pair<QuantizedBox2D, QuantizedBox2D> viewportAndScis : viewportAndScissors)
    {
        Int2D viewportSize = viewportAndScis.first.size();
        viewports.emplace_back(VkViewport{ float(viewportAndScis.first.minBound.x), float(viewportAndScis.first.minBound.y )
            , float(viewportSize.x), float(viewportSize.y), 0.f/* Min depth */, 1.f/* Max depth */ });

        viewportAndScis.second.fixAABB();
        Size2D scissorSize = viewportAndScis.second.size();
        scissors.emplace_back(VkRect2D{ { viewportAndScis.second.minBound.x, viewportAndScis.second.minBound.y }, { scissorSize.x, scissorSize.y} });
    }

    vDevice->vkCmdSetViewport(rawCmdBuffer, firstViewport, uint32(viewports.size()), viewports.data());
    vDevice->vkCmdSetScissor(rawCmdBuffer, firstViewport, uint32(scissors.size()), scissors.data());
}

void VulkanCommandList::cmdSetViewportAndScissor(const GraphicsResource* cmdBuffer, const QuantizedBox2D& viewport, const QuantizedBox2D& scissor, uint32 atViewport /*= 0*/) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    Int2D viewportSize = viewport.size();
    VkViewport vulkanViewport{ float(viewport.minBound.x), float(viewport.minBound.y)
            , float(viewportSize.x), float(viewportSize.y), 0.f/* Min depth */, 1.f/* Max depth */ };
    vDevice->vkCmdSetViewport(rawCmdBuffer, atViewport, 1, &vulkanViewport);

    if (scissor.isValidAABB())
    {
        Size2D scissorSize = scissor.size();
        VkRect2D vulkanScissor{ { scissor.minBound.x, scissor.minBound.y }, { scissorSize.x, scissorSize.y} };
        vDevice->vkCmdSetScissor(rawCmdBuffer, atViewport, 1, &vulkanScissor);
    }
    else
    {
        QuantizedBox2D tempScissor(scissor);
        tempScissor.fixAABB();

        Size2D scissorSize = tempScissor.size();
        VkRect2D vulkanScissor{ { tempScissor.minBound.x, tempScissor.minBound.y }, { scissorSize.x, scissorSize.y} };
        vDevice->vkCmdSetScissor(rawCmdBuffer, atViewport, 1, &vulkanScissor);
    }
}

void VulkanCommandList::cmdBeginBufferMarker(const GraphicsResource* commandBuffer, const String& name, const LinearColor& color /*= LinearColorConst::WHITE*/) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(commandBuffer);
    VulkanGraphicsHelper::debugGraphics(gInstance)->beginCmdBufferMarker(rawCmdBuffer, name, color);
}

void VulkanCommandList::cmdInsertBufferMarker(const GraphicsResource* commandBuffer, const String& name, const LinearColor& color /*= LinearColorConst::WHITE*/) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(commandBuffer);
    VulkanGraphicsHelper::debugGraphics(gInstance)->insertCmdBufferMarker(rawCmdBuffer, name, color);
}

void VulkanCommandList::cmdEndBufferMarker(const GraphicsResource* commandBuffer) const
{
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(commandBuffer);
    VulkanGraphicsHelper::debugGraphics(gInstance)->endCmdBufferMarker(rawCmdBuffer);
}

void VulkanCommandList::copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyPixelsToImageInfo& copyInfo)
{
    fatalAssert(dst->isValid(), "Invalid image resource %s", dst->getResourceName().getChar());
    if (EPixelDataFormat::isDepthFormat(dst->imageFormat()) || EPixelDataFormat::isFloatingFormat(dst->imageFormat()))
    {
        Logger::error("VulkanCommandList", "%s() : Depth/Float format is not supported for copying from Color data", __func__);
        return;
    }
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dst->imageFormat());

    // Add 32 bit extra space to staging to compensate 32 mask out of range when copying data
    uint32 dataMargin = uint32(Math::ceil(float(sizeof(uint32)) / formatInfo->pixelDataSize));
    GraphicsRBuffer stagingBuffer = GraphicsRBuffer(formatInfo->pixelDataSize, uint32(pixelData.size()) + dataMargin);
    stagingBuffer.setAsStagingResource(true);
    stagingBuffer.init();

    uint8* stagingPtr = reinterpret_cast<uint8*>(GraphicsHelper::borrowMappedPtr(gInstance, &stagingBuffer));
    copyPixelsTo(&stagingBuffer, stagingPtr, pixelData, formatInfo);
    GraphicsHelper::returnMappedPtr(gInstance, &stagingBuffer);

    copyToImage_Internal(dst, &stagingBuffer, copyInfo);
    stagingBuffer.release();
}

void VulkanCommandList::copyToImage(ImageResource* dst, const std::vector<class LinearColor>& pixelData, const CopyPixelsToImageInfo& copyInfo)
{
    fatalAssert(dst->isValid(), "Invalid image resource %s", dst->getResourceName().getChar());
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dst->imageFormat());
    if (EPixelDataFormat::isDepthFormat(dst->imageFormat())
        && (formatInfo->componentSize[0] != sizeof(32) || EPixelDataFormat::isStencilFormat(dst->imageFormat())))
    {
        Logger::error("VulkanCommandList", "%s() : Depth/Float format with size other than 32bit is not supported for copying from Color data", __func__);
        return;
    }

    // Add 32 bit extra space to staging to compensate 32 mask out of range when copying data
    uint32 dataMargin = uint32(Math::ceil(float(sizeof(uint32)) / formatInfo->pixelDataSize));
    GraphicsRBuffer stagingBuffer = GraphicsRBuffer(formatInfo->pixelDataSize, uint32(pixelData.size()) + dataMargin);
    stagingBuffer.setAsStagingResource(true);
    stagingBuffer.init();

    uint8* stagingPtr = reinterpret_cast<uint8*>(GraphicsHelper::borrowMappedPtr(gInstance, &stagingBuffer));
    copyPixelsTo(&stagingBuffer, stagingPtr, pixelData, formatInfo
        , EPixelDataFormat::isDepthFormat(dst->imageFormat()) || EPixelDataFormat::isFloatingFormat(dst->imageFormat()));
    GraphicsHelper::returnMappedPtr(gInstance, &stagingBuffer);

    copyToImage_Internal(dst, &stagingBuffer, copyInfo);
    stagingBuffer.release();
}

void VulkanCommandList::copyToImageLinearMapped(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyPixelsToImageInfo& copyInfo)
{
    fatalAssert(dst->isValid(), "Invalid image resource %s", dst->getResourceName().getChar());
    if (EPixelDataFormat::isDepthFormat(dst->imageFormat()) || EPixelDataFormat::isFloatingFormat(dst->imageFormat()))
    {
        Logger::error("VulkanCommandList", "%s() : Depth/Float format is not supported for copying from Color data", __func__);
        return;
    }

    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dst->imageFormat());

    // Add 32 bit extra space to staging to compensate 32 mask out of range when copying data
    uint32 dataMargin = uint32(Math::ceil(float(sizeof(uint32)) / formatInfo->pixelDataSize));
    GraphicsRBuffer stagingBuffer = GraphicsRBuffer(formatInfo->pixelDataSize, uint32(pixelData.size()) + dataMargin);
    stagingBuffer.setAsStagingResource(true);
    stagingBuffer.init();

    uint8* stagingPtr = reinterpret_cast<uint8*>(GraphicsHelper::borrowMappedPtr(gInstance, &stagingBuffer));
    copyPixelsLinearMappedTo(&stagingBuffer, stagingPtr, pixelData, formatInfo);
    GraphicsHelper::returnMappedPtr(gInstance, &stagingBuffer);

    copyToImage_Internal(dst, &stagingBuffer, copyInfo);
    stagingBuffer.release();
}

void VulkanCommandList::copyToImage_Internal(ImageResource* dst, const BufferResource* pixelData, const CopyPixelsToImageInfo& copyInfo)
{
    const VkFilter filtering = VkFilter(ESamplerFiltering::getFilterInfo(GraphicsHelper::getClampedFiltering(gInstance
        , copyInfo.mipFiltering, dst->imageFormat()))->filterTypeValue);

    const VkImageAspectFlagBits imageAspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

    // Layout that is acceptable for this image
    VkImageLayout postCopyLayout = determineImageLayout(dst);
    VkAccessFlags postCopyAccessMask = determineImageAccessMask(dst);

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
            vkCopyInfo.imageSubresource = { imageAspect, copyInfo.subres.baseMip + mipLevel, copyInfo.subres.baseLayer, copyInfo.subres.layersCount };

            copies.emplace_back(vkCopyInfo);

            mipLinearOffset += mipSize.x * mipSize.y * mipSize.z * copyInfo.subres.layersCount;
            mipSize = Math::max(mipSize / 2u, Size3D{ 1, 1, 1 });
            mipSizeOffset /= 2u;
        }
    }

    const GraphicsResource* cmdBuffer = cmdBufferManager.beginTempCmdBuffer("CopyPixelToImage_" + dst->getResourceName()
        , copyInfo.bGenerateMips ? EQueueFunction::Graphics : EQueueFunction::Transfer);
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    // Transitioning all MIPs to Transfer Destination layout
    {
        IMAGE_MEMORY_BARRIER(layoutTransition);
        layoutTransition.oldLayout = currentLayout;
        layoutTransition.newLayout = currentLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        layoutTransition.srcQueueFamilyIndex = layoutTransition.dstQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);
        layoutTransition.srcAccessMask = postCopyAccessMask;
        layoutTransition.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        layoutTransition.image = static_cast<VulkanImageResource*>(dst)->image;
        layoutTransition.subresourceRange = { imageAspect, copyInfo.subres.baseMip, copyInfo.subres.mipCount, copyInfo.subres.baseLayer, copyInfo.subres.layersCount };

        vDevice->vkCmdPipelineBarrier(rawCmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT
            , VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT
            , 0, nullptr, 0, nullptr, 1, &layoutTransition);
    }

    vDevice->vkCmdCopyBufferToImage(rawCmdBuffer, static_cast<const VulkanBufferResource*>(pixelData)->buffer
        , static_cast<VulkanImageResource*>(dst)->image, currentLayout, uint32(copies.size()), copies.data());

    if (copyInfo.bGenerateMips && copyInfo.subres.mipCount > 1)
    {
        IMAGE_MEMORY_BARRIER(transitionToSrc);
        transitionToSrc.oldLayout = currentLayout;
        transitionToSrc.newLayout = currentLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        transitionToSrc.srcQueueFamilyIndex = transitionToSrc.dstQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Graphics);
        transitionToSrc.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        transitionToSrc.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
        transitionToSrc.image = static_cast<VulkanImageResource*>(dst)->image;
        transitionToSrc.subresourceRange = { imageAspect, copyInfo.subres.baseMip, 1,  copyInfo.subres.baseLayer, copyInfo.subres.layersCount };

        Size3D srcMipSize = copyInfo.extent;
        Size3D srcMipSizeOffset = copyInfo.dstOffset;
        for (uint32 mipLevel = 1; mipLevel < copyInfo.subres.mipCount; ++mipLevel)
        {
            transitionToSrc.subresourceRange.baseMipLevel = copyInfo.subres.baseMip + mipLevel - 1;
            vDevice->vkCmdPipelineBarrier(rawCmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT
                , VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT
                , 0, nullptr, 0, nullptr, 1, &transitionToSrc);

            Size3D dstMipSize = Math::max(srcMipSize / 2u, Size3D{ 1, 1, 1 });
            Size3D dstMipSizeOffset = srcMipSizeOffset / 2u;
            VkImageBlit blitRegion;
            blitRegion.srcOffsets[0] = { int32(srcMipSizeOffset.x), int32(srcMipSizeOffset.y), int32(srcMipSizeOffset.z) };;
            blitRegion.srcOffsets[1] = { int32(srcMipSize.x), int32(srcMipSize.y), int32(srcMipSize.z) };
            blitRegion.dstOffsets[0] = { int32(dstMipSizeOffset.x), int32(dstMipSizeOffset.y), int32(dstMipSizeOffset.z) };
            blitRegion.dstOffsets[1] = { int32(dstMipSize.x), int32(dstMipSize.y), int32(dstMipSize.z) };
            blitRegion.dstSubresource = blitRegion.srcSubresource = { imageAspect, copyInfo.subres.baseMip + mipLevel, copyInfo.subres.baseLayer, copyInfo.subres.layersCount };
            blitRegion.srcSubresource.mipLevel = transitionToSrc.subresourceRange.baseMipLevel;

            vDevice->vkCmdBlitImage(rawCmdBuffer, transitionToSrc.image, currentLayout, transitionToSrc.image, transitionToSrc.oldLayout
                , 1, &blitRegion, filtering);

            srcMipSize = dstMipSize;
            srcMipSizeOffset = dstMipSizeOffset;
        }
        // 2 needed as lowest MIP will be in transfer dst layout while others will be in transfer src layout
        std::array<VkImageMemoryBarrier, 2> toFinalLayout;

        // Lowest MIP from dst to post copy
        transitionToSrc.newLayout = postCopyLayout;
        transitionToSrc.dstAccessMask = postCopyAccessMask;
        transitionToSrc.subresourceRange.baseMipLevel = copyInfo.subres.baseMip + copyInfo.subres.mipCount - 1;
        toFinalLayout[0] = transitionToSrc;

        // base MIP to MIP count - 1 from src to post copy
        transitionToSrc.oldLayout = currentLayout;
        transitionToSrc.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
        transitionToSrc.subresourceRange.baseMipLevel = copyInfo.subres.baseMip;
        transitionToSrc.subresourceRange.levelCount = copyInfo.subres.mipCount - 1;
        toFinalLayout[1] = transitionToSrc;

        currentLayout = transitionToSrc.newLayout;
        vDevice->vkCmdPipelineBarrier(rawCmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT
            , VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT
            , 0, nullptr, 0, nullptr, uint32(toFinalLayout.size()), toFinalLayout.data());
    }
    else
    {
        IMAGE_MEMORY_BARRIER(layoutTransition);
        layoutTransition.oldLayout = currentLayout;
        layoutTransition.newLayout = postCopyLayout;
        layoutTransition.srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(cmdBuffer);
        layoutTransition.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        layoutTransition.dstQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Graphics);
        layoutTransition.dstAccessMask = postCopyAccessMask;
        layoutTransition.image = static_cast<VulkanImageResource*>(dst)->image;
        layoutTransition.subresourceRange = { imageAspect, copyInfo.subres.baseMip, copyInfo.subres.mipCount, copyInfo.subres.baseLayer, copyInfo.subres.layersCount };

        vDevice->vkCmdPipelineBarrier(rawCmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT
            , VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT
            , 0, nullptr, 0, nullptr, 1, &layoutTransition);
    }

    SharedPtr<GraphicsFence> tempFence = GraphicsHelper::createFence(gInstance, "TempCpyImageFence");
    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdBufferManager.endCmdBuffer(cmdBuffer);
    cmdBufferManager.submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence);
    tempFence->waitForSignal();

    cmdBufferManager.freeCmdBuffer(cmdBuffer);
    tempFence->release();
}

void VulkanCommandList::copyOrResolveImage(ImageResource* src, ImageResource* dst, const CopyImageInfo& srcInfo, const CopyImageInfo& dstInfo)
{
    bool bCanSimpleCopy = src->getImageSize() == dst->getImageSize() && src->imageFormat() == dst->imageFormat()
        && srcInfo.isCopyCompatible(dstInfo);
    if (srcInfo.subres.mipCount != dstInfo.subres.mipCount || srcInfo.extent != dstInfo.extent)
    {
        Logger::error("VulkanCommandList", "%s : MIP counts && extent must be same between source and destination regions", __func__);
        return;
    }
    {
        SizeBox3D srcBound(srcInfo.offset, Size3D(srcInfo.offset + srcInfo.extent));
        SizeBox3D dstBound(dstInfo.offset, Size3D(dstInfo.offset + dstInfo.extent));
        if (src == dst && srcBound.intersect(dstBound))
        {
            Logger::error("VulkanCommandList", "%s : Cannot copy to same image with intersecting region", __func__);
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
    VkImageLayout copySrcLayout = src == dst && srcInfo.subres.baseMip == dstInfo.subres.baseMip
        ? VkImageLayout::VK_IMAGE_LAYOUT_GENERAL : VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkImageLayout copyDstLayout = src == dst && srcInfo.subres.baseMip == dstInfo.subres.baseMip
        ? VkImageLayout::VK_IMAGE_LAYOUT_GENERAL : VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;


    const GraphicsResource* cmdBuffer = cmdBufferManager.beginTempCmdBuffer((bCanSimpleCopy ? "CopyImage_" : "ResolveImage_")
        + src->getResourceName() + "_to_" + dst->getResourceName(), EQueueFunction::Transfer);
    VkCommandBuffer rawCmdBuffer = cmdBufferManager.getRawBuffer(cmdBuffer);

    // Transition to transferable layout one for src and dst
    std::array<VkImageMemoryBarrier, 2> transitionInfo;
    transitionInfo[0].oldLayout = srcOriginalLayout;
    transitionInfo[0].srcAccessMask = srcAccessFlags;
    transitionInfo[0].srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Graphics);
    transitionInfo[0].newLayout = copySrcLayout;
    transitionInfo[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
    transitionInfo[0].dstQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Transfer);
    transitionInfo[0].subresourceRange = { srcImageAspect, srcInfo.subres.baseMip, srcInfo.subres.mipCount
        , srcInfo.subres.baseLayer, srcInfo.subres.layersCount };
    transitionInfo[0].image = static_cast<VulkanImageResource*>(src)->image;

    transitionInfo[1].oldLayout = dstOriginalLayout;
    transitionInfo[1].srcAccessMask = dstAccessFlags;
    transitionInfo[1].srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Graphics);
    transitionInfo[1].newLayout = copyDstLayout;
    transitionInfo[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
    transitionInfo[1].dstQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Transfer);
    transitionInfo[1].subresourceRange = { dstImageAspect, dstInfo.subres.baseMip, dstInfo.subres.mipCount
        , dstInfo.subres.baseLayer, dstInfo.subres.layersCount };
    transitionInfo[1].image = static_cast<VulkanImageResource*>(dst)->image;

    vDevice->vkCmdPipelineBarrier(rawCmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT
        , VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT
        , 0, nullptr, 0, nullptr, uint32(transitionInfo.size()), transitionInfo.data());

    if (bCanSimpleCopy)
    {
        std::vector<VkImageCopy> imageCopyRegions(srcInfo.subres.mipCount);

        Size3D mipSize = srcInfo.extent;
        Size3D srcMipSizeOffset = srcInfo.offset;
        Size3D dstMipSizeOffset = dstInfo.offset;
        for (uint32 mipLevel = 0; mipLevel < srcInfo.subres.mipCount; ++mipLevel)
        {
            imageCopyRegions[mipLevel].srcOffset = { int32(srcMipSizeOffset.x),int32(srcMipSizeOffset.y),int32(srcMipSizeOffset.z) };
            imageCopyRegions[mipLevel].srcSubresource = { srcImageAspect, srcInfo.subres.baseMip + mipLevel, srcInfo.subres.baseLayer, srcInfo.subres.layersCount };
            imageCopyRegions[mipLevel].dstOffset = { int32(dstMipSizeOffset.x),int32(dstMipSizeOffset.y),int32(dstMipSizeOffset.z) };
            imageCopyRegions[mipLevel].dstSubresource = { dstImageAspect, dstInfo.subres.baseMip + mipLevel, dstInfo.subres.baseLayer, dstInfo.subres.layersCount };
            imageCopyRegions[mipLevel].extent = { mipSize.x, mipSize.y, mipSize.z };

            srcMipSizeOffset /= 2u;
            dstMipSizeOffset /= 2u;
            mipSize = Math::max(mipSize / 2u, Size3D{ 1,1,1 });
        }

        vDevice->vkCmdCopyImage(rawCmdBuffer, transitionInfo[0].image, copySrcLayout, transitionInfo[1].image, copyDstLayout
            , uint32(imageCopyRegions.size()), imageCopyRegions.data());
    }
    else
    {
        std::vector<VkImageResolve> imageResolveRegions;
        imageResolveRegions.reserve(srcInfo.subres.mipCount);

        Size3D mipSize = srcInfo.extent;
        Size3D srcMipSizeOffset = srcInfo.offset;
        Size3D dstMipSizeOffset = dstInfo.offset;
        for (uint32 mipLevel = 0; mipLevel < srcInfo.subres.mipCount; ++mipLevel)
        {
            imageResolveRegions[mipLevel].srcOffset = { int32(srcMipSizeOffset.x),int32(srcMipSizeOffset.y),int32(srcMipSizeOffset.z) };
            imageResolveRegions[mipLevel].srcSubresource = { srcImageAspect, srcInfo.subres.baseMip + mipLevel, srcInfo.subres.baseLayer, srcInfo.subres.layersCount };
            imageResolveRegions[mipLevel].dstOffset = { int32(dstMipSizeOffset.x),int32(dstMipSizeOffset.y),int32(dstMipSizeOffset.z) };
            imageResolveRegions[mipLevel].dstSubresource = { dstImageAspect, dstInfo.subres.baseMip + mipLevel, dstInfo.subres.baseLayer, dstInfo.subres.layersCount };
            imageResolveRegions[mipLevel].extent = { mipSize.x, mipSize.y, mipSize.z };

            srcMipSizeOffset /= 2u;
            dstMipSizeOffset /= 2u;
            mipSize = Math::max(mipSize / 2u, Size3D{ 1,1,1 });
        }

        vDevice->vkCmdResolveImage(rawCmdBuffer, transitionInfo[0].image, copySrcLayout, transitionInfo[1].image, copyDstLayout
            , uint32(imageResolveRegions.size()), imageResolveRegions.data());
    }

    // Transition back to original
    transitionInfo[0].oldLayout = copySrcLayout;
    transitionInfo[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
    transitionInfo[0].srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Transfer);
    transitionInfo[0].newLayout = srcOriginalLayout;
    transitionInfo[0].dstAccessMask = srcAccessFlags;
    transitionInfo[0].dstQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Graphics);

    transitionInfo[1].oldLayout = copyDstLayout;
    transitionInfo[1].srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
    transitionInfo[1].srcQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Transfer);
    transitionInfo[1].newLayout = dstOriginalLayout;
    transitionInfo[1].dstAccessMask = dstAccessFlags;
    transitionInfo[1].dstQueueFamilyIndex = cmdBufferManager.getQueueFamilyIdx(EQueueFunction::Graphics);

    vDevice->vkCmdPipelineBarrier(rawCmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT
        , VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT
        , 0, nullptr, 0, nullptr, uint32(transitionInfo.size()), transitionInfo.data());

    cmdBufferManager.endCmdBuffer(cmdBuffer);
    SharedPtr<GraphicsFence> tempFence = GraphicsHelper::createFence(gInstance, "CopyOrResolveImage");
    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdBufferManager.submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence);

    tempFence->waitForSignal();

    cmdBufferManager.freeCmdBuffer(cmdBuffer);
    tempFence->release();
}
