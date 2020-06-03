#include "VulkanRenderCmdList.h"
#include "../../../RenderInterface/Resources/GraphicsSyncResource.h"
#include "../../../RenderInterface/PlatformIndependentHeaders.h"
#include "../../../RenderInterface/PlatformIndependentHelper.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "VulkanCommandBufferManager.h"
#include "../../../Core/Math/Math.h"

#include <array>

VulkanCommandList::VulkanCommandList(IGraphicsInstance* graphicsInstance, VulkanDevice* vulkanDevice)
    : gInstance(graphicsInstance)
    , vDevice(vulkanDevice)
{
    cmdBufferManager = new VulkanCmdBufferManager(vDevice);
}

VulkanCommandList::~VulkanCommandList()
{
    delete cmdBufferManager;
}

void VulkanCommandList::copyBuffer(BufferResource* src, BufferResource* dst, const CopyBufferInfo& copyInfo)
{
    SharedPtr<GraphicsFence> tempFence = GraphicsHelper::createFence(gInstance, "CopyBufferTemp", false);

    const GraphicsResource* commandBuffer = cmdBufferManager->beginTempCmdBuffer("Copy buffer", EQueueFunction::Transfer);
    VkCommandBuffer rawCmdBuffer = cmdBufferManager->getRawBuffer(commandBuffer);

    VkBufferCopy bufferCopyRegion{ copyInfo.srcOffset, copyInfo.dstOffset, copyInfo.copySize };
    vDevice->vkCmdCopyBuffer(rawCmdBuffer, static_cast<VulkanBufferResource*>(src)->buffer
        , static_cast<VulkanBufferResource*>(dst)->buffer, 1, &bufferCopyRegion);

    cmdBufferManager->endCmdBuffer(commandBuffer);

    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.push_back(commandBuffer);
    cmdBufferManager->submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence.get());

    tempFence->waitForSignal();

    cmdBufferManager->freeCmdBuffer(commandBuffer);
    tempFence->release();
}

void VulkanCommandList::copyToBuffer(BufferResource* dst, uint32 dstOffset, const void* dataToCopy, uint32 size)
{
    if (dst->getType()->isChildOf<GraphicsWBuffer>() || dst->getType()->isChildOf<GraphicsWTexelBuffer>())
    {
        Logger::error("VulkanCommandList", "%s() : Cannot copy to buffer(%s) that is write only", __func__, dst->getResourceName().getChar());
        return;
    }
    debugAssert((dst->getResourceSize() - dstOffset) >= size);

    if (dst->isStagingResource())
    {
        auto* vulkanDst = static_cast<VulkanBufferResource*>(dst);
        void* stagingPtr = reinterpret_cast<uint8*>(GraphicsHelper::borrowMappedPtr(gInstance, dst)) + dstOffset;
        memcpy(stagingPtr, dataToCopy, size);
        GraphicsHelper::returnMappedPtr(gInstance, dst);
    }
    else
    {
        uint64 stagingSize = dst->getResourceSize() - dstOffset;

        CopyBufferInfo copyInfo{ 0, dstOffset, size};

        if (dst->getType()->isChildOf<GraphicsRBuffer>() || dst->getType()->isChildOf<GraphicsRWBuffer>()
            || dst->getType()->isChildOf<GraphicsVertexBuffer>() || dst->getType()->isChildOf<GraphicsIndexBuffer>())
        {
            // In case of buffer larger than 4GB using UINT32 will create issue
            auto stagingBuffer = GraphicsRBuffer(uint32(stagingSize));
            stagingBuffer.setAsStagingResource(true);
            stagingBuffer.init();

            fatalAssert(stagingBuffer.isValid(), "Initializing staging buffer failed");
            copyToBuffer(&stagingBuffer, 0, dataToCopy, size);
            copyBuffer(&stagingBuffer, dst, copyInfo);

            stagingBuffer.release();
        }
        else if(dst->getType()->isChildOf<GraphicsRTexelBuffer>() || dst->getType()->isChildOf<GraphicsRWTexelBuffer>())
        {
            // In case of buffer larger than 4GB using UINT32 will create issue
            auto stagingBuffer = GraphicsRTexelBuffer(dst->texelFormat(), uint32(stagingSize / EPixelDataFormat::getFormatInfo(dst->texelFormat())->pixelDataSize));
            stagingBuffer.setAsStagingResource(true);
            stagingBuffer.init();

            fatalAssert(stagingBuffer.isValid(), "Initializing staging buffer failed");
            copyToBuffer(&stagingBuffer, 0, dataToCopy, size);
            copyBuffer(&stagingBuffer, dst, copyInfo);

            stagingBuffer.release();
        }
        else
        {
            Logger::error("VulkanCommandList", "%s() : Copying buffer type is invalid", __func__);
        }
    }
}

