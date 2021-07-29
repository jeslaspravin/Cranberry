#pragma once
#include "../../../RenderInterface/Resources/Pipelines.h"
#include "../../Resources/IVulkanResources.h"
#include "../VulkanMacros.h"

class VulkanPipelineCache : public PipelineCacheBase, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanPipelineCache,, PipelineCacheBase,)

public:
    VkPipelineCache pipelineCacheRead;

    /* IVulanResources overrides */
public:
    String getObjectName() const override;
    uint64 getDispatchableHandle() const override;

    /* GraphicsResource overrides */
    void init() override;
    void reinitResources() override;
    void release() override;

    /* PipelineCacheBase overrides */
protected:
    std::vector<uint8> getRawToWrite() const override;

    /* Override ends */
};

class VulkanGraphicsPipeline : public GraphicsPipelineBase, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanGraphicsPipeline,, GraphicsPipelineBase,)
private:
    // Just a replica of VkGraphicsPipelineCreateInfo but non const
    struct VulkanPipelineCreateInfo
    {
        // Ptr for common informations
        std::vector<VkSpecializationMapEntry>* specializationConstEntries;
        std::vector<uint8>* specializationConstData;
        std::vector<VkSpecializationInfo>* specializationInfo;
        std::vector<VkPipelineShaderStageCreateInfo>* shaderStageCIs;

        // VertexInputs
        std::vector<VkVertexInputBindingDescription>* vertexInputBindings;
        std::vector<VkVertexInputAttributeDescription>* vertexInputAttribs;
        VkPipelineVertexInputStateCreateInfo* vertexInputStateCI;
        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo* inputAsmStateCI;
        // Tessellation
        VkPipelineTessellationStateCreateInfo* tessStateCI;
        // Viewport and scissors
        VkPipelineViewportStateCreateInfo* viewportStateCI;
        // Rasterization state - unique to each variant
        // Multi sampling
        VkPipelineMultisampleStateCreateInfo* multisampleStateCI;
        // Depth and stencil
        VkPipelineDepthStencilStateCreateInfo* depthStencilStateCI;
        // Color blending
        std::vector<VkPipelineColorBlendAttachmentState>* colorBlendAttachmentStates;
        VkPipelineColorBlendStateCreateInfo* colorBlendStateCI;

        // Unique informations
        VkPipelineCreateFlags pipelineFlags;
        VkPipeline basePipelineHandle = VK_NULL_HANDLE;
        int32 basePipelineIdx = -1;

        VkPipelineRasterizationStateCreateInfo rasterizationStateCI;
        std::vector<VkDynamicState> dynamicStates;
    };

    std::vector<VkPipeline> pipelines;
    VkRenderPass compatibleRenderpass;
public:
    VkPipelineCache pipelineLocalCache;

    // Just copies original resource handled at GlobalRenderingContext
    VkPipelineLayout pipelineLayout;
private:
    void fillShaderStages(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages) const;
    void fillSpecializationConsts(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages
        , std::vector<VkSpecializationMapEntry>& specEntries, std::vector<uint8>& specData, std::vector<VkSpecializationInfo>& specializationInfo) const;
    void fillVertexInputState(VkPipelineVertexInputStateCreateInfo& vertexInputStateCI
        , std::vector<VkVertexInputBindingDescription>& bindings, std::vector<VkVertexInputAttributeDescription>& attributes) const;
    void fillMultisampleState(VkPipelineMultisampleStateCreateInfo& multisampleStateCI) const;
    void fillDepthStencilState(VkPipelineDepthStencilStateCreateInfo& depthStencilStateCI, std::vector<VkDynamicState>& dynamicStates) const;
    void fillColorBlendStates(VkPipelineColorBlendStateCreateInfo& colorBlendStateCI, std::vector<VkPipelineColorBlendAttachmentState>& blendStates
        , std::vector<VkDynamicState>& dynamicStates) const;

    void fillPipelineStates(VulkanPipelineCreateInfo& createInfo) const;
    void fillDynamicPermutedStates(VulkanPipelineCreateInfo& createInfo
        , const GraphicsPipelineQueryParams& params) const;

    void validateCreateInfo(VulkanPipelineCreateInfo& createInfo) const;

    void createPipelines(const std::vector<VulkanPipelineCreateInfo>& createInfos);
public:
    VulkanGraphicsPipeline(const GraphicsPipelineBase* parent);
    VulkanGraphicsPipeline() = default;

    /* IVulkanResources overrides */
    String getObjectName() const final;

    /* GraphicsResource overrides */
    void init() final;
    void reinitResources() final;
    void release() final;

    /* Override ends */

    void setCompatibleRenderpass(VkRenderPass renderpass);

    VkPipeline getPipeline(const GraphicsPipelineQueryParams& pipelineQuery) const;
};

class VulkanComputePipeline : public ComputePipelineBase, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanComputePipeline, , ComputePipelineBase, )
private:

    VkPipeline pipeline;
public:
    VkPipelineCache pipelineLocalCache;

    // Just copies original resource handled at GlobalRenderingContext
    VkPipelineLayout pipelineLayout;
private:
    void fillShaderStages(VkPipelineShaderStageCreateInfo& shaderStage) const;
    void fillSpecializationConsts(VkPipelineShaderStageCreateInfo& shaderStages
        , std::vector<VkSpecializationMapEntry>& specEntries, std::vector<uint8>& specData, VkSpecializationInfo& specializationInfo) const;
public:
    VulkanComputePipeline(const ComputePipelineBase* parent);
    VulkanComputePipeline() = default;

    /* IVulkanResources overrides */
    String getObjectName() const final;

    /* ComputeResource overrides */
    void init() final;
    void reinitResources() final;
    void release() final;

    /* Override ends */

    VkPipeline getPipeline() const { return pipeline; }
};