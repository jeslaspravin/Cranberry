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
#include "../Core/Types/Textures/RenderTargetTextures.h"
#include "../Core/Types/Textures/Texture2D.h"
#include "../Core/Types/Colors.h"
#include "../Core/Types/Time.h"
#include "../Core/Types/Delegates/Delegate.h"
#include "../Core/Engine/Config/EngineGlobalConfigs.h"
#include "../Core/Input/InputSystem.h"
#include "../Core/Math/Vector2D.h"
#include "../Core/Math/Vector3D.h"

#include <glm/ext/vector_float3.hpp>
#include <array>

void ExperimentalEngine::tempTest()
{

}

void ExperimentalEngine::tempTestPerFrame()
{

}

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
    normalBuffer.buffer = new GraphicsRBuffer(sizeof(Vector3D), 1);
    normalBuffer.buffer->setResourceName("Test_Buffer");
    normalBuffer.buffer->init();
    texelBuffer.buffer = new GraphicsRTexelBuffer(EPixelDataFormat::RG_SF32, 3);
    texelBuffer.buffer->setResourceName("Test_TexelBuffer");
    texelBuffer.buffer->init();
    texelBuffer.bufferView = static_cast<VulkanBufferResource*>(texelBuffer.buffer)->getBufferView(BufferViewInfo());
    graphicsDbg->markObject((uint64)texelBuffer.bufferView, "Test_TexelBufferView", VK_OBJECT_TYPE_BUFFER_VIEW);
}

void ExperimentalEngine::writeBuffers()
{
    IGraphicsInstance* graphicsInst = getRenderApi()->getGraphicsInstance();
    VkCommandBuffer uploadCmdBuffer;
    CMD_BUFFER_ALLOC_INFO(allocationInfo);
    allocationInfo.commandBufferCount = 1;
    allocationInfo.commandPool = pools[EQueueFunction::Transfer].tempCommandsPool;
    fatalAssert(vDevice->vkAllocateCommandBuffers(device, &allocationInfo, &uploadCmdBuffer) == VK_SUCCESS,
        "Failed allocating cmd buffer for copying buffers");
    GraphicsRTexelBuffer stagingResTriVerts = GraphicsRTexelBuffer(EPixelDataFormat::RG_SF32, 3);//cannot access format from buffer as they will be available only in wrapper of buffer.

    // Triangles data to texel buffer
    {
        std::array<Vector2D, 3> triVerts = { Vector2D(0.0f , -0.75f),Vector2D(0.75f , 0.75f), Vector2D(-0.75f , 0.75f) };
        stagingResTriVerts.setAsStagingResource(true);
        stagingResTriVerts.init();

        GraphicsHelper::mapResource(graphicsInst, &stagingResTriVerts);
        memcpy(stagingResTriVerts.getMappedMemory(), triVerts.data(), stagingResTriVerts.getResourceSize());
        GraphicsHelper::unmapResource(graphicsInst, &stagingResTriVerts);
    }

    CMD_BUFFER_BEGIN_INFO(beginInfo);
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vDevice->vkBeginCommandBuffer(uploadCmdBuffer, &beginInfo);
    VkBufferCopy texelCopy{ 0,0, texelBuffer.buffer->getResourceSize() };
    vDevice->vkCmdCopyBuffer(uploadCmdBuffer, stagingResTriVerts.buffer, static_cast<VulkanBufferResource*>(texelBuffer.buffer)->buffer, 1, &texelCopy);
    vDevice->vkEndCommandBuffer(uploadCmdBuffer);

    SUBMIT_INFO(cmdSubmitInfo);
    cmdSubmitInfo.commandBufferCount = 1;
    cmdSubmitInfo.pCommandBuffers = &uploadCmdBuffer;    
    fatalAssert(vDevice->vkQueueSubmit(getQueue<EQueueFunction::Transfer>(*deviceQueues, vDevice)->getQueueOfPriority<EQueuePriority::High>()
        ,1,&cmdSubmitInfo,static_cast<VulkanFence*>(cmdSubmitFence.get())->fence) == VK_SUCCESS, "Failure at submitting copy commands");
    cmdSubmitFence->waitForSignal();
    cmdSubmitFence->resetSignal();

    stagingResTriVerts.release();
    vDevice->vkFreeCommandBuffers(device, pools[EQueueFunction::Transfer].tempCommandsPool, 1, &uploadCmdBuffer);
}

