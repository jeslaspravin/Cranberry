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
#include "../Core/Platform/PlatformAssertionErrors.h"
#include "../VulkanRI/Resources/VulkanShaderResources.h"
#include "Shaders/TriangleShader.h"
#include "../RenderInterface/Shaders/DrawQuadFromInputAttachment.h"
#include "../Core/Types/Colors.h"

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
        rtTexture.sampleCount = EPixelSampleCount::SampleCount1;
        rtTexture.format = EPixelDataFormat::ABGR_U8_NormPacked;
        rtTexture.image = new GraphicsRenderTargetResource(rtTexture.format);
        rtTexture.image->setResourceName("Test_RT_Texture");
        Size3D rtTextureSize(0,0,1);
        getApplicationInstance()->appWindowManager.getMainWindow()->windowSize(rtTextureSize.x,rtTextureSize.y);
        rtTexture.image->setImageSize(rtTextureSize);
        rtTexture.image->setSampleCounts(rtTexture.sampleCount);
        rtTexture.image->init();

        rtTexture.imageView = static_cast<VulkanImageResource*>(rtTexture.image)->getImageView(ImageViewInfo());

        if (rtTexture.imageView != nullptr)
        {
            graphicsDbg->markObject((uint64)rtTexture.imageView, "Test_RT_TextureView", VK_OBJECT_TYPE_IMAGE_VIEW);
        }
    }
    // common shader sampling texture
    {
        texture.format = EPixelDataFormat::ABGR_U8_NormPacked;
        texture.sampleCount = EPixelSampleCount::SampleCount8;
        texture.image = new GraphicsImageResource(texture.format);
        texture.image->setResourceName("Test_Texture");
        texture.image->setShaderUsage(EImageShaderUsage::Sampling);
        texture.image->setImageSize(Size3D(1024, 1024, 1));
        texture.image->setSampleCounts(texture.sampleCount);
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
        // Since our shader pipeline descriptor set has only 4 + 1 input attachment types of descriptor
        std::array<VkDescriptorPoolSize, 5> descriptorsAndCounts;
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
        // For frag shader, input attachment
        descriptorsAndCounts[4].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorsAndCounts[4].descriptorCount = 1;

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

void ExperimentalEngine::createInputAttachmentDescriptors()
{
    {
        VkDescriptorSetLayoutBinding inputAttachmentDescBinding;
        inputAttachmentDescBinding.binding = 0;
        inputAttachmentDescBinding.descriptorCount = 1;
        inputAttachmentDescBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        inputAttachmentDescBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        inputAttachmentDescBinding.pImmutableSamplers = nullptr;
        DESCRIPTOR_SET_LAYOUT_CREATE_INFO(layoutCreateInfo);
        layoutCreateInfo.bindingCount = 1;
        layoutCreateInfo.pBindings = &inputAttachmentDescBinding;

        if (vDevice->vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &subpass1DescLayout) != VK_SUCCESS)
        {
            Logger::error("ExperimentalEngine", "%s() : Failed creating descriptors set layout for input attachment in subpass1", __func__);
            subpass1DescLayout = nullptr;
            return;
        }
    }

    {
        DESCRIPTOR_SET_ALLOCATE_INFO(descAllocateInfo);
        descAllocateInfo.descriptorPool = descriptorsPool;
        descAllocateInfo.descriptorSetCount = 1;
        descAllocateInfo.pSetLayouts = &subpass1DescLayout;

        if (vDevice->vkAllocateDescriptorSets(device, &descAllocateInfo, &subpass1DescSet) != VK_SUCCESS)
        {
            Logger::error("ExperimentalEngine", "%s() : Failed creating descriptors set for input attachment in subpass1", __func__);
            subpass1DescSet = nullptr;
            return;
        }
    }

    VkDescriptorImageInfo textureInfo;
    textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    textureInfo.imageView = rtTexture.imageView;
    textureInfo.sampler = nullptr;

    WRITE_RESOURCE_TO_DESCRIPTORS_SET(writeToDescSet);
    writeToDescSet.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    writeToDescSet.dstBinding = 0;
    writeToDescSet.dstSet = subpass1DescSet;
    writeToDescSet.pImageInfo = &textureInfo;

    vDevice->vkUpdateDescriptorSets(device, 1, &writeToDescSet, 0, nullptr);
}

