#include "VulkanShaderParamResources.h"
#include "../../../RenderInterface/Resources/ShaderResources.h"
#include "../Resources/VulkanSampler.h"
#include "../../../RenderApi/Material/MaterialCommonUniforms.h"
#include "../../../Core/Engine/GameEngine.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../VulkanGraphicsHelper.h"
#include "../../../Core/Math/Math.h"
#include "../Debugging.h"
#include "ShaderReflected.h"
#include "../../../RenderApi/Scene/RenderScene.h"
#include "../../../RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "../VulkanDescriptorAllocator.h"
#include "../../../RenderInterface/Rendering/IRenderCommandList.h"
#include "../../../RenderInterface/ShaderCore/ShaderParameterUtility.h"

void fillDescriptorsSet(std::vector<VkDescriptorPoolSize>& poolAllocateInfo, std::vector<VkDescriptorSetLayoutBinding>& descLayoutBindings
    , const ReflectDescriptorBody& descReflected, const std::vector<std::vector<SpecializationConstantEntry>>& stageSpecializationConsts)
{
    for (const DescEntryBuffer& descriptorInfo : descReflected.uniforms)
    {
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = 1;

        descLayoutBindings[descriptorInfo.data.binding].binding = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = 1;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntryBuffer& descriptorInfo : descReflected.buffers)
    {
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = 1;

        descLayoutBindings[descriptorInfo.data.binding].binding = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = 1;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntryTexelBuffer& descriptorInfo : descReflected.imageBuffers)
    {
        uint32 descCount = 1;
        for (const ArrayDefinition& arrayDimInfo : descriptorInfo.data.data.arraySize)
        {
            if (arrayDimInfo.isSpecializationConst)
            {
                uint32 tempDim;
                if (!SpecializationConstUtility::asValue(tempDim, stageSpecializationConsts[arrayDimInfo.stageIdx][arrayDimInfo.dimension]))
                {
                    fatalAssert(false, "Specialized data retrieval failed");
                }
                descCount *= tempDim;
            }
            else
            {
                descCount *= arrayDimInfo.dimension;
            }
        }
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = descCount;

        descLayoutBindings[descriptorInfo.data.binding].binding = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = descCount;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntryTexelBuffer& descriptorInfo : descReflected.samplerBuffers)
    {
        uint32 descCount = 1;
        for (const ArrayDefinition& arrayDimInfo : descriptorInfo.data.data.arraySize)
        {
            if (arrayDimInfo.isSpecializationConst)
            {
                uint32 tempDim;
                if (!SpecializationConstUtility::asValue(tempDim, stageSpecializationConsts[arrayDimInfo.stageIdx][arrayDimInfo.dimension]))
                {
                    fatalAssert(false, "Specialized data retrieval failed");
                }
                descCount *= tempDim;
            }
            else
            {
                descCount *= arrayDimInfo.dimension;
            }
        }
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = descCount;

        descLayoutBindings[descriptorInfo.data.binding].binding = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = descCount;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntryTexture& descriptorInfo : descReflected.imagesAndImgArrays)
    {
        uint32 descCount = 1;
        for (const ArrayDefinition& arrayDimInfo : descriptorInfo.data.data.arraySize)
        {
            if (arrayDimInfo.isSpecializationConst)
            {
                uint32 tempDim;
                if (!SpecializationConstUtility::asValue(tempDim, stageSpecializationConsts[arrayDimInfo.stageIdx][arrayDimInfo.dimension]))
                {
                    fatalAssert(false, "Specialized data retrieval failed");
                }
                descCount *= tempDim;
            }
            else
            {
                descCount *= arrayDimInfo.dimension;
            }
        }
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = descCount;

        descLayoutBindings[descriptorInfo.data.binding].binding = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = descCount;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntryTexture& descriptorInfo : descReflected.textureAndArrays)
    {
        uint32 descCount = 1;
        for (const ArrayDefinition& arrayDimInfo : descriptorInfo.data.data.arraySize)
        {
            if (arrayDimInfo.isSpecializationConst)
            {
                uint32 tempDim;
                if (!SpecializationConstUtility::asValue(tempDim, stageSpecializationConsts[arrayDimInfo.stageIdx][arrayDimInfo.dimension]))
                {
                    fatalAssert(false, "Specialized data retrieval failed");
                }
                descCount *= tempDim;
            }
            else
            {
                descCount *= arrayDimInfo.dimension;
            }
        }
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = descCount;

        descLayoutBindings[descriptorInfo.data.binding].binding = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = descCount;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntryTexture& descriptorInfo : descReflected.sampledTexAndArrays)
    {
        uint32 descCount = 1;
        for (const ArrayDefinition& arrayDimInfo : descriptorInfo.data.data.arraySize)
        {
            if (arrayDimInfo.isSpecializationConst)
            {
                uint32 tempDim;
                if (!SpecializationConstUtility::asValue(tempDim, stageSpecializationConsts[arrayDimInfo.stageIdx][arrayDimInfo.dimension]))
                {
                    fatalAssert(false, "Specialized data retrieval failed");
                }
                descCount *= tempDim;
            }
            else
            {
                descCount *= arrayDimInfo.dimension;
            }
        }
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = descCount;

        descLayoutBindings[descriptorInfo.data.binding].binding = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = descCount;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntrySampler& descriptorInfo : descReflected.samplers)
    {
        uint32 descCount = 1;
        for (const ArrayDefinition& arrayDimInfo : descriptorInfo.data.data)
        {
            if (arrayDimInfo.isSpecializationConst)
            {
                uint32 tempDim;
                if (!SpecializationConstUtility::asValue(tempDim, stageSpecializationConsts[arrayDimInfo.stageIdx][arrayDimInfo.dimension]))
                {
                    fatalAssert(false, "Specialized data retrieval failed");
                }
                descCount *= tempDim;
            }
            else
            {
                descCount *= arrayDimInfo.dimension;
            }
        }
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = descCount;

        descLayoutBindings[descriptorInfo.data.binding].binding = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = descCount;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
    for (const DescEntrySubpassInput& descriptorInfo : descReflected.subpassInputs)
    {
        poolAllocateInfo[descriptorInfo.data.binding].type = VkDescriptorType(descriptorInfo.data.type);
        poolAllocateInfo[descriptorInfo.data.binding].descriptorCount = 1;

        descLayoutBindings[descriptorInfo.data.binding].binding = descriptorInfo.data.binding;
        descLayoutBindings[descriptorInfo.data.binding].descriptorCount = 1;
        descLayoutBindings[descriptorInfo.data.binding].descriptorType = VkDescriptorType(descriptorInfo.data.type);
        descLayoutBindings[descriptorInfo.data.binding].stageFlags = descriptorInfo.data.stagesUsed;
    }
}

//////////////////////////////////////////////////////////////////////////
// VulkanShaderParamsLayout
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanShaderSetParamsLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)

