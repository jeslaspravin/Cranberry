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
#include "../Assets/Asset/StaticMeshAsset.h"
#include "../Assets/Asset/TextureAsset.h"
#include "../RenderInterface/Shaders/StaticMesh/StaticMeshUnlit.h"
#include "../RenderInterface/Shaders/DrawQuadFromTexture.h"
#include "../RenderInterface/Rendering/IRenderCommandList.h"
#include "../Core/Types/Textures/RenderTargetTextures.h"
#include "../Core/Types/Textures/Texture2D.h"
#include "../VulkanRI/VulkanInternals/VulkanDescriptorAllocator.h"
#include "../RenderInterface/GlobalRenderVariables.h"
#include "../RenderApi/GBuffersAndTextures.h"
#include "../Core/Types/Colors.h"
#include "../Core/Types/Time.h"
#include "../Core/Types/Delegates/Delegate.h"
#include "../Core/Reflections/MemberField.h"
#include "../Core/Engine/Config/EngineGlobalConfigs.h"
#include "../Core/Input/InputSystem.h"
#include "../Core/Math/Vector2D.h"
#include "../Core/Math/Vector4D.h"
#include "../Core/Math/Vector3D.h"
#include "../Core/Math/Matrix4.h"
#include "../Core/Math/Math.h"

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
    viewBuffer.buffer = new GraphicsRBuffer(smUniformBinding["viewData"]->paramStride(), 1);
    viewBuffer.buffer->setResourceName("ViewData");
    viewBuffer.buffer->init();

    instanceBuffer.buffer = new GraphicsRBuffer(smUniformBinding["instanceData"]->paramStride(), 1);
    instanceBuffer.buffer->setResourceName("InstanceData");
    instanceBuffer.buffer->init();
}

void ExperimentalEngine::destroyBuffers()
{
    viewBuffer.buffer->release();
    delete viewBuffer.buffer;
    viewBuffer.buffer = nullptr;

    instanceBuffer.buffer->release();
    delete instanceBuffer.buffer;
    instanceBuffer.buffer = nullptr;

    for (const std::pair<String,ShaderBufferParamInfo*>& pairs : smUniformBinding)
    {
        delete pairs.second;
    }
    smUniformBinding.clear();
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

        Texture2DCreateParams t2dCreateParams
        {
            "NormalTexture",
            ESamplerFiltering::Nearest,
            Size2D(1024, 1024)
        };
        t2dCreateParams.defaultColor = Color(0, 0, 1);
        normalTexture.image = TextureBase::createTexture<Texture2D>(t2dCreateParams);
        normalTexture.imageView = static_cast<VulkanImageResource*>(normalTexture.image->getTextureResource())->getImageView({});
        if (normalTexture.imageView != nullptr)
        {
            graphicsDbg->markObject((uint64)normalTexture.imageView, "NormalTextureView", VK_OBJECT_TYPE_IMAGE_VIEW);
        }
    }
}

void ExperimentalEngine::destroyImages()
{
    commonSampler->release();

    TextureBase::destroyTexture<Texture2D>(normalTexture.image);
    normalTexture.image = nullptr;
    normalTexture.imageView = nullptr;
}

void ExperimentalEngine::fillBindings()
{
    smTextureBinding["diffuseTexture"] = &texture;
    smTextureBinding["normalTexture"] = &normalTexture;
    smUniformBinding["viewData"] = new ViewDataBufferParamInfo();
    smUniformBinding["instanceData"] = new InstanceDataBufferParamInfo();

    const ShaderReflected* reflectedData = static_cast<ShaderResource*>(StaticMeshUnlit::staticType()->getDefault())->getReflection();
    // Only doing for fields and none arrays as that is only needed here
    for (const ReflectDescriptorBody& descriptorsSet : reflectedData->descriptorsSets)
    {
        for (const DescEntryBuffer& uniformBuff : descriptorsSet.uniforms)
        {
            auto buffItr = smUniformBinding.find(uniformBuff.attributeName);
            if (buffItr != smUniformBinding.end())
            {
                buffItr->second->setStride(uniformBuff.data.data.stride);
                ShaderBufferFieldNode* node = &buffItr->second->startNode;
                while (node->isValid())
                {
                    if (!node->field->bIsStruct)
                    {
                        for (const ReflectBufferEntry& field : uniformBuff.data.data.bufferFields)
                        {
                            if (field.attributeName == node->field->paramName)
                            {
                                node->field->offset = field.data.offset;
                                node->field->size = field.data.totalSize;
                                node->field->stride = field.data.stride;
                                break;
                            }
                        }
                    }
                    node = node->nextNode;
                }
            }
        }
    }

    for (const ReflectInputOutput& inputVertexAttrib : reflectedData->inputs)
    {
        ShaderVertexFieldNode* node = &MeshAsset::getShaderParamInfo<StaticMeshAsset>()->startNode;
        while (node->isValid())
        {
            if (inputVertexAttrib.attributeName == node->field->attributeName)
            {
                node->field->format = EShaderInputAttribFormat::getInputFormat(inputVertexAttrib.data.type);
                node->field->location = inputVertexAttrib.data.location;
                break;
            }
            node = node->nextNode;
        }
    }    
}

