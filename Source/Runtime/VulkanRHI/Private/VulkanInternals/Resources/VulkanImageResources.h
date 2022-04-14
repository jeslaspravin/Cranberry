/*!
 * \file VulkanImageResources.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "VulkanInternals/Resources/VulkanMemoryResources.h"

class VulkanRenderTargetResource : public VulkanImageResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanRenderTargetResource, , VulkanImageResource, )

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
} // namespace GraphicsTypes