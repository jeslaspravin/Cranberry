#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////
// Include this file where headers are necessary,Else use PlatformIndependenGraphicsTypes.h//
/////////////////////////////////////////////////////////////////////////////////////////////

#if RENDERAPI_VULKAN
#include "../VulkanRI/VulkanGraphicsInstance.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanWindowCanvas.h"
#include "../VulkanRI/VulkanGraphicsTypes.h"
#elif RENDERAPI_OPENGL
static_assert(false, "Not supported render API");
#else
static_assert(false, "No available render API");
#endif

typedef GraphicsTypes::GraphicInstance GraphicInstance;
typedef GraphicsTypes::WindowCanvas WindowCanvas;
typedef GraphicsTypes::GraphicInstance GraphicInstance;