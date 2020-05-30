#include "VulkanRenderCmdList.h"
#include "../../../RenderInterface/Resources/GraphicsSyncResource.h"
#include "../../../RenderInterface/PlatformIndependentHeaders.h"
#include "../../../RenderInterface/PlatformIndependentHelper.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "VulkanCommandBufferManager.h"

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

void VulkanCommandList::copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData)
{
    if (pixelData.size() < (dst->getImageSize().z * dst->getImageSize().y * dst->getImageSize().x) * dst->getLayerCount())
    {
        Logger::error("VulkanCommandList", "%s() : Texel data count is not sufficient to fill all texels of %s",__func__
            , dst->getResourceName().getChar());
        return;
    }
    CopyImageInfo copyInfo;
    copyInfo.dstOffset = { 0,0,0 };
    copyInfo.extent = dst->getImageSize();
    copyInfo.layerBase = 0;
    copyInfo.layerCount = dst->getLayerCount();
    copyInfo.mipBase = 0;
    copyInfo.mipCount = 1;
    copyInfo.bGenerateMips = false;
    copyToImage(dst, pixelData, copyInfo);
}

void VulkanCommandList::copyToImage(ImageResource* dst, const std::vector<class Color>& pixelData, const CopyImageInfo& copyInfo)
{
    // TODO(ASAP) 
}

void VulkanCommandList::copyOrResolveImage(ImageResource* src, ImageResource* dst, const CopyImageInfo& copyInfo)
{
    // TODO(ASAP) 
}