VulkanShaderSetParamsLayout::VulkanShaderSetParamsLayout(const ShaderResource* shaderResource, uint32 setID)
    : BaseType(shaderResource, setID)
{}

String VulkanShaderSetParamsLayout::getObjectName() const
{
    return "";
}

uint64 VulkanShaderSetParamsLayout::getDispatchableHandle() const
{
    return uint64(descriptorLayout);
}

void VulkanShaderSetParamsLayout::init()
{
    BaseType::init();

    std::vector<std::vector<SpecializationConstantEntry>> specializationConsts;
    {
        std::map<String, SpecializationConstantEntry> specConsts;
        respectiveShaderRes->getSpecializationConsts(specConsts);
        ShaderParameterUtility::convertNamedSpecConstsToPerStage(specializationConsts, specConsts, respectiveShaderRes->getReflection());
    }
    for (const ReflectDescriptorBody& descriptorsSet : respectiveShaderRes->getReflection()->descriptorsSets)
    {
        if (descriptorsSet.set == shaderSetID)
        {
            // Since bindings are sorted ascending
            uint32 maxBinding = descriptorsSet.usedBindings.back() + 1;
            poolAllocation.resize(maxBinding);
            layoutBindings.resize(maxBinding);
            fillDescriptorsSet(poolAllocation, layoutBindings, descriptorsSet, specializationConsts);
        }
    }

    // Remove unnecessary descriptors set info
    for (int32 i = 0; i < poolAllocation.size();)
    {
        if (poolAllocation[i].descriptorCount == 0)
        {
            std::iter_swap(poolAllocation.begin() + i, poolAllocation.end() - 1);
            poolAllocation.pop_back();

            std::iter_swap(layoutBindings.begin() + i, layoutBindings.end() - 1);
            layoutBindings.pop_back();
        }
        else
        {
            ++i;
        }
    }
    // sort and merge type duplicate descriptor pool size
    std::sort(poolAllocation.begin(), poolAllocation.end(), [](const VkDescriptorPoolSize& lhs, const VkDescriptorPoolSize& rhs)
        {
            return lhs.type < rhs.type;
        }
    );
    for (std::vector<VkDescriptorPoolSize>::iterator itr = poolAllocation.begin(); itr != poolAllocation.end();)
    {
        auto endItr = itr + 1;
        while (endItr != poolAllocation.end() && endItr->type == itr->type)
        {
            itr->descriptorCount += endItr->descriptorCount;
            ++endItr;
        }
        itr = poolAllocation.erase(itr + 1, endItr);
    }


    //reinitResources();
    IGraphicsInstance* graphicsInstance = gEngine->getRenderManager()->getGraphicsInstance();

    DESCRIPTOR_SET_LAYOUT_CREATE_INFO(descLayoutCreateInfo);
    descLayoutCreateInfo.bindingCount = uint32(layoutBindings.size());
    descLayoutCreateInfo.pBindings = layoutBindings.data();
    descriptorLayout = VulkanGraphicsHelper::createDescriptorsSetLayout(graphicsInstance, descLayoutCreateInfo);
    VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject(this);
}

void VulkanShaderSetParamsLayout::release()
{
    if (descriptorLayout != nullptr)
    {
        IGraphicsInstance* graphicsInstance = gEngine->getRenderManager()->getGraphicsInstance();
        VulkanGraphicsHelper::destroyDescriptorsSetLayout(graphicsInstance, descriptorLayout);
        descriptorLayout = nullptr;
    }
    BaseType::release();
}

String VulkanShaderSetParamsLayout::getResourceName() const
{
    return getObjectName();
}

const std::vector<VkDescriptorPoolSize>& VulkanShaderSetParamsLayout::getDescPoolAllocInfo() const
{
    return poolAllocation;
}

//////////////////////////////////////////////////////////////////////////
// VulkanShaderUniqDescLayout
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanShaderUniqDescLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)

VulkanShaderUniqDescLayout::VulkanShaderUniqDescLayout(const ShaderResource* shaderResource)
    : BaseType(shaderResource, DESC_SET_ID)
{}