void ExperimentalEngine::destroyInputAttachmentDescriptors()
{
    if (subpass1DescLayout)
    {
        vDevice->vkDestroyDescriptorSetLayout(device, subpass1DescLayout, nullptr);
        subpass1DescSet = nullptr;
    }
}

void ExperimentalEngine::createRenderpass()
{
    GenericWindowCanvas* windowCanvas = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow());
    framebuffers.resize(windowCanvas->imagesCount());

    attachmentsClearColors.resize(2);
    std::array<VkAttachmentDescription, 2> attachmentsDesc;
    std::array<VkImageView, 2> framebufferAttachments;
    {
        VkAttachmentDescription renderTargetAttachment;
        renderTargetAttachment.flags = 0;
        renderTargetAttachment.format = (VkFormat)EPixelDataFormat::getFormatInfo(rtTexture.format)->format;
        renderTargetAttachment.samples = (VkSampleCountFlagBits)rtTexture.sampleCount;
        renderTargetAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        renderTargetAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        renderTargetAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        renderTargetAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        renderTargetAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        renderTargetAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentsDesc[0] = renderTargetAttachment;
        framebufferAttachments[0] = rtTexture.imageView;
        attachmentsClearColors[0].color = { 0.0f ,0.0f,0.0f,1.0f };// Green

        VkAttachmentDescription swapchainAttachment;
        swapchainAttachment.flags = 0;
        swapchainAttachment.format = (VkFormat)EPixelDataFormat::getFormatInfo(windowCanvas->windowCanvasFormat())->format;
        swapchainAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        swapchainAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        swapchainAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        swapchainAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        swapchainAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        swapchainAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        swapchainAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentsDesc[1] = swapchainAttachment;
        // Has to create render passes of same count as swap chain images. Will change in real application 
        //framebufferAttachments[1] = static_cast<VulkanWindowCanvas*>(windowCanvas)
        attachmentsClearColors[1].color = { 0.267f ,0.4f,0.0f,1.0f };// Some good color
    }
    std::array<VkSubpassDescription,2> subpasses;
    VkSubpassDependency subpassesDependency;

    VkAttachmentReference pass1RtAttachmentRef;
    pass1RtAttachmentRef.attachment = 0;// Index in attachment description
    pass1RtAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpasses[0].flags = 0;
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].pDepthStencilAttachment = nullptr;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = &pass1RtAttachmentRef;
    subpasses[0].pResolveAttachments = nullptr;
    subpasses[0].inputAttachmentCount = 0;
    subpasses[0].pInputAttachments = nullptr;
    subpasses[0].preserveAttachmentCount = 0;
    subpasses[0].pPreserveAttachments = nullptr;

    VkAttachmentReference pass2SwapchainAttachmentRef;
    pass2SwapchainAttachmentRef.attachment = 1;
    pass2SwapchainAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference pass2RtInputAttachmentRef;
    pass2RtInputAttachmentRef.attachment = 0;
    pass2RtInputAttachmentRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    subpasses[1].flags = 0;
    subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].pDepthStencilAttachment = nullptr;
    subpasses[1].colorAttachmentCount = 1;
    subpasses[1].pColorAttachments = &pass2SwapchainAttachmentRef;
    subpasses[1].pResolveAttachments = nullptr;
    subpasses[1].inputAttachmentCount = 1;
    subpasses[1].pInputAttachments = &pass2RtInputAttachmentRef;
    subpasses[1].preserveAttachmentCount = 0;
    subpasses[1].pPreserveAttachments = nullptr;

    subpassesDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    subpassesDependency.srcSubpass = 0;
    subpassesDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassesDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassesDependency.dstSubpass = 1;
    subpassesDependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    subpassesDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    RENDERPASS_CREATE_INFO(renderPassCreateInfo);
    renderPassCreateInfo.attachmentCount = (uint32)attachmentsDesc.size();
    renderPassCreateInfo.pAttachments = attachmentsDesc.data();
    renderPassCreateInfo.subpassCount = (uint32)subpasses.size();
    renderPassCreateInfo.pSubpasses = subpasses.data();
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassesDependency;

    if (vDevice->vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        Logger::error("ExperimentalEngine", "%s() : Failed creating render pass", __func__);
        renderPass = nullptr;
        return;
    }

    FRAMEBUFFER_CREATE_INFO(framebufferCreateInfo);
    framebufferCreateInfo.renderPass = renderPass;// Dont have to be original renderpass, just a render pass with matching attachment format and sample description is enough
    framebufferCreateInfo.attachmentCount = (uint32)framebufferAttachments.size();
    framebufferCreateInfo.pAttachments = framebufferAttachments.data();
    getApplicationInstance()->appWindowManager.getMainWindow()->windowSize(framebufferCreateInfo.width, framebufferCreateInfo.height);
    framebufferCreateInfo.layers = 1;

    for (int32 i = 0; i < windowCanvas->imagesCount(); ++i)
    {
        framebufferAttachments[1] = static_cast<VulkanWindowCanvas*>(windowCanvas)->swapchainImageView(i);
        
        if (vDevice->vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
        {
            Logger::error("ExperimentalEngine", "%s() : Creating frame buffer failed for swapchain index %d", __func__, i);
        }
    }

    createInputAttachmentDescriptors();
}

