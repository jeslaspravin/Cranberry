#pragma once
#include "VulkanRI/VulkanInternals/Resources/VulkanMemoryResources.h"

class ENGINERENDERER_EXPORT VulkanRenderTargetResource : public VulkanImageResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanRenderTargetResource,,VulkanImageResource,)

private:
    VulkanRenderTargetResource();
public:
    VulkanRenderTargetResource(EPixelDataFormat::Type imageFormat);
};

class ENGINERENDERER_EXPORT VulkanCubeImageResource : public VulkanImageResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanCubeImageResource, , VulkanImageResource, )

protected:
    VulkanCubeImageResource();
public:
    VulkanCubeImageResource(EPixelDataFormat::Type imageFormat);
};

class ENGINERENDERER_EXPORT VulkanCubeRTImageResource : public VulkanCubeImageResource
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