void VulkanCommandList::copyToBuffer(const std::vector<BatchCopyBufferData>& batchCopies)
{
    // For each buffer there will be bunch of copies associated to it
    std::map<VulkanBufferResource*, std::pair<VulkanBufferResource*, std::vector<const BatchCopyBufferData*>>> dstToStagingBufferMap;
    
    // Filling per buffer copy region data and staging data
    for (const BatchCopyBufferData& copyData : batchCopies)
    {
        auto* vulkanDst = static_cast<VulkanBufferResource*>(copyData.dst);
        if (vulkanDst->isStagingResource())
        {
            copyToBuffer(vulkanDst, copyData.dstOffset, copyData.dataToCopy, copyData.size);
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
            }
            else
            {
                stagingBuffer = stagingBufferItr->second.first;
                stagingBufferItr->second.second.push_back(&copyData);
            }
            copyToBuffer(stagingBuffer, copyData.dstOffset, copyData.dataToCopy, copyData.size);
        }
    }

    // Copying between buffers
    SharedPtr<GraphicsFence> tempFence = GraphicsHelper::createFence(gInstance, "BatchCopyBufferTemp", false);
    const GraphicsResource* commandBuffer = cmdBufferManager->beginTempCmdBuffer("Batch copy buffers", EQueueFunction::Transfer);
    VkCommandBuffer rawCmdBuffer = cmdBufferManager->getRawBuffer(commandBuffer);

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

    cmdBufferManager->endCmdBuffer(commandBuffer);
    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.push_back(commandBuffer);
    cmdBufferManager->submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence.get());
    tempFence->waitForSignal();
    cmdBufferManager->freeCmdBuffer(commandBuffer);
    tempFence->release();

    for (const auto& dstToStagingPair : dstToStagingBufferMap)
    {
        dstToStagingPair.second.first->release();
        delete dstToStagingPair.second.first;
    }
    dstToStagingBufferMap.clear();
}

const GraphicsResource* VulkanCommandList::startCmd(String uniqueName, EQueueFunction queue, bool bIsReusable)
{
    if (bIsReusable)
    {
        return cmdBufferManager->beginReuseCmdBuffer(uniqueName, queue);
    }
    else
    {
        return cmdBufferManager->beginRecordOnceCmdBuffer(uniqueName, queue);
    }
}

void VulkanCommandList::endCmd(const GraphicsResource* cmdBuffer)
{
    cmdBufferManager->endCmdBuffer(cmdBuffer);
}

void VulkanCommandList::freeCmd(const GraphicsResource* cmdBuffer)
{
    cmdBufferManager->freeCmdBuffer(cmdBuffer);
}

void VulkanCommandList::submitCmd(EQueuePriority::Enum priority
    , const CommandSubmitInfo& submitInfo, const SharedPtr<GraphicsFence>& fence)
{
    cmdBufferManager->submitCmd(priority, submitInfo, (bool(fence)? fence.get() : nullptr));
}

void VulkanCommandList::submitWaitCmd(EQueuePriority::Enum priority
    , const CommandSubmitInfo& submitInfo)
{
    SharedPtr<GraphicsFence> fence = GraphicsHelper::createFence(gInstance, "CommandSubmitFence");
    cmdBufferManager->submitCmd(priority, submitInfo, fence.get());
    fence->waitForSignal();
    fence->release();
}

void VulkanCommandList::waitIdle()
{
    vDevice->vkDeviceWaitIdle(VulkanGraphicsHelper::getDevice(vDevice));
}

