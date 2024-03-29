/*!
 * \file VulkanPipelines.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "VulkanInternals/Resources/VulkanPipelines.h"
#include "RenderInterface/GlobalRenderVariables.h"
#include "RenderInterface/ShaderCore/ShaderParameterUtility.h"
#include "RenderApi/Shaders/Base/DrawMeshShader.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"
#include "ShaderReflected.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanGraphicsHelper.h"
#include "VulkanGraphicsInstance.h"
#include "VulkanInternals/Resources/VulkanShaderResources.h"
#include "VulkanInternals/VulkanGraphicsTypes.h"
#include "VulkanRHIModule.h"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanPipelineCache, VK_OBJECT_TYPE_PIPELINE_CACHE)

void VulkanPipelineCache::init()
{
    BaseType::init();
    reinitResources();
}

void VulkanPipelineCache::reinitResources()
{
    release();
    BaseType::reinitResources();

    std::vector<uint8> cacheData = getRawFromFile();
    if (!cacheData.empty())
    {
        pipelineCacheRead = VulkanGraphicsHelper::createPipelineCache(IVulkanRHIModule::get()->getGraphicsInstance(), cacheData);
    }
    VulkanGraphicsHelper::debugGraphics(IVulkanRHIModule::get()->getGraphicsInstance())->markObject(this);
}

void VulkanPipelineCache::release()
{
    BaseType::release();

    if (pipelineCacheRead != nullptr)
    {
        VulkanGraphicsHelper::destroyPipelineCache(IVulkanRHIModule::get()->getGraphicsInstance(), pipelineCacheRead);
        pipelineCacheRead = nullptr;
    }
}

String VulkanPipelineCache::getObjectName() const { return getResourceName(); }

uint64 VulkanPipelineCache::getDispatchableHandle() const { return uint64(pipelineCacheRead); }

std::vector<uint8> VulkanPipelineCache::getRawToWrite() const
{
    std::vector<uint8> dataToWriteOut;
    VulkanGraphicsHelper::getMergedCacheData(IVulkanRHIModule::get()->getGraphicsInstance(), dataToWriteOut, pipelinesToCache);
    return dataToWriteOut;
}

void VulkanGraphicsHelper::getMergedCacheData(
    class IGraphicsInstance *graphicsInstance, std::vector<uint8> &cacheData, const std::vector<const PipelineBase *> &pipelines
)
{
    std::vector<VkPipelineCache> cachesToMerge;
    cachesToMerge.reserve(pipelines.size());
    for (const PipelineBase *pipeline : pipelines)
    {
        if (pipeline->getType()->isChildOf<VulkanGraphicsPipeline>())
        {
            if (VkPipelineCache pipelineCache = static_cast<const VulkanGraphicsPipeline *>(pipeline)->pipelineLocalCache)
            {
                cachesToMerge.emplace_back(pipelineCache);
            }
        }
        else if (pipeline->getType()->isChildOf<VulkanComputePipeline>())
        {
            if (VkPipelineCache pipelineCache = static_cast<const VulkanComputePipeline *>(pipeline)->pipelineLocalCache)
            {
                cachesToMerge.emplace_back(pipelineCache);
            }
        }
    }

    VkPipelineCache cacheToWrite = createPipelineCache(graphicsInstance);
    mergePipelineCaches(graphicsInstance, cacheToWrite, cachesToMerge);
    getPipelineCacheData(graphicsInstance, cacheToWrite, cacheData);
    destroyPipelineCache(graphicsInstance, cacheToWrite);
}

//////////////////////////////////////////////////////////////////////////
// VulkanGraphicsPipeline
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanGraphicsPipeline, VK_OBJECT_TYPE_PIPELINE)

VulkanGraphicsPipeline::VulkanGraphicsPipeline(const GraphicsPipelineBase *parent)
    : BaseType(parent)
    , pipelineLocalCache(nullptr)
    , pipelineLayout(static_cast<const VulkanGraphicsPipeline *>(parent)->pipelineLayout)
    , compatibleRenderpass(static_cast<const VulkanGraphicsPipeline *>(parent)->compatibleRenderpass)
{}

void VulkanGraphicsPipeline::fillPipelineStates(VulkanGraphicsPipeline::VulkanPipelineCreateInfo &createInfo) const
{
    fillShaderStages(*createInfo.shaderStageCIs);
    fillSpecializationConsts(
        *createInfo.shaderStageCIs, *createInfo.specializationConstEntries, *createInfo.specializationConstData, *createInfo.specializationInfo
    );
    fillVertexInputState(*createInfo.vertexInputStateCI, *createInfo.vertexInputBindings, *createInfo.vertexInputAttribs);
    fillMultisampleState(*createInfo.multisampleStateCI);
    fillDepthStencilState(*createInfo.depthStencilStateCI, createInfo.dynamicStates);
    fillColorBlendStates(*createInfo.colorBlendStateCI, *createInfo.colorBlendAttachmentStates, createInfo.dynamicStates);

    // Input assembly
    createInfo.inputAsmStateCI->topology = EngineToVulkanAPI::vulkanPrimitiveTopology(config.primitiveTopology);
    // If line then allow dynamic width
    if (config.primitiveTopology == EPrimitiveTopology::Line)
    {
        createInfo.dynamicStates.emplace_back(VkDynamicState::VK_DYNAMIC_STATE_LINE_WIDTH);
    }
    // Tessellation
    createInfo.tessStateCI->patchControlPoints = config.cntrlPts;
    // Viewport
    createInfo.viewportStateCI->scissorCount = 1;
    createInfo.viewportStateCI->viewportCount = 1;
    createInfo.dynamicStates.emplace_back(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT);
    createInfo.dynamicStates.emplace_back(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR);
}

void VulkanGraphicsPipeline::fillVertexInputState(
    VkPipelineVertexInputStateCreateInfo &vertexInputStateCI, std::vector<VkVertexInputBindingDescription> &bindings,
    std::vector<VkVertexInputAttributeDescription> &attributes
) const
{
    EVertexType::Type shaderVertUsage;
    if (pipelineShader->getShaderConfig()->getType()->isChildOf<DrawMeshShaderConfig>())
    {
        shaderVertUsage = static_cast<const DrawMeshShaderConfig *>(pipelineShader->getShaderConfig())->vertexUsage();
    }
    else if (pipelineShader->getShaderConfig()->getType()->isChildOf<UniqueUtilityShaderConfig>())
    {
        shaderVertUsage = static_cast<const UniqueUtilityShaderConfig *>(pipelineShader->getShaderConfig())->vertexUsage();
    }

    const std::vector<ShaderVertexParamInfo *> &vertexParamsInfo = EVertexType::vertexParamInfo(shaderVertUsage);
    bindings.resize(vertexParamsInfo.size());
    attributes.reserve(pipelineShader->getReflection()->inputs.size());

    uint32 bindingIdx = 0;
    for (const ShaderVertexParamInfo *paramInfo : vertexParamsInfo)
    {
        VkVertexInputBindingDescription bindingDesc;
        bindingDesc.binding = bindingIdx;
        if (paramInfo != nullptr)
        {
            bindingDesc.inputRate = paramInfo->inputFrequency() == EShaderInputFrequency::PerVertex
                                        ? VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX
                                        : VkVertexInputRate::VK_VERTEX_INPUT_RATE_INSTANCE;
            bindingDesc.stride = paramInfo->paramStride();

            for (const ShaderVertexField *attributeField : *paramInfo)
            {
                VkVertexInputAttributeDescription attributeDesc;
                attributeDesc.binding = bindingIdx;
                attributeDesc.format = EngineToVulkanAPI::vulkanDataFormat(EPixelDataFormat::Type(attributeField->format));
                attributeDesc.location = attributeField->location;
                attributeDesc.offset = attributeField->offset;

                attributes.emplace_back(attributeDesc);
            }
        }
        else // This case mostly will not occur and if there is need for this case check if Vulkan
             // allowing this.
        {
            bindingDesc.stride = 0;
        }
        bindings[bindingIdx] = bindingDesc;
        bindingIdx++;
    }
    vertexInputStateCI.vertexBindingDescriptionCount = uint32(bindings.size());
    vertexInputStateCI.pVertexBindingDescriptions = bindings.data();
    vertexInputStateCI.vertexAttributeDescriptionCount = uint32(attributes.size());
    vertexInputStateCI.pVertexAttributeDescriptions = attributes.data();
}

void VulkanGraphicsPipeline::fillMultisampleState(VkPipelineMultisampleStateCreateInfo &multisampleStateCI) const
{
    multisampleStateCI.alphaToCoverageEnable = multisampleStateCI.alphaToOneEnable = multisampleStateCI.sampleShadingEnable = VK_FALSE;
    multisampleStateCI.pSampleMask = nullptr;
    multisampleStateCI.minSampleShading = 1.0f;
    if (pipelineShader->getType()->isChildOf<DrawMeshShaderConfig>())
    {
        multisampleStateCI.rasterizationSamples = VkSampleCountFlagBits(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
    }
    else if (pipelineShader->getType()->isChildOf<UniqueUtilityShaderConfig>())
    {
        multisampleStateCI.rasterizationSamples = VkSampleCountFlagBits(config.renderpassProps.multisampleCount);
    }
    else
    {
        multisampleStateCI.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
    }
}

void VulkanGraphicsPipeline::fillDepthStencilState(
    VkPipelineDepthStencilStateCreateInfo &depthStencilStateCI, std::vector<VkDynamicState> &dynamicStates
) const
{
    depthStencilStateCI.depthBoundsTestEnable = depthStencilStateCI.stencilTestEnable = VK_FALSE;
    depthStencilStateCI.depthTestEnable = config.depthState.compareOp != CoreGraphicsTypes::ECompareOp::Always ? VK_TRUE : VK_FALSE;
    depthStencilStateCI.depthWriteEnable = config.depthState.bEnableWrite ? VK_TRUE : VK_FALSE;
    depthStencilStateCI.depthCompareOp = EngineToVulkanAPI::vulkanCompareOp(config.depthState.compareOp);
    depthStencilStateCI.minDepthBounds = 0.0f;
    depthStencilStateCI.maxDepthBounds = 1.0f;

    depthStencilStateCI.front.compareMask = 0xFFFFFFFF;
    depthStencilStateCI.front.writeMask = 0xFFFFFFFF;
    depthStencilStateCI.front.compareOp = EngineToVulkanAPI::vulkanCompareOp(config.stencilStateFront.compareOp);
    depthStencilStateCI.front.passOp = VkStencilOp(config.stencilStateFront.passOp);
    depthStencilStateCI.front.failOp = VkStencilOp(config.stencilStateFront.failOp);
    depthStencilStateCI.front.depthFailOp = VkStencilOp(config.stencilStateFront.depthFailOp);

    depthStencilStateCI.back.compareMask = 0xFFFFFFFF;
    depthStencilStateCI.back.writeMask = 0xFFFFFFFF;
    depthStencilStateCI.back.compareOp = EngineToVulkanAPI::vulkanCompareOp(config.stencilStateBack.compareOp);
    depthStencilStateCI.back.passOp = VkStencilOp(config.stencilStateBack.passOp);
    depthStencilStateCI.back.failOp = VkStencilOp(config.stencilStateBack.failOp);
    depthStencilStateCI.back.depthFailOp = VkStencilOp(config.stencilStateBack.depthFailOp);

    if (config.stencilStateBack.compareOp != CoreGraphicsTypes::ECompareOp::Never
        || config.stencilStateFront.compareOp != CoreGraphicsTypes::ECompareOp::Never)
    {
        depthStencilStateCI.stencilTestEnable = VK_TRUE;
        dynamicStates.emplace_back(VkDynamicState::VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    }
}

void VulkanGraphicsPipeline::fillShaderStages(std::vector<VkPipelineShaderStageCreateInfo> &shaderStages) const
{
    shaderStages.reserve(pipelineShader->getShaders().size());

    for (const ShaderStageDescription &shaderStageDesc : pipelineShader->getReflection()->stages)
    {
        const SharedPtr<ShaderCodeResource> &shaderCode = pipelineShader->getShaders().at(EShaderStage::Type(shaderStageDesc.stage));
        PIPELINE_SHADER_STAGE_CREATE_INFO(shaderStageCreateInfo);
        shaderStageCreateInfo.stage = EngineToVulkanAPI::vulkanShaderStage(shaderCode->shaderStage());
        shaderStageCreateInfo.pName = shaderCode->entryPoint().c_str();
        shaderStageCreateInfo.module = static_cast<VulkanShaderCodeResource *>(shaderCode.get())->shaderModule;
        // filled later
        shaderStageCreateInfo.pSpecializationInfo = VK_NULL_HANDLE;

        shaderStages.emplace_back(shaderStageCreateInfo);
    }
}

void VulkanGraphicsPipeline::fillSpecializationConsts(
    std::vector<VkPipelineShaderStageCreateInfo> &shaderStages, std::vector<VkSpecializationMapEntry> &specEntries,
    std::vector<uint8> &specData, std::vector<VkSpecializationInfo> &specializationInfo
) const
{
    uint32 specConstsCount = 0;
    std::vector<std::vector<SpecializationConstantEntry>> specConstsPerStage;
    {
        SpecConstantNamedMap specConsts;
        pipelineShader->getSpecializationConsts(specConsts);
        specConstsCount
            = ShaderParameterUtility::convertNamedSpecConstsToPerStage(specConstsPerStage, specConsts, pipelineShader->getReflection());
    }
    if (specConstsCount == 0)
    {
        return;
    }
    fatalAssertf(
        specConstsPerStage.size() == pipelineShader->getShaders().size(), "Specialization constant stage count does not match shader stages"
    );

    specEntries.reserve(specConstsCount);
    specData.clear();
    for (const std::vector<SpecializationConstantEntry> &specConsts : specConstsPerStage)
    {
        for (const SpecializationConstantEntry &value : specConsts)
        {
            VkSpecializationMapEntry entry;
            entry.constantID = value.constantId;
            entry.offset = uint32(specData.size());

            switch (value.type)
            {
            case ReflectPrimitive_bool:
                entry.size = sizeof(bool);
                break;
            case ReflectPrimitive_int:
                entry.size = sizeof(int32);
                break;
            case ReflectPrimitive_uint:
                entry.size = sizeof(uint32);
                break;
            case ReflectPrimitive_float:
                entry.size = sizeof(float);
                break;
            case ReflectPrimitive_double:
                entry.size = sizeof(double);
                break;
            case RelectPrimitive_invalid:
            default:
                fatalAssertf(!"Invalid primitive type", "Invalid primitive type");
            }

            specData.resize(specData.size() + entry.size);
            memcpy(&specData[entry.offset], &value.defaultValue.defaultValue, entry.size);
            specEntries.emplace_back(entry);
        }
    }

    specializationInfo.resize(specConstsPerStage.size());
    int32 shaderStageIdx = 0;
    int32 specEntryIdx = 0;
    for (const std::vector<SpecializationConstantEntry> &specConsts : specConstsPerStage)
    {
        VkSpecializationInfo &specInfo = specializationInfo[shaderStageIdx];
        specInfo.dataSize = uint32(specData.size());
        specInfo.pData = specData.data();
        if (specConsts.empty())
        {
            specInfo.pMapEntries = nullptr;
            specInfo.mapEntryCount = 0;
        }
        else
        {
            specInfo.pMapEntries = &specEntries[specEntryIdx];
            specInfo.mapEntryCount = uint32(specConsts.size());
        }

        VkPipelineShaderStageCreateInfo &stageCI = shaderStages[shaderStageIdx];
        stageCI.pSpecializationInfo = &specializationInfo[shaderStageIdx];

        specEntryIdx += specInfo.mapEntryCount;
        ++shaderStageIdx;
    }
}

void VulkanGraphicsPipeline::fillColorBlendStates(
    VkPipelineColorBlendStateCreateInfo &colorBlendStateCI, std::vector<VkPipelineColorBlendAttachmentState> &vulkanAttachmentBlendStates,
    std::vector<VkDynamicState> &dynamicStates
) const
{
    bool bHasConstant = false;
    vulkanAttachmentBlendStates.reserve(config.attachmentBlendStates.size());
    for (const AttachmentBlendState &attachmentBlendState : config.attachmentBlendStates)
    {
        bHasConstant = bHasConstant || attachmentBlendState.usesBlendConstant();

        VkPipelineColorBlendAttachmentState colorBlendState;
        colorBlendState.blendEnable = attachmentBlendState.bBlendEnable ? VK_TRUE : VK_FALSE;
        colorBlendState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
                                         | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT
                                         | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
        colorBlendState.srcColorBlendFactor = VkBlendFactor(attachmentBlendState.srcColorFactor);
        colorBlendState.dstColorBlendFactor = VkBlendFactor(attachmentBlendState.dstColorFactor);
        colorBlendState.colorBlendOp = VkBlendOp(attachmentBlendState.colorBlendOp);
        colorBlendState.srcAlphaBlendFactor = VkBlendFactor(attachmentBlendState.srcAlphaFactor);
        colorBlendState.dstAlphaBlendFactor = VkBlendFactor(attachmentBlendState.dstAlphaFactor);
        colorBlendState.alphaBlendOp = VkBlendOp(attachmentBlendState.alphaBlendOp);
        vulkanAttachmentBlendStates.emplace_back(colorBlendState);
    }

    colorBlendStateCI.attachmentCount = uint32(vulkanAttachmentBlendStates.size());
    colorBlendStateCI.pAttachments = vulkanAttachmentBlendStates.data();
    if (bHasConstant)
    {
        dynamicStates.emplace_back(VkDynamicState::VK_DYNAMIC_STATE_BLEND_CONSTANTS);
    }
}

void VulkanGraphicsPipeline::fillDynamicPermutedStates(
    VulkanGraphicsPipeline::VulkanPipelineCreateInfo &createInfo, const GraphicsPipelineQueryParams &params
) const
{
    // Rasterization state
    PIPELINE_RASTERIZATION_STATE_CREATE_INFO(rasterizationStateCI);
    rasterizationStateCI.cullMode = VkCullModeFlagBits(params.cullingMode);
    rasterizationStateCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationStateCI.depthBiasEnable = config.bEnableDepthBias ? VK_TRUE : VK_FALSE;
    rasterizationStateCI.depthClampEnable = config.bEnableDepthClamp ? VK_TRUE : VK_FALSE;
    if (GlobalRenderVariables::ENABLE_NON_FILL_DRAWS.get())
    {
        rasterizationStateCI.polygonMode = VkPolygonMode(params.drawMode);
    }
    else
    {
        rasterizationStateCI.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
    }

    createInfo.rasterizationStateCI = rasterizationStateCI;
    if (config.bEnableDepthBias)
    {
        createInfo.dynamicStates.emplace_back(VkDynamicState::VK_DYNAMIC_STATE_DEPTH_BIAS);
    }
    if (params.drawMode != EPolygonDrawMode::Fill)
    {
        createInfo.dynamicStates.emplace_back(VkDynamicState::VK_DYNAMIC_STATE_LINE_WIDTH);
    }
}

void VulkanGraphicsPipeline::validateCreateInfo(VulkanPipelineCreateInfo &createInfo) const
{
    // Unique Dynamic states
    std::sort(createInfo.dynamicStates.begin(), createInfo.dynamicStates.end());
    auto newEnd = std::unique(createInfo.dynamicStates.begin(), createInfo.dynamicStates.end());
    createInfo.dynamicStates.erase(newEnd, createInfo.dynamicStates.end());
}

void VulkanGraphicsPipeline::createPipelines(const std::vector<VulkanGraphicsPipeline::VulkanPipelineCreateInfo> &createInfos)
{
    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();

    std::vector<VkPipelineDynamicStateCreateInfo> dynamicStateCIs(createInfos.size());
    std::vector<VkGraphicsPipelineCreateInfo> pipelineCIs(createInfos.size());
    for (int32 createInfoIdx = 0; createInfoIdx < createInfos.size(); ++createInfoIdx)
    {
        const VulkanPipelineCreateInfo &ci = createInfos[createInfoIdx];

        GRAPHICS_PIPELINE_CREATE_INFO(graphicsPipelineCI);
        graphicsPipelineCI.flags = ci.pipelineFlags;
        graphicsPipelineCI.basePipelineHandle = ci.basePipelineHandle;
        graphicsPipelineCI.basePipelineIndex = ci.basePipelineIdx;
        graphicsPipelineCI.layout = pipelineLayout;
        graphicsPipelineCI.renderPass = compatibleRenderpass;
        graphicsPipelineCI.subpass = 0;

        graphicsPipelineCI.stageCount = uint32(ci.shaderStageCIs->size());
        graphicsPipelineCI.pStages = ci.shaderStageCIs->data();
        graphicsPipelineCI.pVertexInputState = ci.vertexInputStateCI;
        graphicsPipelineCI.pInputAssemblyState = ci.inputAsmStateCI;
        graphicsPipelineCI.pTessellationState = ci.tessStateCI;
        graphicsPipelineCI.pViewportState = ci.viewportStateCI;
        graphicsPipelineCI.pMultisampleState = ci.multisampleStateCI;
        graphicsPipelineCI.pDepthStencilState = ci.depthStencilStateCI;
        graphicsPipelineCI.pColorBlendState = ci.colorBlendStateCI;

        // Unique states per variant
        PIPELINE_DYNAMIC_STATE_CREATE_INFO(dynamicStateCI);
        dynamicStateCI.dynamicStateCount = uint32(ci.dynamicStates.size());
        dynamicStateCI.pDynamicStates = ci.dynamicStates.data();
        dynamicStateCIs[createInfoIdx] = dynamicStateCI;

        graphicsPipelineCI.pRasterizationState = &ci.rasterizationStateCI;
        graphicsPipelineCI.pDynamicState = &dynamicStateCIs[createInfoIdx];

        pipelineCIs[createInfoIdx] = graphicsPipelineCI;
    }

    pipelines = VulkanGraphicsHelper::createGraphicsPipeline(graphicsInstance, pipelineCIs, pipelineLocalCache);
}

String VulkanGraphicsPipeline::getObjectName() const { return getResourceName(); }

void VulkanGraphicsPipeline::init()
{
    fatalAssertf(
        config.attachmentBlendStates.size() == pipelineShader->getReflection()->outputs.size(),
        "Blend states has to be equivalent to color attachments count"
    );
    fatalAssertf(
        pipelineShader->getShaderConfig()->getType()->isChildOf<DrawMeshShaderConfig>()
            || pipelineShader->getShaderConfig()->getType()->isChildOf<UniqueUtilityShaderConfig>(),
        "Not supported shader for graphics pipeline"
    );

    BaseType::init();

    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    // Must be created always for the cache to be captured when creating pipeline
    pipelineLocalCache = VulkanGraphicsHelper::createPipelineCache(graphicsInstance);
    if (parentCache && static_cast<const VulkanPipelineCache *>(parentCache)->pipelineCacheRead != nullptr)
    {
        VkPipelineCache pipelineCaches[] = { static_cast<const VulkanPipelineCache *>(parentCache)->pipelineCacheRead };
        VulkanGraphicsHelper::mergePipelineCaches(graphicsInstance, pipelineLocalCache, pipelineCaches);
    }

    reinitResources();
}

void VulkanGraphicsPipeline::reinitResources()
{
    BaseType::reinitResources();
    // Release reinitializing resources
    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    for (VkPipeline &graphicsPipeline : pipelines)
    {
        VulkanGraphicsHelper::destroyPipeline(graphicsInstance, graphicsPipeline);
    }

    // Common to all pipelines
    std::vector<VkSpecializationMapEntry> specializationConstEntries;
    std::vector<uint8> specializationConstData;
    std::vector<VkSpecializationInfo> specializationInfo;
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    std::vector<VkVertexInputBindingDescription> vertexBindings;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;
    std::vector<VkPipelineColorBlendAttachmentState> vulkanAttachmentBlendStates;
    std::vector<VkDynamicState> dynamicStates;

    PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO(vertexInputStateCI);
    PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO(inputAssemblyStateCI);
    PIPELINE_TESSELLATION_STATE_CREATE_INFO(tessellationStateCI);
    PIPELINE_VIEWPORT_STATE_CREATE_INFO(viewportStateCI);
    PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO(depthStencilStateCI);
    PIPELINE_MULTISAMPLE_STATE_CREATE_INFO(multisampleStateCI);
    PIPELINE_COLOR_BLEND_STATE_CREATE_INFO(colorBlendStateCI);

    int32 totalPipelinesCount = pipelinesCount();
    std::vector<VulkanPipelineCreateInfo> tempPipelineCIs(totalPipelinesCount);

    // Pipeline 0
    {
        VulkanPipelineCreateInfo &graphicsPipelineCI = tempPipelineCIs[0];
        // Setting if we will derive from this pipeline or gets derived from something else
        graphicsPipelineCI.pipelineFlags
            = bCanBeParent || tempPipelineCIs.size() > 1 ? VkPipelineCreateFlagBits::VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT : 0;
        if (parentPipeline != nullptr)
        {
            graphicsPipelineCI.pipelineFlags |= VkPipelineCreateFlagBits::VK_PIPELINE_CREATE_DERIVATIVE_BIT;
            graphicsPipelineCI.basePipelineHandle = static_cast<const VulkanGraphicsPipeline *>(parentPipeline)->pipelines[0];
        }

        graphicsPipelineCI.specializationConstEntries = &specializationConstEntries;
        graphicsPipelineCI.specializationConstData = &specializationConstData;
        graphicsPipelineCI.specializationInfo = &specializationInfo;
        graphicsPipelineCI.shaderStageCIs = &stages;
        graphicsPipelineCI.vertexInputBindings = &vertexBindings;
        graphicsPipelineCI.vertexInputAttribs = &vertexAttributes;
        graphicsPipelineCI.vertexInputStateCI = &vertexInputStateCI;
        graphicsPipelineCI.inputAsmStateCI = &inputAssemblyStateCI;
        graphicsPipelineCI.tessStateCI = &tessellationStateCI;
        graphicsPipelineCI.viewportStateCI = &viewportStateCI;
        graphicsPipelineCI.multisampleStateCI = &multisampleStateCI;
        graphicsPipelineCI.depthStencilStateCI = &depthStencilStateCI;
        graphicsPipelineCI.colorBlendAttachmentStates = &vulkanAttachmentBlendStates;
        graphicsPipelineCI.colorBlendStateCI = &colorBlendStateCI;

        fillDynamicPermutedStates(graphicsPipelineCI, paramForIdx(0));
        fillPipelineStates(graphicsPipelineCI);

        validateCreateInfo(graphicsPipelineCI);
    }
    for (int32 pipelineIdx = 1; pipelineIdx < totalPipelinesCount; ++pipelineIdx)
    {
        VulkanPipelineCreateInfo &graphicsPipelineCI = tempPipelineCIs[pipelineIdx];

        graphicsPipelineCI = tempPipelineCIs[0];
        graphicsPipelineCI.pipelineFlags = VkPipelineCreateFlagBits::VK_PIPELINE_CREATE_DERIVATIVE_BIT;
        graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;
        graphicsPipelineCI.basePipelineIdx = 0;

        fillDynamicPermutedStates(graphicsPipelineCI, paramForIdx(pipelineIdx));

        validateCreateInfo(graphicsPipelineCI);
    }

    createPipelines(tempPipelineCIs);
}

void VulkanGraphicsPipeline::release()
{
    BaseType::release();
    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();

    if (pipelineLocalCache != nullptr)
    {
        VulkanGraphicsHelper::destroyPipelineCache(graphicsInstance, pipelineLocalCache);
        pipelineLocalCache = nullptr;
    }

    for (VkPipeline &graphicsPipeline : pipelines)
    {
        VulkanGraphicsHelper::destroyPipeline(graphicsInstance, graphicsPipeline);
    }
    pipelines.clear();
}

void VulkanGraphicsPipeline::setCompatibleRenderpass(VkRenderPass renderpass) { compatibleRenderpass = renderpass; }

VkPipeline VulkanGraphicsPipeline::getPipeline(const GraphicsPipelineQueryParams &pipelineQuery) const
{
    return pipelines[idxFromParam(pipelineQuery)];
}

DEFINE_VK_GRAPHICS_RESOURCE(VulkanComputePipeline, VkObjectType::VK_OBJECT_TYPE_PIPELINE)

VulkanComputePipeline::VulkanComputePipeline(const ComputePipelineBase *parent)
    : BaseType(parent)
    , pipeline(nullptr)
    , pipelineLocalCache(nullptr)
    , pipelineLayout(nullptr)
{}

void VulkanComputePipeline::fillShaderStages(VkPipelineShaderStageCreateInfo &shaderStage) const
{
    auto computeShaderCodeItr = pipelineShader->getShaders().find(EShaderStage::Compute);
    fatalAssertf(
        pipelineShader->getShaders().size() == 1 && computeShaderCodeItr != pipelineShader->getShaders().cend(),
        "Compute shader suppots only one stage | Compute shader is invalid"
    );

    PIPELINE_SHADER_STAGE_CREATE_INFO(shaderStageCreateInfo);
    shaderStageCreateInfo.stage = EngineToVulkanAPI::vulkanShaderStage(computeShaderCodeItr->second->shaderStage());
    shaderStageCreateInfo.pName = computeShaderCodeItr->second->entryPoint().c_str();
    shaderStageCreateInfo.module = static_cast<VulkanShaderCodeResource *>(computeShaderCodeItr->second.get())->shaderModule;
    // filled later
    shaderStageCreateInfo.pSpecializationInfo = VK_NULL_HANDLE;

    shaderStage = shaderStageCreateInfo;
}

void VulkanComputePipeline::fillSpecializationConsts(
    VkPipelineShaderStageCreateInfo &shaderStages, std::vector<VkSpecializationMapEntry> &specEntries, std::vector<uint8> &specData,
    VkSpecializationInfo &specializationInfo
) const
{
    uint32 specConstsCount = 0;
    std::vector<std::vector<SpecializationConstantEntry>> specConstsPerStage;
    {
        SpecConstantNamedMap specConsts;
        pipelineShader->getSpecializationConsts(specConsts);
        specConstsCount
            = ShaderParameterUtility::convertNamedSpecConstsToPerStage(specConstsPerStage, specConsts, pipelineShader->getReflection());
    }
    if (specConstsCount == 0)
    {
        return;
    }
    fatalAssertf(
        specConstsPerStage.size() == pipelineShader->getShaders().size(), "Specialization constant stage count does not match shader stages"
    );

    specEntries.reserve(specConstsCount);
    specData.clear();
    for (const SpecializationConstantEntry &value : specConstsPerStage[0])
    {
        VkSpecializationMapEntry entry;
        entry.constantID = value.constantId;
        entry.offset = uint32(specData.size());

        switch (value.type)
        {
        case ReflectPrimitive_bool:
            entry.size = sizeof(bool);
            break;
        case ReflectPrimitive_int:
            entry.size = sizeof(int32);
            break;
        case ReflectPrimitive_uint:
            entry.size = sizeof(uint32);
            break;
        case ReflectPrimitive_float:
            entry.size = sizeof(float);
            break;
        case ReflectPrimitive_double:
            entry.size = sizeof(double);
            break;
        case RelectPrimitive_invalid:
        default:
            fatalAssertf(!"Invalid primitive type", "Invalid primitive type");
        }

        specData.resize(specData.size() + entry.size);
        // Since we have work group size spec constant in 1-3
        if (value.constantId >= 1 && value.constantId < 4)
        {
            memcpy(
                &specData[entry.offset],
                &static_cast<const ComputeShaderConfig *>(pipelineShader->getShaderConfig())->getSubGroupSize()[value.constantId - 1],
                entry.size
            );
        }
        else
        {
            memcpy(&specData[entry.offset], &value.defaultValue.defaultValue, entry.size);
        }
        specEntries.emplace_back(entry);
    }

    specializationInfo.dataSize = uint32(specData.size());
    specializationInfo.pData = specData.data();
    specializationInfo.pMapEntries = specEntries.data();
    specializationInfo.mapEntryCount = uint32(specConstsPerStage[0].size());

    shaderStages.pSpecializationInfo = &specializationInfo;
}

String VulkanComputePipeline::getObjectName() const { return getResourceName(); }

void VulkanComputePipeline::init()
{
    BaseType::init();

    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();

    // Must be created always for the cache to be captured when creating pipeline
    pipelineLocalCache = VulkanGraphicsHelper::createPipelineCache(graphicsInstance);
    if (parentCache && static_cast<const VulkanPipelineCache *>(parentCache)->pipelineCacheRead != nullptr)
    {
        VkPipelineCache pipelineCaches[] = { static_cast<const VulkanPipelineCache *>(parentCache)->pipelineCacheRead };
        VulkanGraphicsHelper::mergePipelineCaches(graphicsInstance, pipelineLocalCache, pipelineCaches);
    }

    reinitResources();
}

void VulkanComputePipeline::reinitResources()
{
    BaseType::reinitResources();
    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();
    if (pipeline)
    {
        VulkanGraphicsHelper::destroyPipeline(graphicsInstance, pipeline);
        pipeline = nullptr;
    }

    std::vector<VkSpecializationMapEntry> specializationConstEntries;
    std::vector<uint8> specializationConstData;
    VkSpecializationInfo specializationInfo;

    COMPUTE_PIPELINE_CREATE_INFO(createInfo);

    createInfo.flags = bCanBeParent ? VkPipelineCreateFlagBits::VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT : 0;
    if (parentPipeline != nullptr)
    {
        createInfo.flags |= VkPipelineCreateFlagBits::VK_PIPELINE_CREATE_DERIVATIVE_BIT;
        createInfo.basePipelineHandle = static_cast<const VulkanComputePipeline *>(parentPipeline)->pipeline;
    }
    fillShaderStages(createInfo.stage);
    fillSpecializationConsts(createInfo.stage, specializationConstEntries, specializationConstData, specializationInfo);
    createInfo.layout = pipelineLayout;

    pipeline = VulkanGraphicsHelper::createComputePipeline(graphicsInstance, { createInfo }, pipelineLocalCache)[0];
}

void VulkanComputePipeline::release()
{
    BaseType::release();
    IGraphicsInstance *graphicsInstance = IVulkanRHIModule::get()->getGraphicsInstance();

    if (pipelineLocalCache != nullptr)
    {
        VulkanGraphicsHelper::destroyPipelineCache(graphicsInstance, pipelineLocalCache);
        pipelineLocalCache = nullptr;
    }

    VulkanGraphicsHelper::destroyPipeline(graphicsInstance, pipeline);
    pipeline = nullptr;
}