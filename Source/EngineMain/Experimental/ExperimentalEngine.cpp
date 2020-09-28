#include "ExperimentalEngine.h"

#include "../RenderInterface/Shaders/EngineShaders/GoochModel.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanQueueResource.h"
#include "../VulkanRI/VulkanInternals/Debugging.h"
#include "../RenderInterface/PlatformIndependentHeaders.h"
#include "../RenderInterface/PlatformIndependentHelper.h"
#include "../Core/Platform/PlatformAssertionErrors.h"
#include "../VulkanRI/VulkanInternals/VulkanDescriptorAllocator.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanSampler.h"
#include "../Core/Math/Vector3D.h"
#include "../RenderInterface/Rendering/IRenderCommandList.h"
#include "../Assets/Asset/TextureAsset.h"
#include "../Core/Types/Textures/TexturesBase.h"
#include "../RenderApi/GBuffersAndTextures.h"
#include "../Core/Input/Keys.h"
#include "../Core/Engine/Config/EngineGlobalConfigs.h"
#include "../Assets/Asset/StaticMeshAsset.h"
#include "../VulkanRI/VulkanGraphicsHelper.h"
#include "../RenderApi/RenderApi.h"
#include "../Core/Platform/GenericAppInstance.h"
#include "../Core/Input/InputSystem.h"
#include "../Core/Math/Math.h"
#include "../RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "../RenderInterface/CoreGraphicsTypes.h"
#include "../VulkanRI/VulkanInternals/ShaderCore/VulkanShaderParamResources.h"
#include "../RenderApi/Scene/RenderScene.h"
#include "../RenderApi/Material/MaterialCommonUniforms.h"
#include "../Core/Math/RotationMatrix.h"

#include <array>

void ExperimentalEngine::tempTest()
{

}

void ExperimentalEngine::tempTestPerFrame()
{

}

template <EQueueFunction QueueFunction> VulkanQueueResource<QueueFunction>* getQueue(const VulkanDevice* device);