void ExperimentalEngine::destroyBuffers()
{
    normalBuffer.buffer->release();
    delete normalBuffer.buffer;
    normalBuffer.buffer = nullptr;

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
        RenderTextureCreateParams rtCreateParam;
        rtCreateParam.textureSize = EngineSettings::surfaceSize.get();
        rtCreateParam.format = ERenderTargetFormat::RT_U8;
        rtCreateParam.sampleCount = EPixelSampleCount::SampleCount1;
        for (int32 i = 0; i < getApplicationInstance()->appWindowManager
            .getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow())->imagesCount(); ++i)
        {
            const String indexString = std::to_string(i);
            rtCreateParam.textureName = "Test_RT_Texture" + indexString;
            const String textureViewName = "Test_RT_TextureView" + indexString;

            frameResources[i].rtTexture.image = TextureBase::createTexture<RenderTargetTexture>(rtCreateParam);

            frameResources[i].rtTexture.imageView = static_cast<VulkanImageResource*>(
                static_cast<RenderTargetTexture*>(frameResources[i].rtTexture.image)->getRtTexture())->getImageView(ImageViewInfo());

            if (frameResources[i].rtTexture.imageView != nullptr)
            {
                graphicsDbg->markObject((uint64)frameResources[i].rtTexture.imageView, textureViewName, VK_OBJECT_TYPE_IMAGE_VIEW);
            }
        }
    }
    // common shader sampling texture
    {
        Texture2DCreateParams t2dCreateParams
        {
            "Test_Texture",
            Size2D(1024, 1024),
            EPixelSampleCount::SampleCount8
        };
        texture.image = TextureBase::createTexture<Texture2D>(t2dCreateParams);

        texture.imageView = static_cast<VulkanImageResource*>(texture.image->getTextureResource())->getImageView(ImageViewInfo());

        if (texture.imageView != nullptr)
        {
            graphicsDbg->markObject((uint64)texture.imageView, "Test_TextureView", VK_OBJECT_TYPE_IMAGE_VIEW);
        }
    }
}

void ExperimentalEngine::writeImages()
{
    // TODO(Jeslas) : load and write images
}

void ExperimentalEngine::destroyImages()
{
    commonSampler->release();

    TextureBase::destroyTexture<Texture2D>(texture.image);
    texture.image = nullptr;
    texture.imageView = nullptr;

    for (int32 i = 0; i < getApplicationInstance()->appWindowManager
        .getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow())->imagesCount(); ++i)
    {
        TextureBase::destroyTexture<RenderTargetTexture>(frameResources[i].rtTexture.image);
        frameResources[i].rtTexture.image = nullptr;
        frameResources[i].rtTexture.imageView = nullptr;
    }
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
        // Index ordering doesn't matter between layout's binding and pool allocatable descriptors 
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
        descriptorsAndCounts[4].descriptorCount = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow())->imagesCount();

        DESCRIPTOR_POOL_CREATE_INFO(descPoolCreateInfo);
        descPoolCreateInfo.maxSets = 4;
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

        // Index ordering doesn't matter between layout's binding and write descriptors 
        // For vertex shader, vertex float texel buffer.
        WRITE_RESOURCE_TO_DESCRIPTORS_SET(triVertexAsTexelToSet);
        triVertexAsTexelToSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        triVertexAsTexelToSet.dstBinding = 0;
        triVertexAsTexelToSet.dstSet = descriptorsSet;
        triVertexAsTexelToSet.pTexelBufferView = &texelBuffer.bufferView;
        descriptorsToWrite[0] = triVertexAsTexelToSet;
        
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

    int32 frameResCount = getApplicationInstance()->appWindowManager
        .getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow())->imagesCount();
    std::vector<VkWriteDescriptorSet> writeSets(frameResCount);
    std::vector<VkDescriptorImageInfo> imageInfos(frameResCount);

    DESCRIPTOR_SET_ALLOCATE_INFO(descAllocateInfo);
    descAllocateInfo.descriptorPool = descriptorsPool;
    descAllocateInfo.descriptorSetCount = 1;
    descAllocateInfo.pSetLayouts = &subpass1DescLayout;

    for (int32 i = 0; i < frameResCount; ++i)
    {
        if (vDevice->vkAllocateDescriptorSets(device, &descAllocateInfo, &frameResources[i].iAttachSetSubpass1) != VK_SUCCESS)
        {
            Logger::error("ExperimentalEngine", "%s() : Failed creating descriptors set for input attachment for rt at %d in subpass1", __func__,i);
            frameResources[i].iAttachSetSubpass1 = nullptr;
            return;
        }

        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[i].imageView = frameResources[i].rtTexture.imageView;
        imageInfos[i].sampler = nullptr;

        WRITE_RESOURCE_TO_DESCRIPTORS_SET(writeToDescSet);
        writeToDescSet.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        writeToDescSet.dstBinding = 0;
        writeToDescSet.dstSet = frameResources[i].iAttachSetSubpass1;
        writeToDescSet.pImageInfo = &imageInfos[i];
        writeSets[i] = writeToDescSet;
    }

    vDevice->vkUpdateDescriptorSets(device, (uint32)writeSets.size(), writeSets.data() , 0, nullptr);
}

