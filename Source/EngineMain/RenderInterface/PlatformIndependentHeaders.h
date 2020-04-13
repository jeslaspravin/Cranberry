#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////
// Include this file where headers are necessary,Else use PlatformIndependenGraphicsTypes.h//
/////////////////////////////////////////////////////////////////////////////////////////////

#if RENDERAPI_VULKAN
#include "../VulkanRI/VulkanGraphicsInstance.h"
#include "../VulkanRI/VulkanInternals/Resources/VulkanWindowCanvas.h"

#include "../VulkanRI/Resources/VulkanBufferResources.h"
#include "../VulkanRI/Resources/VulkanImageResources.h"

#include "../VulkanRI/Resources/VulkanShaderResources.h"

#include "../VulkanRI/VulkanGraphicsTypes.h"
#elif RENDERAPI_OPENGL
static_assert(false, "Not supported render API");
#else
static_assert(false, "No available render API");
#endif

using GraphicInstance = GraphicsTypes::GraphicInstance;
using WindowCanvas = GraphicsTypes::WindowCanvas;

using GraphicsRBuffer = GraphicsTypes::GraphicsRBuffer;
using GraphicsWBuffer = GraphicsTypes::GraphicsWBuffer;
using GraphicsRWBuffer = GraphicsTypes::GraphicsRWBuffer;
using GraphicsRTexelBuffer = GraphicsTypes::GraphicsRTexelBuffer;
using GraphicsWTexelBuffer = GraphicsTypes::GraphicsWTexelBuffer;
using GraphicsRWTexelBuffer = GraphicsTypes::GraphicsRWTexelBuffer;

using GraphicsRenderTargetResource = GraphicsTypes::GraphicsRenderTargetResource;
using GraphicsCubeImageResource = GraphicsTypes::GraphicsCubeImageResource;
using GraphicsImageResource = GraphicsTypes::GraphicsImageResource;

template <typename Type>
using GraphicsDeviceConstant = GraphicsTypes::GraphicsDeviceConstant<Type>;

using GraphicsShaderResource = GraphicsTypes::GraphicsShaderResource;