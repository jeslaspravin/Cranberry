#pragma once

#include <vulkan_core.h>

#include "RenderInterface/CoreGraphicsTypes.h"
#include "RenderInterface/Resources/ShaderResources.h"
#include "RenderInterface/Resources/Pipelines.h"

namespace EngineToVulkanAPI
{
    VkCompareOp vulkanCompareOp(CoreGraphicsTypes::ECompareOp::Type compareOp);

    VkFormat vulkanDataFormat(EPixelDataFormat::Type dataFormat);
    EPixelDataFormat::Type vulkanToEngineDataFormat(VkFormat dataFormat);

    VkFilter vulkanFilter(ESamplerFiltering::Type filter);
    VkSamplerMipmapMode vulkanSamplerMipFilter(ESamplerFiltering::Type filter);

    VkSamplerAddressMode vulkanSamplerAddressing(ESamplerTilingMode::Type tilingMode);
    VkComponentSwizzle vulkanComponentSwizzle(EPixelComponentMapping::Type mapping);

    VkShaderStageFlagBits vulkanShaderStage(EShaderStage::Type shaderStage);
    VkShaderStageFlags vulkanShaderStageFlags(uint32 shaderStages);

    VkPipelineStageFlagBits2KHR vulkanPipelineStage(EPipelineStages::Type pipelineStage);
    VkPipelineStageFlags2KHR vulkanPipelineStageFlags(uint32 pipelineStages);

    VkPrimitiveTopology vulkanPrimitiveTopology(EPrimitiveTopology::Type inputAssembly);

    VkAttachmentLoadOp vulkanLoadOp(EAttachmentOp::LoadOp loadOp);
    VkAttachmentStoreOp vulkanStoreOp(EAttachmentOp::StoreOp storeOp);
}