void VulkanShaderUniqDescLayout::bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const
{
    respectiveShaderRes->bindBufferParamInfo(bindingBuffers);
}

String VulkanShaderUniqDescLayout::getObjectName() const
{
    return respectiveShaderRes->getResourceName() + "_DescriptorsSetLayout2";
}

//////////////////////////////////////////////////////////////////////////
// VulkanVertexUniqDescLayout
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanVertexUniqDescLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)

VulkanVertexUniqDescLayout::VulkanVertexUniqDescLayout(const ShaderResource* shaderResource)
    : BaseType(shaderResource, DESC_SET_ID)
{}

String VulkanVertexUniqDescLayout::getObjectName() const
{
    return respectiveShaderRes->getResourceName() + "_DescriptorsSetLayout1";
}

void VulkanVertexUniqDescLayout::bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const
{
    const std::map<String, ShaderBufferParamInfo*>& vertexSpecificBufferInfo = 
        MaterialVertexUniforms::bufferParamInfo(static_cast<const DrawMeshShader*>(respectiveShaderRes)->vertexUsage());

    for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : vertexSpecificBufferInfo)
    {
        auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

        debugAssert(foundDescBinding != bindingBuffers.end());

        foundDescBinding->second->bufferParamInfo = bufferInfo.second;
    }
}

//////////////////////////////////////////////////////////////////////////
// VulkanViewUniqDescLayout
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanViewUniqDescLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)

VulkanViewUniqDescLayout::VulkanViewUniqDescLayout(const ShaderResource* shaderResource)
    : BaseType(shaderResource, DESC_SET_ID)
{}

String VulkanViewUniqDescLayout::getObjectName() const
{
    return respectiveShaderRes->getResourceName() + "_DescriptorsSetLayout0";
}

void VulkanViewUniqDescLayout::bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const
{
    const std::map<String, ShaderBufferParamInfo*>& viewSpecificBufferInfo = RenderSceneBase::sceneViewParamInfo();

    for (const std::pair<const String, ShaderBufferParamInfo*>& bufferInfo : viewSpecificBufferInfo)
    {
        auto foundDescBinding = bindingBuffers.find(bufferInfo.first);

        debugAssert(foundDescBinding != bindingBuffers.end());

        foundDescBinding->second->bufferParamInfo = bufferInfo.second;
    }
}

//////////////////////////////////////////////////////////////////////////
// VulkanShaderParametersLayout
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanShaderParametersLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)

VulkanShaderParametersLayout::VulkanShaderParametersLayout(const ShaderResource* shaderResource)
    : BaseType(shaderResource)
{}

String VulkanShaderParametersLayout::getObjectName() const
{
    return getResourceName();
}

void VulkanShaderParametersLayout::init()
{
    BaseType::init();
    std::vector<std::vector<SpecializationConstantEntry>> specializationConsts;
    {
        std::map<String, SpecializationConstantEntry> specConsts;
        respectiveShaderRes->getSpecializationConsts(specConsts);
        ShaderParameterUtility::convertNamedSpecConstsToPerStage(specializationConsts, specConsts, respectiveShaderRes->getReflection());
    }

    for (const ReflectDescriptorBody& descriptorsSet : respectiveShaderRes->getReflection()->descriptorsSets)
    {
        SetParametersLayoutInfo& descSetLayoutInfo = setToLayoutInfo[descriptorsSet.set];

        // Since bindings are sorted ascending
        uint32 maxBinding = descriptorsSet.usedBindings.back() + 1;
        descSetLayoutInfo.poolAllocation.resize(maxBinding);
        descSetLayoutInfo.layoutBindings.resize(maxBinding);
        fillDescriptorsSet(descSetLayoutInfo.poolAllocation, descSetLayoutInfo.layoutBindings, descriptorsSet, specializationConsts);

        // Remove unnecessary descriptors set info
        for (int32 i = 0; i < descSetLayoutInfo.poolAllocation.size();)
        {
            if (descSetLayoutInfo.poolAllocation[i].descriptorCount == 0)
            {
                std::iter_swap(descSetLayoutInfo.poolAllocation.begin() + i, descSetLayoutInfo.poolAllocation.end() - 1);
                descSetLayoutInfo.poolAllocation.pop_back();

                std::iter_swap(descSetLayoutInfo.layoutBindings.begin() + i, descSetLayoutInfo.layoutBindings.end() - 1);
                descSetLayoutInfo.layoutBindings.pop_back();
            }
            else
            {
                ++i;
            }
        }
        // sort and merge type duplicate descriptor pool size
        std::sort(descSetLayoutInfo.poolAllocation.begin(), descSetLayoutInfo.poolAllocation.end()
            , [](const VkDescriptorPoolSize& lhs, const VkDescriptorPoolSize& rhs)
            {
                return lhs.type < rhs.type;
            }
        );
        for (std::vector<VkDescriptorPoolSize>::iterator itr = descSetLayoutInfo.poolAllocation.begin()
            ; itr != descSetLayoutInfo.poolAllocation.end();)
        {
            auto endItr = itr + 1;
            while (endItr != descSetLayoutInfo.poolAllocation.end() && endItr->type == itr->type)
            {
                itr->descriptorCount += endItr->descriptorCount;
                ++endItr;
            }
            itr = descSetLayoutInfo.poolAllocation.erase(itr + 1, endItr);
        }
    }

    //reinitResources();
    IGraphicsInstance* graphicsInstance = gEngine->getRenderManager()->getGraphicsInstance();
    for (std::pair<const uint32, SetParametersLayoutInfo>& setParamsLayout : setToLayoutInfo)
    {
        DESCRIPTOR_SET_LAYOUT_CREATE_INFO(descLayoutCreateInfo);
        descLayoutCreateInfo.bindingCount = uint32(setParamsLayout.second.layoutBindings.size());
        descLayoutCreateInfo.pBindings = setParamsLayout.second.layoutBindings.data();
        setParamsLayout.second.descriptorLayout = VulkanGraphicsHelper::createDescriptorsSetLayout(graphicsInstance, descLayoutCreateInfo);

        VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject(uint64(setParamsLayout.second.descriptorLayout)
            , getResourceName() + std::to_string(setParamsLayout.first), getObjectType());
    }
}

