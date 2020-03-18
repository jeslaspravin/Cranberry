#pragma once
#include "../VulkanInternals/Resources/VulkanMemoryResources.h"

class VulkanRenderTargetResource : public VulkanImageResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanRenderTargetResource,,VulkanImageResource,)

private:
    VulkanRenderTargetResource();
public:
    VulkanRenderTargetResource(EPixelDataFormat::Type imageFormat);
};

class VulkanCubeImageResource : public VulkanImageResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanCubeImageResource, , VulkanImageResource, )

private:
    VulkanCubeImageResource();
public:
    VulkanCubeImageResource(EPixelDataFormat::Type imageFormat);
};

namespace GraphicsTypes
{
    typedef VulkanRenderTargetResource GraphicsRenderTargetResource;
    typedef VulkanCubeImageResource GraphicsCubeImageResource;
    typedef VulkanImageResource GraphicsImageResource;
}