
#ifndef EXPORTED_VK_FUNCTIONS
#define EXPORTED_VK_FUNCTIONS(function)
#endif

EXPORTED_VK_FUNCTIONS(vkGetInstanceProcAddr)

#undef EXPORTED_VK_FUNCTIONS

#ifndef GLOBAL_VK_FUNCTIONS
#define GLOBAL_VK_FUNCTIONS(function)
#endif
GLOBAL_VK_FUNCTIONS(vkEnumerateInstanceExtensionProperties)
GLOBAL_VK_FUNCTIONS(vkEnumerateInstanceLayerProperties)
GLOBAL_VK_FUNCTIONS(vkCreateInstance)
#undef GLOBAL_VK_FUNCTIONS

#ifndef INSTANCE_VK_FUNCTIONS
#define INSTANCE_VK_FUNCTIONS(function)
#endif
INSTANCE_VK_FUNCTIONS(vkDestroyInstance)
INSTANCE_VK_FUNCTIONS(vkEnumeratePhysicalDevices)
INSTANCE_VK_FUNCTIONS(vkGetPhysicalDeviceProperties)
INSTANCE_VK_FUNCTIONS(vkGetPhysicalDeviceFeatures)
INSTANCE_VK_FUNCTIONS(vkGetPhysicalDeviceQueueFamilyProperties)
INSTANCE_VK_FUNCTIONS(vkEnumerateDeviceExtensionProperties)
INSTANCE_VK_FUNCTIONS(vkEnumerateDeviceLayerProperties)
INSTANCE_VK_FUNCTIONS(vkCreateDevice)
INSTANCE_VK_FUNCTIONS(vkGetDeviceProcAddr)
#undef INSTANCE_VK_FUNCTIONS

#ifndef INSTANCE_VK_EXT_FUNCTIONS
#define INSTANCE_VK_EXT_FUNCTIONS(function,extension)
#endif
INSTANCE_VK_EXT_FUNCTIONS(vkGetPhysicalDeviceSurfaceSupportKHR, VK_KHR_SURFACE_EXTENSION_NAME)
INSTANCE_VK_EXT_FUNCTIONS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, VK_KHR_SURFACE_EXTENSION_NAME)
INSTANCE_VK_EXT_FUNCTIONS(vkGetPhysicalDeviceSurfaceFormatsKHR, VK_KHR_SURFACE_EXTENSION_NAME)
#undef INSTANCE_VK_EXT_FUNCTIONS

#ifndef INSTANCE_VK_PLATFORM_EXT_FUNCTIONS
#define INSTANCE_VK_PLATFORM_EXT_FUNCTIONS(function,extension)
#endif
INSTANCE_VK_PLATFORM_EXT_FUNCTIONS(vkCreatePlatformSurfaceKHR, VK_KHR_SURFACE_EXTENSION_NAME)
#undef INSTANCE_VK_PLATFORM_EXT_FUNCTIONS

#ifndef DEVICE_VK_FUNCTIONS
#define DEVICE_VK_FUNCTIONS(function)
#endif
DEVICE_VK_FUNCTIONS(vkDestroyDevice)
DEVICE_VK_FUNCTIONS(vkGetDeviceQueue)
DEVICE_VK_FUNCTIONS(vkDeviceWaitIdle)
DEVICE_VK_FUNCTIONS(vkCreateBuffer)
DEVICE_VK_FUNCTIONS(vkGetBufferMemoryRequirements)
#undef DEVICE_VK_FUNCTIONS

#ifndef DEVICE_VK_EXT_FUNCTIONS
#define DEVICE_VK_EXT_FUNCTIONS(function,extension)
#endif
DEVICE_VK_EXT_FUNCTIONS(vkCreateSwapchainKHR,VK_KHR_SWAPCHAIN_EXTENSION_NAME)
DEVICE_VK_EXT_FUNCTIONS(vkGetSwapchainImagesKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME)
DEVICE_VK_EXT_FUNCTIONS(vkAcquireNextImageKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME)
DEVICE_VK_EXT_FUNCTIONS(vkQueuePresentKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME)
DEVICE_VK_EXT_FUNCTIONS(vkDestroySwapchainKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME)
#undef DEVICE_VK_EXT_FUNCTIONS