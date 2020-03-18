#include "VulkanImageResources.h"

//////////////////////////////////////////////////////////////////////////
//// Render target image resource
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanRenderTargetResource,VK_OBJECT_TYPE_IMAGE)

VulkanRenderTargetResource::VulkanRenderTargetResource() : BaseType()
{
    isRenderTarget = true;
}

VulkanRenderTargetResource::VulkanRenderTargetResource(EPixelDataFormat::Type imageFormat)
    : BaseType(imageFormat,false)
{
    isRenderTarget = true;
}

//////////////////////////////////////////////////////////////////////////
//// Cube map image resource
//////////////////////////////////////////////////////////////////////////


DEFINE_VK_GRAPHICS_RESOURCE(VulkanCubeImageResource, VK_OBJECT_TYPE_IMAGE)

VulkanCubeImageResource::VulkanCubeImageResource() : BaseType()
{
    createFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    layerCount = 6;
}

VulkanCubeImageResource::VulkanCubeImageResource(EPixelDataFormat::Type imageFormat)
    : BaseType(imageFormat, false)
{
    createFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    layerCount = 6;
}
