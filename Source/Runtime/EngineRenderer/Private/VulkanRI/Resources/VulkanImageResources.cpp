#include "VulkanRI/Resources/VulkanImageResources.h"

//////////////////////////////////////////////////////////////////////////
//// Render target image resource
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanRenderTargetResource,VK_OBJECT_TYPE_IMAGE)

VulkanRenderTargetResource::VulkanRenderTargetResource() : BaseType()
{
    isRenderTarget = true;
    shaderUsage = 0;
}

VulkanRenderTargetResource::VulkanRenderTargetResource(EPixelDataFormat::Type imageFormat)
    : BaseType(imageFormat,false)
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

VulkanCubeImageResource::VulkanCubeImageResource(EPixelDataFormat::Type imageFormat)
    : BaseType(imageFormat, false)
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

VulkanCubeRTImageResource::VulkanCubeRTImageResource(EPixelDataFormat::Type imageFormat)
    : BaseType(imageFormat)
{
    isRenderTarget = true;
}