void ExperimentalEngine::createPools()
{
    {
        VulkanQueueResource<EQueueFunction::Compute>* queue = getQueue<EQueueFunction::Compute>(vDevice);
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
        VulkanQueueResource<EQueueFunction::Graphics>* queue = getQueue<EQueueFunction::Graphics>(vDevice);
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
        VulkanQueueResource<EQueueFunction::Transfer>* queue = getQueue<EQueueFunction::Transfer>(vDevice);
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
        VulkanQueueResource<EQueueFunction::Present>* queue = getQueue<EQueueFunction::Present>(vDevice);
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
    for (const std::pair<EQueueFunction, QueueCommandPool>& pool : pools)
    {
        vDevice->vkDestroyCommandPool(device, pool.second.oneTimeRecordPool, nullptr);
        vDevice->vkDestroyCommandPool(device, pool.second.resetableCommandPool, nullptr);
        vDevice->vkDestroyCommandPool(device, pool.second.tempCommandsPool, nullptr);
    }
}

void ExperimentalEngine::createBuffers()
{
    const PipelineBase* smPipeline = static_cast<const GraphicsPipelineBase*>(drawSmDefaultPipelineContext.getPipeline());
    const ShaderSetParametersLayout* paramSetLayout = static_cast<const ShaderSetParametersLayout*>(smPipeline->getParamLayoutAtSet(0));// 0 is view data

    const ShaderBufferDescriptorType* paramDescription = static_cast<const ShaderBufferDescriptorType*>(paramSetLayout->parameterDescription("viewData"));
    viewBuffer.buffer = new GraphicsRBuffer(paramDescription->bufferParamInfo->paramStride(), 1);
    viewBuffer.buffer->setResourceName("ViewData");
    viewBuffer.buffer->init();

    paramSetLayout = static_cast<const ShaderSetParametersLayout*>(smPipeline->getParamLayoutAtSet(1));// 1 is instance data
    paramDescription = static_cast<const ShaderBufferDescriptorType*>(paramSetLayout->parameterDescription("instanceData"));
    instanceBuffer.buffer = new GraphicsRBuffer(paramDescription->bufferParamInfo->paramStride(), 1);
    instanceBuffer.buffer->setResourceName("InstanceData");
    instanceBuffer.buffer->init();

    smPipeline = static_cast<const GraphicsPipelineBase*>(drawSmGoochPipelineContext.getPipeline());
    paramSetLayout = static_cast<const ShaderSetParametersLayout*>(smPipeline->getParamLayoutAtSet(2));// 2 is shader data
    paramDescription = static_cast<const ShaderBufferDescriptorType*>(paramSetLayout->parameterDescription("surfaceData"));
    goochSurfaceDataBuffer.buffer = new GraphicsRBuffer(paramDescription->bufferParamInfo->paramStride(), 1);
    goochSurfaceDataBuffer.buffer->setResourceName("GoochSurfaceData");
    goochSurfaceDataBuffer.buffer->init();
}

void ExperimentalEngine::destroyBuffers()
{
    viewBuffer.buffer->release();
    delete viewBuffer.buffer;
    viewBuffer.buffer = nullptr;

    instanceBuffer.buffer->release();
    delete instanceBuffer.buffer;
    instanceBuffer.buffer = nullptr;

    goochSurfaceDataBuffer.buffer->release();
    delete goochSurfaceDataBuffer.buffer;
    goochSurfaceDataBuffer.buffer = nullptr;
}

void ExperimentalEngine::createImages()
{
    commonSampler = GraphicsHelper::createSampler(gEngine->getRenderApi()->getGraphicsInstance(), "CommonSampler",
        ESamplerTilingMode::Repeat, ESamplerFiltering::Nearest);

    // common shader sampling texture
    {
        texture.image = static_cast<TextureAsset*>(appInstance().assetManager.getOrLoadAsset("TestImageData.png"))->getTexture();
        texture.imageView = static_cast<VulkanImageResource*>(texture.image->getTextureResource())->getImageView({});

        if (texture.imageView != nullptr)
        {
            graphicsDbg->markObject((uint64)texture.imageView, "DiffuseTextureView", VK_OBJECT_TYPE_IMAGE_VIEW);
        }
    }
}

void ExperimentalEngine::destroyImages()
{
    commonSampler->release();
}

void ExperimentalEngine::createShaderResDescriptors()
{
    uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();
    VulkanDescriptorsSetAllocator* descsSetAllocator = VulkanGraphicsHelper::getDescriptorsSetAllocator(gEngine->getRenderApi()->getGraphicsInstance());
    // Static mesh unlit rendering
    {
        staticMeshDescs.clear();

        const PipelineBase* smPipeline = static_cast<const GraphicsPipelineBase*>(drawSmDefaultPipelineContext.getPipeline());
        const ShaderReflected* reflectedData = smPipeline->getShaderResource()->getReflection();

        staticMeshDescs.reserve(reflectedData->descriptorsSets.size());
        for (const ReflectDescriptorBody& descriptorsSet : reflectedData->descriptorsSets)
        {
            const VulkanShaderSetParamsLayout* paramSetLayout = static_cast<const VulkanShaderSetParamsLayout*>(smPipeline->getParamLayoutAtSet(descriptorsSet.set));

            VkDescriptorSet descSet;
            DescriptorsSetQuery query;
            query.supportedTypes.insert(paramSetLayout->getDescPoolAllocInfo().cbegin(), paramSetLayout->getDescPoolAllocInfo().cend());
            descSet = descsSetAllocator->allocDescriptorsSet(query, paramSetLayout->descriptorLayout);

            staticMeshDescs.push_back(descSet);
        }
    }
    // Gooch model rendering
    {
        const PipelineBase* smPipeline = static_cast<const GraphicsPipelineBase*>(drawSmGoochPipelineContext.getPipeline());
        // 2 has shader specific descriptors set layout
        const VulkanShaderSetParamsLayout* paramSetLayout = static_cast<const VulkanShaderSetParamsLayout*>(smPipeline->getParamLayoutAtSet(2));

        VkDescriptorSet descSet;
        DescriptorsSetQuery query;
        query.supportedTypes.insert(paramSetLayout->getDescPoolAllocInfo().cbegin(), paramSetLayout->getDescPoolAllocInfo().cend());
        descSet = descsSetAllocator->allocDescriptorsSet(query, paramSetLayout->descriptorLayout);

        staticMeshDescs.push_back(descSet);
    }

    // Drawing textures to quad
    {
        drawQuadTextureDescs.resize(swapchainCount);
        drawQuadNormalDescs.resize(swapchainCount);
        drawQuadDepthDescs.resize(swapchainCount);

        const PipelineBase* drawQuadPipeline = static_cast<const GraphicsPipelineBase*>(drawQuadPipelineContext.getPipeline());
        const VulkanShaderParametersLayout* paramsLayout = static_cast<const VulkanShaderParametersLayout*>(drawQuadPipeline->getParamLayoutAtSet(0));
        const ShaderReflected* reflectedData = drawQuadPipeline->getShaderResource()->getReflection();

        for (const ReflectDescriptorBody& descriptorsSet : reflectedData->descriptorsSets)
        {
            const std::vector<VkDescriptorPoolSize> descriptorsPoolSize = paramsLayout->getDescPoolAllocInfo(descriptorsSet.set);
            DescriptorsSetQuery query;
            query.supportedTypes.insert(descriptorsPoolSize.cbegin(), descriptorsPoolSize.cend());

            VkDescriptorSetLayout setLayout = paramsLayout->getDescSetLayout(descriptorsSet.set);

            for (uint32 i = 0; i < swapchainCount; ++i)
            {
                drawQuadTextureDescs[i].emplace_back(descsSetAllocator->allocDescriptorsSet(query, setLayout));

                drawQuadNormalDescs[i].emplace_back(descsSetAllocator->allocDescriptorsSet(query, setLayout));

                drawQuadDepthDescs[i].emplace_back(descsSetAllocator->allocDescriptorsSet(query, setLayout));
            }
        }
    }

    std::vector<std::pair<VkWriteDescriptorSet, uint32>> writingBufferDescriptors;
    std::vector<VkDescriptorBufferInfo> bufferInfo;
    std::vector<std::pair<VkWriteDescriptorSet, uint32>> writingImageDescriptors;
    std::vector<VkDescriptorImageInfo> imageInfo;

    // Static mesh descriptors
    {
        const PipelineBase* smPipeline = static_cast<const GraphicsPipelineBase*>(drawSmDefaultPipelineContext.getPipeline());
        const ShaderReflected* reflectedData = smPipeline->getShaderResource()->getReflection();

        for (const ReflectDescriptorBody& descriptorsSet : reflectedData->descriptorsSets)
        {
            const ShaderSetParametersLayout* setParamsLayout = static_cast<const ShaderSetParametersLayout*>(smPipeline->getParamLayoutAtSet(descriptorsSet.set));
            const ShaderBufferDescriptorType* paramDescription = static_cast<const ShaderBufferDescriptorType*>(setParamsLayout->parameterDescription("viewData"));

            if (paramDescription != nullptr)
            {
                uint32 bufferInfoIdx = uint32(bufferInfo.size());
                bufferInfo.push_back({});
                bufferInfo[bufferInfoIdx].buffer = static_cast<VulkanBufferResource*>(viewBuffer.buffer)->buffer;
                bufferInfo[bufferInfoIdx].offset = 0;
                bufferInfo[bufferInfoIdx].range = viewBuffer.buffer->getResourceSize();

                WRITE_RESOURCE_TO_DESCRIPTORS_SET(viewDataDescWrite);
                viewDataDescWrite.dstSet = staticMeshDescs[descriptorsSet.set];
                viewDataDescWrite.descriptorType = VkDescriptorType(paramDescription->bufferEntryPtr->data.type);
                viewDataDescWrite.dstBinding = paramDescription->bufferEntryPtr->data.binding;
                writingBufferDescriptors.push_back({ viewDataDescWrite, bufferInfoIdx });
            }

            paramDescription = static_cast<const ShaderBufferDescriptorType*>(setParamsLayout->parameterDescription("instanceData"));
            if (paramDescription != nullptr)
            {
                uint32 bufferInfoIdx = uint32(bufferInfo.size());
                bufferInfo.push_back({});
                bufferInfo[bufferInfoIdx].buffer = static_cast<VulkanBufferResource*>(instanceBuffer.buffer)->buffer;
                bufferInfo[bufferInfoIdx].offset = 0;
                bufferInfo[bufferInfoIdx].range = instanceBuffer.buffer->getResourceSize();

                WRITE_RESOURCE_TO_DESCRIPTORS_SET(instanceDataDescWrite);
                instanceDataDescWrite.dstSet = staticMeshDescs[descriptorsSet.set];
                instanceDataDescWrite.descriptorType = VkDescriptorType(paramDescription->bufferEntryPtr->data.type);
                instanceDataDescWrite.dstBinding = paramDescription->bufferEntryPtr->data.binding;
                writingBufferDescriptors.push_back({ instanceDataDescWrite, bufferInfoIdx });
            }
        }
    }
    // Gooch model rendering
    {
        const PipelineBase* smPipeline = static_cast<const GraphicsPipelineBase*>(drawSmGoochPipelineContext.getPipeline());
        // 2 has shader specific descriptors set layout
        const VulkanShaderSetParamsLayout* paramSetLayout = static_cast<const VulkanShaderSetParamsLayout*>(smPipeline->getParamLayoutAtSet(2));
        const ShaderBufferDescriptorType* paramDescription = static_cast<const ShaderBufferDescriptorType*>(paramSetLayout->parameterDescription("surfaceData"));

        debugAssert(paramDescription);

        uint32 bufferInfoIdx = uint32(bufferInfo.size());
        bufferInfo.push_back({});
        bufferInfo[bufferInfoIdx].buffer = static_cast<VulkanBufferResource*>(goochSurfaceDataBuffer.buffer)->buffer;
        bufferInfo[bufferInfoIdx].offset = 0;
        bufferInfo[bufferInfoIdx].range = goochSurfaceDataBuffer.buffer->getResourceSize();

        WRITE_RESOURCE_TO_DESCRIPTORS_SET(instanceDataDescWrite);
        instanceDataDescWrite.dstSet = staticMeshDescs[2];
        instanceDataDescWrite.descriptorType = VkDescriptorType(paramDescription->bufferEntryPtr->data.type);
        instanceDataDescWrite.dstBinding = paramDescription->bufferEntryPtr->data.binding;
        writingBufferDescriptors.push_back({ instanceDataDescWrite, bufferInfoIdx });
    }

    // Draw quad descriptors
    {
        const PipelineBase* smPipeline = static_cast<const GraphicsPipelineBase*>(drawQuadPipelineContext.getPipeline());
        const VulkanShaderParametersLayout* paramsLayout = static_cast<const VulkanShaderParametersLayout*>(smPipeline->getParamLayoutAtSet(0));
        uint32 paramSetIdx;
        const ShaderTextureDescriptorType* textureDescription = static_cast<const ShaderTextureDescriptorType*>(paramsLayout->parameterDescription(paramSetIdx, "quadTexture"));

        if (textureDescription != nullptr)
        {
            FramebufferFormat multibufferFormat(ERenderPassFormat::Multibuffers);
            for (uint32 swapchainIdx = 0; swapchainIdx < swapchainCount; ++swapchainIdx)
            {
                WRITE_RESOURCE_TO_DESCRIPTORS_SET(quadTextureDescWrite);
                quadTextureDescWrite.descriptorType = VkDescriptorType(textureDescription->textureEntryPtr->data.type);
                quadTextureDescWrite.dstBinding = textureDescription->textureEntryPtr->data.binding;

                Framebuffer* fb = GBuffers::getFramebuffer(multibufferFormat, swapchainIdx);
                fatalAssert(fb != nullptr, "Framebuffer is invalid");

                uint32 imageInfoIdx = uint32(imageInfo.size());
                imageInfo.push_back({});
                imageInfo[imageInfoIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo[imageInfoIdx].imageView = static_cast<VulkanImageResource*>(fb->textures[1])->getImageView({});// Diffuse is at 0
                imageInfo[imageInfoIdx].sampler = static_cast<VulkanSampler*>(commonSampler.get())->sampler;

                quadTextureDescWrite.dstSet = drawQuadTextureDescs[swapchainIdx][paramSetIdx];
                writingImageDescriptors.push_back({ quadTextureDescWrite, imageInfoIdx });

                imageInfoIdx = uint32(imageInfo.size());
                imageInfo.push_back({});
                imageInfo[imageInfoIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo[imageInfoIdx].imageView = static_cast<VulkanImageResource*>(fb->textures[3])->getImageView({});// Normal texture is at 1
                imageInfo[imageInfoIdx].sampler = static_cast<VulkanSampler*>(commonSampler.get())->sampler;

                quadTextureDescWrite.dstSet = drawQuadNormalDescs[swapchainIdx][paramSetIdx];
                writingImageDescriptors.push_back({ quadTextureDescWrite, imageInfoIdx });

                imageInfoIdx = uint32(imageInfo.size());
                imageInfo.push_back({});
                imageInfo[imageInfoIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo[imageInfoIdx].imageView = static_cast<VulkanImageResource*>(fb->textures[5])->getImageView({
                    {EPixelComponentMapping::SameComponent, EPixelComponentMapping::R,EPixelComponentMapping::R,EPixelComponentMapping::R} });// Depth is at 2
                imageInfo[imageInfoIdx].sampler = static_cast<VulkanSampler*>(commonSampler.get())->sampler;

                quadTextureDescWrite.dstSet = drawQuadDepthDescs[swapchainIdx][paramSetIdx];
                writingImageDescriptors.push_back({ quadTextureDescWrite, imageInfoIdx });
            }
        }
    }

    std::vector<VkWriteDescriptorSet> writingDescriptors;
    writingDescriptors.reserve(writingBufferDescriptors.size() + writingImageDescriptors.size());
    for (std::pair<VkWriteDescriptorSet, uint32>& writingBufferDesc : writingBufferDescriptors)
    {
        writingBufferDesc.first.pBufferInfo = &bufferInfo[writingBufferDesc.second];
        writingDescriptors.push_back(writingBufferDesc.first);
    }
    for (std::pair<VkWriteDescriptorSet, uint32>& writingImageDesc : writingImageDescriptors)
    {
        writingImageDesc.first.pImageInfo = &imageInfo[writingImageDesc.second];
        writingDescriptors.push_back(writingImageDesc.first);
    }

    vDevice->vkUpdateDescriptorSets(device, uint32(writingDescriptors.size()), writingDescriptors.data(), 0, nullptr);
}

void ExperimentalEngine::writeUnlitBuffToQuadDrawDescs()
{
    uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();
    std::vector<std::pair<VkWriteDescriptorSet, uint32>> writingImageDescriptors;
    std::vector<VkDescriptorImageInfo> imageInfo;

    // Draw quad descriptors
    {
        const PipelineBase* smPipeline = static_cast<const GraphicsPipelineBase*>(drawQuadPipelineContext.getPipeline());
        const VulkanShaderParametersLayout* paramsLayout = static_cast<const VulkanShaderParametersLayout*>(smPipeline->getParamLayoutAtSet(0));
        uint32 paramSetIdx;
        const ShaderTextureDescriptorType* textureDescription = static_cast<const ShaderTextureDescriptorType*>(paramsLayout->parameterDescription(paramSetIdx, "quadTexture"));

        if (textureDescription != nullptr)
        {
            FramebufferFormat multibufferFormat(ERenderPassFormat::Multibuffers);
            for (uint32 swapchainIdx = 0; swapchainIdx < swapchainCount; ++swapchainIdx)
            {
                WRITE_RESOURCE_TO_DESCRIPTORS_SET(quadTextureDescWrite);
                quadTextureDescWrite.descriptorType = VkDescriptorType(textureDescription->textureEntryPtr->data.type);
                quadTextureDescWrite.dstBinding = textureDescription->textureEntryPtr->data.binding;

                Framebuffer* fb = GBuffers::getFramebuffer(multibufferFormat, swapchainIdx);
                fatalAssert(fb != nullptr, "Framebuffer is invalid");

                uint32 imageInfoIdx = uint32(imageInfo.size());
                imageInfo.push_back({});
                imageInfo[imageInfoIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo[imageInfoIdx].imageView = static_cast<VulkanImageResource*>(fb->textures[1])->getImageView({});// Diffuse is at 0
                imageInfo[imageInfoIdx].sampler = static_cast<VulkanSampler*>(commonSampler.get())->sampler;

                quadTextureDescWrite.dstSet = drawQuadTextureDescs[swapchainIdx][paramSetIdx];
                writingImageDescriptors.push_back({ quadTextureDescWrite, imageInfoIdx });

                imageInfoIdx = uint32(imageInfo.size());
                imageInfo.push_back({});
                imageInfo[imageInfoIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo[imageInfoIdx].imageView = static_cast<VulkanImageResource*>(fb->textures[3])->getImageView({});// Normal texture is at 1
                imageInfo[imageInfoIdx].sampler = static_cast<VulkanSampler*>(commonSampler.get())->sampler;

                quadTextureDescWrite.dstSet = drawQuadNormalDescs[swapchainIdx][paramSetIdx];
                writingImageDescriptors.push_back({ quadTextureDescWrite, imageInfoIdx });

                imageInfoIdx = uint32(imageInfo.size());
                imageInfo.push_back({});
                imageInfo[imageInfoIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo[imageInfoIdx].imageView = static_cast<VulkanImageResource*>(fb->textures[5])->getImageView({
                    {EPixelComponentMapping::SameComponent, EPixelComponentMapping::R,EPixelComponentMapping::R,EPixelComponentMapping::R} });// Depth is at 2
                imageInfo[imageInfoIdx].sampler = static_cast<VulkanSampler*>(commonSampler.get())->sampler;

                quadTextureDescWrite.dstSet = drawQuadDepthDescs[swapchainIdx][paramSetIdx];
                writingImageDescriptors.push_back({ quadTextureDescWrite, imageInfoIdx });
            }
        }
    }

    std::vector<VkWriteDescriptorSet> writingDescriptors;
    for (std::pair<VkWriteDescriptorSet, uint32>& writingImageDesc : writingImageDescriptors)
    {
        writingImageDesc.first.pImageInfo = &imageInfo[writingImageDesc.second];
        writingDescriptors.push_back(writingImageDesc.first);
    }

    vDevice->vkUpdateDescriptorSets(device, uint32(writingDescriptors.size()), writingDescriptors.data(), 0, nullptr);
}

void ExperimentalEngine::destroyShaderResDescriptors()
{
    VulkanDescriptorsSetAllocator* descsSetAllocator = VulkanGraphicsHelper::getDescriptorsSetAllocator(gEngine->getRenderApi()->getGraphicsInstance());

    for (VkDescriptorSet& descSet : staticMeshDescs)
    {
        descsSetAllocator->releaseDescriptorsSet(descSet);
    }
    staticMeshDescs.clear();

    for (std::vector<VkDescriptorSet>& descSets : drawQuadTextureDescs)
    {
        for(VkDescriptorSet& descSet : descSets)
        {
            descsSetAllocator->releaseDescriptorsSet(descSet);
        }
    }
    drawQuadTextureDescs.clear();
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
        frameResources[i].perFrameCommands = nullptr;
        frameResources[i].usageWaitSemaphore[0].reset();
        frameResources[i].recordingFence.reset();
    }

    vDevice->vkFreeCommandBuffers(device, pools[EQueueFunction::Graphics].resetableCommandPool, (uint32)cmdBuffers.size(), cmdBuffers.data());
}

void ExperimentalEngine::getPipelineForSubpass()
{
    VulkanGlobalRenderingContext* vulkanRenderingContext = static_cast<VulkanGlobalRenderingContext*>(getRenderApi()->getGlobalRenderingContext());

    drawSmDefaultPipelineContext.forVertexType = EVertexType::StaticMesh;
    drawSmDefaultPipelineContext.materialName = DEFAULT_SHADER_NAME;
    drawSmDefaultPipelineContext.renderpassFormat = ERenderPassFormat::Multibuffers;
    drawSmDefaultPipelineContext.swapchainIdx = 0;
    vulkanRenderingContext->preparePipelineContext(&drawSmDefaultPipelineContext);
    drawSmRenderPass = vulkanRenderingContext->getRenderPass(drawSmDefaultPipelineContext.renderpassFormat, {});

    // Gooch model
    drawSmGoochPipelineContext.forVertexType = EVertexType::StaticMesh;
    drawSmGoochPipelineContext.renderpassFormat = ERenderPassFormat::Multibuffers;
    drawSmGoochPipelineContext.swapchainIdx = 0;
    drawSmGoochPipelineContext.materialName = "GoochModel";
    vulkanRenderingContext->preparePipelineContext(&drawSmGoochPipelineContext);

    RenderPassAdditionalProps renderPassAdditionalProps;
    renderPassAdditionalProps.bUsedAsPresentSource = true;

    drawQuadPipelineContext.bUseSwapchainFb = true;
    drawQuadPipelineContext.materialName = "DrawQuadFromTexture";
    drawQuadPipelineContext.renderpassFormat = ERenderPassFormat::Generic;
    drawQuadPipelineContext.swapchainIdx = 0;
    vulkanRenderingContext->preparePipelineContext(&drawQuadPipelineContext);
    drawQuadRenderPass = vulkanRenderingContext->getRenderPass(
        static_cast<const GraphicsPipelineBase*>(drawQuadPipelineContext.getPipeline())->getRenderpassProperties(), renderPassAdditionalProps);
}

void ExperimentalEngine::createPipelineResources()
{
    VkClearValue baseClearValue;
    baseClearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
    baseClearValue.depthStencil.depth = 0;
    baseClearValue.depthStencil.stencil = 0;
    smAttachmentsClearColors.resize(drawSmDefaultPipelineContext.getFb()->textures.size(), baseClearValue);
    swapchainClearColor = baseClearValue;

    ENQUEUE_COMMAND(QuadVerticesInit,LAMBDA_BODY
        (
            const std::array<Vector3D, 4> quadVerts = { Vector3D(-1,-1,0),Vector3D(1,-1,0),Vector3D(-1,1,0),Vector3D(1,1,0) };
            const std::array<uint32, 6> quadIndices = { 0,3,2,0,1,3 };// 3 Per tri of quad

            quadVertexBuffer = new GraphicsVertexBuffer(sizeof(Vector3D), static_cast<uint32>(quadVerts.size()));
            quadVertexBuffer->setResourceName("ScreenQuadVertices");
            quadVertexBuffer->init();
            quadIndexBuffer = new GraphicsIndexBuffer(sizeof(uint32), static_cast<uint32>(quadIndices.size()));
            quadIndexBuffer->setResourceName("ScreenQuadIndices");
            quadIndexBuffer->init();

            cmdList->copyToBuffer(quadVertexBuffer, 0, quadVerts.data(), uint32(quadVertexBuffer->getResourceSize()));
            cmdList->copyToBuffer(quadIndexBuffer, 0, quadIndices.data(), uint32(quadIndexBuffer->getResourceSize()));
        )
        , this);

    // Shader pipeline's buffers and image access
    createShaderResDescriptors();
}

void ExperimentalEngine::destroyPipelineResources()
{
    ENQUEUE_COMMAND(QuadVerticesRelease,LAMBDA_BODY
        (
            quadVertexBuffer->release();
            quadIndexBuffer->release();
            delete quadVertexBuffer;
            quadVertexBuffer = nullptr;
            delete quadIndexBuffer;
            quadIndexBuffer = nullptr;
        )
    , this);
    // Shader pipeline's buffers and image access
    destroyShaderResDescriptors();
}

void ExperimentalEngine::writeBuffers()
{
    ViewData viewData;
    viewData.projection = camera.projectionMatrix();
    viewData.invProjection = viewData.projection.inverse();
    viewData.view = camera.viewMatrix();
    viewData.invView = viewData.view.inverse();

    InstanceData instanceData;
    instanceData.model = Transform3D(modelRotation).getTransformMatrix();
    instanceData.invModel = instanceData.model.inverse();

    SurfaceData surfaceData;
    surfaceData.lightPos = Transform3D(Rotation(0, 0, lightRotation)).transformPoint(lightTranslation);
    surfaceData.highlightColor = Vector4D(1, 1, 1, 1);
    surfaceData.surfaceColor = Vector4D(0.80f, 0.78f, 0.60f, 1.0f);

    ENQUEUE_COMMAND(WritingUniforms,
        {
            const ShaderSetParametersLayout * setParamsLayout = static_cast<const ShaderSetParametersLayout*>(drawSmDefaultPipelineContext.getPipeline()->getParamLayoutAtSet(0));
            const ShaderBufferDescriptorType * bufferDesc = static_cast<const ShaderBufferDescriptorType*>(setParamsLayout->parameterDescription("viewData"));
            cmdList->copyToBuffer<ViewData>(viewBuffer.buffer, 0, &viewData, bufferDesc->bufferParamInfo);

            setParamsLayout = static_cast<const ShaderSetParametersLayout*>(drawSmDefaultPipelineContext.getPipeline()->getParamLayoutAtSet(1));
            bufferDesc = static_cast<const ShaderBufferDescriptorType*>(setParamsLayout->parameterDescription("instanceData"));
            cmdList->copyToBuffer<InstanceData>(instanceBuffer.buffer, 0, &instanceData, bufferDesc->bufferParamInfo);

            setParamsLayout = static_cast<const ShaderSetParametersLayout*>(drawSmGoochPipelineContext.getPipeline()->getParamLayoutAtSet(2));
            bufferDesc = static_cast<const ShaderBufferDescriptorType*>(setParamsLayout->parameterDescription("surfaceData"));
            cmdList->copyToBuffer<SurfaceData>(goochSurfaceDataBuffer.buffer, 0, &surfaceData, bufferDesc->bufferParamInfo);
        }
    , this, viewData, instanceData, surfaceData);
}

void ExperimentalEngine::updateCameraParams()
{
    if (appInstance().inputSystem()->isKeyPressed(Keys::RMB))
    {
        cameraRotation.yaw() += appInstance().inputSystem()->analogState(AnalogStates::RelMouseX)->currentValue * timeData.deltaTime * timeData.activeTimeDilation * 15.0f;
        cameraRotation.pitch() += appInstance().inputSystem()->analogState(AnalogStates::RelMouseY)->currentValue * timeData.deltaTime * timeData.activeTimeDilation * 15.0f;
    }

    if (appInstance().inputSystem()->isKeyPressed(Keys::A))
    {
        cameraTranslation -= cameraRotation.rightVector() * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::D))
    {
        cameraTranslation += cameraRotation.rightVector() * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::W))
    {
        cameraTranslation += cameraRotation.fwdVector() * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::S))
    {
        cameraTranslation -= cameraRotation.fwdVector() * timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::Q))
    {
        useVertexColor = Math::min(useVertexColor + timeData.deltaTime * timeData.activeTimeDilation, 1.0f);
    }
    else
    {
        useVertexColor = Math::max(useVertexColor - timeData.deltaTime * timeData.activeTimeDilation, 0.0f);
    }
    if (appInstance().inputSystem()->keyState(Keys::P)->keyWentUp)
    {
        camera.cameraProjection = camera.cameraProjection == ECameraProjection::Perspective ? ECameraProjection::Orthographic : ECameraProjection::Perspective;
    }
    if (appInstance().inputSystem()->keyState(Keys::X)->keyWentUp)
    {
        toggleRes = !toggleRes;
        ENQUEUE_COMMAND(WritingDescs,
            {
                const Size2D & screenSize = toggleRes ? EngineSettings::surfaceSize.get() : Size2D(1280, 720);
                GBuffers::onScreenResized(screenSize);
                writeUnlitBuffToQuadDrawDescs();
                EngineSettings::screenSize.set(screenSize);
            }, this);
    }
    if (appInstance().inputSystem()->keyState(Keys::LSHIFT)->keyWentUp)
    {
        useSuzanne = !useSuzanne;
    }
    if (appInstance().inputSystem()->keyState(Keys::R)->keyWentUp)
    {
        cameraRotation = Rotation();
    }

    camera.setRotation(cameraRotation);
    camera.setTranslation(cameraTranslation);

    //modelRotation.pitch() += timeData.deltaTime * timeData.activeTimeDilation * 15.0f;
    //modelRotation.roll() -= timeData.deltaTime * timeData.activeTimeDilation * 30.0f;
    lightRotation += timeData.deltaTime * timeData.activeTimeDilation * 90.0f;

    AssetHeader staticMeshHeader;
    staticMeshHeader.type = EAssetType::StaticMesh;
    if (useSuzanne)
    {
        staticMeshHeader.assetPath = FileSystemFunctions::combinePath(FileSystemFunctions::applicationDirectory(staticMeshHeader.assetName), "Assets/TestOb.obj");
        staticMeshHeader.assetName = "Suzanne";
    }
    else
    {
        staticMeshHeader.assetPath = FileSystemFunctions::combinePath(FileSystemFunctions::applicationDirectory(staticMeshHeader.assetName), "Assets/Gizmos.obj");
        staticMeshHeader.assetName = "Gizmos";
    }
    meshAsset = static_cast<StaticMeshAsset*>(appInstance().assetManager.getOrLoadAsset(staticMeshHeader));
}

void ExperimentalEngine::onStartUp()
{
    GameEngine::onStartUp();

    ENQUEUE_COMMAND(EngineStartUp, { startUpRenderInit(); }, this);

    camera.cameraProjection = ECameraProjection::Perspective;
    camera.setOrthoSize({ 1280,720 });
    camera.setClippingPlane(1.f, 600.f);
    camera.setFOV(110.f, 90.f);

    cameraTranslation = Vector3D(0.f, 1.f, 0.0f).safeNormalize() * (500);
    lightTranslation = Vector3D(100,0,0);

    camera.setTranslation(cameraTranslation);
    camera.lookAt(Vector3D::ZERO);
    cameraRotation = camera.rotation();

    tempTest();
}

void ExperimentalEngine::startUpRenderInit()
{
    vDevice = VulkanGraphicsHelper::getVulkanDevice(getRenderApi()->getGraphicsInstance());
    device = VulkanGraphicsHelper::getDevice(vDevice);
    graphicsDbg = VulkanGraphicsHelper::debugGraphics(getRenderApi()->getGraphicsInstance());
    createPools();
    frameResources.resize(getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow())->imagesCount());

    getPipelineForSubpass();
    createBuffers();
    createImages();
    createFrameResources();
    createPipelineResources();
}

void ExperimentalEngine::onQuit()
{
    ENQUEUE_COMMAND(EngineQuit, { renderQuit(); }, this);

    GameEngine::onQuit();
}

void ExperimentalEngine::renderQuit()
{
    vDevice->vkDeviceWaitIdle(device);

    destroyPipelineResources();
    destroyFrameResources();

    destroyBuffers();
    destroyImages();

    destroyPools();
}

void ExperimentalEngine::frameRender()
{
    writeBuffers();

    VkViewport viewport;
    viewport.x = 0;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    viewport.width = float(EngineSettings::screenSize.get().x);
    // Since view matrix positive y is along up while vulkan positive y in view is down
    viewport.height = -(float(EngineSettings::screenSize.get().y));
    viewport.y = float(EngineSettings::screenSize.get().y);
    VkRect2D scissor = { {0,0},{EngineSettings::screenSize.get().x,EngineSettings::screenSize.get().y} };

    SharedPtr<GraphicsSemaphore> waitSemaphore;
    uint32 index = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow())->requestNextImage(&waitSemaphore, nullptr);
    drawSmDefaultPipelineContext.swapchainIdx = drawQuadPipelineContext.swapchainIdx = index;
    getRenderApi()->getGlobalRenderingContext()->preparePipelineContext(&drawSmDefaultPipelineContext);
    getRenderApi()->getGlobalRenderingContext()->preparePipelineContext(&drawQuadPipelineContext);

    GraphicsPipelineQueryParams queryParam;
    queryParam.cullingMode = ECullingMode::BackFace;
    queryParam.drawMode = EPolygonDrawMode::Fill;

    std::vector<VkDescriptorSet>* drawQuadDescSets = nullptr;
    switch (frameVisualizeId)
    {
    case 1:
        drawQuadDescSets = &drawQuadNormalDescs[index];
        break;
    case 2:
        drawQuadDescSets = &drawQuadDepthDescs[index];
        break;
    case 0:
    default:
        drawQuadDescSets = &drawQuadTextureDescs[index];
        break;
    }


    if (!frameResources[index].recordingFence->isSignaled())
    {
        frameResources[index].recordingFence->waitForSignal();
    }
    frameResources[index].recordingFence->resetSignal();

    CMD_BUFFER_BEGIN_INFO(cmdBeginInfo);
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vDevice->vkBeginCommandBuffer(frameResources[index].perFrameCommands, &cmdBeginInfo);
    {
        const GraphicsPipeline* tempPipeline = static_cast<const GraphicsPipeline*>(drawSmGoochPipelineContext.getPipeline());

        SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, ExperimentalEngineFrame);

        RENDERPASS_BEGIN_INFO(renderPassBeginInfo);
        renderPassBeginInfo.renderPass = drawSmRenderPass;
        renderPassBeginInfo.framebuffer = VulkanGraphicsHelper::getFramebuffer(GBuffers::getFramebuffer(ERenderPassFormat::Multibuffers, index));

        renderPassBeginInfo.pClearValues = smAttachmentsClearColors.data();
        renderPassBeginInfo.clearValueCount = (uint32)smAttachmentsClearColors.size();
        renderPassBeginInfo.renderArea = scissor;

        vDevice->vkCmdBeginRenderPass(frameResources[index].perFrameCommands, &renderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
        {
            SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, MainUnlitPass);

            vDevice->vkCmdSetViewport(frameResources[index].perFrameCommands, 0, 1, &viewport);
            vDevice->vkCmdSetScissor(frameResources[index].perFrameCommands, 0, 1, &scissor);

            //vDevice->vkCmdPushConstants(frameResources[index].perFrameCommands, tempPipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &useVertexColor);

            vDevice->vkCmdBindPipeline(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS, tempPipeline->getPipeline(queryParam));
            vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS
                , tempPipeline->pipelineLayout, 0, uint32(staticMeshDescs.size()), staticMeshDescs.data(), 0, nullptr);

            uint64 vertexBufferOffset = 0;
            vDevice->vkCmdBindVertexBuffers(frameResources[index].perFrameCommands, 0, 1, &static_cast<VulkanBufferResource*>(meshAsset->vertexBuffer)->buffer, &vertexBufferOffset);
            vDevice->vkCmdBindIndexBuffer(frameResources[index].perFrameCommands, static_cast<VulkanBufferResource*>(meshAsset->indexBuffer)->buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
            for (const MeshVertexView& meshBatch : meshAsset->meshBatches)
            {
                vDevice->vkCmdDrawIndexed(frameResources[index].perFrameCommands, meshBatch.numOfIndices, 1, meshBatch.startIndex, 0, 0);
            }
        }
        vDevice->vkCmdEndRenderPass(frameResources[index].perFrameCommands);

        tempPipeline = static_cast<const GraphicsPipeline*>(drawQuadPipelineContext.getPipeline());

        viewport.x = 0;
        viewport.y = 0;
        viewport.width = float(EngineSettings::surfaceSize.get().x);
        viewport.height = float(EngineSettings::surfaceSize.get().y);
        scissor = { {0,0},{EngineSettings::surfaceSize.get().x,EngineSettings::surfaceSize.get().y} };

        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &swapchainClearColor;
        renderPassBeginInfo.framebuffer = VulkanGraphicsHelper::getFramebuffer(GBuffers::getSwapchainFramebuffer(index));
        renderPassBeginInfo.renderArea = scissor;
        renderPassBeginInfo.renderPass = drawQuadRenderPass;

        vDevice->vkCmdBeginRenderPass(frameResources[index].perFrameCommands, &renderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
        {
            SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, ResolveToSwapchain);

            vDevice->vkCmdSetViewport(frameResources[index].perFrameCommands, 0, 1, &viewport);
            vDevice->vkCmdSetScissor(frameResources[index].perFrameCommands, 0, 1, &scissor);

            vDevice->vkCmdBindPipeline(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS, tempPipeline->getPipeline(queryParam));
            vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS
                , tempPipeline->pipelineLayout, 0, uint32(drawQuadDescSets->size()), drawQuadDescSets->data(), 0, nullptr);

            uint64 vertexBufferOffset = 0;
            vDevice->vkCmdBindVertexBuffers(frameResources[index].perFrameCommands, 0, 1, &static_cast<VulkanBufferResource*>(quadVertexBuffer)->buffer, &vertexBufferOffset);
            vDevice->vkCmdBindIndexBuffer(frameResources[index].perFrameCommands, static_cast<VulkanBufferResource*>(quadIndexBuffer)->buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

            vDevice->vkCmdDrawIndexed(frameResources[index].perFrameCommands, 6, 1, 0, 0, 0);
        }
        vDevice->vkCmdEndRenderPass(frameResources[index].perFrameCommands);
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

    vDevice->vkQueueSubmit(getQueue<EQueueFunction::Graphics>(vDevice)->getQueueOfPriority<EQueuePriority::High>()
        , 1, &qSubmitInfo, static_cast<VulkanFence*>(frameResources[index].recordingFence.get())->fence);

    std::vector<GenericWindowCanvas*> canvases = { getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow()) };
    std::vector<uint32> indices = { index };
    GraphicsHelper::presentImage(getRenderApi()->getGraphicsInstance(), &canvases, &indices, &frameResources[index].usageWaitSemaphore);

}

void ExperimentalEngine::tickEngine()
{
    GameEngine::tickEngine();
    updateCameraParams();

    if (getApplicationInstance()->inputSystem()->isKeyPressed(Keys::ONE))
    {
        frameVisualizeId = 0;
    }
    else if(getApplicationInstance()->inputSystem()->isKeyPressed(Keys::TWO))
    {
        frameVisualizeId = 1;
    }
    else if (getApplicationInstance()->inputSystem()->isKeyPressed(Keys::THREE))
    {
        frameVisualizeId = 2;
    }

    ENQUEUE_COMMAND(TickFrame, { frameRender(); }, this);

    tempTestPerFrame();
}

GameEngine* GameEngineWrapper::createEngineInstance()
{
    static ExperimentalEngine gameEngine;
    return &gameEngine;
}