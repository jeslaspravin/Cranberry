#include "ExperimentalEngine.h"
#include "../RenderInterface/Resources/GenericWindowCanvas.h"
#include "../RenderInterface/PlatformIndependentHelper.h"
#include "../Core/Engine/WindowManager.h"
#include "../Core/Platform/GenericAppWindow.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanQueueResource.h"
#include "../VulkanRI/VulkanInternals/VulkanDevice.h"
#include "../VulkanRI/VulkanInternals/Debugging.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanWindowCanvas.h"

template <EQueueFunction QueueFunction>
VulkanQueueResource<QueueFunction>* getQueue(const std::vector<QueueResourceBase*>& allQueues, const VulkanDevice* device);

void ExperimentalEngine::createPools()
{
    {
        VulkanQueueResource<EQueueFunction::Compute>* queue = getQueue<EQueueFunction::Compute>(*deviceQueues, vDevice);
        if (queue != nullptr)
        {
            QueueCommandPool& pool = pools[EQueueFunction::Compute];
            CREATE_COMMAND_POOL_INFO(commandPoolCreateInfo);
            commandPoolCreateInfo.queueFamilyIndex = queue->queueFamilyIndex();

            commandPoolCreateInfo.flags = 0;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.oneTimeRecordPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.tempCommandsPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.resetableCommandPool);

            graphicsDbg->markObject((uint64)pool.oneTimeRecordPool, "Compute_OneTimeRecordPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.tempCommandsPool, "Compute_TempCmdsPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.resetableCommandPool, "Compute_ResetableCmdPool", VK_OBJECT_TYPE_COMMAND_POOL);
        }
    }
    {
        VulkanQueueResource<EQueueFunction::Graphics>* queue = getQueue<EQueueFunction::Graphics>(*deviceQueues, vDevice);
        if (queue != nullptr)
        {
            QueueCommandPool& pool = pools[EQueueFunction::Graphics];
            CREATE_COMMAND_POOL_INFO(commandPoolCreateInfo);
            commandPoolCreateInfo.queueFamilyIndex = queue->queueFamilyIndex();

            commandPoolCreateInfo.flags = 0;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.oneTimeRecordPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.tempCommandsPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.resetableCommandPool);

            graphicsDbg->markObject((uint64)pool.oneTimeRecordPool, "Graphics_OneTimeRecordPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.tempCommandsPool, "Graphics_TempCmdsPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.resetableCommandPool, "Graphics_ResetableCmdPool", VK_OBJECT_TYPE_COMMAND_POOL);
        }
    }
    {
        VulkanQueueResource<EQueueFunction::Transfer>* queue = getQueue<EQueueFunction::Transfer>(*deviceQueues, vDevice);
        if (queue != nullptr)
        {
            QueueCommandPool& pool = pools[EQueueFunction::Transfer];
            CREATE_COMMAND_POOL_INFO(commandPoolCreateInfo);
            commandPoolCreateInfo.queueFamilyIndex = queue->queueFamilyIndex();

            commandPoolCreateInfo.flags = 0;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.oneTimeRecordPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.tempCommandsPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.resetableCommandPool);

            graphicsDbg->markObject((uint64)pool.oneTimeRecordPool, "Transfer_OneTimeRecordPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.tempCommandsPool, "Transfer_TempCmdsPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.resetableCommandPool, "Transfer_ResetableCmdPool", VK_OBJECT_TYPE_COMMAND_POOL);
        }
    }
    {
        VulkanQueueResource<EQueueFunction::Present>* queue = getQueue<EQueueFunction::Present>(*deviceQueues, vDevice);
        if (queue != nullptr)
        {
            QueueCommandPool& pool = pools[EQueueFunction::Present];
            CREATE_COMMAND_POOL_INFO(commandPoolCreateInfo);
            commandPoolCreateInfo.queueFamilyIndex = queue->queueFamilyIndex();

            commandPoolCreateInfo.flags = 0;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.oneTimeRecordPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.tempCommandsPool);

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            vDevice->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool.resetableCommandPool);

            graphicsDbg->markObject((uint64)pool.oneTimeRecordPool, "Present_OneTimeRecordPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.tempCommandsPool, "Present_TempCmdsPool", VK_OBJECT_TYPE_COMMAND_POOL);
            graphicsDbg->markObject((uint64)pool.resetableCommandPool, "Present_ResetableCmdPool", VK_OBJECT_TYPE_COMMAND_POOL);
        }
    }
}

