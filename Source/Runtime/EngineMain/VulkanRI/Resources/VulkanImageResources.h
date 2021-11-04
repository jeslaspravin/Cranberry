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

protected:
    VulkanCubeImageResource();
public:
    VulkanCubeImageResource(EPixelDataFormat::Type imageFormat);
};

class VulkanCubeRTImageResource : public VulkanCubeImageResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanCubeRTImageResource, , VulkanCubeImageResource, )

private:
    VulkanCubeRTImageResource();
public:
    VulkanCubeRTImageResource(EPixelDataFormat::Type imageFormat);
};

namespace GraphicsTypes
{
    typedef VulkanRenderTargetResource GraphicsRenderTargetResource;
    typedef VulkanCubeImageResource GraphicsCubeImageResource;
    typedef VulkanCubeRTImageResource GraphicsCubeRTImageResource;
    typedef VulkanImageResource GraphicsImageResource;
}