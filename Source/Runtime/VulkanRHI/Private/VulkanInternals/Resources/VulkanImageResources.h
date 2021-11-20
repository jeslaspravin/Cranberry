#pragma once
#include "VulkanInternals/Resources/VulkanMemoryResources.h"

class VulkanRenderTargetResource : public VulkanImageResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanRenderTargetResource,,VulkanImageResource,)

private:
    VulkanRenderTargetResource();
public:
    VulkanRenderTargetResource(ImageResourceCreateInfo createInfo);
};

class VulkanCubeImageResource : public VulkanImageResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanCubeImageResource, , VulkanImageResource, )

protected:
    VulkanCubeImageResource();
public:
    VulkanCubeImageResource(ImageResourceCreateInfo createInfo, bool cpuAccessible = false);
};

class VulkanCubeRTImageResource : public VulkanCubeImageResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanCubeRTImageResource, , VulkanCubeImageResource, )

private:
    VulkanCubeRTImageResource();
public:
    VulkanCubeRTImageResource(ImageResourceCreateInfo createInfo);
};

namespace GraphicsTypes
{
    typedef VulkanRenderTargetResource GraphicsRenderTargetResource;
    typedef VulkanCubeImageResource GraphicsCubeImageResource;
    typedef VulkanCubeRTImageResource GraphicsCubeRTImageResource;
    typedef VulkanImageResource GraphicsImageResource;
}