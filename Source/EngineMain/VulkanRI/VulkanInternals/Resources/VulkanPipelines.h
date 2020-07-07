#pragma once
#include "../../../RenderInterface/Resources/Pipelines.h"
#include "../../Resources/IVulkanResources.h"
#include "../VulkanMacros.h"

class VulkanGraphicsPipeline : public GraphicsPipeline, public IVulkanResources
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanGraphicsPipeline,, GraphicsPipeline,)
private:
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    VkPipelineVertexInputStateCreateInfo vertexInputState;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
    VkPipelineTessellationStateCreateInfo tessellationState;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterizationState;
    VkPipelineMultisampleStateCreateInfo multisampleState;
    VkPipelineDepthStencilStateCreateInfo depthStencilState;
    VkPipelineColorBlendStateCreateInfo colorBlendState;
    VkPipelineDynamicStateCreateInfo dynamicState;

public:
    std::map<ECullingMode, VkPipeline> pipelines;

    // Just copies original resource handled at GlobalRenderingContext
    VkPipelineLayout pipelineLayout;
    VkRenderPass compatibleRenderpass;

private:
    VkGraphicsPipelineCreateInfo generateCreateInfo() const;
public:
    VulkanGraphicsPipeline(const VulkanGraphicsPipeline* parent);
    VulkanGraphicsPipeline() = default;

    /* IVulkanResources overrides */
    String getObjectName() const final;

    /* GraphicsResource overrides */
    void init() final;
    void reinitResources() final;
    void release() final;

    /* Override ends */


};