void VulkanShaderParametersLayout::release()
{
    for (std::pair<const uint32, SetParametersLayoutInfo>& setParamsLayout : setToLayoutInfo)
    {
        if (setParamsLayout.second.descriptorLayout != nullptr)
        {
            IGraphicsInstance* graphicsInstance = gEngine->getRenderManager()->getGraphicsInstance();
            VulkanGraphicsHelper::destroyDescriptorsSetLayout(graphicsInstance, setParamsLayout.second.descriptorLayout);
            setParamsLayout.second.descriptorLayout = nullptr;
        }
    }
    BaseType::release();
}

String VulkanShaderParametersLayout::getResourceName() const
{
    return respectiveShaderRes->getResourceName() + "_DescSetLayout";
}

const std::vector<VkDescriptorPoolSize>& VulkanShaderParametersLayout::getDescPoolAllocInfo(uint32 setIdx) const
{
    auto foundItr = setToLayoutInfo.find(setIdx);
    debugAssert(foundItr != setToLayoutInfo.cend());
    return foundItr->second.poolAllocation;
}

VkDescriptorSetLayout VulkanShaderParametersLayout::getDescSetLayout(uint32 setIdx) const
{
    auto foundItr = setToLayoutInfo.find(setIdx);
    debugAssert(foundItr != setToLayoutInfo.cend());
    return foundItr->second.descriptorLayout;
}

//////////////////////////////////////////////////////////////////////////
// VulkanShaderSetParameters implementation
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanShaderSetParameters, VK_OBJECT_TYPE_DESCRIPTOR_SET)

String VulkanShaderSetParameters::getObjectName() const
{
    return getResourceName() + "_DescSet" + std::to_string(static_cast<const ShaderSetParametersLayout*>(paramLayout)->getSetID());
}

void VulkanShaderSetParameters::init()
{
    ignoredSets.clear();
    BaseType::init();

    IGraphicsInstance* graphicsInstance = gEngine->getRenderManager()->getGraphicsInstance();
    VulkanDescriptorsSetAllocator* descsSetAllocator = VulkanGraphicsHelper::getDescriptorsSetAllocator(graphicsInstance);
    DescriptorsSetQuery query;
    query.supportedTypes.insert(static_cast<const VulkanShaderSetParamsLayout*>(paramLayout)->getDescPoolAllocInfo().cbegin()
        , static_cast<const VulkanShaderSetParamsLayout*>(paramLayout)->getDescPoolAllocInfo().cend());
    descriptorsSet = descsSetAllocator->allocDescriptorsSet(query, static_cast<const VulkanShaderSetParamsLayout*>(paramLayout)->descriptorLayout);
    debugAssert(descriptorsSet != VK_NULL_HANDLE);

    VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject(this);

    std::vector<VkWriteDescriptorSet> bufferDescWrites;
    bufferDescWrites.reserve(shaderBuffers.size());
    std::vector<VkDescriptorBufferInfo> bufferInfos(shaderBuffers.size());
    uint32 descWriteIdx = 0;
    for (const std::pair<const String, BufferParametersData>& bufferParam : shaderBuffers)
    {
        WRITE_RESOURCE_TO_DESCRIPTORS_SET(writeDescSet);
        writeDescSet.dstSet = descriptorsSet;
        writeDescSet.dstBinding = bufferParam.second.descriptorInfo->bufferEntryPtr->data.binding;
        writeDescSet.descriptorCount = 1;
        writeDescSet.descriptorType = VkDescriptorType(bufferParam.second.descriptorInfo->bufferEntryPtr->data.type);
        writeDescSet.pBufferInfo = &bufferInfos[descWriteIdx];
        bufferDescWrites.emplace_back(writeDescSet);

        bufferInfos[descWriteIdx].buffer = static_cast<VulkanBufferResource*>(bufferParam.second.gpuBuffer)->buffer;
        bufferInfos[descWriteIdx].offset = 0;
        bufferInfos[descWriteIdx].range = bufferParam.second.gpuBuffer->getResourceSize();

        ++descWriteIdx;
    }

    VulkanGraphicsHelper::updateDescriptorsSet(graphicsInstance, bufferDescWrites, {});
    ENQUEUE_COMMAND_NODEBUG(FinalizeShaderParams, LAMBDA_BODY
    (
        updateParams(cmdList, graphicsInstance);
    ), this);
}

void VulkanShaderSetParameters::release()
{
    VulkanDescriptorsSetAllocator* descsSetAllocator = VulkanGraphicsHelper::getDescriptorsSetAllocator(gEngine->getRenderManager()->getGraphicsInstance());
    descsSetAllocator->releaseDescriptorsSet(descriptorsSet);
    descriptorsSet = VK_NULL_HANDLE;
    BaseType::release();
}

