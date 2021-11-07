#include "VulkanRI/VulkanInternals/VulkanFunctions.h"

namespace Vk
{
#define EXPORTED_VK_FUNCTIONS(function) PFN_##function function;
#define GLOBAL_VK_FUNCTIONS(function) PFN_##function function;
#define INSTANCE_VK_FUNCTIONS(function) PFN_##function function;
#define INSTANCE_VK_EXT_FUNCTIONS(function,extension) PFN_##function function;
#define INSTANCE_VK_PLATFORM_EXT_FUNCTIONS(function) PFN_##function function;

#include "VulkanFunctionLists.inl"
}