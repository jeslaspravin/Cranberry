#include "ExperimentalEngine.h"
#include "../RenderInterface/Resources/GenericWindowCanvas.h"
#include "../RenderInterface/PlatformIndependentHelper.h"
#include "../Core/Engine/WindowManager.h"
#include "../Core/Platform/GenericAppWindow.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanQueueResource.h"
#include "../VulkanRI/VulkanInternals/VulkanDevice.h"
#include "../VulkanRI/VulkanInternals/Debugging.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanWindowCanvas.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanMemoryResources.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanSampler.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"
#include "../RenderInterface/Resources/Samplers/SamplerInterface.h"

#include <glm/ext/vector_float3.hpp>
#include <array>

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
    normalBuffer.buffer = new GraphicsRBuffer(sizeof(glm::vec3), 1);
    normalBuffer.buffer->setResourceName("Test_Buffer");
    normalBuffer.buffer->init();
    texelBuffer.buffer = new GraphicsRTexelBuffer(EPixelDataFormat::R_SF32, 3);
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
    commonSampler = GraphicsHelper::createSampler(gEngine->getRenderApi()->getGraphicsInstance(), "CommonSampler",
        ESamplerTilingMode::Repeat, ESamplerFiltering::Linear);
    // Render target texture
    {
        rtTexture.image = new GraphicsRenderTargetResource(EPixelDataFormat::RGBA_U8_NormPacked);
        rtTexture.image->setResourceName("Test_RT_Texture");
        rtTexture.image->setImageSize(Size3D(512, 512, 1));
        rtTexture.image->setSampleCounts(EPixelSampleCount::SampleCount1);
        rtTexture.image->init();

        rtTexture.imageView = static_cast<VulkanImageResource*>(rtTexture.image)->getImageView(ImageViewInfo());

        if (rtTexture.imageView != nullptr)
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

        texture.imageView = static_cast<VulkanImageResource*>(texture.image)->getImageView(ImageViewInfo());

        if (static_cast<VulkanImageResource*>(texture.image) != nullptr)
        {
            graphicsDbg->markObject((uint64)texture.imageView, "Test_TextureView", VK_OBJECT_TYPE_IMAGE_VIEW);
        }
    }
}

void ExperimentalEngine::destroyImages()
{
    commonSampler->release();

    texture.image->release();
    delete texture.image;
    texture.image = nullptr;
    texture.imageView = nullptr;

    rtTexture.image->release();
    delete rtTexture.image;
    rtTexture.image = nullptr;
    rtTexture.imageView = nullptr;
}