void VulkanShaderSetParameters::updateParams(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
{
    BaseType::updateParams(cmdList, graphicsInstance);
    std::vector<DescriptorWriteData> writeDescs;
    writeDescs.reserve(bufferResourceUpdates.size() + textureUpdates.size() + texelUpdates.size() + samplerUpdates.size());
    const uint32 texelStartIdx = uint32(bufferResourceUpdates.size());
    const uint32 textureStartIdx = texelStartIdx + uint32(texelUpdates.size());
    const uint32 samplerStartIdx = textureStartIdx + uint32(textureUpdates.size());

    if (writeDescs.capacity() == 0)
    {
        return;
    }

    std::vector<VkDescriptorBufferInfo> bufferInfos;
    bufferInfos.reserve(bufferResourceUpdates.size());
    std::vector<VkBufferView> texelViews;
    texelViews.reserve(texelUpdates.size());
    std::vector<VkDescriptorImageInfo> imageAndSamplerInfos;
    imageAndSamplerInfos.reserve(textureUpdates.size() + samplerUpdates.size());


    for (const String& bufferParamName : bufferResourceUpdates)
    {
        DescriptorWriteData& bufferWriteData = writeDescs.emplace_back();
        bufferWriteData.ParamData.buffer = &shaderBuffers.at(bufferParamName);
        bufferWriteData.writeInfoIdx = uint32(bufferInfos.size());

        VkDescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = static_cast<VulkanBufferResource*>(bufferWriteData.ParamData.buffer->gpuBuffer)->buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = bufferWriteData.ParamData.buffer->gpuBuffer->getResourceSize();
        bufferInfos.emplace_back(bufferInfo);
    }
    for (const std::pair<String, uint32>& texelBufferUpdate : texelUpdates)
    {
        DescriptorWriteData& texelWriteData = writeDescs.emplace_back();
        texelWriteData.ParamData.texel = &shaderTexels.at(texelBufferUpdate.first);
        texelWriteData.writeInfoIdx = uint32(texelViews.size());
        texelWriteData.arrayIdx = texelBufferUpdate.second;

        texelViews.emplace_back(
            static_cast<VulkanBufferResource*>(texelWriteData.ParamData.texel->gpuBuffers[texelBufferUpdate.second])->getBufferView({}));
    }
    for (const std::pair<String, uint32>& textureUpdate : textureUpdates)
    {
        DescriptorWriteData& texWriteData = writeDescs.emplace_back();
        texWriteData.ParamData.texture = &shaderTextures.at(textureUpdate.first);
        texWriteData.writeInfoIdx = uint32(imageAndSamplerInfos.size());
        texWriteData.arrayIdx = textureUpdate.second;

        VkDescriptorImageInfo& imageInfo = imageAndSamplerInfos.emplace_back();
        imageInfo.imageLayout = texWriteData.ParamData.texture->textures[textureUpdate.second].texture->isShaderWrite()
            ? VkImageLayout::VK_IMAGE_LAYOUT_GENERAL : VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler = texWriteData.ParamData.texture->textures[textureUpdate.second].sampler
            ? static_cast<VulkanSampler*>(texWriteData.ParamData.texture->textures[textureUpdate.second].sampler.get())->sampler : nullptr;

        imageInfo.imageView = static_cast<VulkanImageResource*>(texWriteData.ParamData.texture->textures[textureUpdate.second].texture)
            ->getImageView(texWriteData.ParamData.texture->textures[textureUpdate.second].viewInfo);
    }
    for (const std::pair<String, uint32>& samplerUpdate : samplerUpdates)
    {
        DescriptorWriteData& samplerWriteData = writeDescs.emplace_back();
        samplerWriteData.ParamData.sampler = &shaderSamplers.at(samplerUpdate.first);
        samplerWriteData.writeInfoIdx = uint32(imageAndSamplerInfos.size());
        samplerWriteData.arrayIdx = samplerUpdate.second;

        VkDescriptorImageInfo& imageInfo = imageAndSamplerInfos.emplace_back();
        imageInfo.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = nullptr;
        imageInfo.sampler = static_cast<VulkanSampler*>(samplerWriteData.ParamData.sampler->samplers[samplerUpdate.second].get())->sampler;
    }

    std::vector<VkWriteDescriptorSet> vkWrites(writeDescs.size());
    for (uint32 i = 0; i < texelStartIdx; ++i)
    {
        WRITE_RESOURCE_TO_DESCRIPTORS_SET(writeDescSet);
        writeDescSet.dstSet = descriptorsSet;
        writeDescSet.pBufferInfo = &bufferInfos[writeDescs[i].writeInfoIdx];
        writeDescSet.dstBinding = writeDescs[i].ParamData.buffer->descriptorInfo->bufferEntryPtr->data.binding;
        writeDescSet.descriptorType = VkDescriptorType(writeDescs[i].ParamData.buffer->descriptorInfo->bufferEntryPtr->data.type);
        writeDescSet.descriptorCount = 1;
        writeDescSet.dstArrayElement = writeDescs[i].arrayIdx;
        vkWrites[i] = writeDescSet;
    }
    for (uint32 i = texelStartIdx; i < textureStartIdx; ++i)
    {
        WRITE_RESOURCE_TO_DESCRIPTORS_SET(writeDescSet);
        writeDescSet.dstSet = descriptorsSet;
        writeDescSet.pTexelBufferView = &texelViews[writeDescs[i].writeInfoIdx];
        writeDescSet.dstBinding = writeDescs[i].ParamData.texel->descriptorInfo->texelBufferEntryPtr->data.binding;
        writeDescSet.descriptorType = VkDescriptorType(writeDescs[i].ParamData.texel->descriptorInfo->texelBufferEntryPtr->data.type);
        writeDescSet.descriptorCount = 1;
        writeDescSet.dstArrayElement = writeDescs[i].arrayIdx;
        vkWrites[i] = writeDescSet;
    }
    for (uint32 i = textureStartIdx; i < samplerStartIdx; ++i)
    {
        WRITE_RESOURCE_TO_DESCRIPTORS_SET(writeDescSet);
        writeDescSet.dstSet = descriptorsSet;
        writeDescSet.pImageInfo = &imageAndSamplerInfos[writeDescs[i].writeInfoIdx];
        writeDescSet.dstBinding = writeDescs[i].ParamData.texture->descriptorInfo->textureEntryPtr->data.binding;
        writeDescSet.descriptorType = VkDescriptorType(writeDescs[i].ParamData.texture->descriptorInfo->textureEntryPtr->data.type);
        writeDescSet.descriptorCount = 1;
        writeDescSet.dstArrayElement = writeDescs[i].arrayIdx;
        vkWrites[i] = writeDescSet;
    }
    const uint32 endIdx = uint32(writeDescs.size());
    for (uint32 i = samplerStartIdx; i < endIdx; ++i)
    {
        WRITE_RESOURCE_TO_DESCRIPTORS_SET(writeDescSet);
        writeDescSet.dstSet = descriptorsSet;
        writeDescSet.pImageInfo = &imageAndSamplerInfos[writeDescs[i].writeInfoIdx];
        writeDescSet.dstBinding = writeDescs[i].ParamData.sampler->descriptorInfo->samplerEntryPtr->data.binding;
        writeDescSet.descriptorType = VkDescriptorType(writeDescs[i].ParamData.sampler->descriptorInfo->samplerEntryPtr->data.type);
        writeDescSet.descriptorCount = 1;
        writeDescSet.dstArrayElement = writeDescs[i].arrayIdx;
        vkWrites[i] = writeDescSet;
    }

    VulkanGraphicsHelper::updateDescriptorsSet(graphicsInstance, vkWrites, {});
    texelUpdates.clear();
    textureUpdates.clear();
    samplerUpdates.clear();
}

uint64 VulkanShaderSetParameters::getDispatchableHandle() const
{
    return uint64(descriptorsSet);
}

//////////////////////////////////////////////////////////////////////////
// VulkanShaderParameters implementation
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanShaderParameters, VK_OBJECT_TYPE_DESCRIPTOR_SET)

