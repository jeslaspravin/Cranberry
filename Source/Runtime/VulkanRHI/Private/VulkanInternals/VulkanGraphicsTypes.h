/*!
 * \file VulkanGraphicsTypes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include <vulkan/vulkan_core.h>

#include "RenderInterface/CoreGraphicsTypes.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/Resources/ShaderResources.h"

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

VkPipelineStageFlagBits2 vulkanPipelineStage(EPipelineStages::Type pipelineStage);
VkPipelineStageFlags2 vulkanPipelineStageFlags(uint64 pipelineStages);

VkPipelineStageFlags2 shaderToPipelineStageFlags(VkShaderStageFlags shaderStageFlags);
VkShaderStageFlags pipelineToShaderStageFlags(VkPipelineStageFlags2 pipelineStageFlags);
VkPipelineStageFlags2 pipelinesSupportedPerQueue(VkQueueFlags queueFlags);
VkAccessFlags2 accessMaskForStages(VkPipelineStageFlags2 pipelineStages);
VkAccessFlags2 accessMaskPerQueue(VkQueueFlags queueFlags);

VkPrimitiveTopology vulkanPrimitiveTopology(EPrimitiveTopology::Type inputAssembly);

VkAttachmentLoadOp vulkanLoadOp(EAttachmentOp::LoadOp loadOp);
VkAttachmentStoreOp vulkanStoreOp(EAttachmentOp::StoreOp storeOp);
} // namespace EngineToVulkanAPI