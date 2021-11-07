
#ifndef VULKANGRAPHICSTYPES_H
#define VULKANGRAPHICSTYPES_H

#include "../Core/Engine/Config/EngineVariableTypes.h"

class VulkanGraphicsInstance;
class VulkanWindowCanvas;

class VulkanRBuffer;
class VulkanWBuffer;
class VulkanRWBuffer;
class VulkanRTexelBuffer;
class VulkanWTexelBuffer;
class VulkanRWTexelBuffer;
class VulkanVertexBuffer;
class VulkanIndexBuffer;
class VulkanRIndirectBuffer;
class VulkanWIndirectBuffer;

class VulkanRenderTargetResource;
class VulkanCubeImageResource;
class VulkanCubeRTImageResource;
class VulkanImageResource;

class VulkanShaderResource;
class VulkanGraphicsPipeline;
class VulkanComputePipeline;
class VulkanGlobalRenderingContext;

namespace GraphicsTypes 
{
    typedef VulkanGraphicsInstance GraphicInstance;
    typedef VulkanWindowCanvas WindowCanvas;
    
    typedef VulkanRBuffer GraphicsRBuffer;
    typedef VulkanWBuffer GraphicsWBuffer;
    typedef VulkanRWBuffer GraphicsRWBuffer;
    typedef VulkanRTexelBuffer GraphicsRTexelBuffer;
    typedef VulkanWTexelBuffer GraphicsWTexelBuffer;
    typedef VulkanRWTexelBuffer GraphicsRWTexelBuffer;
    typedef VulkanVertexBuffer GraphicsVertexBuffer;
    typedef VulkanIndexBuffer GraphicsIndexBuffer;
    typedef VulkanRIndirectBuffer GraphicsRIndirectBuffer;
    typedef VulkanWIndirectBuffer GraphicsWIndirectBuffer;

    typedef VulkanRenderTargetResource GraphicsRenderTargetResource;
    typedef VulkanCubeImageResource GraphicsCubeImageResource;
    typedef VulkanCubeRTImageResource GraphicsCubeRTImageResource;
    typedef VulkanImageResource GraphicsImageResource;

    template <typename Type>
    using GraphicsDeviceConstant = EngineConstant<Type, class VulkanDevice>;

    typedef VulkanShaderResource GraphicsShaderResource;

    typedef VulkanGlobalRenderingContext GlobalRenderingContext;
    typedef VulkanGraphicsPipeline GraphicsPipeline;
    typedef VulkanComputePipeline ComputePipeline;
}

#endif