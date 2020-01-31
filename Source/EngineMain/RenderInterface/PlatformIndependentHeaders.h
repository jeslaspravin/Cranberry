#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////
// Include this file where headers are necessary,Else use PlatformIndependenGraphicsTypes.h//
/////////////////////////////////////////////////////////////////////////////////////////////

#if RENDERAPI_VULKAN
#include "../VulkanRI/VulkanGraphicsTypes.h"
#include "../VulkanRI/VulkanGraphicsInstance.h"
#elif RENDERAPI_OPENGL
static_assert(false, "Not supported render API");
#else
static_assert(false, "No available render API");
#endif

using namespace GrpahicsTypes;