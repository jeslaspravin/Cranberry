#pragma once

#include "vulkan.h"
#include "Platform/PlatformVulkanFunctions.h"

namespace Vk
{
#define EXPORTED_VK_FUNCTIONS(function) extern PFN_##function function;
#define GLOBAL_VK_FUNCTIONS(function) extern PFN_##function function;
#define INSTANCE_VK_FUNCTIONS(function) extern PFN_##function function;
#define INSTANCE_VK_EXT_FUNCTIONS(function,extension) extern PFN_##function function;
#define INSTANCE_VK_PLATFORM_EXT_FUNCTIONS(function,extension) extern PFN_##function function;
#define DEVICE_VK_FUNCTIONS(function) extern PFN_##function function;

#include "VulkanFunctionLists.inl"
}