void ExperimentalEngine::destroyInputAttachmentDescriptors()
{
    if (subpass1DescLayout)
    {
        vDevice->vkDestroyDescriptorSetLayout(device, subpass1DescLayout, nullptr);
    }
}

void ExperimentalEngine::createFrameResources()
{
    GenericWindowCanvas* windowCanvas = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow());

    std::vector<VkCommandBuffer> cmdBuffers(windowCanvas->imagesCount());

    CMD_BUFFER_ALLOC_INFO(cmdBufAllocInfo);
    cmdBufAllocInfo.commandPool = pools[EQueueFunction::Graphics].resetableCommandPool;
    cmdBufAllocInfo.commandBufferCount = windowCanvas->imagesCount();
    vDevice->vkAllocateCommandBuffers(device, &cmdBufAllocInfo, cmdBuffers.data());

    for (int32 i = 0; i < windowCanvas->imagesCount(); ++i)
    {
        String name = "Frame";
        name.append(std::to_string(i));

        std::array<VkImageView, 2> attachments = { frameResources[i].rtTexture.imageView
            , static_cast<VulkanWindowCanvas*>(windowCanvas)->swapchainImageView(i) };

        FRAMEBUFFER_CREATE_INFO(frameBufferCreateInfo);
        frameBufferCreateInfo.layers = 1;
        frameBufferCreateInfo.attachmentCount = (uint32)attachments.size();
        frameBufferCreateInfo.pAttachments = attachments.data();
        frameBufferCreateInfo.renderPass = renderPass;
        frameBufferCreateInfo.width = EngineSettings::surfaceSize.get().x;
        frameBufferCreateInfo.height = EngineSettings::surfaceSize.get().y;

        fatalAssert(vDevice->vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameResources[i].frameBuffer) == VK_SUCCESS, "Frame buffer creation failed");
        graphicsDbg->markObject((uint64)frameResources[i].frameBuffer, (name + "FrameBuffer").c_str(), VK_OBJECT_TYPE_FRAMEBUFFER);

        frameResources[i].perFrameCommands = cmdBuffers[i];
        frameResources[i].usageWaitSemaphore.push_back(GraphicsHelper::createSemaphore(getRenderApi()->getGraphicsInstance(), (name + "QueueSubmit").c_str()));
        frameResources[i].recordingFence = GraphicsHelper::createFence(getRenderApi()->getGraphicsInstance(), (name + "RecordingGaurd").c_str(),true);
    }
}

void ExperimentalEngine::destroyFrameResources()
{
    GenericWindowCanvas* windowCanvas = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow());

    std::vector<VkCommandBuffer> cmdBuffers(windowCanvas->imagesCount());
    for (int32 i = 0; i < windowCanvas->imagesCount(); ++i)
    {
        cmdBuffers[i] = frameResources[i].perFrameCommands;
        frameResources[i].usageWaitSemaphore[0]->release();
        frameResources[i].recordingFence->release();
        frameResources[i].iAttachSetSubpass1 = nullptr;
        vDevice->vkDestroyFramebuffer(device, frameResources[i].frameBuffer, nullptr);
        frameResources[i].frameBuffer = nullptr;
        frameResources[i].perFrameCommands = nullptr;
        frameResources[i].usageWaitSemaphore[0].reset();
        frameResources[i].recordingFence.reset();
    }

    vDevice->vkFreeCommandBuffers(device, pools[EQueueFunction::Graphics].resetableCommandPool, (uint32)cmdBuffers.size(), cmdBuffers.data());
}

