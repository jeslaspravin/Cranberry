/*!
 * \file VulkanShaderParamResources.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "VulkanShaderParamResources.h"
#include "RenderInterface/Resources/ShaderResources.h"
#include "VulkanInternals/Resources/VulkanSampler.h"
#include "RenderApi/Material/MaterialCommonUniforms.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanGraphicsHelper.h"
#include "Math/Math.h"
#include "VulkanInternals/Debugging.h"
#include "ShaderReflected.h"
#include "RenderApi/Scene/RenderScene.h"
#include "RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "VulkanInternals/VulkanDescriptorAllocator.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "RenderInterface/ShaderCore/ShaderParameterUtility.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "VulkanInternals/Resources/VulkanMemoryResources.h"
#include "VulkanRHIModule.h"

void fillDescriptorsSet(std::vector<VkDescriptorPoolSize>& poolAllocateInfo, std::vector<VkDescriptorSetLayoutBinding>& descLayoutBindings
    , std::vector<bool>& runtimeArray, const ReflectDescriptorBody& descReflected, const std::vector<std::vector<SpecializationConstantEntry>>& stageSpecializationConsts)
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
        // 0 means unbound array 
        if (descCount == 0)
        {
            String attribName{ UTF8_TO_TCHAR(descriptorInfo.attributeName.c_str()) };
            auto itr = std::as_const(ShaderParameterUtility::unboundArrayResourcesCount()).find(attribName);
            fatalAssert(itr != ShaderParameterUtility::unboundArrayResourcesCount().cend()
                , "Unbound image(texel) buffer array is not allowed for parameter %s", attribName);
            descCount = itr->second;
            runtimeArray[descriptorInfo.data.binding] = true;
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
        // 0 means unbound array 
        if (descCount == 0)
        {
            String attribName{ UTF8_TO_TCHAR(descriptorInfo.attributeName.c_str()) };
            auto itr = std::as_const(ShaderParameterUtility::unboundArrayResourcesCount()).find(attribName);
            fatalAssert(itr != ShaderParameterUtility::unboundArrayResourcesCount().cend()
                , "Unbound sampled(texel) buffer array is not allowed for parameter %s", attribName);
            descCount = itr->second;
            runtimeArray[descriptorInfo.data.binding] = true;
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
        // 0 means unbound array 
        if (descCount == 0)
        {
            String attribName{ UTF8_TO_TCHAR(descriptorInfo.attributeName.c_str()) };
            auto itr = std::as_const(ShaderParameterUtility::unboundArrayResourcesCount()).find(attribName);
            fatalAssert(itr != ShaderParameterUtility::unboundArrayResourcesCount().cend()
                , "Unbound array of images or imageArray is not allowed for parameter %s", attribName);
            descCount = itr->second;
            runtimeArray[descriptorInfo.data.binding] = true;
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
        // 0 means unbound array 
        if (descCount == 0)
        {
            String attribName{ UTF8_TO_TCHAR(descriptorInfo.attributeName.c_str()) };
            auto itr = std::as_const(ShaderParameterUtility::unboundArrayResourcesCount()).find(attribName);
            fatalAssert(itr != ShaderParameterUtility::unboundArrayResourcesCount().cend()
                , "Unbound array of textures or textureArray is not allowed for parameter %s", attribName);
            descCount = itr->second;
            runtimeArray[descriptorInfo.data.binding] = true;
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
        // 0 means unbound array 
        if (descCount == 0)
        {
            String attribName{ UTF8_TO_TCHAR(descriptorInfo.attributeName.c_str()) };
            auto itr = std::as_const(ShaderParameterUtility::unboundArrayResourcesCount()).find(attribName);
            fatalAssert(itr != ShaderParameterUtility::unboundArrayResourcesCount().cend()
                , "Unbound array of sampled textures or sampled textureArray is not allowed for parameter %s", attribName);
            descCount = itr->second;
            runtimeArray[descriptorInfo.data.binding] = true;
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
    return {};
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
    std::vector<bool> runtimeArray;
    for (const ReflectDescriptorBody& descriptorsSet : respectiveShaderRes->getReflection()->descriptorsSets)
    {
        if (descriptorsSet.set == shaderSetID)
        {
            // Since bindings are sorted ascending
            uint32 maxBinding = descriptorsSet.usedBindings.back() + 1;
            poolAllocation.resize(maxBinding);
            layoutBindings.resize(maxBinding);
            runtimeArray.resize(maxBinding, false);
            fillDescriptorsSet(poolAllocation, layoutBindings, runtimeArray, descriptorsSet, specializationConsts);
        }
    }

    // Remove unnecessary descriptors set info
    for (int32 i = 0; i < poolAllocation.size();)
    {
        bHasBindless = bHasBindless || runtimeArray[i];
        if (poolAllocation[i].descriptorCount == 0)
        {
            std::iter_swap(poolAllocation.begin() + i, poolAllocation.end() - 1);
            poolAllocation.pop_back();

            std::iter_swap(layoutBindings.begin() + i, layoutBindings.end() - 1);
            layoutBindings.pop_back();

            std::iter_swap(runtimeArray.begin() + i, runtimeArray.end() - 1);
            runtimeArray.pop_back();
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

    // Sort bindings so that it will be easier when querying for descriptors
    std::sort(layoutBindings.begin(), layoutBindings.end(),
        [](const VkDescriptorSetLayoutBinding& lhs, const VkDescriptorSetLayoutBinding& rhs)
        {
            return lhs.binding < rhs.binding;
        }
    );

    //reinitResources();
    IGraphicsInstance* graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();

    if (layoutBindings.empty())
    {
        descriptorLayout = nullptr;
    }
    else
    {
        std::vector<VkDescriptorBindingFlags> bindingFlags;
        DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO(descLayoutBindingFlagsCI);
        DESCRIPTOR_SET_LAYOUT_CREATE_INFO(descLayoutCreateInfo);
        descLayoutCreateInfo.bindingCount = uint32(layoutBindings.size());
        descLayoutCreateInfo.pBindings = layoutBindings.data();
        if (bHasBindless)
        {
            bindingFlags.resize(runtimeArray.size());
            for (int32 i = 0; i < bindingFlags.size(); ++i)
            {
                bindingFlags[i] = runtimeArray[i]
                    ? VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
                    | ((GlobalRenderVariables::ENABLED_RESOURCE_UPDATE_AFTER_BIND) ? VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT : 0)
                    | ((GlobalRenderVariables::ENABLED_RESOURCE_UPDATE_UNUSED) ? VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT : 0)
                    : 0;
                // We set all flags for bindless/runtime array
                layoutBindings[i].stageFlags = runtimeArray[i] 
                    ? VkShaderStageFlagBits::VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM 
                    : layoutBindings[i].stageFlags;
            }
            descLayoutBindingFlagsCI.bindingCount = uint32(bindingFlags.size());
            descLayoutBindingFlagsCI.pBindingFlags = bindingFlags.data();
            descLayoutCreateInfo.flags |= (GlobalRenderVariables::ENABLED_RESOURCE_UPDATE_AFTER_BIND)
                ? VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT : 0;
            descLayoutCreateInfo.pNext = &descLayoutBindingFlagsCI;
        }

        descriptorLayout = VulkanGraphicsHelper::createDescriptorsSetLayout(graphicsInstance, descLayoutCreateInfo);
        VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject(this);
    }
}

void VulkanShaderSetParamsLayout::release()
{
    if (descriptorLayout != nullptr)
    {
        IGraphicsInstance* graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
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

const std::vector<VkDescriptorSetLayoutBinding>& VulkanShaderSetParamsLayout::getDescSetBindings() const
{
    return layoutBindings;
}

//////////////////////////////////////////////////////////////////////////
// VulkanShaderUniqDescLayout
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanShaderUniqDescLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)

VulkanShaderUniqDescLayout::VulkanShaderUniqDescLayout(const ShaderResource* shaderResource, uint32 descSetIdx)
    : BaseType(shaderResource, descSetIdx)
{}

void VulkanShaderUniqDescLayout::bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const
{
    respectiveShaderRes->bindBufferParamInfo(bindingBuffers);
}

String VulkanShaderUniqDescLayout::getObjectName() const
{
    return respectiveShaderRes->getResourceName() + TCHAR("_DescriptorsSetLayout") + String::toString(getSetID());
}

//////////////////////////////////////////////////////////////////////////
// VulkanVertexUniqDescLayout
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanVertexUniqDescLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)

VulkanVertexUniqDescLayout::VulkanVertexUniqDescLayout(const ShaderResource* shaderResource)
    : BaseType(shaderResource, ShaderParameterUtility::INSTANCE_UNIQ_SET)
{}

String VulkanVertexUniqDescLayout::getObjectName() const
{
    return respectiveShaderRes->getResourceName() + TCHAR("_DescriptorsSetLayout") + String::toString(ShaderParameterUtility::INSTANCE_UNIQ_SET);
}

void VulkanVertexUniqDescLayout::bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType*>& bindingBuffers) const
{
    const std::map<String, ShaderBufferParamInfo*>& vertexSpecificBufferInfo = 
        MaterialVertexUniforms::bufferParamInfo(static_cast<const DrawMeshShaderConfig*>(respectiveShaderRes->getShaderConfig())->vertexUsage());

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
    : BaseType(shaderResource, ShaderParameterUtility::VIEW_UNIQ_SET)
{}

String VulkanViewUniqDescLayout::getObjectName() const
{
    return respectiveShaderRes->getResourceName() + TCHAR("_DescriptorsSetLayout") + String::toString(ShaderParameterUtility::VIEW_UNIQ_SET);
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
// VulkanBindlessDescLayout
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanBindlessDescLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)

VulkanBindlessDescLayout::VulkanBindlessDescLayout(const ShaderResource* shaderResource)
    : BaseType(shaderResource, ShaderParameterUtility::BINDLESS_SET)
{}

String VulkanBindlessDescLayout::getObjectName() const
{
    return respectiveShaderRes->getResourceName() + TCHAR("_BindlessDescriptorsSetLayout") + String::toString(ShaderParameterUtility::BINDLESS_SET);
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
    IGraphicsInstance* graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();

    std::vector<std::vector<SpecializationConstantEntry>> specializationConsts;
    {
        std::map<String, SpecializationConstantEntry> specConsts;
        respectiveShaderRes->getSpecializationConsts(specConsts);
        ShaderParameterUtility::convertNamedSpecConstsToPerStage(specializationConsts, specConsts, respectiveShaderRes->getReflection());
    }

    std::vector<bool> runtimeArray;
    for (const ReflectDescriptorBody& descriptorsSet : respectiveShaderRes->getReflection()->descriptorsSets)
    {
        SetParametersLayoutInfo& descSetLayoutInfo = setToLayoutInfo[descriptorsSet.set];

        // Since bindings are sorted ascending
        uint32 maxBinding = descriptorsSet.usedBindings.back() + 1;
        runtimeArray.resize(maxBinding);
        descSetLayoutInfo.poolAllocation.resize(maxBinding);
        descSetLayoutInfo.layoutBindings.resize(maxBinding);
        fillDescriptorsSet(descSetLayoutInfo.poolAllocation, descSetLayoutInfo.layoutBindings, runtimeArray, descriptorsSet, specializationConsts);

        // Remove unnecessary descriptors set info
        for (int32 i = 0; i < descSetLayoutInfo.poolAllocation.size();)
        {
            descSetLayoutInfo.bHasBindless = descSetLayoutInfo.bHasBindless || runtimeArray[i];
            if (descSetLayoutInfo.poolAllocation[i].descriptorCount == 0)
            {
                std::iter_swap(descSetLayoutInfo.poolAllocation.begin() + i, descSetLayoutInfo.poolAllocation.end() - 1);
                descSetLayoutInfo.poolAllocation.pop_back();

                std::iter_swap(descSetLayoutInfo.layoutBindings.begin() + i, descSetLayoutInfo.layoutBindings.end() - 1);
                descSetLayoutInfo.layoutBindings.pop_back();

                std::iter_swap(runtimeArray.begin() + i, runtimeArray.end() - 1);
                runtimeArray.pop_back();
            }
            else
            {
                ++i;
            }
        }
        // sort and merge type duplicate descriptor pool size
        std::sort(descSetLayoutInfo.poolAllocation.begin(), descSetLayoutInfo.poolAllocation.end(),
            [](const VkDescriptorPoolSize& lhs, const VkDescriptorPoolSize& rhs)
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

        // Sort bindings so that it will be easier when querying for descriptors
        std::sort(descSetLayoutInfo.layoutBindings.begin(), descSetLayoutInfo.layoutBindings.end(), 
            [](const VkDescriptorSetLayoutBinding& lhs, const VkDescriptorSetLayoutBinding& rhs)
            {
                return lhs.binding < rhs.binding;
            }
        );

        // Create descriptors set
        std::vector<VkDescriptorBindingFlags> bindingFlags;
        DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO(descLayoutBindingFlagsCI);
        DESCRIPTOR_SET_LAYOUT_CREATE_INFO(descLayoutCreateInfo);
        descLayoutCreateInfo.bindingCount = uint32(descSetLayoutInfo.layoutBindings.size());
        descLayoutCreateInfo.pBindings = descSetLayoutInfo.layoutBindings.data();
        if (descSetLayoutInfo.bHasBindless)
        {
            bindingFlags.resize(runtimeArray.size());
            for (int32 i = 0; i < bindingFlags.size(); ++i)
            {
                bindingFlags[i] = runtimeArray[i]
                    ? VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
                    | ((GlobalRenderVariables::ENABLED_RESOURCE_UPDATE_AFTER_BIND) ? VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT : 0)
                    | ((GlobalRenderVariables::ENABLED_RESOURCE_UPDATE_UNUSED) ? VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT : 0)
                    : 0;
                // We set all flags for bindless/runtime array
                descSetLayoutInfo.layoutBindings[i].stageFlags = runtimeArray[i] 
                    ? VkShaderStageFlagBits::VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM 
                    : descSetLayoutInfo.layoutBindings[i].stageFlags;
            }
            descLayoutBindingFlagsCI.bindingCount = uint32(bindingFlags.size());
            descLayoutBindingFlagsCI.pBindingFlags = bindingFlags.data();
            descLayoutCreateInfo.flags |= (GlobalRenderVariables::ENABLED_RESOURCE_UPDATE_AFTER_BIND)
                ? VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT : 0;
            descLayoutCreateInfo.pNext = &descLayoutBindingFlagsCI;
        }
        descSetLayoutInfo.descriptorLayout = VulkanGraphicsHelper::createDescriptorsSetLayout(graphicsInstance, descLayoutCreateInfo);
        VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject(uint64(descSetLayoutInfo.descriptorLayout)
            , getResourceName() + String::toString(descriptorsSet.set), getObjectType());
    }
}

void VulkanShaderParametersLayout::release()
{
    for (std::pair<const uint32, SetParametersLayoutInfo>& setParamsLayout : setToLayoutInfo)
    {
        if (setParamsLayout.second.descriptorLayout != nullptr)
        {
            IGraphicsInstance* graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
            VulkanGraphicsHelper::destroyDescriptorsSetLayout(graphicsInstance, setParamsLayout.second.descriptorLayout);
            setParamsLayout.second.descriptorLayout = nullptr;
        }
    }
    BaseType::release();
}

String VulkanShaderParametersLayout::getResourceName() const
{
    return respectiveShaderRes->getResourceName() + TCHAR("_DescSetLayout");
}

bool VulkanShaderParametersLayout::hasBindless(uint32 setIdx) const
{
    auto foundItr = setToLayoutInfo.find(setIdx);
    debugAssert(foundItr != setToLayoutInfo.cend());
    return foundItr->second.bHasBindless;
}

const std::vector<VkDescriptorPoolSize>& VulkanShaderParametersLayout::getDescPoolAllocInfo(uint32 setIdx) const
{
    auto foundItr = setToLayoutInfo.find(setIdx);
    debugAssert(foundItr != setToLayoutInfo.cend());
    return foundItr->second.poolAllocation;
}

const std::vector<VkDescriptorSetLayoutBinding>& VulkanShaderParametersLayout::getDescSetBindings(uint32 setIdx) const
{
    auto foundItr = setToLayoutInfo.find(setIdx);
    debugAssert(foundItr != setToLayoutInfo.cend());
    return foundItr->second.layoutBindings;
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
    return getResourceName() + TCHAR("_DescSet") + String::toString(static_cast<const ShaderSetParametersLayout*>(paramLayout)->getSetID());
}

void VulkanShaderSetParameters::init()
{
    ignoredSets.clear();
    BaseType::init();

    IGraphicsInstance* graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    VulkanDescriptorsSetAllocator* descsSetAllocator = VulkanGraphicsHelper::getDescriptorsSetAllocator(graphicsInstance);
    DescriptorsSetQuery query;
    query.bHasBindless = static_cast<const VulkanShaderSetParamsLayout*>(paramLayout)->hasBindless();
    query.supportedTypes.insert(static_cast<const VulkanShaderSetParamsLayout*>(paramLayout)->getDescPoolAllocInfo().cbegin()
        , static_cast<const VulkanShaderSetParamsLayout*>(paramLayout)->getDescPoolAllocInfo().cend());
    query.allocatedBindings = &static_cast<const VulkanShaderSetParamsLayout*>(paramLayout)->getDescSetBindings();
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

        bufferInfos[descWriteIdx].buffer = static_cast<VulkanBufferResource*>(bufferParam.second.gpuBuffer.reference())->buffer;
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
    VulkanDescriptorsSetAllocator* descsSetAllocator = VulkanGraphicsHelper::getDescriptorsSetAllocator(IVulkanRHIModule::get()->getGraphicsInstance());
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
        bufferInfo.buffer = static_cast<VulkanBufferResource*>(bufferWriteData.ParamData.buffer->gpuBuffer.reference())->buffer;
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
            static_cast<VulkanBufferResource*>(texelWriteData.ParamData.texel->gpuBuffers[texelBufferUpdate.second].reference())->getBufferView({}));
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
        imageInfo.sampler = texWriteData.ParamData.texture->textures[textureUpdate.second].sampler.isValid()
            ? static_cast<VulkanSampler*>(texWriteData.ParamData.texture->textures[textureUpdate.second].sampler.reference())->sampler : nullptr;

        imageInfo.imageView = static_cast<VulkanImageResource*>(texWriteData.ParamData.texture->textures[textureUpdate.second].texture.reference())
            ->getImageView(texWriteData.ParamData.texture->textures[textureUpdate.second].viewInfo
                , texWriteData.ParamData.texture->descriptorInfo->textureEntryPtr->data.data.imageViewType);
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
        imageInfo.sampler = samplerWriteData.ParamData.sampler->samplers[samplerUpdate.second].reference<VulkanSampler>()->sampler;
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
    bufferResourceUpdates.clear();
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
    return getResourceName() + TCHAR("_DescSet");
}

void VulkanShaderParameters::init()
{
    BaseType::init();
    IGraphicsInstance* graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    VulkanDescriptorsSetAllocator* descsSetAllocator = VulkanGraphicsHelper::getDescriptorsSetAllocator(graphicsInstance);

    // Compress all descriptors set descriptor type size to common pool sizes to alloc all sets from same pool
    {
        const ShaderReflected* reflectedData = static_cast<const ShaderParametersLayout*>(paramLayout)->getShaderResource()->getReflection();
        for (const ReflectDescriptorBody& descriptorsBody : reflectedData->descriptorsSets)
        {
            if (ignoredSets.find(descriptorsBody.set) != ignoredSets.end())
            {
                continue;
            }

            VkDescriptorSetLayout layout = static_cast<const VulkanShaderParametersLayout*>(paramLayout)
                ->getDescSetLayout(descriptorsBody.set);
            const auto& setPoolSizes = static_cast<const VulkanShaderParametersLayout*>(paramLayout)
                ->getDescPoolAllocInfo(descriptorsBody.set);

            DescriptorsSetQuery query;
            query.supportedTypes.insert(setPoolSizes.cbegin(), setPoolSizes.cend());
            query.bHasBindless = static_cast<const VulkanShaderParametersLayout*>(paramLayout)
                ->hasBindless(descriptorsBody.set);
            query.allocatedBindings = &static_cast<const VulkanShaderParametersLayout*>(paramLayout)->getDescSetBindings(descriptorsBody.set);
            if (VkDescriptorSet descSet = descsSetAllocator->allocDescriptorsSet(query, layout))
            {
                descriptorsSets[descriptorsBody.set] = descSet;
                VulkanGraphicsHelper::debugGraphics(graphicsInstance)->markObject(uint64(descSet)
                    , getObjectName() + String::toString(descriptorsBody.set), getObjectType());
            }
            else
            {
                LOG_ERROR("VulkanShaderParameters", "%s() : Allocation of descriptors set %d failed %s", __func__, descriptorsBody.set, getResourceName().getChar());
                return;
            }
        }
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

        bufferInfos[descWriteIdx].buffer = static_cast<VulkanBufferResource*>(bufferParam.second.gpuBuffer.reference())->buffer;
        bufferInfos[descWriteIdx].offset = 0;
        bufferInfos[descWriteIdx].range = bufferParam.second.gpuBuffer->getResourceSize();

        ++descWriteIdx;
    }

    VulkanGraphicsHelper::updateDescriptorsSet(graphicsInstance, bufferDescWrites, {});
    ENQUEUE_COMMAND(FinalizeShaderParams)(
        [this](IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
        {
            updateParams(cmdList, graphicsInstance);
        });
}

void VulkanShaderParameters::release()
{
    VulkanDescriptorsSetAllocator* descsSetAllocator = VulkanGraphicsHelper::getDescriptorsSetAllocator(IVulkanRHIModule::get()->getGraphicsInstance());
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
        bufferInfo.buffer = static_cast<VulkanBufferResource*>(bufferWriteData.ParamData.buffer->gpuBuffer.reference())->buffer;
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
            static_cast<VulkanBufferResource*>(texelWriteData.ParamData.texel->gpuBuffers[texelBufferUpdate.second].reference())->getBufferView({}));
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
        imageInfo.sampler = texWriteData.ParamData.texture->textures[textureUpdate.second].sampler.isValid()
            ? static_cast<VulkanSampler*>(texWriteData.ParamData.texture->textures[textureUpdate.second].sampler.reference())->sampler : nullptr;

        imageInfo.imageView = static_cast<VulkanImageResource*>(texWriteData.ParamData.texture->textures[textureUpdate.second].texture.reference())
            ->getImageView(texWriteData.ParamData.texture->textures[textureUpdate.second].viewInfo
                , texWriteData.ParamData.texture->descriptorInfo->textureEntryPtr->data.data.imageViewType);
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
        imageInfo.sampler = static_cast<VulkanSampler*>(samplerWriteData.ParamData.sampler->samplers[samplerUpdate.second].reference())->sampler;
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
    bufferResourceUpdates.clear();
    texelUpdates.clear();
    textureUpdates.clear();
    samplerUpdates.clear();
}