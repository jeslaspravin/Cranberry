#include "VulkanInternals/Resources/VulkanImageResources.h"

//////////////////////////////////////////////////////////////////////////
//// Render target image resource
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanRenderTargetResource, VK_OBJECT_TYPE_IMAGE)

VulkanRenderTargetResource::VulkanRenderTargetResource() : BaseType()
{
    isRenderTarget = true;
    shaderUsage = 0;
}

VulkanRenderTargetResource::VulkanRenderTargetResource(ImageResourceCreateInfo createInfo)
    : BaseType(createInfo, false)
{
    isRenderTarget = true;
    shaderUsage = 0;
}

//////////////////////////////////////////////////////////////////////////
//// Cube map image resource
//////////////////////////////////////////////////////////////////////////


DEFINE_VK_GRAPHICS_RESOURCE(VulkanCubeImageResource, VK_OBJECT_TYPE_IMAGE)

VulkanCubeImageResource::VulkanCubeImageResource() : BaseType()
{
    createFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    layerCount = 6;
    viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
}

VulkanCubeImageResource::VulkanCubeImageResource(ImageResourceCreateInfo createInfo, bool cpuAccessible)
    : BaseType(createInfo, cpuAccessible)
{
    createFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    layerCount = 6;
    viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
}

DEFINE_VK_GRAPHICS_RESOURCE(VulkanCubeRTImageResource, VK_OBJECT_TYPE_IMAGE)

VulkanCubeRTImageResource::VulkanCubeRTImageResource()
    : BaseType()
{
    isRenderTarget = true;
}

VulkanCubeRTImageResource::VulkanCubeRTImageResource(ImageResourceCreateInfo createInfo)
    : BaseType(createInfo)
{
    isRenderTarget = true;
}
