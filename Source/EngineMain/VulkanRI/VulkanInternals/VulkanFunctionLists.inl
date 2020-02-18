
#ifndef EXPORTED_VK_FUNCTIONS
#define EXPORTED_VK_FUNCTIONS(function)
#endif

EXPORTED_VK_FUNCTIONS(vkGetInstanceProcAddr)

#undef EXPORTED_VK_FUNCTIONS

#ifndef GLOBAL_VK_FUNCTIONS
#define GLOBAL_VK_FUNCTIONS(function)
#endif
GLOBAL_VK_FUNCTIONS(vkEnumerateInstanceVersion)
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
INSTANCE_VK_EXT_FUNCTIONS(vkGetPhysicalDeviceProperties2KHR, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
INSTANCE_VK_EXT_FUNCTIONS(vkGetPhysicalDeviceFeatures2KHR, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
INSTANCE_VK_EXT_FUNCTIONS(vkDestroySurfaceKHR, VK_KHR_SURFACE_EXTENSION_NAME)
INSTANCE_VK_EXT_FUNCTIONS(vkGetPhysicalDeviceSurfaceSupportKHR, VK_KHR_SURFACE_EXTENSION_NAME)
INSTANCE_VK_EXT_FUNCTIONS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, VK_KHR_SURFACE_EXTENSION_NAME)
INSTANCE_VK_EXT_FUNCTIONS(vkGetPhysicalDeviceSurfaceFormatsKHR, VK_KHR_SURFACE_EXTENSION_NAME)
INSTANCE_VK_EXT_FUNCTIONS(vkGetPhysicalDeviceSurfacePresentModesKHR, VK_KHR_SURFACE_EXTENSION_NAME)
/* Debug Functions */
INSTANCE_VK_EXT_FUNCTIONS(vkCreateDebugUtilsMessengerEXT, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
INSTANCE_VK_EXT_FUNCTIONS(vkDestroyDebugUtilsMessengerEXT, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)

#undef INSTANCE_VK_EXT_FUNCTIONS

#ifndef INSTANCE_VK_PLATFORM_EXT_FUNCTIONS
#define INSTANCE_VK_PLATFORM_EXT_FUNCTIONS(function)
#endif
INSTANCE_VK_PLATFORM_EXT_FUNCTIONS(vkCreatePlatformSurfaceKHR)
#undef INSTANCE_VK_PLATFORM_EXT_FUNCTIONS

#ifndef DEVICE_VK_FUNCTIONS
#define DEVICE_VK_FUNCTIONS(function)
#endif
DEVICE_VK_FUNCTIONS(vkDestroyDevice)
DEVICE_VK_FUNCTIONS(vkGetDeviceQueue)
DEVICE_VK_FUNCTIONS(vkQueueSubmit)
DEVICE_VK_FUNCTIONS(vkDeviceWaitIdle)

DEVICE_VK_FUNCTIONS(vkCreateBuffer)
DEVICE_VK_FUNCTIONS(vkGetBufferMemoryRequirements)

/* Synchronizing Functions */
DEVICE_VK_FUNCTIONS(vkCreateSemaphore)
DEVICE_VK_FUNCTIONS(vkDestroySemaphore)
DEVICE_VK_FUNCTIONS(vkCreateFence)
DEVICE_VK_FUNCTIONS(vkGetFenceStatus)
DEVICE_VK_FUNCTIONS(vkResetFences)
DEVICE_VK_FUNCTIONS(vkWaitForFences)
DEVICE_VK_FUNCTIONS(vkDestroyFence)

/* CommandPool Functions */
DEVICE_VK_FUNCTIONS(vkCreateCommandPool)
DEVICE_VK_FUNCTIONS(vkDestroyCommandPool)
DEVICE_VK_FUNCTIONS(vkResetCommandPool)

/* Command buffer Functions */
DEVICE_VK_FUNCTIONS(vkAllocateCommandBuffers)
DEVICE_VK_FUNCTIONS(vkResetCommandBuffer)
DEVICE_VK_FUNCTIONS(vkFreeCommandBuffers)
DEVICE_VK_FUNCTIONS(vkBeginCommandBuffer)
DEVICE_VK_FUNCTIONS(vkEndCommandBuffer);

/* Commands */
DEVICE_VK_FUNCTIONS(vkCmdPipelineBarrier)

/* Debug Functions */
DEVICE_VK_FUNCTIONS(vkQueueBeginDebugUtilsLabelEXT)
DEVICE_VK_FUNCTIONS(vkQueueEndDebugUtilsLabelEXT)
DEVICE_VK_FUNCTIONS(vkQueueInsertDebugUtilsLabelEXT)
DEVICE_VK_FUNCTIONS(vkCmdBeginDebugUtilsLabelEXT)
DEVICE_VK_FUNCTIONS(vkCmdEndDebugUtilsLabelEXT)
DEVICE_VK_FUNCTIONS(vkCmdInsertDebugUtilsLabelEXT)
DEVICE_VK_FUNCTIONS(vkSetDebugUtilsObjectNameEXT)

#undef DEVICE_VK_FUNCTIONS

#ifndef DEVICE_VK_EXT_FUNCTIONS
#define DEVICE_VK_EXT_FUNCTIONS(function,extension)
#endif
/* Swapchain & Present Functions */
DEVICE_VK_EXT_FUNCTIONS(vkCreateSwapchainKHR,VK_KHR_SWAPCHAIN_EXTENSION_NAME)
DEVICE_VK_EXT_FUNCTIONS(vkDestroySwapchainKHR,VK_KHR_SWAPCHAIN_EXTENSION_NAME)
DEVICE_VK_EXT_FUNCTIONS(vkGetSwapchainImagesKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME)
DEVICE_VK_EXT_FUNCTIONS(vkAcquireNextImageKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME)
DEVICE_VK_EXT_FUNCTIONS(vkQueuePresentKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME)

/* Synchronizing Functions */
DEVICE_VK_EXT_FUNCTIONS(vkGetSemaphoreCounterValueKHR, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)
DEVICE_VK_EXT_FUNCTIONS(vkWaitSemaphoresKHR, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)
DEVICE_VK_EXT_FUNCTIONS(vkSignalSemaphoreKHR, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)
#undef DEVICE_VK_EXT_FUNCTIONS