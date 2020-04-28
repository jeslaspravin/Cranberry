
#ifndef VULKANGRAPHICSTYPES_H
#define VULKANGRAPHICSTYPES_H

#include "../Core/Engine/Config/EngineVariableTypes.h"

namespace GraphicsTypes {
    typedef class VulkanGraphicsInstance GraphicInstance;
    typedef class VulkanWindowCanvas WindowCanvas;
    
    typedef class VulkanRBuffer GraphicsRBuffer;
    typedef class VulkanWBuffer GraphicsWBuffer;
    typedef class VulkanRWBuffer GraphicsRWBuffer;
    typedef class VulkanRTexelBuffer GraphicsRTexelBuffer;
    typedef class VulkanWTexelBuffer GraphicsWTexelBuffer;
    typedef class VulkanRWTexelBuffer GraphicsRWTexelBuffer;
    typedef class VulkanVertexBuffer GraphicsVertexBuffer;
    typedef class VulkanIndexBuffer GraphicsIndexBuffer;

    typedef class VulkanRenderTargetResource GraphicsRenderTargetResource;
    typedef class VulkanCubeImageResource GraphicsCubeImageResource;
    typedef class VulkanImageResource GraphicsImageResource;

    template <typename Type>
    using GraphicsDeviceConstant = EngineConstant<Type, class VulkanDevice>;

    typedef class VulkanShaderResource GraphicsShaderResource;
}

#endif