void ExperimentalEngine::createShaderResDescriptors()
{
    // Descriptors for shaders
    {
        std::array<VkDescriptorSetLayoutBinding, 4> bindings;
        {
            VkDescriptorSetLayoutBinding texelBufferBinding;
            texelBufferBinding.binding = 0;
            texelBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            texelBufferBinding.descriptorCount = 1;
            texelBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            texelBufferBinding.pImmutableSamplers = nullptr;
            bindings[0] = texelBufferBinding;
        }
        {
            VkDescriptorSetLayoutBinding lightBufferBinding;
            lightBufferBinding.binding = 1;
            lightBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            lightBufferBinding.descriptorCount = 1;
            lightBufferBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            lightBufferBinding.pImmutableSamplers = nullptr;
            bindings[1] = lightBufferBinding;
        }
        {
            VkDescriptorSetLayoutBinding samplerBinding;
            samplerBinding.binding = 2;
            samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            samplerBinding.descriptorCount = 1;
            samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            samplerBinding.pImmutableSamplers = nullptr;
            bindings[2] = samplerBinding;
        }
        {
            VkDescriptorSetLayoutBinding sampledImageBinding;
            sampledImageBinding.binding = 3;
            sampledImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            sampledImageBinding.descriptorCount = 1;
            sampledImageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            sampledImageBinding.pImmutableSamplers = nullptr;
            bindings[3] = sampledImageBinding;
        }

        DESCRIPTOR_SET_LAYOUT_CREATE_INFO(descSetLayoutCreateInfo);
        descSetLayoutCreateInfo.bindingCount = (uint32)bindings.size();
        descSetLayoutCreateInfo.pBindings = bindings.data();

        if (vDevice->vkCreateDescriptorSetLayout(device, &descSetLayoutCreateInfo, nullptr, &descriptorsSetLayout) != VK_SUCCESS)
        {
            Logger::error("ExperimentalEngine", "%s() : Failure in creating descriptor set layout", __func__);
            descriptorsSetLayout = nullptr;
            requestExit();
            return;
        }
    }
    // Descriptors set allocation and descriptors pool creation
    {
        // Since our shader pipeline descriptor set has only 4 types of descriptor
        std::array<VkDescriptorPoolSize, 4> descriptorsAndCounts;
        // Index ordering doesnt matter between layout's binding and pool allocatable descriptors 
        // For vertex shader, vertex float texel buffer.
        descriptorsAndCounts[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        descriptorsAndCounts[3].descriptorCount = 1;
        // For frag shader, Light locations
        descriptorsAndCounts[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorsAndCounts[2].descriptorCount = 1;
        // For frag shader, texture sampler
        descriptorsAndCounts[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorsAndCounts[1].descriptorCount = 1;
        // For frag shader, texture
        descriptorsAndCounts[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorsAndCounts[0].descriptorCount = 1;

        DESCRIPTOR_POOL_CREATE_INFO(descPoolCreateInfo);
        descPoolCreateInfo.maxSets = 3;
        descPoolCreateInfo.poolSizeCount = (uint32)descriptorsAndCounts.size();
        descPoolCreateInfo.pPoolSizes = descriptorsAndCounts.data();

        if (vDevice->vkCreateDescriptorPool(device, &descPoolCreateInfo, nullptr, &descriptorsPool) != VK_SUCCESS)
        {
            Logger::error("ExperimentalEngine", "%s() : Failed creating descriptors pool", __func__);
            descriptorsPool = nullptr;
            requestExit();
            return;
        }

        DESCRIPTOR_SET_ALLOCATE_INFO(descSetsAllocInfo);
        descSetsAllocInfo.descriptorSetCount = 1;
        descSetsAllocInfo.pSetLayouts = &descriptorsSetLayout;
        descSetsAllocInfo.descriptorPool = descriptorsPool;
        if (vDevice->vkAllocateDescriptorSets(device, &descSetsAllocInfo, &descriptorsSet) != VK_SUCCESS)
        {
            Logger::error("ExperimentalEngine", "%s() : Descriptors set allocation failed", __func__);
            descriptorsSet = nullptr;
            requestExit();
            return;
        }
    }
    // Updating the descriptors set with resource
    {
        // Since our shader pipeline descriptor set has only 4 types of descriptor
        std::array<VkWriteDescriptorSet, 4> descriptorsToWrite;        

        // Index ordering doesnt matter between layout's binding and write descriptors 
        // For vertex shader, vertex float texel buffer.
        WRITE_RESOURCE_TO_DESCRIPTORS_SET(vertexColorOffsetWriteToSet);
        vertexColorOffsetWriteToSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        vertexColorOffsetWriteToSet.dstBinding = 0;
        vertexColorOffsetWriteToSet.dstSet = descriptorsSet;
        vertexColorOffsetWriteToSet.pTexelBufferView = &texelBuffer.bufferView;
        descriptorsToWrite[0] = vertexColorOffsetWriteToSet;
        
        // For frag shader, Light locations
        VkDescriptorBufferInfo lightPosBufferInfo;
        lightPosBufferInfo.buffer = static_cast<VulkanBufferResource*>(normalBuffer.buffer)->buffer;
        lightPosBufferInfo.offset = 0;
        lightPosBufferInfo.range = VK_WHOLE_SIZE;

        WRITE_RESOURCE_TO_DESCRIPTORS_SET(lightPositionWriteToSet);
        lightPositionWriteToSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lightPositionWriteToSet.dstBinding = 1;
        lightPositionWriteToSet.dstSet = descriptorsSet;
        lightPositionWriteToSet.pBufferInfo = &lightPosBufferInfo;
        descriptorsToWrite[1] = lightPositionWriteToSet;

        // For frag shader, texture sampler
        VkDescriptorImageInfo samplerInfo;
        samplerInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        samplerInfo.imageView = nullptr;
        samplerInfo.sampler = static_cast<VulkanSampler*>(&(*commonSampler))->sampler;

        WRITE_RESOURCE_TO_DESCRIPTORS_SET(textureSamplerWriteToSet);
        textureSamplerWriteToSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        textureSamplerWriteToSet.dstBinding = 2;
        textureSamplerWriteToSet.dstSet = descriptorsSet;
        textureSamplerWriteToSet.pImageInfo = &samplerInfo;
        descriptorsToWrite[2] = textureSamplerWriteToSet;

        // For frag shader, texture
        VkDescriptorImageInfo textureInfo;
        textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        textureInfo.imageView = texture.imageView;
        textureInfo.sampler = nullptr;

        WRITE_RESOURCE_TO_DESCRIPTORS_SET(textureWriteToSet);
        textureWriteToSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        textureWriteToSet.dstBinding = 3;
        textureWriteToSet.dstSet = descriptorsSet;
        textureWriteToSet.pImageInfo = &textureInfo;
        descriptorsToWrite[3] = textureWriteToSet;

        vDevice->vkUpdateDescriptorSets(device, (uint32)descriptorsToWrite.size(), descriptorsToWrite.data(), 0, nullptr);
    }
}

void ExperimentalEngine::destroyShaderResDescriptors()
{
    if (descriptorsSetLayout)
    {
        vDevice->vkDestroyDescriptorSetLayout(device, descriptorsSetLayout, nullptr);

        if (descriptorsPool)
        {
            vDevice->vkResetDescriptorPool(device, descriptorsPool, 0);
            vDevice->vkDestroyDescriptorPool(device, descriptorsPool, nullptr);
            descriptorsSet = nullptr;
        }
    }
}

void ExperimentalEngine::createPipelineResources()
{
    // Shader pipeline's buffers and image access
    createShaderResDescriptors();
}

void ExperimentalEngine::destroyPipelineResources()
{

    // Shader pipeline's buffers and image access
    destroyShaderResDescriptors();
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
    createPipelineResources();
}

void ExperimentalEngine::onQuit()
{
    vFence->release();
    vFence.reset();

    destroyPipelineResources();

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