void fillDescriptorsSet(std::vector<VkDescriptorPoolSize>& poolAllocateInfo, std::map<String, uint32>& bindingNames, std::vector<VkDescriptorSetLayoutBinding>& descLayoutBindings
    , const ReflectDescriptorBody& descReflected)
{
    for (const DescEntryBuffer& descriptorInfo : descReflected.uniforms)
    {
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = 1;

        descLayoutBindings[descriptorInfo.data.binding].binding = bindingNames[descriptorInfo.attributeName] = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = 1;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntryBuffer& descriptorInfo : descReflected.buffers)
    {
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = 1;

        descLayoutBindings[descriptorInfo.data.binding].binding = bindingNames[descriptorInfo.attributeName] = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = 1;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntryTexelBuffer& descriptorInfo : descReflected.imageBuffers)
    {
        uint32 descCount = 1;
        for (const ArrayDefinition& arrayDimInfo : descriptorInfo.data.data.arraySize)
        {
            fatalAssert(arrayDimInfo.isSpecializationConst == false, "Specialized data is not supported yet");
            descCount *= arrayDimInfo.dimension;
        }
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = descCount;

        descLayoutBindings[descriptorInfo.data.binding].binding = bindingNames[descriptorInfo.attributeName] = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = descCount;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntryTexelBuffer& descriptorInfo : descReflected.samplerBuffers)
    {
        uint32 descCount = 1;
        for (const ArrayDefinition& arrayDimInfo : descriptorInfo.data.data.arraySize)
        {
            fatalAssert(arrayDimInfo.isSpecializationConst == false, "Specialized data is not supported yet");
            descCount *= arrayDimInfo.dimension;
        }
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = descCount;

        descLayoutBindings[descriptorInfo.data.binding].binding = bindingNames[descriptorInfo.attributeName] = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = descCount;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntryTexture& descriptorInfo : descReflected.imagesAndImgArrays)
    {
        uint32 descCount = 1;
        for (const ArrayDefinition& arrayDimInfo : descriptorInfo.data.data.arraySize)
        {
            fatalAssert(arrayDimInfo.isSpecializationConst == false, "Specialized data is not supported yet");
            descCount *= arrayDimInfo.dimension;
        }
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = descCount;

        descLayoutBindings[descriptorInfo.data.binding].binding = bindingNames[descriptorInfo.attributeName] = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = descCount;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntryTexture& descriptorInfo : descReflected.textureAndArrays)
    {
        uint32 descCount = 1;
        for (const ArrayDefinition& arrayDimInfo : descriptorInfo.data.data.arraySize)
        {
            fatalAssert(arrayDimInfo.isSpecializationConst == false, "Specialized data is not supported yet");
            descCount *= arrayDimInfo.dimension;
        }
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = descCount;

        descLayoutBindings[descriptorInfo.data.binding].binding = bindingNames[descriptorInfo.attributeName] = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = descCount;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntryTexture& descriptorInfo : descReflected.sampledTexAndArrays)
    {
        uint32 descCount = 1;
        for (const ArrayDefinition& arrayDimInfo : descriptorInfo.data.data.arraySize)
        {
            fatalAssert(arrayDimInfo.isSpecializationConst == false, "Specialized data is not supported yet");
            descCount *= arrayDimInfo.dimension;
        }
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = descCount;

        descLayoutBindings[descriptorInfo.data.binding].binding = bindingNames[descriptorInfo.attributeName] = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = descCount;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntrySampler& descriptorInfo : descReflected.samplers)
    {
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = 1;

        descLayoutBindings[descriptorInfo.data.binding].binding = bindingNames[descriptorInfo.attributeName] = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = 1;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntrySubpassInput& descriptorInfo : descReflected.subpassInputs)
    {
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = 1;

        descLayoutBindings[descriptorInfo.data.binding].binding = bindingNames[descriptorInfo.attributeName] = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = 1;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
}

void ExperimentalEngine::createShaderResDescriptors()
{
    uint32 swapchainCount = appInstance().appWindowManager.getWindowCanvas(appInstance().appWindowManager.getMainWindow())->imagesCount();
    VulkanDescriptorsSetAllocator* descsSetAllocator = VulkanGraphicsHelper::getDescriptorsSetAllocator(gEngine->getRenderApi()->getGraphicsInstance());
    // Static mesh unlit rendering
    {
        staticMeshDescs.clear();

        const ShaderReflected* reflectedData = static_cast<ShaderResource*>(StaticMeshUnlit::staticType()->getDefault())->getReflection();

        staticMeshDescs.reserve(reflectedData->descriptorsSets.size());
        for (const ReflectDescriptorBody& descriptorsSet : reflectedData->descriptorsSets)
        {
            DescSetInfo setInfo;
            setInfo.descLayoutInfo.resize(descriptorsSet.usedBindings.size());
            std::vector<VkDescriptorSetLayoutBinding> layoutBindings(descriptorsSet.usedBindings.size());

            fillDescriptorsSet(setInfo.descLayoutInfo, setInfo.descBindingNames, layoutBindings, descriptorsSet);

            DESCRIPTOR_SET_LAYOUT_CREATE_INFO(descLayoutCreateInfo);
            descLayoutCreateInfo.bindingCount = uint32(layoutBindings.size());
            descLayoutCreateInfo.pBindings = layoutBindings.data();

            fatalAssert(vDevice->vkCreateDescriptorSetLayout(device, &descLayoutCreateInfo, nullptr, &setInfo.descLayout) == VK_SUCCESS, "Failed creating descriptors set layout for unlit static mesh descriptors");

            DescriptorsSetQuery query;
            query.supportedTypes.insert(setInfo.descLayoutInfo.cbegin(), setInfo.descLayoutInfo.cend());
            setInfo.descSet = descsSetAllocator->allocDescriptorsSet(query, setInfo.descLayout);

            staticMeshDescs.push_back(std::move(setInfo));
        }
    }

    // Drawing textures to quad
    {
        drawQuadTextureDescs.resize(swapchainCount);
        drawQuadNormalDescs.resize(swapchainCount);
        drawQuadDepthDescs.resize(swapchainCount);

        const ShaderReflected* reflectedData = static_cast<ShaderResource*>(DrawQuadFromTexture::staticType()->getDefault())->getReflection();

        for (const ReflectDescriptorBody& descriptorsSet : reflectedData->descriptorsSets)
        {
            DescSetInfo setInfo;
            setInfo.descLayoutInfo.resize(descriptorsSet.usedBindings.size());
            std::vector<VkDescriptorSetLayoutBinding> layoutBindings(descriptorsSet.usedBindings.size());

            fillDescriptorsSet(setInfo.descLayoutInfo, setInfo.descBindingNames, layoutBindings, descriptorsSet);

            DESCRIPTOR_SET_LAYOUT_CREATE_INFO(descLayoutCreateInfo);
            descLayoutCreateInfo.bindingCount = uint32(layoutBindings.size());
            descLayoutCreateInfo.pBindings = layoutBindings.data();

            fatalAssert(vDevice->vkCreateDescriptorSetLayout(device, &descLayoutCreateInfo, nullptr, &setInfo.descLayout) == VK_SUCCESS, "Failed creating descriptors set layout for draw quad from texture descriptors");

            DescriptorsSetQuery query;
            query.supportedTypes.insert(setInfo.descLayoutInfo.cbegin(), setInfo.descLayoutInfo.cend());

            for (uint32 i = 0; i < swapchainCount; ++i)
            {
                DescSetInfo diffuseDescSetInfo = setInfo;
                DescSetInfo normalDescSetInfo = setInfo;
                DescSetInfo depthDescSetInfo = setInfo;

                diffuseDescSetInfo.descSet = descsSetAllocator->allocDescriptorsSet(query, setInfo.descLayout);
                drawQuadTextureDescs[i].push_back(std::move(diffuseDescSetInfo));

                normalDescSetInfo.descSet = descsSetAllocator->allocDescriptorsSet(query, setInfo.descLayout);
                drawQuadNormalDescs[i].push_back(std::move(normalDescSetInfo));

                depthDescSetInfo.descSet = descsSetAllocator->allocDescriptorsSet(query, setInfo.descLayout);
                drawQuadDepthDescs[i].push_back(std::move(depthDescSetInfo));
            }
        }
    }

    std::vector<std::pair<VkWriteDescriptorSet, uint32>> writingBufferDescriptors;
    std::vector<VkDescriptorBufferInfo> bufferInfo;
    std::vector<std::pair<VkWriteDescriptorSet, uint32>> writingImageDescriptors;
    std::vector<VkDescriptorImageInfo> imageInfo;

    // Static mesh descriptors
    {
        for (const DescSetInfo& descSetInfo : staticMeshDescs)
        {
            auto foundItr = descSetInfo.descBindingNames.find("viewData");
            if (foundItr != descSetInfo.descBindingNames.end())
            {
                uint32 bufferInfoIdx = uint32(bufferInfo.size());
                bufferInfo.push_back({});
                bufferInfo[bufferInfoIdx].buffer = static_cast<VulkanBufferResource*>(viewBuffer.buffer)->buffer;
                bufferInfo[bufferInfoIdx].offset = 0;
                bufferInfo[bufferInfoIdx].range = viewBuffer.buffer->getResourceSize();

                WRITE_RESOURCE_TO_DESCRIPTORS_SET(viewDataDescWrite);
                viewDataDescWrite.dstSet = descSetInfo.descSet;
                viewDataDescWrite.descriptorType = descSetInfo.descLayoutInfo[foundItr->second].type;
                viewDataDescWrite.dstBinding = foundItr->second;
                writingBufferDescriptors.push_back({ viewDataDescWrite, bufferInfoIdx });
            }
            foundItr = descSetInfo.descBindingNames.find("instanceData");
            if (foundItr != descSetInfo.descBindingNames.end())
            {
                uint32 bufferInfoIdx = uint32(bufferInfo.size());
                bufferInfo.push_back({});
                bufferInfo[bufferInfoIdx].buffer = static_cast<VulkanBufferResource*>(instanceBuffer.buffer)->buffer;
                bufferInfo[bufferInfoIdx].offset = 0;
                bufferInfo[bufferInfoIdx].range = instanceBuffer.buffer->getResourceSize();

                WRITE_RESOURCE_TO_DESCRIPTORS_SET(instanceDataDescWrite);
                instanceDataDescWrite.dstSet = descSetInfo.descSet;
                instanceDataDescWrite.descriptorType = descSetInfo.descLayoutInfo[foundItr->second].type;
                instanceDataDescWrite.dstBinding = foundItr->second;
                writingBufferDescriptors.push_back({ instanceDataDescWrite, bufferInfoIdx });
            }

            for (const std::pair<const String, ImageData*>& texturePairs : smTextureBinding)
            {
                foundItr = descSetInfo.descBindingNames.find(texturePairs.first);
                if (foundItr != descSetInfo.descBindingNames.end())
                {
                    uint32 imageInfoIdx = uint32(imageInfo.size());
                    imageInfo.push_back({});
                    imageInfo[imageInfoIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfo[imageInfoIdx].imageView = texturePairs.second->imageView;
                    imageInfo[imageInfoIdx].sampler = static_cast<VulkanSampler*>(commonSampler.get())->sampler;

                    WRITE_RESOURCE_TO_DESCRIPTORS_SET(textureDescWrite);
                    textureDescWrite.dstSet = descSetInfo.descSet;
                    textureDescWrite.descriptorType = descSetInfo.descLayoutInfo[foundItr->second].type;
                    textureDescWrite.dstBinding = foundItr->second;
                    textureDescWrite.pImageInfo = &imageInfo[imageInfoIdx];
                    writingImageDescriptors.push_back({ textureDescWrite, imageInfoIdx });
                }
            }
        }
    }
    // Draw quad descriptors
    {
        const FramebufferFormat unlitFbFormat = { { EPixelDataFormat::BGRA_U8_Norm, EPixelDataFormat::ABGR8_S32_NormPacked, EPixelDataFormat::R_SF32, EPixelDataFormat::D_SF32 } };
        for (uint32 swapchainIdx = 0; swapchainIdx < swapchainCount; ++swapchainIdx)
        {
            for (uint32 i = 0; i < drawQuadTextureDescs[swapchainIdx].size(); ++i)
            {
                const DescSetInfo& descSetInfo = drawQuadTextureDescs[swapchainIdx][i];

                auto foundItr = descSetInfo.descBindingNames.find("quadTexture");
                if (foundItr != descSetInfo.descBindingNames.end())
                {
                    WRITE_RESOURCE_TO_DESCRIPTORS_SET(quadTextureDescWrite);
                    quadTextureDescWrite.descriptorType = descSetInfo.descLayoutInfo[foundItr->second].type;
                    quadTextureDescWrite.dstBinding = foundItr->second;

                    Framebuffer* fb = GBuffers::getFramebuffer(unlitFbFormat, swapchainIdx);
                    fatalAssert(fb != nullptr, "Framebuffer is invalid");

                    uint32 imageInfoIdx = uint32(imageInfo.size());
                    imageInfo.push_back({});
                    imageInfo[imageInfoIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfo[imageInfoIdx].imageView = static_cast<VulkanImageResource*>(fb->textures[1])->getImageView({});// Diffuse is at 0
                    imageInfo[imageInfoIdx].sampler = static_cast<VulkanSampler*>(commonSampler.get())->sampler;

                    quadTextureDescWrite.dstSet = drawQuadTextureDescs[swapchainIdx][i].descSet;
                    writingImageDescriptors.push_back({ quadTextureDescWrite, imageInfoIdx });

                    imageInfoIdx = uint32(imageInfo.size());
                    imageInfo.push_back({});
                    imageInfo[imageInfoIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfo[imageInfoIdx].imageView = static_cast<VulkanImageResource*>(fb->textures[3])->getImageView({});// Normal texture is at 1
                    imageInfo[imageInfoIdx].sampler = static_cast<VulkanSampler*>(commonSampler.get())->sampler;

                    quadTextureDescWrite.dstSet = drawQuadNormalDescs[swapchainIdx][i].descSet;
                    writingImageDescriptors.push_back({ quadTextureDescWrite, imageInfoIdx });

                    imageInfoIdx = uint32(imageInfo.size());
                    imageInfo.push_back({});
                    imageInfo[imageInfoIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfo[imageInfoIdx].imageView = static_cast<VulkanImageResource*>(fb->textures[5])->getImageView({ 
                        {EPixelComponentMapping::SameComponent, EPixelComponentMapping::R,EPixelComponentMapping::R,EPixelComponentMapping::R} });// Depth is at 2
                    imageInfo[imageInfoIdx].sampler = static_cast<VulkanSampler*>(commonSampler.get())->sampler;

                    quadTextureDescWrite.dstSet = drawQuadDepthDescs[swapchainIdx][i].descSet;
                    writingImageDescriptors.push_back({ quadTextureDescWrite, imageInfoIdx });
                }
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
        for (uint32 swapchainIdx = 0; swapchainIdx < swapchainCount; ++swapchainIdx)
        {
            for (uint32 i = 0; i < drawQuadTextureDescs[swapchainIdx].size(); ++i)
            {
                const DescSetInfo& descSetInfo = drawQuadTextureDescs[swapchainIdx][i];

                auto foundItr = descSetInfo.descBindingNames.find("quadTexture");
                if (foundItr != descSetInfo.descBindingNames.end())
                {
                    WRITE_RESOURCE_TO_DESCRIPTORS_SET(quadTextureDescWrite);
                    quadTextureDescWrite.descriptorType = descSetInfo.descLayoutInfo[foundItr->second].type;
                    quadTextureDescWrite.dstBinding = foundItr->second;

                    FramebufferFormat unlitFbFormat = { { EPixelDataFormat::BGRA_U8_Norm, EPixelDataFormat::ABGR8_S32_NormPacked, EPixelDataFormat::R_SF32, EPixelDataFormat::D_SF32 } };
                    Framebuffer* fb = GBuffers::getFramebuffer(unlitFbFormat, swapchainIdx);
                    fatalAssert(fb != nullptr, "Framebuffer is invalid");

                    uint32 imageInfoIdx = uint32(imageInfo.size());
                    imageInfo.push_back({});
                    imageInfo[imageInfoIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfo[imageInfoIdx].imageView = static_cast<VulkanImageResource*>(fb->textures[1])->getImageView({});// Diffuse is at 0
                    imageInfo[imageInfoIdx].sampler = static_cast<VulkanSampler*>(commonSampler.get())->sampler;

                    quadTextureDescWrite.dstSet = drawQuadTextureDescs[swapchainIdx][i].descSet;
                    writingImageDescriptors.push_back({ quadTextureDescWrite, imageInfoIdx });

                    imageInfoIdx = uint32(imageInfo.size());
                    imageInfo.push_back({});
                    imageInfo[imageInfoIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfo[imageInfoIdx].imageView = static_cast<VulkanImageResource*>(fb->textures[3])->getImageView({});// Normal texture is at 1
                    imageInfo[imageInfoIdx].sampler = static_cast<VulkanSampler*>(commonSampler.get())->sampler;

                    quadTextureDescWrite.dstSet = drawQuadNormalDescs[swapchainIdx][i].descSet;
                    writingImageDescriptors.push_back({ quadTextureDescWrite, imageInfoIdx });

                    imageInfoIdx = uint32(imageInfo.size());
                    imageInfo.push_back({});
                    imageInfo[imageInfoIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfo[imageInfoIdx].imageView = static_cast<VulkanImageResource*>(fb->textures[5])->getImageView({
                        {EPixelComponentMapping::SameComponent, EPixelComponentMapping::R,EPixelComponentMapping::R,EPixelComponentMapping::R} });// Depth is at 2
                    imageInfo[imageInfoIdx].sampler = static_cast<VulkanSampler*>(commonSampler.get())->sampler;

                    quadTextureDescWrite.dstSet = drawQuadDepthDescs[swapchainIdx][i].descSet;
                    writingImageDescriptors.push_back({ quadTextureDescWrite, imageInfoIdx });
                }
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

    for (DescSetInfo& descInfo : staticMeshDescs)
    {
        descsSetAllocator->releaseDescriptorsSet(descInfo.descSet);
        descInfo.descLayoutInfo.clear();
        vDevice->vkDestroyDescriptorSetLayout(device, descInfo.descLayout, nullptr);
    }
    staticMeshDescs.clear();

    vDevice->vkDestroyDescriptorSetLayout(device, drawQuadTextureDescs[0][0].descLayout, nullptr);
    for (std::vector<DescSetInfo>& descInfos : drawQuadTextureDescs)
    {
        for(DescSetInfo& descInfo : descInfos)
        {
            descsSetAllocator->releaseDescriptorsSet(descInfo.descSet);
            descInfo.descLayoutInfo.clear();
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

void ExperimentalEngine::createRenderpass()
{
    GenericWindowCanvas* windowCanvas = getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()
        ->appWindowManager.getMainWindow());
    // Static mesh unlit render pass
    {
        smAttachmentsClearColors.resize(7);
        std::array<VkAttachmentReference, 4> attachmentRefs;
        std::array<VkAttachmentReference, 3> resolveAttachmentRefs;
        std::array<VkAttachmentDescription, 7> attachmentsDesc;
        {
            VkAttachmentDescription diffuseTargetAttachment;
            diffuseTargetAttachment.flags = 0;
            diffuseTargetAttachment.format = VkFormat(EPixelDataFormat::getFormatInfo(EPixelDataFormat::BGRA_U8_Norm)->format);
            diffuseTargetAttachment.samples = VkSampleCountFlagBits(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
            diffuseTargetAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            diffuseTargetAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            diffuseTargetAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            diffuseTargetAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            diffuseTargetAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            diffuseTargetAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentsDesc[0] = diffuseTargetAttachment;
            smAttachmentsClearColors[0].color = smAttachmentsClearColors[1].color = { 0.267f ,0.4f,0.0f,1.0f };// Some good color
            attachmentRefs[0].attachment = 0;
            attachmentRefs[0].layout = resolveAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            diffuseTargetAttachment.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
            diffuseTargetAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachmentsDesc[1] = diffuseTargetAttachment;
            resolveAttachmentRefs[0].attachment = 1;

            VkAttachmentDescription normalTargetAttachment;
            normalTargetAttachment.flags = 0;
            normalTargetAttachment.format = VkFormat(EPixelDataFormat::getFormatInfo(EPixelDataFormat::ABGR8_S32_NormPacked)->format);
            normalTargetAttachment.samples = VkSampleCountFlagBits(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
            normalTargetAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            normalTargetAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            normalTargetAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            normalTargetAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            normalTargetAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            normalTargetAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentsDesc[2] = normalTargetAttachment;
            smAttachmentsClearColors[2].color = smAttachmentsClearColors[3].color = { 0.267f ,0.4f,0.0f,1.0f };// Some good color
            attachmentRefs[1].attachment = 2;
            attachmentRefs[1].layout = resolveAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            normalTargetAttachment.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
            normalTargetAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachmentsDesc[3] = normalTargetAttachment;
            resolveAttachmentRefs[1].attachment = 3;

            VkAttachmentDescription depthTargetAttachment;
            depthTargetAttachment.flags = 0;
            depthTargetAttachment.format = VkFormat(EPixelDataFormat::getFormatInfo(EPixelDataFormat::R_SF32)->format);
            depthTargetAttachment.samples = VkSampleCountFlagBits(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
            depthTargetAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthTargetAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            depthTargetAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthTargetAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthTargetAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthTargetAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentsDesc[4] = depthTargetAttachment;
            smAttachmentsClearColors[4].color = smAttachmentsClearColors[5].color = { 0.0f,0.0f,0.0f,0.0f };// Black
            attachmentRefs[2].attachment = 4;
            attachmentRefs[2].layout = resolveAttachmentRefs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            depthTargetAttachment.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
            depthTargetAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachmentsDesc[5] = depthTargetAttachment;
            resolveAttachmentRefs[2].attachment = 5;

            VkAttachmentDescription realDepthAttachment;
            realDepthAttachment.flags = 0;
            realDepthAttachment.format = VkFormat(EPixelDataFormat::getFormatInfo(EPixelDataFormat::D_SF32)->format);
            realDepthAttachment.samples = VkSampleCountFlagBits(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
            realDepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            realDepthAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
            realDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            realDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            realDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            realDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentsDesc[6] = realDepthAttachment;
            smAttachmentsClearColors[6].color = { 0.0f,0.0f,0.0f,0.0f };// Black
            attachmentRefs[3].attachment = 6;
            attachmentRefs[3].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription subpass;
        subpass.flags = 0;
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pDepthStencilAttachment = &attachmentRefs[resolveAttachmentRefs.size()];
        subpass.colorAttachmentCount = uint32(resolveAttachmentRefs.size());
        subpass.pColorAttachments = attachmentRefs.data();
        subpass.pResolveAttachments = resolveAttachmentRefs.data();
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;

        RENDERPASS_CREATE_INFO(renderPassCreateInfo);
        renderPassCreateInfo.attachmentCount = (uint32)attachmentsDesc.size();
        renderPassCreateInfo.pAttachments = attachmentsDesc.data();
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;
        renderPassCreateInfo.dependencyCount = 0;
        renderPassCreateInfo.pDependencies = nullptr;

        if (vDevice->vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &smRenderPass) != VK_SUCCESS)
        {
            Logger::error("ExperimentalEngine", "%s() : Failed creating render pass", __func__);
            smRenderPass = nullptr;
            return;
        }
    }

    // Draw quad render pass
    {
        VkAttachmentDescription quadTarget;
        quadTarget.flags = 0;
        quadTarget.format = VkFormat(EPixelDataFormat::getFormatInfo(windowCanvas->windowCanvasFormat())->format);
        quadTarget.samples = VK_SAMPLE_COUNT_1_BIT;
        quadTarget.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        quadTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        quadTarget.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        quadTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        quadTarget.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        quadTarget.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        swapchainClearColor.color = { 0.0f,0.0f,0.0f,0.0f };// Black

        VkAttachmentReference quadAttachRef;
        quadAttachRef.attachment = 0;
        quadAttachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass;
        subpass.flags = 0;
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pDepthStencilAttachment = nullptr;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &quadAttachRef;
        subpass.pResolveAttachments = nullptr;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;


        RENDERPASS_CREATE_INFO(renderPassCreateInfo);
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &quadTarget;
        renderPassCreateInfo.dependencyCount = 0;
        renderPassCreateInfo.pDependencies = nullptr;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;

        if (vDevice->vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &swapchainRenderPass) != VK_SUCCESS)
        {
            Logger::error("ExperimentalEngine", "%s() : Failed creating render pass", __func__);
            swapchainRenderPass = nullptr;
            return;
        }
    }

    createFrameResources();
}

void ExperimentalEngine::destroyRenderpass()
{
    destroyFrameResources();
    vDevice->vkDestroyRenderPass(device, smRenderPass, nullptr);
    smRenderPass = nullptr;

    vDevice->vkDestroyRenderPass(device, swapchainRenderPass, nullptr);
    swapchainRenderPass = nullptr;
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

    if (vDevice->vkCreatePipelineCache(device,&pipelineCacheCreateInfo,nullptr,&drawSmPipeline.cache) != VK_SUCCESS)
    {
        Logger::warn("ExperimentalEngine", "%s() : Staticmesh drawing pipeline cache creation failed", __func__);
        drawSmPipeline.cache = nullptr;
    }
    else
    {
        graphicsDbg->markObject((uint64)(drawSmPipeline.cache), "ExperimentalTrianglePipelineCache", VK_OBJECT_TYPE_PIPELINE_CACHE);
    }
    if (vDevice->vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &drawQuadPipeline.cache) != VK_SUCCESS)
    {
        Logger::warn("ExperimentalEngine", "%s() : Quad drawing pipeline cache creation failed", __func__);
        drawQuadPipeline.cache = nullptr;
    }
    else
    {
        graphicsDbg->markObject((uint64)(drawSmPipeline.cache), "DrawQuadPipelineCache", VK_OBJECT_TYPE_PIPELINE_CACHE);
    }

    pipelineCacheFile.closeFile();
}

void ExperimentalEngine::writeAndDestroyPipelineCache()
{
    std::vector<VkPipelineCache> pipelineCaches;
    if (drawSmPipeline.cache != nullptr)
    {
        pipelineCaches.push_back(drawSmPipeline.cache);
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
            uint64 cacheDataSize;
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
    createSmPipeline();


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

    createQuadDrawPipeline();
}

void ExperimentalEngine::destroySubpassPipelines()
{
    writeAndDestroyPipelineCache();

    vDevice->vkDestroyPipelineLayout(device, drawSmPipeline.layout, nullptr);
    vDevice->vkDestroyPipeline(device, drawSmPipeline.pipeline, nullptr);
    vDevice->vkDestroyPipelineLayout(device, drawQuadPipeline.layout, nullptr);
    vDevice->vkDestroyPipeline(device, drawQuadPipeline.pipeline, nullptr);

    ENQUEUE_COMMAND(QuadVerticesRelease,
        {
            quadVertexBuffer->release();
            quadIndexBuffer->release();
            delete quadVertexBuffer;
            quadVertexBuffer = nullptr;
            delete quadIndexBuffer;
            quadIndexBuffer = nullptr;
        }
        , this);
}

void ExperimentalEngine::createSmPipeline()
{
    auto* shaderResource = static_cast<ShaderResource*>(StaticMeshUnlit::staticType()->getDefault());
    const ShaderReflected* shaderReflected = shaderResource->getReflection();

    GRAPHICS_PIPELINE_CREATE_INFO(graphicsPipelineCreateInfo);
    graphicsPipelineCreateInfo.pTessellationState = nullptr;// No tessellation right now

    PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO(depthStencilState);
    depthStencilState.depthWriteEnable = VK_TRUE; 
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER;
    depthStencilState.front = depthStencilState.back = { VK_STENCIL_OP_KEEP,VK_STENCIL_OP_KEEP,VK_STENCIL_OP_KEEP,VK_COMPARE_OP_NEVER,0,0,0 };

    graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilState;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(shaderResource->getShaders().size());
    for (const std::pair<const EShaderStage::Type, SharedPtr<ShaderCodeResource>>& shader : shaderResource->getShaders())
    {
        const EShaderStage::ShaderStageInfo* stageInfo = EShaderStage::getShaderStageInfo(shader.second->shaderStage());
        PIPELINE_SHADER_STAGE_CREATE_INFO(shaderStageCreateInfo);
        shaderStageCreateInfo.stage = (VkShaderStageFlagBits)stageInfo->shaderStage;
        shaderStageCreateInfo.pName = shader.second->entryPoint().getChar();
        shaderStageCreateInfo.module = static_cast<VulkanShaderCodeResource*>(shader.second.get())->shaderModule;
        shaderStageCreateInfo.pSpecializationInfo = VK_NULL_HANDLE;// Compile time constants

        shaderStages.push_back(shaderStageCreateInfo);
    }
    graphicsPipelineCreateInfo.stageCount = static_cast<uint32>(shaderStages.size());
    graphicsPipelineCreateInfo.pStages = shaderStages.data();

    std::vector<VkVertexInputBindingDescription> vertBindings{
    {
        0,
        MeshAsset::getShaderParamInfo<StaticMeshAsset>()->paramStride(),
        VK_VERTEX_INPUT_RATE_VERTEX
    }};
    std::vector<VkVertexInputAttributeDescription> vertAttributes;
    const ShaderVertexFieldNode* node = &MeshAsset::getShaderParamInfo<StaticMeshAsset>()->startNode;
    while (node->isValid())
    {
        VkVertexInputAttributeDescription vertAttribute;
        vertAttribute.binding = 0;
        vertAttribute.format = VkFormat(EPixelDataFormat::getFormatInfo(EPixelDataFormat::Type(node->field->format))->format);
        vertAttribute.location = node->field->location;
        vertAttribute.offset = node->field->offset;
        vertAttributes.push_back(vertAttribute);

        node = node->nextNode;
    }

    PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO(vertexInputCreateInfo);
    vertexInputCreateInfo.vertexBindingDescriptionCount = uint32(vertBindings.size());
    vertexInputCreateInfo.pVertexBindingDescriptions = vertBindings.data();
    vertexInputCreateInfo.vertexAttributeDescriptionCount = uint32(vertAttributes.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions = vertAttributes.data();
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    
    PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO(inputAssemCreateInfo);
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemCreateInfo;

    PIPELINE_VIEWPORT_STATE_CREATE_INFO(viewportCreateInfo);
    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    VkRect2D scissor = { {0,0},{ EngineSettings::screenSize.get().x, EngineSettings::screenSize.get().y} };
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
    multisampleCreateInfo.rasterizationSamples = VkSampleCountFlagBits(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
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
    std::vector<VkPipelineColorBlendAttachmentState> colorBlends;
    colorBlends.insert(colorBlends.end(), 3, colorAttachmentBlendState);

    colorBlendOpCreateInfo.attachmentCount = uint32(colorBlends.size());
    colorBlendOpCreateInfo.pAttachments = colorBlends.data();
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendOpCreateInfo;

    PIPELINE_DYNAMIC_STATE_CREATE_INFO(dynamicStateCreateInfo);
    std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    dynamicStateCreateInfo.dynamicStateCount = static_cast<VkDynamicState>(dynamicStates.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;


    std::vector<VkDescriptorSetLayout> descSetsLayouts;
    descSetsLayouts.reserve(staticMeshDescs.size());
    for (const DescSetInfo& smDescSetInfo : staticMeshDescs) { descSetsLayouts.push_back(smDescSetInfo.descLayout); }
    PIPELINE_LAYOUT_CREATE_INFO(pipelineLayoutCreateInfo);
    // TODO(Jeslas)(Less priority) : change this to get from reflection(anyway in real pipeline creation this gets changed. 
    VkPushConstantRange pushConstRange{ VK_SHADER_STAGE_FRAGMENT_BIT, 0 , sizeof(float)};
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstRange;
    pipelineLayoutCreateInfo.setLayoutCount = uint32(descSetsLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = descSetsLayouts.data();
    fatalAssert(vDevice->vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &drawSmPipeline.layout)
        == VK_SUCCESS, "Failed creating draw staticmesh pipeline layout");

    graphicsPipelineCreateInfo.layout = drawSmPipeline.layout;
    graphicsPipelineCreateInfo.renderPass = smRenderPass;
    graphicsPipelineCreateInfo.subpass = 0;

    fatalAssert(vDevice->vkCreateGraphicsPipelines(device, drawSmPipeline.cache,1,&graphicsPipelineCreateInfo, nullptr, &drawSmPipeline.pipeline)
        == VK_SUCCESS, "Failure in creating draw staticmesh pipelines");
    graphicsDbg->markObject((uint64)(drawSmPipeline.pipeline), "StaticMeshPipeline", VK_OBJECT_TYPE_PIPELINE);
}

void ExperimentalEngine::createQuadDrawPipeline()
{
    ShaderResource* shaderResource = static_cast<ShaderResource*>(DrawQuadFromTexture::staticType()->getDefault());

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

    std::vector<VkDescriptorSetLayout> descSetsLayouts;
    descSetsLayouts.reserve(drawQuadTextureDescs[0].size());
    for (const DescSetInfo& descSetInfo : drawQuadTextureDescs[0]) { descSetsLayouts.push_back(descSetInfo.descLayout); }

    PIPELINE_LAYOUT_CREATE_INFO(pipelineLayoutCreateInfo);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    pipelineLayoutCreateInfo.setLayoutCount = uint32(descSetsLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = descSetsLayouts.data();
    fatalAssert(vDevice->vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &drawQuadPipeline.layout)
        == VK_SUCCESS, "Failed creating draw triangle pipeline layout");
    graphicsPipelineCreateInfo.layout = drawQuadPipeline.layout;
    graphicsPipelineCreateInfo.renderPass = swapchainRenderPass;
    graphicsPipelineCreateInfo.subpass = 0;

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

void ExperimentalEngine::writeBuffers()
{
    ViewData viewData;
    viewData.projection = camera.projectionMatrix();
    viewData.invProjection = viewData.projection.inverse();
    viewData.view = camera.viewMatrix();
    viewData.invView = viewData.view.inverse();

    InstanceData instanceData;
    instanceData.invModel = Matrix4::IDENTITY;
    instanceData.model = Matrix4::IDENTITY;

    ENQUEUE_COMMAND(WritingUniforms,
        {
            cmdList->copyToBuffer<ViewData>(viewBuffer.buffer, 0, &viewData, smUniformBinding["viewData"]);
            cmdList->copyToBuffer<InstanceData>(instanceBuffer.buffer, 0, &instanceData, smUniformBinding["instanceData"]);
        }
    , this, viewData, instanceData);
}

void ExperimentalEngine::updateCameraParams()
{
    if (appInstance().inputSystem()->isKeyPressed(Keys::A))
    {
        rotationOffset += timeData.deltaTime * timeData.activeTimeDilation * 15.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::D))
    {
        rotationOffset -= timeData.deltaTime * timeData.activeTimeDilation * 15.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::W))
    {
        distanceOffset -= timeData.deltaTime * timeData.activeTimeDilation * 100.f;
    }
    if (appInstance().inputSystem()->isKeyPressed(Keys::S))
    {
        distanceOffset += timeData.deltaTime * timeData.activeTimeDilation * 100.f;
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

    Transform3D translation;
    translation.setTranslation(Vector3D(0.f, 1.f ,0.75f).safeNormalize() * (500 + distanceOffset));
    
    Transform3D cameraTransform;
    cameraTransform.setRotation(Rotation(0, 0, rotationOffset));

    camera.setTranslation(cameraTransform.transform(translation).getTranslation());
    camera.lookAt(Vector3D::ZERO);

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
    
    tempTest();
}

void ExperimentalEngine::startUpRenderInit()
{
    vDevice = VulkanGraphicsHelper::getVulkanDevice(getRenderApi()->getGraphicsInstance());
    device = VulkanGraphicsHelper::getDevice(vDevice);
    deviceQueues = VulkanGraphicsHelper::getVDAllQueues(vDevice);
    graphicsDbg = VulkanGraphicsHelper::debugGraphics(getRenderApi()->getGraphicsInstance());
    createPools();
    frameResources.resize(getApplicationInstance()->appWindowManager.getWindowCanvas(getApplicationInstance()->appWindowManager.getMainWindow())->imagesCount());
    cmdSubmitFence = GraphicsHelper::createFence(getRenderApi()->getGraphicsInstance(), "cmdSubmitFence");

    fillBindings();
    createBuffers();
    createImages();
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

    destroyBuffers();
    destroyImages();

    cmdSubmitFence->release();
    cmdSubmitFence.reset();
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

    std::vector<DescSetInfo>* drawQuadDescs = nullptr;
    switch (frameVisualizeId)
    {
    case 1:
        drawQuadDescs = &drawQuadNormalDescs[index];
        break;
    case 2:
        drawQuadDescs = &drawQuadDepthDescs[index];
        break;
    case 0:
    default:
        drawQuadDescs = &drawQuadTextureDescs[index];
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
        SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, ExperimentalEngineFrame);

        std::vector<VkDescriptorSet> descSets;
        descSets.reserve(staticMeshDescs.size());
        for (const DescSetInfo& descSetInfo : staticMeshDescs)
        {
            descSets.push_back(descSetInfo.descSet);
        }

        RENDERPASS_BEGIN_INFO(renderPassBeginInfo);
        renderPassBeginInfo.renderPass = smRenderPass;
        renderPassBeginInfo.framebuffer = VulkanGraphicsHelper::getFramebuffer(GBuffers::getFramebuffer(
            { { EPixelDataFormat::BGRA_U8_Norm, EPixelDataFormat::ABGR8_S32_NormPacked, EPixelDataFormat::R_SF32, EPixelDataFormat::D_SF32 } }, index));

        renderPassBeginInfo.pClearValues = smAttachmentsClearColors.data();
        renderPassBeginInfo.clearValueCount = (uint32)smAttachmentsClearColors.size();
        renderPassBeginInfo.renderArea = scissor;

        vDevice->vkCmdBeginRenderPass(frameResources[index].perFrameCommands, &renderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
        {
            SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, MainUnlitPass);

            vDevice->vkCmdSetViewport(frameResources[index].perFrameCommands, 0, 1, &viewport);
            vDevice->vkCmdSetScissor(frameResources[index].perFrameCommands, 0, 1, &scissor);

            vDevice->vkCmdPushConstants(frameResources[index].perFrameCommands, drawSmPipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &useVertexColor);

            vDevice->vkCmdBindPipeline(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS, drawSmPipeline.pipeline);
            vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS
                , drawSmPipeline.layout, 0, uint32(descSets.size()), descSets.data(), 0, nullptr);

            uint64 vertexBufferOffset = 0;
            vDevice->vkCmdBindVertexBuffers(frameResources[index].perFrameCommands, 0, 1, &static_cast<VulkanBufferResource*>(meshAsset->vertexBuffer)->buffer, &vertexBufferOffset);
            vDevice->vkCmdBindIndexBuffer(frameResources[index].perFrameCommands, static_cast<VulkanBufferResource*>(meshAsset->indexBuffer)->buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
            for (const MeshVertexView& meshBatch : meshAsset->meshBatches)
            {
                vDevice->vkCmdDrawIndexed(frameResources[index].perFrameCommands, meshBatch.numOfIndices, 1, meshBatch.startIndex, 0, 0);
            }
        }
        vDevice->vkCmdEndRenderPass(frameResources[index].perFrameCommands);

        viewport.x = 0;
        viewport.y = 0;
        viewport.width = float(EngineSettings::surfaceSize.get().x);
        viewport.height = float(EngineSettings::surfaceSize.get().y);
        scissor = { {0,0},{EngineSettings::surfaceSize.get().x,EngineSettings::surfaceSize.get().y} };

        // Copying to swapchain
        descSets.clear();
        descSets.reserve(drawQuadDescs->size());
        for (const DescSetInfo& descSetInfo : *drawQuadDescs)
        {
            descSets.push_back(descSetInfo.descSet);
        }

        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &swapchainClearColor;
        renderPassBeginInfo.framebuffer = VulkanGraphicsHelper::getFramebuffer(GBuffers::getSwapchainFramebuffer(index));
        renderPassBeginInfo.renderArea = scissor;
        renderPassBeginInfo.renderPass = swapchainRenderPass;

        vDevice->vkCmdBeginRenderPass(frameResources[index].perFrameCommands, &renderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
        {
            SCOPED_CMD_MARKER(frameResources[index].perFrameCommands, ResolveToSwapchain);

            vDevice->vkCmdSetViewport(frameResources[index].perFrameCommands, 0, 1, &viewport);
            vDevice->vkCmdSetScissor(frameResources[index].perFrameCommands, 0, 1, &scissor);

            vDevice->vkCmdBindPipeline(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS, drawQuadPipeline.pipeline);
            vDevice->vkCmdBindDescriptorSets(frameResources[index].perFrameCommands, VK_PIPELINE_BIND_POINT_GRAPHICS
                , drawQuadPipeline.layout, 0, uint32(descSets.size()), descSets.data(), 0, nullptr);

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