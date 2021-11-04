#pragma once

#if _WIN32

#include "Windows/WindowsVulkanFunctions.h"

#elif __unix__
static_assert(false, "Platform not supported!");
#elif __linux__
static_assert(false, "Platform not supported!");
#elif __APPLE__
static_assert(false, "Platform not supported!");
#endif

typedef GVulkanPlatform::PFN_vkCreatePlatformSurfaceKHR PFN_vkCreatePlatformSurfaceKHR;