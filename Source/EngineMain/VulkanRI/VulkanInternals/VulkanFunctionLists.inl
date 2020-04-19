
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
INSTANCE_VK_FUNCTIONS(vkGetPhysicalDeviceMemoryProperties)
INSTANCE_VK_FUNCTIONS(vkGetPhysicalDeviceFormatProperties)
INSTANCE_VK_FUNCTIONS(vkGetPhysicalDeviceImageFormatProperties)
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
INSTANCE_VK_EXT_FUNCTIONS(vkGetPhysicalDeviceMemoryProperties2KHR, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
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

/* Synchronizing Functions */
DEVICE_VK_FUNCTIONS(vkCreateSemaphore)
DEVICE_VK_FUNCTIONS(vkDestroySemaphore)
DEVICE_VK_FUNCTIONS(vkCreateFence)
DEVICE_VK_FUNCTIONS(vkGetFenceStatus)
DEVICE_VK_FUNCTIONS(vkResetFences)
DEVICE_VK_FUNCTIONS(vkWaitForFences)
DEVICE_VK_FUNCTIONS(vkDestroyFence)
//DEVICE_VK_FUNCTIONS(vkGetSemaphoreCounterValue)
//DEVICE_VK_FUNCTIONS(vkWaitSemaphores)
//DEVICE_VK_FUNCTIONS(vkSignalSemaphore)

/* Memory and Buffers */
DEVICE_VK_FUNCTIONS(vkAllocateMemory)
DEVICE_VK_FUNCTIONS(vkFreeMemory)

DEVICE_VK_FUNCTIONS(vkCreateBuffer)
DEVICE_VK_FUNCTIONS(vkGetBufferMemoryRequirements)
DEVICE_VK_FUNCTIONS(vkBindBufferMemory)
DEVICE_VK_FUNCTIONS(vkDestroyBuffer)
DEVICE_VK_FUNCTIONS(vkCreateBufferView)
DEVICE_VK_FUNCTIONS(vkDestroyBufferView)

DEVICE_VK_FUNCTIONS(vkCreateImage)
DEVICE_VK_FUNCTIONS(vkGetImageMemoryRequirements)
DEVICE_VK_FUNCTIONS(vkBindImageMemory)
DEVICE_VK_FUNCTIONS(vkDestroyImage)
DEVICE_VK_FUNCTIONS(vkCreateImageView)
DEVICE_VK_FUNCTIONS(vkDestroyImageView)

/* Image samplers */
DEVICE_VK_FUNCTIONS(vkCreateSampler)
DEVICE_VK_FUNCTIONS(vkDestroySampler)

/* Pipeline and shader resources */
DEVICE_VK_FUNCTIONS(vkCreateDescriptorSetLayout)
DEVICE_VK_FUNCTIONS(vkDestroyDescriptorSetLayout)
DEVICE_VK_FUNCTIONS(vkCreateDescriptorPool)
DEVICE_VK_FUNCTIONS(vkResetDescriptorPool)
DEVICE_VK_FUNCTIONS(vkDestroyDescriptorPool)
DEVICE_VK_FUNCTIONS(vkAllocateDescriptorSets)
DEVICE_VK_FUNCTIONS(vkFreeDescriptorSets)
DEVICE_VK_FUNCTIONS(vkUpdateDescriptorSets)

DEVICE_VK_FUNCTIONS(vkCreateRenderPass)
DEVICE_VK_FUNCTIONS(vkDestroyRenderPass)
DEVICE_VK_FUNCTIONS(vkCreateFramebuffer)
DEVICE_VK_FUNCTIONS(vkDestroyFramebuffer)

DEVICE_VK_FUNCTIONS(vkCreateShaderModule)
DEVICE_VK_FUNCTIONS(vkDestroyShaderModule)

DEVICE_VK_FUNCTIONS(vkCreatePipelineCache)
DEVICE_VK_FUNCTIONS(vkDestroyPipelineCache)
DEVICE_VK_FUNCTIONS(vkGetPipelineCacheData)
DEVICE_VK_FUNCTIONS(vkMergePipelineCaches)

DEVICE_VK_FUNCTIONS(vkCreatePipelineLayout)
DEVICE_VK_FUNCTIONS(vkDestroyPipelineLayout)

DEVICE_VK_FUNCTIONS(vkCreateGraphicsPipelines)
DEVICE_VK_FUNCTIONS(vkDestroyPipeline)

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
DEVICE_VK_FUNCTIONS(vkCmdClearColorImage)

DEVICE_VK_FUNCTIONS(vkCmdBeginRenderPass)
DEVICE_VK_FUNCTIONS(vkCmdNextSubpass)
DEVICE_VK_FUNCTIONS(vkCmdEndRenderPass)


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

// TODO(Jeslas)(API Update) : Change to non extension functions(under Synchronizing Functions of device functions) once driver providers update to Vulkan 1.2
/* Synchronizing Functions */
DEVICE_VK_EXT_FUNCTIONS(vkGetSemaphoreCounterValueKHR, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)
DEVICE_VK_EXT_FUNCTIONS(vkWaitSemaphoresKHR, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)
DEVICE_VK_EXT_FUNCTIONS(vkSignalSemaphoreKHR, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)

#undef DEVICE_VK_EXT_FUNCTIONS