void ExperimentalEngine::createRenderpass()
{
    GenericWindowCanvas* windowCanvas = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow());

    attachmentsClearColors.resize(2);
    std::array<VkAttachmentDescription, 2> attachmentsDesc;
    {
        VkAttachmentDescription renderTargetAttachment;
        renderTargetAttachment.flags = 0;
        renderTargetAttachment.format = (VkFormat)EPixelDataFormat::getFormatInfo(frameResources[0].rtTexture.image->getFormat())->format;
        renderTargetAttachment.samples = (VkSampleCountFlagBits)frameResources[0].rtTexture.image->getSampleCount();
        renderTargetAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        renderTargetAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        renderTargetAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        renderTargetAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        renderTargetAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        renderTargetAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentsDesc[0] = renderTargetAttachment;
        attachmentsClearColors[0].color = { 0.0f ,0.0f,0.0f,1.0f };// Black

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

    createFrameResources();
    createInputAttachmentDescriptors();
}

void ExperimentalEngine::destroyRenderpass()
{
    destroyInputAttachmentDescriptors();
    destroyFrameResources();
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

    const std::array<Vector3D, 4> quadVerts = { Vector3D(-1,-1,0),Vector3D(1,-1,0),Vector3D(-1,1,0),Vector3D(1,1,0) };
    const std::array<uint32, 6> quadIndices = { 0,3,2,0,1,3 };// 3 Per tri of quad

    quadVertexBuffer = new GraphicsVertexBuffer(sizeof(Vector3D), static_cast<uint32>(quadVerts.size()));
    quadVertexBuffer->setResourceName("ScreenQuadVertices");
    quadVertexBuffer->init();
    quadIndexBuffer = new GraphicsIndexBuffer(sizeof(uint32), static_cast<uint32>(quadIndices.size()));
    quadIndexBuffer->setResourceName("ScreenQuadIndices");
    quadIndexBuffer->init();

    {
        GraphicsVertexBuffer vertStagingBuffer(sizeof(Vector3D), static_cast<uint32>(quadVerts.size()));
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
            cmdSubmitFence->resetSignal();
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
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    VkPushConstantRange pushContant{ VK_SHADER_STAGE_VERTEX_BIT,0,4};// float value for rotation
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushContant;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorsSetLayout;
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
    vertexInputBinding.stride = sizeof(Vector3D);
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
        VK_BLEND_FACTOR_SRC_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
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
    frameResources.resize(getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow())->imagesCount());
    cmdSubmitFence = GraphicsHelper::createFence(getRenderApi()->getGraphicsInstance(), "cmdSubmitFence");

    createBuffers();
    writeBuffers();
    createImages();
    writeImages();
    createPipelineResources();

    tempTest();
}

void ExperimentalEngine::onQuit()
{
    vDevice->vkDeviceWaitIdle(device);

    destroyPipelineResources();

    destroyBuffers();
    destroyImages();

    cmdSubmitFence->release();
    cmdSubmitFence.reset();
    destroyPools();
    GameEngine::onQuit();
}

