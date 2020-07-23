#include "VulkanPipelines.h"
#include "../../../Core/Engine/GameEngine.h"
#include "../../../Core/Platform/PlatformAssertionErrors.h"
#include "../../VulkanGraphicsHelper.h"
#include "ShaderReflected.h"
#include "../../VulkanGraphicsInstance.h"
#include "../../../RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "../../../RenderInterface/Shaders/Base/UtilityShaders.h"
#include "../../../RenderInterface/GlobalRenderVariables.h"

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

    pipelineCacheRead = VulkanGraphicsHelper::createPipelineCache(gEngine->getRenderApi()->getGraphicsInstance(), getRawFromFile());
    VulkanGraphicsHelper::debugGraphics(gEngine->getRenderApi()->getGraphicsInstance())->markObject(this);
}

void VulkanPipelineCache::release()
{
    BaseType::release();

    if (pipelineCacheRead != nullptr)
    {
        VulkanGraphicsHelper::destroyPipelineCache(gEngine->getRenderApi()->getGraphicsInstance(), pipelineCacheRead);
        pipelineCacheRead = nullptr;
    }
}

String VulkanPipelineCache::getObjectName() const
{
    return getResourceName();
}

uint64 VulkanPipelineCache::getDispatchableHandle() const
{
    return uint64(pipelineCacheRead);
}

std::vector<uint8> VulkanPipelineCache::getRawToWrite() const
{
    std::vector<uint8> dataToWriteOut;
    VulkanGraphicsHelper::getMergedCacheData(gEngine->getRenderApi()->getGraphicsInstance(), dataToWriteOut, pipelinesToCache);
    return dataToWriteOut;
}