void ExperimentalEngine::destroyRenderpass()
{
    destroyInputAttachmentDescriptors();
    for (int32 i = 0; i < framebuffers.size(); ++i)
    {
        vDevice->vkDestroyFramebuffer(device, framebuffers[i], nullptr);
    }
    framebuffers.clear();
    vDevice->vkDestroyRenderPass(device, renderPass, nullptr);
    renderPass = nullptr;
}

void ExperimentalEngine::createPipelineCache()
{
    {
        String cacheFilePath;
        cacheFilePath = FileSystemFunctions::combinePath(FileSystemFunctions::applicationDirectory(cacheFilePath), "Cache", "gPipeline.cache");
        pipelineCacheFile = PlatformFile(cacheFilePath);
    }
    pipelineCacheFile.setFileFlags(EFileFlags::Read | EFileFlags::Write | EFileFlags::OpenAlways);
    pipelineCacheFile.setSharingMode(EFileSharing::NoSharing);
    pipelineCacheFile.openOrCreate();

    std::vector<uint8> cacheData;
    pipelineCacheFile.read(cacheData);

    PIPELINE_CACHE_CREATE_INFO(pipelineCacheCreateInfo);
    pipelineCacheCreateInfo.initialDataSize = static_cast<uint32>(cacheData.size());
    pipelineCacheCreateInfo.pInitialData = nullptr;
    if (cacheData.size() > 0)
    {
        pipelineCacheCreateInfo.pInitialData = cacheData.data();
    }
    else
    {
        Logger::debug("ExperimentalEngine", "%s() : Cache for pipeline cache creation is not available", __func__);
    }

    if (vDevice->vkCreatePipelineCache(device,&pipelineCacheCreateInfo,nullptr,&drawTriPipeline.cache) != VK_SUCCESS)
    {
        Logger::warn("ExperimentalEngine", "%s() : Triangle drawing pipeline cache creation failed", __func__);
        drawTriPipeline.cache = nullptr;
    }
    else
    {
        graphicsDbg->markObject((uint64)(drawTriPipeline.cache), "ExperimentalTrianglePipelineCache", VK_OBJECT_TYPE_PIPELINE_CACHE);
    }
    if (vDevice->vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &drawQuadPipeline.cache) != VK_SUCCESS)
    {
        Logger::warn("ExperimentalEngine", "%s() : Quad drawing pipeline cache creation failed", __func__);
        drawQuadPipeline.cache = nullptr;
    }
    else
    {
        graphicsDbg->markObject((uint64)(drawTriPipeline.cache), "DrawQuadPipelineCache", VK_OBJECT_TYPE_PIPELINE_CACHE);
    }

    pipelineCacheFile.closeFile();
}

