#pragma once
#include "../GenericVulkanFunctions.h"
#include "vulkan_core.h"
#include "../../../../Core/Platform/GenericAppInstance.h"
#include <windows.h>

struct PFN_Win32SurfaceKHR : PFN_SurfaceKHR<VkInstance, const void*, const VkAllocationCallbacks*, VkSurfaceKHR*>
{
	HINSTANCE hInstance;
	HWND hWindow;

	PFN_Win32SurfaceKHR():hInstance(nullptr),hWindow(nullptr) {}
	PFN_Win32SurfaceKHR(const GenericAppInstance* appInstance);

	virtual void operator()(VkInstance, const void*, const VkAllocationCallbacks*, VkSurfaceKHR*) const override;
};

namespace GVulkanPlatform
{
	typedef PFN_Win32SurfaceKHR PFN_vkCreatePlatformSurfaceKHR;
}