void ExperimentalEngine::destroyPools()
{
    for (auto& pool : pools)
    {
        vDevice->vkDestroyCommandPool(device, pool.second.oneTimeRecordPool, nullptr);
        vDevice->vkDestroyCommandPool(device, pool.second.resetableCommandPool, nullptr);
        vDevice->vkDestroyCommandPool(device, pool.second.tempCommandsPool, nullptr);
    }
}

void ExperimentalEngine::onStartUp()
{
    GameEngine::onStartUp();
    vDevice = VulkanGraphicsHelper::getVulkanDevice(getRenderApi()->getGraphicsInstance());
    device = VulkanGraphicsHelper::getDevice(vDevice);
    deviceQueues = VulkanGraphicsHelper::getVDAllQueues(vDevice);
    graphicsDbg = VulkanGraphicsHelper::debugGraphics(getRenderApi()->getGraphicsInstance());
    createPools();
    
    String fenceName = "SwapchainImageLayout";
    vFence = GraphicsHelper::createFence(getRenderApi()->getGraphicsInstance(), fenceName.getChar());

    CMD_BUFFER_ALLOC_INFO(cmdAllocInfo);
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.commandPool = pools[EQueueFunction::Present].resetableCommandPool;
    vDevice->vkAllocateCommandBuffers(device, &cmdAllocInfo, &swapchainCmdBuffer);
}

void ExperimentalEngine::onQuit()
{
    vDevice->vkFreeCommandBuffers(device, pools[EQueueFunction::Present].resetableCommandPool, 1, &swapchainCmdBuffer);
    vFence->release();
    vFence.reset();

    destroyPools();
    GameEngine::onQuit();
}

void ExperimentalEngine::tickEngine()
{
    GameEngine::tickEngine();

    // TODO(Jeslas) : Change this
    uint32 index = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow())->requestNextImage(nullptr, nullptr);
    std::vector<GenericWindowCanvas*> canvases = { getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow()) };
    std::vector<uint32> indices = { index };

    CMD_BUFFER_BEGIN_INFO(cmdBeginInfo);
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vDevice->vkBeginCommandBuffer(swapchainCmdBuffer, &cmdBeginInfo);
    
    IMAGE_MEMORY_BARRIER(memBarrier);
    memBarrier.dstQueueFamilyIndex = getQueue<EQueueFunction::Present>(*deviceQueues,vDevice)->queueFamilyIndex();
    memBarrier.srcQueueFamilyIndex = getQueue<EQueueFunction::Present>(*deviceQueues, vDevice)->queueFamilyIndex();
    memBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    memBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    memBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseArrayLayer = 0; range.layerCount = 1;
    range.baseMipLevel = 0; range.levelCount = 1;
    memBarrier.subresourceRange = range;

    memBarrier.image = static_cast<VulkanWindowCanvas*>(canvases[0])->swapchainImage(index);

    vDevice->vkCmdPipelineBarrier(swapchainCmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr,
        0, nullptr, 1, &memBarrier);

    vDevice->vkEndCommandBuffer(swapchainCmdBuffer);
    VkPipelineStageFlags flag = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    SUBMIT_INFO(qSubmitInfo);
    qSubmitInfo.commandBufferCount = 1;
    qSubmitInfo.pCommandBuffers = &swapchainCmdBuffer;
    qSubmitInfo.pWaitDstStageMask = &flag;

    vDevice->vkQueueSubmit(getQueue<EQueueFunction::Present>(*deviceQueues, vDevice)->getQueueOfPriority<EQueuePriority::High>()
        , 1, &qSubmitInfo, static_cast<VulkanFence*>(vFence.get())->fence);
    vFence->waitForSignal();

    GraphicsHelper::presentImage(renderingApi->getGraphicsInstance(), &canvases, &indices, nullptr);
    appInstance().appWindowManager.getMainWindow()->updateWindow();
}