void ExperimentalEngine::writeAndDestroyPipelineCache()
{
    std::vector<VkPipelineCache> pipelineCaches;
    if (drawTriPipeline.cache != nullptr)
    {
        pipelineCaches.push_back(drawTriPipeline.cache);
    }
    if (drawQuadPipeline.cache != nullptr)
    {
        pipelineCaches.push_back(drawQuadPipeline.cache);
    }

    VkPipelineCache mergedCache;
    PIPELINE_CACHE_CREATE_INFO(pipelineCacheCreateInfo);
    pipelineCacheCreateInfo.initialDataSize = 0;
    pipelineCacheCreateInfo.pInitialData = nullptr;
    if (vDevice->vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &mergedCache) == VK_SUCCESS)
    {
        if(vDevice->vkMergePipelineCaches(device,mergedCache,static_cast<uint32>(pipelineCaches.size()),pipelineCaches.data()) == VK_SUCCESS)
        {
            size_t cacheDataSize;
            vDevice->vkGetPipelineCacheData(device, mergedCache, &cacheDataSize, nullptr);
            if (cacheDataSize > 0)
            {
                std::vector<uint8> cacheData(cacheDataSize);
                vDevice->vkGetPipelineCacheData(device, mergedCache, &cacheDataSize, cacheData.data());

                pipelineCacheFile.setCreationAction(EFileFlags::ClearExisting);
                pipelineCacheFile.openFile();

                pipelineCacheFile.write(cacheData);
                pipelineCacheFile.closeFile();
            }
        }
        pipelineCaches.push_back(mergedCache);
    }

    for (VkPipelineCache cache : pipelineCaches)
    {
        vDevice->vkDestroyPipelineCache(device, cache, nullptr);
    }
}

void ExperimentalEngine::createPipelineForSubpass()
{
    createPipelineCache();
    createTriDrawPipeline();

    const std::array<glm::vec3, 4> quadVerts = { glm::vec3(-1,-1,0),glm::vec3(1,-1,0),glm::vec3(-1,1,0),glm::vec3(1,1,0) };
    const std::array<uint32, 6> quadIndices = { 0,3,2,0,1,3 };// 3 Per tri of quad

    quadVertexBuffer = new GraphicsVertexBuffer(sizeof(glm::vec3), static_cast<uint32>(quadVerts.size()));
    quadVertexBuffer->setResourceName("ScreenQuadVertices");
    quadVertexBuffer->init();
    quadIndexBuffer = new GraphicsIndexBuffer(sizeof(uint32), static_cast<uint32>(quadIndices.size()));
    quadVertexBuffer->setResourceName("ScreenQuadIndices");
    quadIndexBuffer->init();

    {
        GraphicsVertexBuffer vertStagingBuffer(sizeof(glm::vec3), static_cast<uint32>(quadVerts.size()));
        vertStagingBuffer.setAsStagingResource(true);
        vertStagingBuffer.init();
        GraphicsIndexBuffer indexStagingBuffer(sizeof(uint32), static_cast<uint32>(quadIndices.size()));
        indexStagingBuffer.setAsStagingResource(true);
        indexStagingBuffer.init();

        // Uploading to staging buffer
        {
            IGraphicsInstance* graphicsInst = getRenderApi()->getGraphicsInstance();
            GraphicsHelper::mapResource(graphicsInst,&vertStagingBuffer);
            GraphicsHelper::mapResource(graphicsInst, &indexStagingBuffer);
            memcpy(vertStagingBuffer.getMappedMemory(), quadVerts.data(), quadVertexBuffer->getResourceSize());
            memcpy(indexStagingBuffer.getMappedMemory(), quadIndices.data(), quadIndexBuffer->getResourceSize());
            GraphicsHelper::unmapResource(graphicsInst, &vertStagingBuffer);
            GraphicsHelper::unmapResource(graphicsInst, &indexStagingBuffer);
        }
        // Copying to actual buffers
        {
            CMD_BUFFER_ALLOC_INFO(cmdAllocInfo);
            cmdAllocInfo.commandBufferCount = 1;
            cmdAllocInfo.commandPool = pools[EQueueFunction::Transfer].tempCommandsPool;
            VkCommandBuffer tempBuffer;
            vDevice->vkAllocateCommandBuffers(device, &cmdAllocInfo, &tempBuffer);

            VkBufferCopy vertCopyRegion{ 0,0, quadVertexBuffer->getResourceSize() };
            VkBufferCopy indexCopyRegion{ 0,0, quadIndexBuffer->getResourceSize() };

            CMD_BUFFER_BEGIN_INFO(cmdBeginInfo);
            cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vDevice->vkBeginCommandBuffer(tempBuffer, &cmdBeginInfo);

            vDevice->vkCmdCopyBuffer(tempBuffer, vertStagingBuffer.buffer, static_cast<VulkanBufferResource*>(quadVertexBuffer)->buffer, 1, &vertCopyRegion);
            vDevice->vkCmdCopyBuffer(tempBuffer, indexStagingBuffer.buffer, static_cast<VulkanBufferResource*>(quadIndexBuffer)->buffer, 1, &indexCopyRegion);

            vDevice->vkEndCommandBuffer(tempBuffer);

            SUBMIT_INFO(tempCmdSubmit);
            tempCmdSubmit.commandBufferCount = 1;
            tempCmdSubmit.pCommandBuffers = &tempBuffer;
            vDevice->vkQueueSubmit(getQueue<EQueueFunction::Transfer>(*deviceQueues, vDevice)->getQueueOfPriority<EQueuePriority::High>(),
                1, &tempCmdSubmit, static_cast<VulkanFence*>(cmdSubmitFence.get())->fence);

            cmdSubmitFence->waitForSignal();
            vDevice->vkFreeCommandBuffers(device, pools[EQueueFunction::Transfer].tempCommandsPool, 1, &tempBuffer);
        }
        vertStagingBuffer.release();
        indexStagingBuffer.release();
    }
    createQuadDrawPipeline();
}

