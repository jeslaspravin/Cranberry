#include "VulkanShaderParamResources.h"
#include "../../../RenderInterface/Resources/ShaderResources.h"
#include "../../../RenderApi/Material/MaterialCommonUniforms.h"
#include "../../../Core/Engine/GameEngine.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../VulkanGraphicsHelper.h"
#include "../Debugging.h"
#include "ShaderReflected.h"
#include "../../../RenderInterface/Resources/ShaderParameterResources.h"
#include "../../../RenderApi/Scene/RenderScene.h"
#include "../../../RenderInterface/Shaders/Base/DrawMeshShader.h"

void fillDescriptorsSet(std::vector<VkDescriptorPoolSize>& poolAllocateInfo, std::vector<VkDescriptorSetLayoutBinding>& descLayoutBindings
    , const ReflectDescriptorBody& descReflected)
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
            fatalAssert(arrayDimInfo.isSpecializationConst == false, "Specialized data is not supported yet");
            descCount *= arrayDimInfo.dimension;
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
            fatalAssert(arrayDimInfo.isSpecializationConst == false, "Specialized data is not supported yet");
            descCount *= arrayDimInfo.dimension;
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
            fatalAssert(arrayDimInfo.isSpecializationConst == false, "Specialized data is not supported yet");
            descCount *= arrayDimInfo.dimension;
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
            fatalAssert(arrayDimInfo.isSpecializationConst == false, "Specialized data is not supported yet");
            descCount *= arrayDimInfo.dimension;
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
            fatalAssert(arrayDimInfo.isSpecializationConst == false, "Specialized data is not supported yet");
            descCount *= arrayDimInfo.dimension;
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
            fatalAssert(arrayDimInfo.isSpecializationConst == false, "Specialized data is not supported yet");
            descCount *= arrayDimInfo.dimension;
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
    for (const ReflectDescriptorBody& descriptorsSet : respectiveShaderRes->getReflection()->descriptorsSets)
    {
        if (descriptorsSet.set == shaderSetID)
        {
            // Since bindings are sorted ascending
            uint32 maxBinding = descriptorsSet.usedBindings.back();
            poolAllocation.resize(maxBinding);
            layoutBindings.resize(maxBinding);
            fillDescriptorsSet(poolAllocation, layoutBindings, descriptorsSet);
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

    reinitResources();
}

void VulkanShaderSetParamsLayout::release()
{
    if (descriptorLayout != nullptr)
    {
        IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
        VulkanGraphicsHelper::destroyDescriptorsSetLayout(graphicsInstance, descriptorLayout);
        descriptorLayout = nullptr;
    }
    BaseType::release();
}

void VulkanShaderSetParamsLayout::reinitResources()
{
    release();
    BaseType::reinitResources();

    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();

    DESCRIPTOR_SET_LAYOUT_CREATE_INFO(descLayoutCreateInfo);
    descLayoutCreateInfo.bindingCount = uint32(layoutBindings.size());
    descLayoutCreateInfo.pBindings = layoutBindings.data();
    descriptorLayout = VulkanGraphicsHelper::createDescriptorsSetLayout(graphicsInstance, descLayoutCreateInfo);
    VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject(this);
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

    for (const std::pair<String, ShaderBufferParamInfo*>& bufferInfo : vertexSpecificBufferInfo)
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

    for (const std::pair<String, ShaderBufferParamInfo*>& bufferInfo : viewSpecificBufferInfo)
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
    for (const ReflectDescriptorBody& descriptorsSet : respectiveShaderRes->getReflection()->descriptorsSets)
    {
        SetParametersLayoutInfo& descSetLayoutInfo = setToLayoutInfo[descriptorsSet.set];

        // Since bindings are sorted ascending
        uint32 maxBinding = descriptorsSet.usedBindings.back();
        descSetLayoutInfo.poolAllocation.resize(maxBinding);
        descSetLayoutInfo.layoutBindings.resize(maxBinding);
        fillDescriptorsSet(descSetLayoutInfo.poolAllocation, descSetLayoutInfo.layoutBindings, descriptorsSet);

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
    }

    reinitResources();
}

void VulkanShaderParametersLayout::release()
{
    for (std::pair<const uint32, SetParametersLayoutInfo>& setParamsLayout : setToLayoutInfo)
    {
        if (setParamsLayout.second.descriptorLayout != nullptr)
        {
            IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
            VulkanGraphicsHelper::destroyDescriptorsSetLayout(graphicsInstance, setParamsLayout.second.descriptorLayout);
            setParamsLayout.second.descriptorLayout = nullptr;
        }
    }
    BaseType::release();
}

void VulkanShaderParametersLayout::reinitResources()
{
    release();
    BaseType::reinitResources();

    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
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
