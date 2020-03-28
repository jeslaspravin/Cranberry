#include "ExperimentalEngine.h"
#include "../RenderInterface/Resources/GenericWindowCanvas.h"
#include "../RenderInterface/PlatformIndependentHelper.h"
#include "../Core/Engine/WindowManager.h"
#include "../Core/Platform/GenericAppWindow.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanQueueResource.h"
#include "../VulkanRI/VulkanInternals/VulkanDevice.h"
#include "../VulkanRI/VulkanInternals/Debugging.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanWindowCanvas.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"

#include <glm/ext/vector_float2.hpp>

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

void ExperimentalEngine::createBuffers()
{
    normalBuffer.buffer = new GraphicsRBuffer(sizeof(glm::vec2), 1);
    normalBuffer.buffer->setResourceName("Test_Buffer");
    normalBuffer.buffer->init();
    texelBuffer.buffer = new GraphicsRTexelBuffer(EPixelDataFormat::R_SF32, 100);
    texelBuffer.buffer->setResourceName("Test_TexelBuffer");
    texelBuffer.buffer->init();
    BUFFER_VIEW_CREATE_INFO(bufferViewCreateInfo);
    bufferViewCreateInfo.buffer = static_cast<VulkanRTexelBuffer*>(texelBuffer.buffer)->buffer;
    bufferViewCreateInfo.format = (VkFormat)EPixelDataFormat::getFormatInfo(EPixelDataFormat::R_SF32)->format;
    if (vDevice->vkCreateBufferView(device, &bufferViewCreateInfo, nullptr, &texelBuffer.bufferView) != VK_SUCCESS)
    {
        texelBuffer.bufferView = nullptr;
        Logger::error("ExperimentalEngine", "%s() : Failed creating buffer view for texel buffer %s",
            __func__, texelBuffer.buffer->getResourceName().getChar());
    }
    else
    {
        graphicsDbg->markObject((uint64)texelBuffer.bufferView, "Test_TexelBufferView", VK_OBJECT_TYPE_BUFFER_VIEW);
    }
}

void ExperimentalEngine::destroyBuffers()
{
    normalBuffer.buffer->release();
    delete normalBuffer.buffer;
    normalBuffer.buffer = nullptr;

    if (texelBuffer.bufferView)
    {
        vDevice->vkDestroyBufferView(device, texelBuffer.bufferView, nullptr);
        texelBuffer.bufferView = nullptr;
    }
    texelBuffer.buffer->release();
    delete texelBuffer.buffer;
    texelBuffer.buffer = nullptr;
}

void ExperimentalEngine::createImages()
{
    // Render target texture
    {
        rtTexture.image = new GraphicsRenderTargetResource(EPixelDataFormat::RGBA_U8_NormPacked);
        rtTexture.image->setResourceName("Test_RT_Texture");
        rtTexture.image->setImageSize(Size3D(512, 512, 1));
        rtTexture.image->setSampleCounts(EPixelSampleCount::SampleCount1);
        rtTexture.image->init();

        IMAGE_VIEW_CREATE_INFO(rtViewCreateInfo);
        rtViewCreateInfo.image = static_cast<VulkanImageResource*>(rtTexture.image)->image;
        rtViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
        rtViewCreateInfo.format = (VkFormat)EPixelDataFormat::getFormatInfo(EPixelDataFormat::RGBA_U8_NormPacked)->format;
        rtViewCreateInfo.subresourceRange = {
            VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT
            , 0
            , VK_REMAINING_MIP_LEVELS
            , 0
            , VK_REMAINING_ARRAY_LAYERS
        };

        if (vDevice->vkCreateImageView(device, &rtViewCreateInfo, nullptr, &rtTexture.imageView) != VK_SUCCESS)
        {
            rtTexture.imageView = nullptr;
            Logger::error("ExperimentalEngine", "%s() : Failed creating image view for RT texture %s",
                __func__, rtTexture.image->getResourceName().getChar());
        }
        else
        {
            graphicsDbg->markObject((uint64)rtTexture.imageView, "Test_RT_TextureView", VK_OBJECT_TYPE_IMAGE_VIEW);
        }
    }
    // common shader sampling texture
    {
        texture.image = new GraphicsImageResource(EPixelDataFormat::RGBA_U8_NormPacked);
        texture.image->setResourceName("Test_Texture");
        texture.image->setShaderUsage(EImageShaderUsage::Sampling);
        texture.image->setImageSize(Size3D(1024, 1024, 1));
        texture.image->setSampleCounts(EPixelSampleCount::SampleCount8);
        texture.image->init();

        IMAGE_VIEW_CREATE_INFO(textureViewCreateInfo);
        textureViewCreateInfo.image = static_cast<VulkanImageResource*>(texture.image)->image;
        textureViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
        textureViewCreateInfo.format = (VkFormat)EPixelDataFormat::getFormatInfo(EPixelDataFormat::RGBA_U8_NormPacked)->format;
        textureViewCreateInfo.subresourceRange = {
            VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT
            , 0
            , VK_REMAINING_MIP_LEVELS
            , 0
            , VK_REMAINING_ARRAY_LAYERS
        };

        if (vDevice->vkCreateImageView(device, &textureViewCreateInfo, nullptr, &texture.imageView) != VK_SUCCESS)
        {
            texture.imageView = nullptr;
            Logger::error("ExperimentalEngine", "%s() : Failed creating image view for texture %s",
                __func__, texture.image->getResourceName().getChar());
        }
        else
        {
            graphicsDbg->markObject((uint64)texture.imageView, "Test_TextureView", VK_OBJECT_TYPE_IMAGE_VIEW);
        }
    }
}

