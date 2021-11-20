#pragma once

#include <vulkan_core.h>

#include "VulkanInternals/Platform/GenericVulkanFunctions.h"

struct PFN_Win32SurfaceKHR : PFN_SurfaceKHR<VkInstance, const void*, const VkAllocationCallbacks*, VkSurfaceKHR*>
{
    void* hInstance;
    void* hWindow;

    static const char* EXT_NAME;

    PFN_Win32SurfaceKHR() : hInstance(nullptr),hWindow(nullptr) {}

    void setInstanceWindow(const ApplicationInstance* instance, const class GenericAppWindow* window) override;
    void operator()(VkInstance instance, const void* pNext, const VkAllocationCallbacks* allocatorCallback,
        VkSurfaceKHR* surface) const override;
};

namespace GVulkanPlatform
{
    typedef PFN_Win32SurfaceKHR PFN_vkCreatePlatformSurfaceKHR;
}