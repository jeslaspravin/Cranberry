/*!
 * \file VulkanFunctions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "vulkan.h"
#include "VulkanInternals/Platform/PlatformVulkanFunctions.h"

namespace Vk
{
#define EXPORTED_VK_FUNCTIONS(function) extern PFN_##function function;
#define GLOBAL_VK_FUNCTIONS(function) extern PFN_##function function;
#define INSTANCE_VK_FUNCTIONS(function) extern PFN_##function function;
#define INSTANCE_VK_EXT_FUNCTIONS(function,extension) extern PFN_##function function;
#define INSTANCE_VK_PLATFORM_EXT_FUNCTIONS(function) extern PFN_##function function;

#include "VulkanFunctionLists.inl"
}