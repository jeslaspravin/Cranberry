#include "WindowsVulkanFunctions.h"
#include "../../../../Core/Platform/PlatformInstances.h"
#include "vulkan_win32.h"
#include "../../VulkanFunctions.h"
#include "../../../../Core/Logger/Logger.h"


PFN_Win32SurfaceKHR::PFN_Win32SurfaceKHR(const GenericAppInstance* appInstance)
{
	const WindowsAppInstance* pAppInstance = static_cast<const WindowsAppInstance*>(appInstance);
	hInstance = pAppInstance->windowsInstance;
	hWindow = pAppInstance->getWindowHandle();
}

void PFN_Win32SurfaceKHR::operator()(VkInstance instance, const void* pNext, const VkAllocationCallbacks* allocatorCallback,
	VkSurfaceKHR* surface) const
{
	if (!hInstance || !hWindow)
	{		
		Logger::error("Vulkan", "PFN_Win32SurfaceKHR() : Cannot create surface withot HINSTANCE or HWND");
		return;
	}

	VkWin32SurfaceCreateInfoKHR createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.flags = 0;
	createInfo.pNext = pNext;
	createInfo.hinstance = hInstance;
	createInfo.hwnd = hWindow;
	
	PFN_vkCreateWin32SurfaceKHR win32SurfaceCreate = (PFN_vkCreateWin32SurfaceKHR)
		Vk::vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
	if (win32SurfaceCreate == nullptr) {
		Logger::error("Vulkan", "PFN_Win32SurfaceKHR() : failed fetching Create Surface function vkCreateWin32SurfaceKHR");
		return;
	}

	VkResult result = win32SurfaceCreate(instance,&createInfo, allocatorCallback, surface);

	if (result != VK_SUCCESS) {
		Logger::error("Vulkan", "PFN_Win32SurfaceKHR() : failed creating surface");
	}
}