void VulkanGraphicsHelper::getMergedCacheData(class IGraphicsInstance* graphicsInstance, std::vector<uint8>& cacheData, const std::vector<const class PipelineBase*>& pipelines)
{
    const auto* gInstance = static_cast<const VulkanGraphicsInstance*>(graphicsInstance);
    const VulkanDevice* device = &gInstance->selectedDevice;

    std::vector<VkPipelineCache> cachesToMerge;
    cachesToMerge.reserve(pipelines.size());
    for (const PipelineBase* pipeline : pipelines)
    {
        if (pipeline->getType()->isChildOf<VulkanGraphicsPipeline>())
        {
            cachesToMerge.emplace_back(static_cast<const VulkanGraphicsPipeline*>(pipeline)->pipelineLocalCache);
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

VulkanGraphicsPipeline::VulkanGraphicsPipeline(const GraphicsPipelineBase* parent)
    : BaseType(parent)
    , pipelineLocalCache(nullptr)
    , pipelineLayout(static_cast<const VulkanGraphicsPipeline*>(parent)->pipelineLayout)
    , compatibleRenderpass(static_cast<const VulkanGraphicsPipeline*>(parent)->compatibleRenderpass)
{}

void VulkanGraphicsPipeline::fillPipelineStates(VulkanGraphicsPipeline::VulkanPipelineCreateInfo& createInfo) const
{
    fillShaderStages(*createInfo.shaderStageCIs);
    fillVertexInputState(*createInfo.vertexInputStateCI, *createInfo.vertexInputBindings, *createInfo.vertexInputAttribs);
    fillMultisampleState(*createInfo.multisampleStateCI);
    fillDepthStencilState(*createInfo.depthStencilStateCI, createInfo.dynamicStates);
    fillColorBlendStates(*createInfo.colorBlendStateCI, *createInfo.colorBlendAttachmentStates, createInfo.dynamicStates);

    // Input assembly
    createInfo.inputAsmStateCI->topology = VkPrimitiveTopology(EPrimitiveTopology::apiInputAssemblyState(primitiveTopology));
    // Tessellation
    createInfo.tessStateCI->patchControlPoints = cntrlPts;
    // Viewport
    createInfo.viewportStateCI->scissorCount = 1;
    createInfo.viewportStateCI->viewportCount = 1;
    createInfo.dynamicStates.emplace_back(VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT);
    createInfo.dynamicStates.emplace_back(VkDynamicState::VK_DYNAMIC_STATE_SCISSOR);
}

void VulkanGraphicsPipeline::fillVertexInputState(VkPipelineVertexInputStateCreateInfo& vertexInputStateCI
    , std::vector<VkVertexInputBindingDescription>& bindings, std::vector<VkVertexInputAttributeDescription>& attributes) const
{
    EVertexType::Type shaderVertUsage;
    if (pipelineShader->getType()->isChildOf<DrawMeshShader>())
    {
        shaderVertUsage = static_cast<const DrawMeshShader*>(pipelineShader)->vertexUsage();
    }
    else if (pipelineShader->getType()->isChildOf<UniqueUtilityShader>())
    {
        shaderVertUsage = static_cast<const UniqueUtilityShader*>(pipelineShader)->vertexUsage();
    }

    const std::vector<ShaderVertexParamInfo*>& vertexParamsInfo = EVertexType::vertexParamInfo(shaderVertUsage);
    bindings.resize(vertexParamsInfo.size());
    attributes.reserve(pipelineShader->getReflection()->inputs.size());

    uint32 bindingIdx = 0;
    for (const ShaderVertexParamInfo* paramInfo : vertexParamsInfo)
    {
        VkVertexInputBindingDescription bindingDesc;
        bindingDesc.binding = bindingIdx;
        if (paramInfo != nullptr)
        {
            bindingDesc.inputRate = paramInfo->inputFrequency() == EShaderInputFrequency::PerVertex
                ? VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX : VkVertexInputRate::VK_VERTEX_INPUT_RATE_INSTANCE;
            bindingDesc.stride = paramInfo->paramStride();

            const ShaderVertexFieldNode* attributeNode = &paramInfo->startNode;

            while (attributeNode->isValid())
            {
                VkVertexInputAttributeDescription attributeDesc;
                attributeDesc.binding = bindingIdx;
                attributeDesc.format = VkFormat(EPixelDataFormat::getFormatInfo(EPixelDataFormat::Type(attributeNode->field->format))->format);
                attributeDesc.location = attributeNode->field->location;
                attributeDesc.offset = attributeNode->field->offset;

                attributes.emplace_back(attributeDesc);
                attributeNode = attributeNode->nextNode;
            }
        }
        else// This case mostly will not occur and if there is need for this case check if Vulkan allowing this.
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

void VulkanGraphicsPipeline::fillMultisampleState(VkPipelineMultisampleStateCreateInfo& multisampleStateCI) const
{
    multisampleStateCI.alphaToCoverageEnable = multisampleStateCI.alphaToOneEnable = multisampleStateCI.sampleShadingEnable = VK_FALSE;
    multisampleStateCI.pSampleMask = nullptr;
    multisampleStateCI.minSampleShading = 1.0f;
    if (pipelineShader->getType()->isChildOf<DrawMeshShader>())
    {
        multisampleStateCI.rasterizationSamples = VkSampleCountFlagBits(GlobalRenderVariables::GBUFFER_SAMPLE_COUNT.get());
    }
    else if (pipelineShader->getType()->isChildOf<UniqueUtilityShader>())
    {
        multisampleStateCI.rasterizationSamples = VkSampleCountFlagBits(renderpassProps.multisampleCount);
    }
    else
    {
        multisampleStateCI.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
    }
}

void VulkanGraphicsPipeline::fillDepthStencilState(VkPipelineDepthStencilStateCreateInfo& depthStencilStateCI, std::vector<VkDynamicState>& dynamicStates) const
{
    depthStencilStateCI.depthBoundsTestEnable = depthStencilStateCI.stencilTestEnable = VK_FALSE;
    depthStencilStateCI.depthTestEnable = depthState.compareOp != CoreGraphicsTypes::ECompareOp::Always ? VK_TRUE : VK_FALSE;
    depthStencilStateCI.depthWriteEnable = depthState.bEnableWrite ? VK_TRUE : VK_FALSE;
    depthStencilStateCI.depthCompareOp = VkCompareOp(CoreGraphicsTypes::getEnumTypeInfo(depthState.compareOp)->value);


    depthStencilStateCI.front.compareMask = 0xFFFFFFFF;
    depthStencilStateCI.front.writeMask = 0xFFFFFFFF;
    depthStencilStateCI.front.compareOp = VkCompareOp(CoreGraphicsTypes::getEnumTypeInfo(stencilStateFront.compareOp)->value);
    depthStencilStateCI.front.passOp = VkStencilOp(stencilStateFront.passOp);
    depthStencilStateCI.front.failOp = VkStencilOp(stencilStateFront.failOp);
    depthStencilStateCI.front.depthFailOp = VkStencilOp(stencilStateFront.depthFailOp);

    depthStencilStateCI.back.compareMask = 0xFFFFFFFF;
    depthStencilStateCI.back.writeMask = 0xFFFFFFFF;
    depthStencilStateCI.back.compareOp = VkCompareOp(CoreGraphicsTypes::getEnumTypeInfo(stencilStateBack.compareOp)->value);
    depthStencilStateCI.back.passOp = VkStencilOp(stencilStateBack.passOp);
    depthStencilStateCI.back.failOp = VkStencilOp(stencilStateBack.failOp);
    depthStencilStateCI.back.depthFailOp = VkStencilOp(stencilStateBack.depthFailOp);

    if (stencilStateBack.compareOp != CoreGraphicsTypes::ECompareOp::Never || stencilStateFront.compareOp != CoreGraphicsTypes::ECompareOp::Never)
    {
        depthStencilStateCI.stencilTestEnable = VK_TRUE;
        dynamicStates.emplace_back(VkDynamicState::VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    }
}

void VulkanGraphicsPipeline::fillShaderStages(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages) const
{
    shaderStages.reserve(pipelineShader->getShaders().size());

    for (const std::pair<const EShaderStage::Type, SharedPtr<ShaderCodeResource>>& shader : pipelineShader->getShaders())
    {
        const EShaderStage::ShaderStageInfo* stageInfo = EShaderStage::getShaderStageInfo(shader.second->shaderStage());
        PIPELINE_SHADER_STAGE_CREATE_INFO(shaderStageCreateInfo);
        shaderStageCreateInfo.stage = VkShaderStageFlagBits(stageInfo->shaderStage);
        shaderStageCreateInfo.pName = shader.second->entryPoint().getChar();
        shaderStageCreateInfo.module = static_cast<VulkanShaderCodeResource*>(shader.second.get())->shaderModule;
        // TODO(Jeslas) : Support for specialization constants
        shaderStageCreateInfo.pSpecializationInfo = VK_NULL_HANDLE;

        shaderStages.emplace_back(shaderStageCreateInfo);
    }
}

void VulkanGraphicsPipeline::fillColorBlendStates(VkPipelineColorBlendStateCreateInfo& colorBlendStateCI
    , std::vector<VkPipelineColorBlendAttachmentState>& vulkanAttachmentBlendStates, std::vector<VkDynamicState>& dynamicStates) const
{
    bool bHasConstant = false;
    vulkanAttachmentBlendStates.reserve(attachmentBlendStates.size());
    for (const AttachmentBlendState& attachmentBlendState : attachmentBlendStates)
    {
        bHasConstant = bHasConstant || attachmentBlendState.usesBlendConstant();

        VkPipelineColorBlendAttachmentState colorBlendState;
        colorBlendState.blendEnable = attachmentBlendState.bBlendEnable ? VK_TRUE : VK_FALSE;
        colorBlendState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
            | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
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

void VulkanGraphicsPipeline::fillDynamicPermutedStates(VulkanGraphicsPipeline::VulkanPipelineCreateInfo& createInfo, const GraphicsPipelineQueryParams& params) const
{
    // Rasterization state
    PIPELINE_RASTERIZATION_STATE_CREATE_INFO(rasterizationStateCI);
    rasterizationStateCI.cullMode = VkCullModeFlagBits(params.cullingMode);
    rasterizationStateCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
    if (GlobalRenderVariables::ENABLE_NON_FILL_DRAWS.get())
    {
        rasterizationStateCI.polygonMode = VkPolygonMode(params.drawMode);
    }
    else
    {
        rasterizationStateCI.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
    }

    createInfo.rasterizationStateCI = rasterizationStateCI;
    if (params.drawMode != EPolygonDrawMode::Fill)
    {
        createInfo.dynamicStates.emplace_back(VkDynamicState::VK_DYNAMIC_STATE_LINE_WIDTH);
    }
}

void VulkanGraphicsPipeline::createPipelines(const std::vector<VulkanGraphicsPipeline::VulkanPipelineCreateInfo>& createInfos)
{
    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();

    std::vector<VkPipelineDynamicStateCreateInfo> dynamicStateCIs(createInfos.size());
    std::vector<VkGraphicsPipelineCreateInfo> pipelineCIs(createInfos.size());    
    for (int32 createInfoIdx = 0; createInfoIdx < createInfos.size(); ++createInfoIdx)
    {
        const VulkanPipelineCreateInfo& ci = createInfos[createInfoIdx];

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

String VulkanGraphicsPipeline::getObjectName() const
{
    return getResourceName();
}

void VulkanGraphicsPipeline::init()
{
    fatalAssert(attachmentBlendStates.size() == pipelineShader->getReflection()->outputs.size()
        , "Blend states has to be equivalent to color attachments count");
    fatalAssert(pipelineShader->getType()->isChildOf<DrawMeshShader>() || pipelineShader->getType()->isChildOf<UniqueUtilityShader>()
        , "Not supported shader for graphics pipeline");

    BaseType::init();
    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
    pipelineLocalCache = VulkanGraphicsHelper::createPipelineCache(graphicsInstance);
    if (parentCache)
    {
        VulkanGraphicsHelper::mergePipelineCaches(graphicsInstance, pipelineLocalCache
            , { static_cast<const VulkanPipelineCache*>(parentCache)->pipelineCacheRead });
    }

    reinitResources();
}

void VulkanGraphicsPipeline::reinitResources()
{
    BaseType::reinitResources();
    // Release reinitializing resources
    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();
    for (VkPipeline& graphicsPipeline : pipelines)
    {
        VulkanGraphicsHelper::destroyPipeline(graphicsInstance, graphicsPipeline);
    }

    // Common to all pipelines
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
        VulkanPipelineCreateInfo& graphicsPipelineCI = tempPipelineCIs[0];
        // Setting if we will derive from this pipeline or gets derived from something else
        graphicsPipelineCI.pipelineFlags = bCanBeParent || tempPipelineCIs.size() > 1 ? VkPipelineCreateFlagBits::VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT : 0;
        if (parentPipeline != nullptr)
        {
            graphicsPipelineCI.pipelineFlags |= VkPipelineCreateFlagBits::VK_PIPELINE_CREATE_DERIVATIVE_BIT;
            graphicsPipelineCI.basePipelineHandle = static_cast<const VulkanGraphicsPipeline*>(parentPipeline)->pipelines[0];
        }

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
    }
    for (int32 pipelineIdx = 1; pipelineIdx < totalPipelinesCount; ++pipelineIdx)
    {
        VulkanPipelineCreateInfo& graphicsPipelineCI = tempPipelineCIs[pipelineIdx];

        graphicsPipelineCI = tempPipelineCIs[0];
        graphicsPipelineCI.pipelineFlags = VkPipelineCreateFlagBits::VK_PIPELINE_CREATE_DERIVATIVE_BIT;
        graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;
        graphicsPipelineCI.basePipelineIdx = 0;

        fillDynamicPermutedStates(graphicsPipelineCI, paramForIdx(pipelineIdx));
    }

    createPipelines(tempPipelineCIs);
}

void VulkanGraphicsPipeline::release()
{
    BaseType::release();
    IGraphicsInstance* graphicsInstance = gEngine->getRenderApi()->getGraphicsInstance();

    if (pipelineLocalCache != nullptr)
    {
        VulkanGraphicsHelper::destroyPipelineCache(graphicsInstance, pipelineLocalCache);
        pipelineLocalCache = nullptr;
    }

    for (VkPipeline& graphicsPipeline : pipelines)
    {
        VulkanGraphicsHelper::destroyPipeline(graphicsInstance, graphicsPipeline);
    }
    pipelines.clear();
}

void VulkanGraphicsPipeline::setCompatibleRenderpass(VkRenderPass renderpass)
{
    compatibleRenderpass = renderpass;
}

VkPipeline VulkanGraphicsPipeline::getPipeline(const GraphicsPipelineQueryParams& pipelineQuery) const
{
    return pipelines[idxFromParam(pipelineQuery)];
}