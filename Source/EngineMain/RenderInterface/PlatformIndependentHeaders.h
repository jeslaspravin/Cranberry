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
#include "../VulkanRI/VulkanInternals/Resources/VulkanPipelines.h"
#include "../VulkanRI/VulkanInternals/Rendering/VulkanRenderingContexts.h"

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
using GraphicsVertexBuffer = GraphicsTypes::GraphicsVertexBuffer;
using GraphicsIndexBuffer = GraphicsTypes::GraphicsIndexBuffer;
using GraphicsRIndirectBuffer = GraphicsTypes::GraphicsRIndirectBuffer;
using GraphicsWIndirectBuffer = GraphicsTypes::GraphicsWIndirectBuffer;

using GraphicsRenderTargetResource = GraphicsTypes::GraphicsRenderTargetResource;
using GraphicsCubeImageResource = GraphicsTypes::GraphicsCubeImageResource;
using GraphicsCubeRTImageResource = GraphicsTypes::GraphicsCubeRTImageResource;
using GraphicsImageResource = GraphicsTypes::GraphicsImageResource;

template <typename Type>
using GraphicsDeviceConstant = GraphicsTypes::GraphicsDeviceConstant<Type>;

using GraphicsShaderResource = GraphicsTypes::GraphicsShaderResource;
using GraphicsPipeline = GraphicsTypes::GraphicsPipeline;
using ComputePipeline = GraphicsTypes::ComputePipeline;
using GlobalRenderingContext = GraphicsTypes::GlobalRenderingContext;