void ExperimentalEngine::destroyImages()
{
    if (texture.imageView)
    {
        vDevice->vkDestroyImageView(device, texture.imageView, nullptr);
        texture.imageView = nullptr;
    }
    texture.image->release();
    delete texture.image;
    texture.image = nullptr;

    if (rtTexture.imageView)
    {
        vDevice->vkDestroyImageView(device, rtTexture.imageView, nullptr);
        rtTexture.imageView = nullptr;
    }
    rtTexture.image->release();
    delete rtTexture.image;
    rtTexture.image = nullptr;
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

    createBuffers();
    createImages();
}

void ExperimentalEngine::onQuit()
{
    vFence->release();
    vFence.reset();

    destroyBuffers();
    destroyImages();

    vDevice->vkFreeCommandBuffers(device, pools[EQueueFunction::Present].resetableCommandPool, 1, &swapchainCmdBuffer);
    destroyPools();
    GameEngine::onQuit();
}

void ExperimentalEngine::tickEngine()
{
    GameEngine::tickEngine();

    SharedPtr<GraphicsSemaphore> waitSemaphore;
    uint32 index = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow())->requestNextImage(&waitSemaphore, nullptr);
    std::vector<GenericWindowCanvas*> canvases = { getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow()) };
    std::vector<uint32> indices = { index };

    CMD_BUFFER_BEGIN_INFO(cmdBeginInfo);
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vDevice->vkBeginCommandBuffer(swapchainCmdBuffer, &cmdBeginInfo);
    
    IMAGE_MEMORY_BARRIER(memBarrier);
    memBarrier.dstQueueFamilyIndex = getQueue<EQueueFunction::Present>(*deviceQueues,vDevice)->queueFamilyIndex();
    memBarrier.srcQueueFamilyIndex = getQueue<EQueueFunction::Present>(*deviceQueues, vDevice)->queueFamilyIndex();
    memBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    memBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    memBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseArrayLayer = 0; range.layerCount = 1;
    range.baseMipLevel = 0; range.levelCount = 1;
    memBarrier.subresourceRange = range;

    memBarrier.image = static_cast<VulkanWindowCanvas*>(canvases[0])->swapchainImage(index);

    vDevice->vkCmdPipelineBarrier(swapchainCmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr,
        0, nullptr, 1, &memBarrier);

    VkClearColorValue clearColor[1];
    clearColor[0].float32[0] = 1.0f;
    clearColor[0].float32[1] = 1.0f;
    clearColor[0].float32[2] = 0.0f;
    clearColor[0].float32[3] = 1.0f;
    vDevice->vkCmdClearColorImage(swapchainCmdBuffer, static_cast<VulkanWindowCanvas*>(canvases[0])->swapchainImage(index), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        clearColor, 1, &range);

    memBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    memBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    vDevice->vkCmdPipelineBarrier(swapchainCmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr,
        0, nullptr, 1, &memBarrier);

    vDevice->vkEndCommandBuffer(swapchainCmdBuffer);
    VkPipelineStageFlags flag = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    SUBMIT_INFO(qSubmitInfo);
    qSubmitInfo.commandBufferCount = 1;
    qSubmitInfo.pCommandBuffers = &swapchainCmdBuffer;
    qSubmitInfo.pWaitDstStageMask = &flag;
    qSubmitInfo.pWaitSemaphores = &static_cast<VulkanSemaphore*>(waitSemaphore.get())->semaphore;
    qSubmitInfo.waitSemaphoreCount = 1;

    vFence->resetSignal();
    vDevice->vkQueueSubmit(getQueue<EQueueFunction::Present>(*deviceQueues, vDevice)->getQueueOfPriority<EQueuePriority::High>()
        , 1, &qSubmitInfo, static_cast<VulkanFence*>(vFence.get())->fence);
    vFence->waitForSignal();
    GraphicsHelper::presentImage(renderingApi->getGraphicsInstance(), &canvases, &indices, nullptr);
    appInstance().appWindowManager.getMainWindow()->updateWindow();
}

GameEngine* GameEngineWrapper::createEngineInstance()
{
    return new ExperimentalEngine();
}