String VulkanShaderParameters::getObjectName() const
{
    return getResourceName() + "_DescSet";
}

void VulkanShaderParameters::init()
{
    BaseType::init();

    std::map<uint32, int32> setIdToVectorIdx;
    std::vector<VkDescriptorSetLayout> layouts;
    DescriptorsSetQuery query;

    // Compress all descriptors set descriptor type size to common pool sizes to alloc all sets from same pool
    {
        std::map<VkDescriptorType, uint32> descriptorPoolSizes;

        const ShaderReflected* reflectedData = static_cast<const ShaderParametersLayout*>(paramLayout)->getShaderResource()->getReflection();
        for (const ReflectDescriptorBody& descriptorsBody : reflectedData->descriptorsSets)
        {
            if (ignoredSets.find(descriptorsBody.set) != ignoredSets.end())
            {
                continue;
            }

            setIdToVectorIdx[descriptorsBody.set] = int32(layouts.size());
            layouts.emplace_back(static_cast<const VulkanShaderParametersLayout*>(paramLayout)
                ->getDescSetLayout(descriptorsBody.set));

            for (const VkDescriptorPoolSize& descriptorPoolSize : static_cast<const VulkanShaderParametersLayout*>(paramLayout)
                ->getDescPoolAllocInfo(descriptorsBody.set))
            {
                 auto poolSizeItr = descriptorPoolSizes.find(descriptorPoolSize.type);
                 if (poolSizeItr == descriptorPoolSizes.end())
                 {
                     descriptorPoolSizes[descriptorPoolSize.type] = descriptorPoolSize.descriptorCount;
                 }
                 else
                 {
                     poolSizeItr->second = Math::max(poolSizeItr->second, descriptorPoolSize.descriptorCount);
                 }
            }
        }

        for (const std::pair<const VkDescriptorType, uint32>& descPoolSize : descriptorPoolSizes)
        {
            query.supportedTypes.insert({ descPoolSize.first, descPoolSize.second });
        }
    }

    std::vector<VkDescriptorSet> descsSets;

    IGraphicsInstance* graphicsInstance = gEngine->getRenderManager()->getGraphicsInstance();
    VulkanDescriptorsSetAllocator* descsSetAllocator = VulkanGraphicsHelper::getDescriptorsSetAllocator(graphicsInstance);
    if (!descsSetAllocator->allocDescriptorsSets(descsSets, query, layouts))
    {
        Logger::error("VulkanShaderParameters", "%s() : Allocation of descriptors set failed %s", __func__, getResourceName().getChar());
        return;
    }

    for (const std::pair<const uint32, int32>& descSetToIdx : setIdToVectorIdx)
    {
        descriptorsSets[descSetToIdx.first] = descsSets[descSetToIdx.second];
        VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject(uint64(descsSets[descSetToIdx.second])
            , getObjectName() + std::to_string(descSetToIdx.second), getObjectType());
    }
    
    std::vector<VkWriteDescriptorSet> bufferDescWrites;
    bufferDescWrites.reserve(shaderBuffers.size());
    std::vector<VkDescriptorBufferInfo> bufferInfos(shaderBuffers.size());
    uint32 descWriteIdx = 0;
    for (const std::pair<const String, BufferParametersData>& bufferParam : shaderBuffers)
    {
        WRITE_RESOURCE_TO_DESCRIPTORS_SET(writeDescSet);
        writeDescSet.dstSet = descriptorsSets[static_cast<const ShaderParametersLayout*>(paramLayout)->getSetID(bufferParam.first)];
        writeDescSet.dstBinding = bufferParam.second.descriptorInfo->bufferEntryPtr->data.binding;
        writeDescSet.descriptorType = VkDescriptorType(bufferParam.second.descriptorInfo->bufferEntryPtr->data.type);
        writeDescSet.descriptorCount = 1;
        writeDescSet.pBufferInfo = &bufferInfos[descWriteIdx];
        bufferDescWrites.emplace_back(writeDescSet);

        bufferInfos[descWriteIdx].buffer = static_cast<VulkanBufferResource*>(bufferParam.second.gpuBuffer)->buffer;
        bufferInfos[descWriteIdx].offset = 0;
        bufferInfos[descWriteIdx].range = bufferParam.second.gpuBuffer->getResourceSize();

        ++descWriteIdx;
    }

    VulkanGraphicsHelper::updateDescriptorsSet(graphicsInstance, bufferDescWrites, {});
    ENQUEUE_COMMAND_NODEBUG(FinalizeShaderParams, LAMBDA_BODY
    (
        updateParams(cmdList, graphicsInstance);
    ), this);
}

