#pragma once

#if PLATFORM_WINDOWS

#include "WindowsVulkanFunctions.h"

#elif PLATFORM_LINUX
static_assert(false, "Platform not supported!");
#elif PLATFORM_APPLE
static_assert(false, "Platform not supported!");
#endif

typedef GVulkanPlatform::PFN_vkCreatePlatformSurfaceKHR PFN_vkCreatePlatformSurfaceKHR;