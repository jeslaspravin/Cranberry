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

typedef GraphicsTypes::GraphicsRBuffer GraphicsRBuffer;
typedef GraphicsTypes::GraphicsWBuffer GraphicsWBuffer;
typedef GraphicsTypes::GraphicsRWBuffer GraphicsRWBuffer;
typedef GraphicsTypes::GraphicsRTexelBuffer GraphicsRTexelBuffer;
typedef GraphicsTypes::GraphicsWTexelBuffer GraphicsWTexelBuffer;
typedef GraphicsTypes::GraphicsRWTexelBuffer GraphicsRWTexelBuffer;

typedef GraphicsTypes::GraphicsRenderTargetResource GraphicsRenderTargetResource;
typedef GraphicsTypes::GraphicsCubeImageResource GraphicsCubeImageResource;
typedef GraphicsTypes::GraphicsImageResource GraphicsImageResource;

template <typename Type>
using GraphicsDeviceConstant = GraphicsTypes::GraphicsDeviceConstant<Type>;