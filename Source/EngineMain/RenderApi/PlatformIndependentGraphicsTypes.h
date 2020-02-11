#pragma once

#if RENDERAPI_VULKAN
#include "../VulkanRI/VulkanGraphicsTypes.h"
#elif RENDERAPI_OPENGL
static_assert(false, "Not supported render API");
#else
static_assert(false, "No available render API");
#endif

typedef GraphicsTypes::GraphicInstance GraphicInstance;
typedef GraphicsTypes::WindowCanvas WindowCanvas;