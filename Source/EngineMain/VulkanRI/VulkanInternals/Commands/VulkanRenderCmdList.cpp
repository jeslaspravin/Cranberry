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
    VkBufferCopy bufferCopyRegion{ copyInfo.srcOffset, copyInfo.dstOffset, copyInfo.copySize };
    SharedPtr<GraphicsFence> tempFence = GraphicsHelper::createFence(gInstance, "CopyBufferTemp", false);

    const GraphicsResource* commandBuffer = cmdBufferManager->beginTempCmdBuffer("Copy buffer", EQueueFunction::Transfer);
    VkCommandBuffer rawCmdBuffer = cmdBufferManager->getRawBuffer(commandBuffer);

    vDevice->vkCmdCopyBuffer(rawCmdBuffer, static_cast<VulkanBufferResource*>(src)->buffer
        , static_cast<VulkanBufferResource*>(dst)->buffer, 1, &bufferCopyRegion);
    cmdBufferManager->endCmdBuffer(commandBuffer);

    VulkanSubmitInfo submitInfo;
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
        void* stagingPtr = GraphicsHelper::borrowMappedPtr(gInstance, dst);
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
            auto stagingBuffer = GraphicsRTexelBuffer(dst->texelFormat(), uint32(stagingSize / EPixelDataFormat::getFormatInfo(dst->texelFormat())->pixelDataSize));
            stagingBuffer.setAsStagingResource(true);
            stagingBuffer.init();

            fatalAssert(stagingBuffer.isValid(), "Initializing staging buffer failed");
            copyToBuffer(&stagingBuffer, 0, dataToCopy, size);
            copyBuffer(&stagingBuffer, dst, copyInfo);

            stagingBuffer.release();
        }
    }
}