void ExperimentalEngine::destroySubpassPipelines()
{
    writeAndDestroyPipelineCache();

    vDevice->vkDestroyPipelineLayout(device, drawTriPipeline.layout, nullptr);
    vDevice->vkDestroyPipeline(device, drawTriPipeline.pipeline, nullptr);
    vDevice->vkDestroyPipelineLayout(device, drawQuadPipeline.layout, nullptr);
    vDevice->vkDestroyPipeline(device, drawQuadPipeline.pipeline, nullptr);

    quadVertexBuffer->release();
    quadIndexBuffer->release();
    delete quadVertexBuffer;
    quadVertexBuffer = nullptr;
    delete quadIndexBuffer;
    quadIndexBuffer = nullptr;
}

void ExperimentalEngine::createTriDrawPipeline()
{
    ShaderResource* shaderResource = static_cast<ShaderResource*>(ExperimentalTriangleShader::staticType()->getDefault());

    GRAPHICS_PIPELINE_CREATE_INFO(graphicsPipelineCreateInfo);
    graphicsPipelineCreateInfo.pTessellationState = nullptr;// No tessellation right now
    graphicsPipelineCreateInfo.pDepthStencilState = nullptr;// No depth tests right now

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(shaderResource->getShaders().size());
    for (const std::pair<const EShaderStage::Type, SharedPtr<ShaderCodeResource>>& shader : shaderResource->getShaders())
    {
        const EShaderStage::ShaderStageInfo* stageInfo = EShaderStage::getShaderStageInfo(shader.first);
        if (stageInfo)
        {
            PIPELINE_SHADER_STAGE_CREATE_INFO(shaderStageCreateInfo);
            shaderStageCreateInfo.stage = (VkShaderStageFlagBits)stageInfo->shaderStage;
            shaderStageCreateInfo.pName = stageInfo->entryPointName.getChar();
            shaderStageCreateInfo.module = static_cast<VulkanShaderCodeResource*>(shader.second.get())->shaderModule;
            shaderStageCreateInfo.pSpecializationInfo = VK_NULL_HANDLE;// Compile time constants

            shaderStages.push_back(shaderStageCreateInfo);
        }
    }
    graphicsPipelineCreateInfo.stageCount = static_cast<uint32>(shaderStages.size());
    graphicsPipelineCreateInfo.pStages = shaderStages.data();
    
    PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO(vertexInputCreateInfo);
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    
    PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO(inputAssemCreateInfo);
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemCreateInfo;

    PIPELINE_VIEWPORT_STATE_CREATE_INFO(viewportCreateInfo);
    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    VkRect2D scissor = { {0,0},{0,0} };
    getApplicationInstance()->appWindowManager.getMainWindow()->windowSize(scissor.extent.width, scissor.extent.height);
    viewport.width = static_cast<float>(scissor.extent.width);
    viewport.height = static_cast<float>(scissor.extent.height);
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.pViewports = &viewport;
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.pScissors = &scissor;
    graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;

    PIPELINE_RASTERIZATION_STATE_CREATE_INFO(rasterizationCreateInfo);
    rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;

    PIPELINE_MULTISAMPLE_STATE_CREATE_INFO(multisampleCreateInfo);
    multisampleCreateInfo.sampleShadingEnable = multisampleCreateInfo.alphaToCoverageEnable = multisampleCreateInfo.alphaToOneEnable = VK_FALSE;
    multisampleCreateInfo.minSampleShading = 1;
    multisampleCreateInfo.pSampleMask = nullptr;
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;

    PIPELINE_COLOR_BLEND_STATE_CREATE_INFO(colorBlendOpCreateInfo);
    memcpy(colorBlendOpCreateInfo.blendConstants, &LinearColorConst::BLACK.getColorValue(), sizeof(glm::vec4));
    VkPipelineColorBlendAttachmentState colorAttachmentBlendState{// Blend state for color attachment in subpass in which this pipeline is used.
        VK_TRUE,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    colorBlendOpCreateInfo.attachmentCount = 1;
    colorBlendOpCreateInfo.pAttachments = &colorAttachmentBlendState;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendOpCreateInfo;

    PIPELINE_DYNAMIC_STATE_CREATE_INFO(dynamicStateCreateInfo);
    std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    dynamicStateCreateInfo.dynamicStateCount = static_cast<VkDynamicState>(dynamicStates.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;


    PIPELINE_LAYOUT_CREATE_INFO(pipelineLayoutCreateInfo);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    fatalAssert(vDevice->vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &drawTriPipeline.layout)
        == VK_SUCCESS, "Failed creating draw triangle pipeline layout");

    graphicsPipelineCreateInfo.layout = drawTriPipeline.layout;
    graphicsPipelineCreateInfo.renderPass = renderPass;
    graphicsPipelineCreateInfo.subpass = 0;

    fatalAssert(vDevice->vkCreateGraphicsPipelines(device, drawTriPipeline.cache,1,&graphicsPipelineCreateInfo, nullptr, &drawTriPipeline.pipeline)
        == VK_SUCCESS, "Failure in creating draw triangle pipelines");
    graphicsDbg->markObject((uint64)(drawTriPipeline.pipeline), "ExperimentalTrianglePipeline", VK_OBJECT_TYPE_PIPELINE);
}

void ExperimentalEngine::createQuadDrawPipeline()
{
    ShaderResource* shaderResource = static_cast<ShaderResource*>(DrawQuadFromInputAttachment::staticType()->getDefault());

    GRAPHICS_PIPELINE_CREATE_INFO(graphicsPipelineCreateInfo);
    graphicsPipelineCreateInfo.pTessellationState = nullptr;// No tessellation right now
    graphicsPipelineCreateInfo.pDepthStencilState = nullptr;// No depth tests right now

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(shaderResource->getShaders().size());
    for (const std::pair<const EShaderStage::Type, SharedPtr<ShaderCodeResource>>& shader : shaderResource->getShaders())
    {
        const EShaderStage::ShaderStageInfo* stageInfo = EShaderStage::getShaderStageInfo(shader.first);
        if (stageInfo)
        {
            PIPELINE_SHADER_STAGE_CREATE_INFO(shaderStageCreateInfo);
            shaderStageCreateInfo.stage = (VkShaderStageFlagBits)stageInfo->shaderStage;
            shaderStageCreateInfo.pName = stageInfo->entryPointName.getChar();
            shaderStageCreateInfo.module = static_cast<VulkanShaderCodeResource*>(shader.second.get())->shaderModule;
            shaderStageCreateInfo.pSpecializationInfo = VK_NULL_HANDLE;// Compile time constants

            shaderStages.push_back(shaderStageCreateInfo);
        }
    }
    graphicsPipelineCreateInfo.stageCount = static_cast<uint32>(shaderStages.size());
    graphicsPipelineCreateInfo.pStages = shaderStages.data();

    PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO(vertexInputCreateInfo);

    VkVertexInputBindingDescription vertexInputBinding;
    vertexInputBinding.binding = 0;
    vertexInputBinding.stride = sizeof(glm::vec3);
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription vertexInputAttribute;
    vertexInputAttribute.binding = 0;
    vertexInputAttribute.location = 0;
    vertexInputAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttribute.offset = 0;

    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &vertexInputBinding;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 1;
    vertexInputCreateInfo.pVertexAttributeDescriptions = &vertexInputAttribute;
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;

    PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO(inputAssemCreateInfo);
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemCreateInfo;

    PIPELINE_VIEWPORT_STATE_CREATE_INFO(viewportCreateInfo);
    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    VkRect2D scissor = { {0,0},{0,0} };
    getApplicationInstance()->appWindowManager.getMainWindow()->windowSize(scissor.extent.width, scissor.extent.height);
    viewport.width = static_cast<float>(scissor.extent.width);
    viewport.height = static_cast<float>(scissor.extent.height);
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.pViewports = &viewport;
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.pScissors = &scissor;
    graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;

    PIPELINE_RASTERIZATION_STATE_CREATE_INFO(rasterizationCreateInfo);
    rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;

    PIPELINE_MULTISAMPLE_STATE_CREATE_INFO(multisampleCreateInfo);
    multisampleCreateInfo.sampleShadingEnable = multisampleCreateInfo.alphaToCoverageEnable = multisampleCreateInfo.alphaToOneEnable = VK_FALSE;
    multisampleCreateInfo.minSampleShading = 1;
    multisampleCreateInfo.pSampleMask = nullptr;
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;

    PIPELINE_COLOR_BLEND_STATE_CREATE_INFO(colorBlendOpCreateInfo);
    memcpy(colorBlendOpCreateInfo.blendConstants, &LinearColorConst::BLACK.getColorValue(), sizeof(glm::vec4));
    VkPipelineColorBlendAttachmentState colorAttachmentBlendState{// Blend state for color attachment in subpass in which this pipeline is used.
        VK_TRUE,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    colorBlendOpCreateInfo.attachmentCount = 1;
    colorBlendOpCreateInfo.pAttachments = &colorAttachmentBlendState;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendOpCreateInfo;

    PIPELINE_DYNAMIC_STATE_CREATE_INFO(dynamicStateCreateInfo);
    std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    dynamicStateCreateInfo.dynamicStateCount = static_cast<VkDynamicState>(dynamicStates.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

    PIPELINE_LAYOUT_CREATE_INFO(pipelineLayoutCreateInfo);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &subpass1DescLayout;
    fatalAssert(vDevice->vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &drawQuadPipeline.layout)
        == VK_SUCCESS, "Failed creating draw triangle pipeline layout");
    graphicsPipelineCreateInfo.layout = drawQuadPipeline.layout;
    graphicsPipelineCreateInfo.renderPass = renderPass;
    graphicsPipelineCreateInfo.subpass = 1;

    fatalAssert(vDevice->vkCreateGraphicsPipelines(device, drawQuadPipeline.cache, 1, &graphicsPipelineCreateInfo, nullptr, &drawQuadPipeline.pipeline)
        == VK_SUCCESS, "Failure in creating draw quad pipelines");
    graphicsDbg->markObject((uint64)(drawQuadPipeline.pipeline), "DrawQuadPipeline", VK_OBJECT_TYPE_PIPELINE);
}

void ExperimentalEngine::createPipelineResources()
{
    // Shader pipeline's buffers and image access
    createShaderResDescriptors();
    createRenderpass();
    createPipelineForSubpass();
}

void ExperimentalEngine::destroyPipelineResources()
{
    destroySubpassPipelines();
    destroyRenderpass();
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
    
    renderpassSemaphore = GraphicsHelper::createSemaphore(getRenderApi()->getGraphicsInstance(), "renderpassSignal");
    presentWaitOn.push_back(renderpassSemaphore);

    CMD_BUFFER_ALLOC_INFO(cmdAllocInfo);
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.commandPool = pools[EQueueFunction::Graphics].resetableCommandPool;
    vDevice->vkAllocateCommandBuffers(device, &cmdAllocInfo, &renderPassCmdBuffer);
    cmdSubmitFence = GraphicsHelper::createFence(getRenderApi()->getGraphicsInstance(), "cmdBufferSubmit");

    createBuffers();
    createImages();
    createPipelineResources();
}

void ExperimentalEngine::onQuit()
{
    vDevice->vkDeviceWaitIdle(device);

    renderpassSemaphore->release();
    renderpassSemaphore.reset();

    destroyPipelineResources();

    destroyBuffers();
    destroyImages();

    cmdSubmitFence->release();
    cmdSubmitFence.reset();
    vDevice->vkFreeCommandBuffers(device, pools[EQueueFunction::Graphics].resetableCommandPool, 1, &renderPassCmdBuffer);
    destroyPools();
    GameEngine::onQuit();
}

void ExperimentalEngine::tickEngine()
{
    GameEngine::tickEngine();
    ScopedCommandMarker(renderPassCmdBuffer, "ExperimentalEngineFrame");

    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    VkRect2D scissor = { {0,0},{0,0} };
    getApplicationInstance()->appWindowManager.getMainWindow()->windowSize(scissor.extent.width, scissor.extent.height);
    viewport.width = static_cast<float>(scissor.extent.width);
    viewport.height = static_cast<float>(scissor.extent.height);

    SharedPtr<GraphicsSemaphore> waitSemaphore;
    uint32 index = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow())->requestNextImage(&waitSemaphore, nullptr);
    std::vector<GenericWindowCanvas*> canvases = { getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow()) };
    std::vector<uint32> indices = { index };

    {
        ScopedCommandMarker(renderPassCmdBuffer, "RenderCommandRecording");
        CMD_BUFFER_BEGIN_INFO(cmdBeginInfo);
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vDevice->vkBeginCommandBuffer(renderPassCmdBuffer, &cmdBeginInfo);

        RENDERPASS_BEGIN_INFO(renderPassBeginInfo);
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = framebuffers[index];
        renderPassBeginInfo.pClearValues = attachmentsClearColors.data();
        renderPassBeginInfo.clearValueCount = (uint32)attachmentsClearColors.size();
        renderPassBeginInfo.renderArea.offset = { 0,0 };
        getApplicationInstance()->appWindowManager.getMainWindow()->windowSize(renderPassBeginInfo.renderArea.extent.width,
            renderPassBeginInfo.renderArea.extent.height);
        vDevice->vkCmdBeginRenderPass(renderPassCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vDevice->vkCmdSetViewport(renderPassCmdBuffer, 0, 1, &viewport);
        vDevice->vkCmdSetScissor(renderPassCmdBuffer, 0, 1, &scissor);

        vDevice->vkCmdBindPipeline(renderPassCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawTriPipeline.pipeline);
        vDevice->vkCmdDraw(renderPassCmdBuffer, 3, 1, 0, 0);

        vDevice->vkCmdNextSubpass(renderPassCmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
        vDevice->vkCmdBindPipeline(renderPassCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawQuadPipeline.pipeline);
        vDevice->vkCmdBindDescriptorSets(renderPassCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawQuadPipeline.layout, 0, 1, &subpass1DescSet, 0, nullptr);
        uint64 vertexBufferOffset = 0;
        vDevice->vkCmdBindVertexBuffers(renderPassCmdBuffer, 0, 1, &static_cast<VulkanBufferResource*>(quadVertexBuffer)->buffer, &vertexBufferOffset);
        vDevice->vkCmdBindIndexBuffer(renderPassCmdBuffer, static_cast<VulkanBufferResource*>(quadIndexBuffer)->buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

        vDevice->vkCmdDrawIndexed(renderPassCmdBuffer, 6, 1, 0, 0, 0);

        vDevice->vkCmdEndRenderPass(renderPassCmdBuffer);
        vDevice->vkEndCommandBuffer(renderPassCmdBuffer);

        VkPipelineStageFlags flag = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        SUBMIT_INFO(qSubmitInfo);
        qSubmitInfo.commandBufferCount = 1;
        qSubmitInfo.pCommandBuffers = &renderPassCmdBuffer;
        qSubmitInfo.waitSemaphoreCount = 1;
        qSubmitInfo.pWaitDstStageMask = &flag;
        qSubmitInfo.pWaitSemaphores = &static_cast<VulkanSemaphore*>(waitSemaphore.get())->semaphore;
        qSubmitInfo.signalSemaphoreCount = 1;
        qSubmitInfo.pSignalSemaphores = &static_cast<VulkanSemaphore*>(renderpassSemaphore.get())->semaphore;

        if (cmdSubmitFence->isSignaled())
        {
            cmdSubmitFence->resetSignal();
        }
        vDevice->vkQueueSubmit(getQueue<EQueueFunction::Graphics>(*deviceQueues, vDevice)->getQueueOfPriority<EQueuePriority::High>()
            , 1, &qSubmitInfo, static_cast<VulkanFence*>(cmdSubmitFence.get())->fence);
    }

    GraphicsHelper::presentImage(renderingApi->getGraphicsInstance(), &canvases, &indices, &presentWaitOn);
    appInstance().appWindowManager.getMainWindow()->updateWindow();

    cmdSubmitFence->waitForSignal();
}

GameEngine* GameEngineWrapper::createEngineInstance()
{
    return new ExperimentalEngine();
}