void VulkanShaderParameters::release()
{
    VulkanDescriptorsSetAllocator* descsSetAllocator = VulkanGraphicsHelper::getDescriptorsSetAllocator(gEngine->getRenderManager()->getGraphicsInstance());
    for (const std::pair<const uint32, VkDescriptorSet>& setIdToSet : descriptorsSets)
    {
        descsSetAllocator->releaseDescriptorsSet(setIdToSet.second);
    }
    descriptorsSets.clear();

    BaseType::release();
}

void VulkanShaderParameters::updateParams(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance)
{
    BaseType::updateParams(cmdList, graphicsInstance);

    std::vector<DescriptorWriteData> writeDescs;
    writeDescs.reserve(bufferResourceUpdates.size() + textureUpdates.size() + texelUpdates.size() + samplerUpdates.size());
    const uint32 texelStartIdx = uint32(bufferResourceUpdates.size());
    const uint32 textureStartIdx = texelStartIdx + uint32(texelUpdates.size());
    const uint32 samplerStartIdx = textureStartIdx + uint32(textureUpdates.size());

    if (writeDescs.capacity() == 0)
    {
        return;
    }

    std::vector<VkDescriptorBufferInfo> bufferInfos;
    bufferInfos.reserve(bufferResourceUpdates.size());
    std::vector<VkBufferView> texelViews;
    texelViews.reserve(texelUpdates.size());
    std::vector<VkDescriptorImageInfo> imageAndSamplerInfos;
    imageAndSamplerInfos.reserve(textureUpdates.size() + samplerUpdates.size());

    for (const String& bufferParamName : bufferResourceUpdates)
    {
        DescriptorWriteData& bufferWriteData = writeDescs.emplace_back();
        bufferWriteData.ParamData.buffer = &shaderBuffers.at(bufferParamName);
        bufferWriteData.setID = static_cast<const ShaderParametersLayout*>(paramLayout)->getSetID(bufferParamName);
        bufferWriteData.writeInfoIdx = uint32(bufferInfos.size());

        VkDescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = static_cast<VulkanBufferResource*>(bufferWriteData.ParamData.buffer->gpuBuffer)->buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = bufferWriteData.ParamData.buffer->gpuBuffer->getResourceSize();
        bufferInfos.emplace_back(bufferInfo);
    }
    for (const std::pair<String, uint32>& texelBufferUpdate : texelUpdates)
    {
        DescriptorWriteData& texelWriteData = writeDescs.emplace_back();
        texelWriteData.ParamData.texel = &shaderTexels.at(texelBufferUpdate.first);
        texelWriteData.setID = static_cast<const ShaderParametersLayout*>(paramLayout)->getSetID(texelBufferUpdate.first);
        texelWriteData.writeInfoIdx = uint32(texelViews.size());
        texelWriteData.arrayIdx = texelBufferUpdate.second;

        texelViews.emplace_back(
            static_cast<VulkanBufferResource*>(texelWriteData.ParamData.texel->gpuBuffers[texelBufferUpdate.second])->getBufferView({}));
    }
    for (const std::pair<String, uint32>& textureUpdate : textureUpdates)
    {
        DescriptorWriteData& texWriteData = writeDescs.emplace_back();
        texWriteData.ParamData.texture = &shaderTextures.at(textureUpdate.first);
        texWriteData.setID = static_cast<const ShaderParametersLayout*>(paramLayout)->getSetID(textureUpdate.first);
        texWriteData.writeInfoIdx = uint32(imageAndSamplerInfos.size());
        texWriteData.arrayIdx = textureUpdate.second;

        VkDescriptorImageInfo& imageInfo = imageAndSamplerInfos.emplace_back();
        imageInfo.imageLayout = texWriteData.ParamData.texture->textures[textureUpdate.second].texture->isShaderWrite()
            ? VkImageLayout::VK_IMAGE_LAYOUT_GENERAL : VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler = texWriteData.ParamData.texture->textures[textureUpdate.second].sampler
            ? static_cast<VulkanSampler*>(texWriteData.ParamData.texture->textures[textureUpdate.second].sampler.get())->sampler : nullptr;

        imageInfo.imageView = static_cast<VulkanImageResource*>(texWriteData.ParamData.texture->textures[textureUpdate.second].texture)
            ->getImageView(texWriteData.ParamData.texture->textures[textureUpdate.second].viewInfo);
    }
    for (const std::pair<String, uint32>& samplerUpdate : samplerUpdates)
    {
        DescriptorWriteData& samplerWriteData = writeDescs.emplace_back();
        samplerWriteData.ParamData.sampler = &shaderSamplers.at(samplerUpdate.first);
        samplerWriteData.setID = static_cast<const ShaderParametersLayout*>(paramLayout)->getSetID(samplerUpdate.first);
        samplerWriteData.writeInfoIdx = uint32(imageAndSamplerInfos.size());
        samplerWriteData.arrayIdx = samplerUpdate.second;

        VkDescriptorImageInfo& imageInfo = imageAndSamplerInfos.emplace_back();
        imageInfo.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = nullptr;
        imageInfo.sampler = static_cast<VulkanSampler*>(samplerWriteData.ParamData.sampler->samplers[samplerUpdate.second].get())->sampler;
    }

    std::vector<VkWriteDescriptorSet> vkWrites(writeDescs.size());
    for (uint32 i = 0; i < texelStartIdx; ++i)
    {
        WRITE_RESOURCE_TO_DESCRIPTORS_SET(writeDescSet);
        writeDescSet.dstSet = descriptorsSets[writeDescs[i].setID];
        writeDescSet.pBufferInfo = &bufferInfos[writeDescs[i].writeInfoIdx];
        writeDescSet.dstBinding = writeDescs[i].ParamData.buffer->descriptorInfo->bufferEntryPtr->data.binding;
        writeDescSet.descriptorType = VkDescriptorType(writeDescs[i].ParamData.buffer->descriptorInfo->bufferEntryPtr->data.type);
        writeDescSet.descriptorCount = 1;
        writeDescSet.dstArrayElement = writeDescs[i].arrayIdx;
        vkWrites[i] = writeDescSet;
    }
    for (uint32 i = texelStartIdx; i < textureStartIdx; ++i)
    {
        WRITE_RESOURCE_TO_DESCRIPTORS_SET(writeDescSet);
        writeDescSet.dstSet = descriptorsSets[writeDescs[i].setID];
        writeDescSet.pTexelBufferView = &texelViews[writeDescs[i].writeInfoIdx];
        writeDescSet.dstBinding = writeDescs[i].ParamData.texel->descriptorInfo->texelBufferEntryPtr->data.binding;
        writeDescSet.descriptorType = VkDescriptorType(writeDescs[i].ParamData.texel->descriptorInfo->texelBufferEntryPtr->data.type);
        writeDescSet.descriptorCount = 1;
        writeDescSet.dstArrayElement = writeDescs[i].arrayIdx;
        vkWrites[i] = writeDescSet;
    }
    for (uint32 i = textureStartIdx; i < samplerStartIdx; ++i)
    {
        WRITE_RESOURCE_TO_DESCRIPTORS_SET(writeDescSet);
        writeDescSet.dstSet = descriptorsSets[writeDescs[i].setID];
        writeDescSet.pImageInfo = &imageAndSamplerInfos[writeDescs[i].writeInfoIdx];
        writeDescSet.dstBinding = writeDescs[i].ParamData.texture->descriptorInfo->textureEntryPtr->data.binding;
        writeDescSet.descriptorType = VkDescriptorType(writeDescs[i].ParamData.texture->descriptorInfo->textureEntryPtr->data.type);
        writeDescSet.descriptorCount = 1;
        writeDescSet.dstArrayElement = writeDescs[i].arrayIdx;
        vkWrites[i] = writeDescSet;
    }
    const uint32 endIdx = uint32(writeDescs.size());
    for (uint32 i = samplerStartIdx; i < endIdx; ++i)
    {
        WRITE_RESOURCE_TO_DESCRIPTORS_SET(writeDescSet);
        writeDescSet.dstSet = descriptorsSets[writeDescs[i].setID];
        writeDescSet.pImageInfo = &imageAndSamplerInfos[writeDescs[i].writeInfoIdx];
        writeDescSet.dstBinding = writeDescs[i].ParamData.sampler->descriptorInfo->samplerEntryPtr->data.binding;
        writeDescSet.descriptorType = VkDescriptorType(writeDescs[i].ParamData.sampler->descriptorInfo->samplerEntryPtr->data.type);
        writeDescSet.descriptorCount = 1;
        writeDescSet.dstArrayElement = writeDescs[i].arrayIdx;
        vkWrites[i] = writeDescSet;
    }
    VulkanGraphicsHelper::updateDescriptorsSet(graphicsInstance, vkWrites, {});
    texelUpdates.clear();
    textureUpdates.clear();
    samplerUpdates.clear();
}

