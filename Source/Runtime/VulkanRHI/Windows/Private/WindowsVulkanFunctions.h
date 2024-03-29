/*!
 * \file WindowsVulkanFunctions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include <vulkan/vulkan_core.h>

#include "VulkanInternals/Platform/GenericVulkanFunctions.h"
#include "Types/Platform/PlatformTypes.h"

struct PFN_Win32SurfaceKHR : PFN_SurfaceKHR<VkInstance, const void *, const VkAllocationCallbacks *, VkSurfaceKHR *>
{
    InstanceHandle hInstance;
    WindowHandle hWindow;

    static const char *EXT_NAME;

    PFN_Win32SurfaceKHR()
        : hInstance(nullptr)
        , hWindow(nullptr)
    {}

    void setInstanceWindow(const ApplicationInstance *instance, const class GenericAppWindow *window) override;
    void
    operator() (VkInstance instance, const void *pNext, const VkAllocationCallbacks *allocatorCallback, VkSurfaceKHR *surface) const override;
};

namespace GVulkanPlatform
{
typedef PFN_Win32SurfaceKHR PFN_vkCreatePlatformSurfaceKHR;
}