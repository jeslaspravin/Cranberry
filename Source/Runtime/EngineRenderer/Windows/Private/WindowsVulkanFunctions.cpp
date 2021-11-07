#include "WindowsVulkanFunctions.h"
#include "WindowsCommonHeaders.h"
#include "WindowsAppInstance.h"
#include "WindowsAppWindow.h"
#include "vulkan_win32.h"
#include "VulkanRI/VulkanInternals/VulkanFunctions.h"
#include "Logger/Logger.h"


const char* PFN_Win32SurfaceKHR::EXT_NAME = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;

void PFN_Win32SurfaceKHR::setInstanceWindow(const GenericAppInstance* instance, const GenericAppWindow* window)
{
    const WindowsAppInstance* pAppInstance = static_cast<const WindowsAppInstance*>(instance);
    hInstance = pAppInstance->windowsInstance;

    const WindowsAppWindow* pAppWindow = static_cast<const WindowsAppWindow*>(window);
    hWindow = pAppWindow->getWindowHandle();
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
    createInfo.hinstance = (HINSTANCE)hInstance;
    createInfo.hwnd = (HWND)hWindow;
    
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