void VulkanCommandList::copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyImageInfo& copyInfo)
{
    fatalAssert(dst->isValid(), "Invalid image resource");
    if (EPixelDataFormat::isDepthFormat(dst->imageFormat()) || EPixelDataFormat::isFloatingFormat(dst->imageFormat()))
    {
        Logger::error("VulkanCommandList", "%s() : Depth/Float format is not supported for copying from Color data", __func__);
        return;
    }
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dst->imageFormat());
    const VkFilter filtering = VkFilter(ESamplerFiltering::getFilterInfo(GraphicsHelper::getClampedFiltering(gInstance
        , copyInfo.mipFiltering, dst->imageFormat()))->filterTypeValue);

    const VkImageAspectFlagBits imageAspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;

    bool bIsRenderTarget = dst->getType()->isChildOf(GraphicsRenderTargetResource::staticType());
    // Layout that is acceptable for this image
    VkImageLayout postCopyLayout = bIsRenderTarget ? VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : dst->isShaderWrite()
        ? VkImageLayout::VK_IMAGE_LAYOUT_GENERAL : VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkAccessFlagBits postCopyAccessMask = bIsRenderTarget ? VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : dst->isShaderWrite()
        ? VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT : VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;

    // TODO(Jeslas) : change this to get final layout from some resource tracked layout
    VkImageLayout currentLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

    GraphicsRTexelBuffer stagingBuffer = GraphicsRTexelBuffer(dst->imageFormat(), uint32(pixelData.size()));
    stagingBuffer.setAsStagingResource(true);
    stagingBuffer.init();
    uint8* stagingPtr = reinterpret_cast<uint8*>(GraphicsHelper::borrowMappedPtr(gInstance, &stagingBuffer));
    copyPixelsTo(&stagingBuffer, stagingPtr, pixelData, formatInfo);

    std::vector<VkBufferImageCopy> copies;
    if (copyInfo.bGenerateMips)
    {
        copies.resize(1);
        copies[0].imageExtent = { dst->getImageSize().x, dst->getImageSize().y, dst->getImageSize().z };
        copies[0].imageOffset = { 0,0,0 };
        copies[0].bufferOffset = copies[0].bufferRowLength = copies[0].bufferImageHeight = 0;
        copies[0].imageSubresource = { imageAspect, copyInfo.mipBase, copyInfo.layerBase, copyInfo.layerCount };
    }
    else
    {
        uint32 mipOffset = 0;
        Size3D mipSize = Math::max(dst->getImageSize() / uint32(Math::pow(2u,copyInfo.mipBase)),Size3D(1,1,1));

        for (uint32 mipLevel = 0; mipLevel < copyInfo.mipCount; ++mipLevel)
        {
            VkBufferImageCopy vkCopyInfo;
            vkCopyInfo.imageExtent = { mipSize.x, mipSize.y, mipSize.z };
            vkCopyInfo.imageOffset = { 0,0,0 };
            vkCopyInfo.bufferOffset = mipOffset;
            vkCopyInfo.bufferRowLength = mipSize.x;
            vkCopyInfo.bufferImageHeight = mipSize.y;
            vkCopyInfo.imageSubresource = { imageAspect, copyInfo.mipBase + mipLevel, copyInfo.layerBase, copyInfo.layerCount };

            copies.emplace_back(vkCopyInfo);

            mipOffset += mipSize.x * mipSize.y * mipSize.z * copyInfo.layerCount;
            mipSize = Math::max(mipSize / 2u, Size3D(1, 1, 1));
        }
    }

    const GraphicsResource* cmdBuffer = cmdBufferManager->beginTempCmdBuffer("CopyPixelToImage_" + dst->getResourceName()
        , copyInfo.bGenerateMips ? EQueueFunction::Graphics : EQueueFunction::Transfer);
    VkCommandBuffer rawCmdBuffer = cmdBufferManager->getRawBuffer(cmdBuffer);
    
    // Transitioning all mips to Transfer Destination layout
    {
        IMAGE_MEMORY_BARRIER(layoutTransition);
        layoutTransition.oldLayout = currentLayout;
        layoutTransition.newLayout = currentLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        layoutTransition.srcQueueFamilyIndex = layoutTransition.dstQueueFamilyIndex = cmdBufferManager->getQueueFamilyIdx(cmdBuffer);
        layoutTransition.srcAccessMask = postCopyAccessMask;
        layoutTransition.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        layoutTransition.image = static_cast<VulkanImageResource*>(dst)->image;
        layoutTransition.subresourceRange = { imageAspect, copyInfo.mipBase, copyInfo.mipCount, copyInfo.layerBase, copyInfo.layerCount };

        vDevice->vkCmdPipelineBarrier(rawCmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT
            , VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT
            , 0, nullptr, 0, nullptr, 1, &layoutTransition);
    }

    vDevice->vkCmdCopyBufferToImage(rawCmdBuffer, static_cast<VulkanBufferResource*>(&stagingBuffer)->buffer
        , static_cast<VulkanImageResource*>(dst)->image, currentLayout, uint32(copies.size()), copies.data());

    if (copyInfo.bGenerateMips && copyInfo.mipCount > 1)
    {
        IMAGE_MEMORY_BARRIER(transitionToSrc);
        transitionToSrc.oldLayout = currentLayout;
        transitionToSrc.newLayout = currentLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        transitionToSrc.srcQueueFamilyIndex = transitionToSrc.dstQueueFamilyIndex = cmdBufferManager->getQueueFamilyIdx(EQueueFunction::Graphics);
        transitionToSrc.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        transitionToSrc.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
        transitionToSrc.image = static_cast<VulkanImageResource*>(dst)->image;
        transitionToSrc.subresourceRange = { imageAspect, copyInfo.mipBase, 1, copyInfo.layerBase, copyInfo.layerCount };

        Size3D srcMipSize = Math::max(dst->getImageSize() / uint32(Math::pow(2u, copyInfo.mipBase)), Size3D(1, 1, 1));
        for (uint32 mipLevel = 1; mipLevel < copyInfo.mipCount; ++mipLevel)
        {
            transitionToSrc.subresourceRange.baseMipLevel = copyInfo.mipBase + mipLevel - 1;
            vDevice->vkCmdPipelineBarrier(rawCmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT
                , VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT
                , 0, nullptr, 0, nullptr, 1, &transitionToSrc);

            Size3D dstMipSize = Math::max(srcMipSize / 2u, Size3D(1, 1, 1));
            VkImageBlit blitRegion;
            blitRegion.dstOffsets[0] = blitRegion.srcOffsets[0] = { 0,0,0 };
            blitRegion.srcOffsets[1] = { int32(srcMipSize.x), int32(srcMipSize.y), int32(srcMipSize.z) };
            blitRegion.dstOffsets[1] = { int32(dstMipSize.x), int32(dstMipSize.y), int32(dstMipSize.z) };
            blitRegion.dstSubresource = blitRegion.srcSubresource = { imageAspect, copyInfo.mipBase + mipLevel, copyInfo.layerBase, copyInfo.layerCount };
            blitRegion.srcSubresource.mipLevel = transitionToSrc.subresourceRange.baseMipLevel;

            vDevice->vkCmdBlitImage(rawCmdBuffer, transitionToSrc.image, currentLayout, transitionToSrc.image, transitionToSrc.oldLayout
                , 1, &blitRegion, filtering);

            srcMipSize = dstMipSize;
        }
        // 2 needed as lowest mip will be in transfer dst layout while others will be in transfer src layout
        std::array<VkImageMemoryBarrier, 2> toFinalLayout;

        // Lowest mip from dst to post copy
        transitionToSrc.newLayout = postCopyLayout;
        transitionToSrc.dstAccessMask = postCopyAccessMask;
        transitionToSrc.subresourceRange.baseMipLevel = copyInfo.mipBase + copyInfo.mipCount - 1;
        toFinalLayout[0] = transitionToSrc;

        // base mip to mip count - 1 from src to post copy
        transitionToSrc.oldLayout = currentLayout;
        transitionToSrc.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
        transitionToSrc.subresourceRange.baseMipLevel = copyInfo.mipBase;
        transitionToSrc.subresourceRange.levelCount = copyInfo.mipCount - 1;
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
        layoutTransition.srcQueueFamilyIndex = cmdBufferManager->getQueueFamilyIdx(cmdBuffer);
        layoutTransition.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        layoutTransition.dstQueueFamilyIndex = cmdBufferManager->getQueueFamilyIdx(EQueueFunction::Graphics);
        layoutTransition.dstAccessMask = postCopyAccessMask;
        layoutTransition.image = static_cast<VulkanImageResource*>(dst)->image;
        layoutTransition.subresourceRange = { imageAspect, copyInfo.mipBase, copyInfo.mipCount, copyInfo.layerBase, copyInfo.layerCount };

        vDevice->vkCmdPipelineBarrier(rawCmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT
            , VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT
            , 0, nullptr, 0, nullptr, 1, &layoutTransition);
    }

    SharedPtr<GraphicsFence> tempFence = GraphicsHelper::createFence(gInstance, "TempCpyImageFence");
    CommandSubmitInfo submitInfo;
    submitInfo.cmdBuffers.emplace_back(cmdBuffer);
    cmdBufferManager->endCmdBuffer(cmdBuffer);
    cmdBufferManager->submitCmd(EQueuePriority::SuperHigh, submitInfo, tempFence.get());
    tempFence->waitForSignal();

    cmdBufferManager->freeCmdBuffer(cmdBuffer);
    stagingBuffer.release();
    tempFence->release();
}

void VulkanCommandList::copyOrResolveImage(ImageResource* src, ImageResource* dst, const CopyImageInfo& copyInfo)
{
    // TODO(ASAP) 
}