void ExperimentalEngine::tickEngine()
{
    GameEngine::tickEngine();
    
    if (appInstance().inputSystem()->isKeyPressed(Keys::RMB))
    {
        rotationOffset += timeData.deltaTime * timeData.activeTimeDilation * 0.5f;
    }
    else if(appInstance().inputSystem()->isKeyPressed(Keys::LMB))
    {
        rotationOffset -= timeData.deltaTime * timeData.activeTimeDilation * 0.5f;
    }

    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    viewport.width = (float)EngineSettings::surfaceSize.get().x;
    viewport.height = (float)EngineSettings::surfaceSize.get().y;
    VkRect2D scissor = { {0,0},{EngineSettings::surfaceSize.get().x,EngineSettings::surfaceSize.get().y} };

    SharedPtr<GraphicsSemaphore> waitSemaphore;
    uint32 index = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow())->requestNextImage(&waitSemaphore, nullptr);
    std::vector<GenericWindowCanvas*> canvases = { getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow()) };
    std::vector<uint32> indices = { index };

    if (!frameResources[index].recordingFence->isSignaled())
    {
        frameResources[index].recordingFence->waitForSignal();
    }
    frameResources[index].recordingFence->resetSignal();

    CMD_BUFFER_BEGIN_INFO(cmdBeginInfo);
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vDevice->vkBeginCommandBuffer(frameResources[index].perFrameCommands, &cmdBeginInfo);
    {
        SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, ExperimentalEngineFrame);

        RENDERPASS_BEGIN_INFO(renderPassBeginInfo);
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = frameResources[index].frameBuffer;
        renderPassBeginInfo.pClearValues = attachmentsClearColors.data();
        renderPassBeginInfo.clearValueCount = (uint32)attachmentsClearColors.size();
        renderPassBeginInfo.renderArea.offset = { 0,0 };
        renderPassBeginInfo.renderArea.extent = scissor.extent;

        vDevice->vkCmdBeginRenderPass(frameResources[index].perFrameCommands, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, Subpass_0);

            vDevice->vkCmdSetViewport(frameResources[index].perFrameCommands, 0, 1, &viewport);
            vDevice->vkCmdSetScissor(frameResources[index].perFrameCommands, 0, 1, &scissor);
            vDevice->vkCmdBindPipeline(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS, drawTriPipeline.pipeline);
            vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS, drawTriPipeline.layout, 0, 1, &descriptorsSet, 0, nullptr);
            vDevice->vkCmdPushConstants(frameResources[index].perFrameCommands, drawTriPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 4, &rotationOffset);
            vDevice->vkCmdDraw(frameResources[index].perFrameCommands, 3, 1, 0, 0);
        }
        vDevice->vkCmdNextSubpass(frameResources[index].perFrameCommands, VK_SUBPASS_CONTENTS_INLINE);
        {
            SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, Subpass_1);

            vDevice->vkCmdBindPipeline(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS, drawQuadPipeline.pipeline);
            vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS, drawQuadPipeline.layout, 0, 1, &frameResources[index].iAttachSetSubpass1, 0, nullptr);
            uint64 vertexBufferOffset = 0;
            vDevice->vkCmdBindVertexBuffers(frameResources[index].perFrameCommands, 0, 1, &static_cast<VulkanBufferResource*>(quadVertexBuffer)->buffer, &vertexBufferOffset);
            vDevice->vkCmdBindIndexBuffer(frameResources[index].perFrameCommands, static_cast<VulkanBufferResource*>(quadIndexBuffer)->buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

            vDevice->vkCmdDrawIndexed(frameResources[index].perFrameCommands, 6, 1, 0, 0, 0);

            vDevice->vkCmdEndRenderPass(frameResources[index].perFrameCommands);
        }
    }
    vDevice->vkEndCommandBuffer(frameResources[index].perFrameCommands);

    VkPipelineStageFlags flag = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    SUBMIT_INFO(qSubmitInfo);
    qSubmitInfo.commandBufferCount = 1;
    qSubmitInfo.pCommandBuffers = &frameResources[index].perFrameCommands;
    qSubmitInfo.waitSemaphoreCount = 1;
    qSubmitInfo.pWaitDstStageMask = &flag;
    qSubmitInfo.pWaitSemaphores = &static_cast<VulkanSemaphore*>(waitSemaphore.get())->semaphore;
    qSubmitInfo.signalSemaphoreCount = 1;
    qSubmitInfo.pSignalSemaphores = &static_cast<VulkanSemaphore*>(frameResources[index].usageWaitSemaphore[0].get())->semaphore;

    vDevice->vkQueueSubmit(getQueue<EQueueFunction::Graphics>(*deviceQueues, vDevice)->getQueueOfPriority<EQueuePriority::High>()
        , 1, &qSubmitInfo, static_cast<VulkanFence*>(frameResources[index].recordingFence.get())->fence);

    GraphicsHelper::presentImage(renderingApi->getGraphicsInstance(), &canvases, &indices, &frameResources[index].usageWaitSemaphore);

    tempTestPerFrame();
}

GameEngine* GameEngineWrapper::createEngineInstance()
{
    return new ExperimentalEngine();
}