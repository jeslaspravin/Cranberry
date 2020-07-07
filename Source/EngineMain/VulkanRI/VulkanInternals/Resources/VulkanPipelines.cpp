#include "VulkanPipelines.h"

DEFINE_VK_GRAPHICS_RESOURCE(VulkanGraphicsPipeline, VK_OBJECT_TYPE_PIPELINE)

VulkanGraphicsPipeline::VulkanGraphicsPipeline(const VulkanGraphicsPipeline* parent)
    : BaseType(parent)
    , pipelineLayout(parent->pipelineLayout)
    , compatibleRenderpass(parent->compatibleRenderpass)
{}

VkGraphicsPipelineCreateInfo VulkanGraphicsPipeline::generateCreateInfo() const
{
    // TODO(Jeslas) : ASAP
}

String VulkanGraphicsPipeline::getObjectName() const
{
    return getResourceName();
}

void VulkanGraphicsPipeline::init()
{
    BaseType::init();
}

void VulkanGraphicsPipeline::reinitResources()
{
    BaseType::reinitResources();
}

void VulkanGraphicsPipeline::release()
{
    BaseType::release();
}
