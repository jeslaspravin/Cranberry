#pragma once
#include "../GenericVulkanFunctions.h"
#include "vulkan_core.h"
#include "../../../../Core/Platform/GenericAppInstance.h"
#include <windows.h>

struct PFN_Win32SurfaceKHR : PFN_SurfaceKHR<VkInstance, const void*, const VkAllocationCallbacks*, VkSurfaceKHR*>
{
	HINSTANCE hInstance;
	HWND hWindow;

	static const char* EXT_NAME;

	PFN_Win32SurfaceKHR():hInstance(nullptr),hWindow(nullptr) {}

	void setInstanceWindow(const struct GenericAppInstance* instance, const class GenericAppWindow* window) override;
	void operator()(VkInstance instance, const void* pNext, const VkAllocationCallbacks* allocatorCallback,
		VkSurfaceKHR* surface) const override;
};

namespace GVulkanPlatform
{
	typedef PFN_Win32SurfaceKHR PFN_vkCreatePlatformSurfaceKHR;
}