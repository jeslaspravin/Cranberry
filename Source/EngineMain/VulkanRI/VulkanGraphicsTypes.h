
#ifndef VULKANGRAPHICSTYPES_H
#define VULKANGRAPHICSTYPES_H

namespace GraphicsTypes {
    typedef class VulkanGraphicsInstance GraphicInstance;
    typedef class VulkanWindowCanvas WindowCanvas;

    typedef class VulkanRBuffer GraphicsRBuffer;
    typedef class VulkanWBuffer GraphicsWBuffer;
    typedef class VulkanRWBuffer GraphicsRWBuffer;
    typedef class VulkanRTexelBuffer GraphicsRTexelBuffer;
    typedef class VulkanWTexelBuffer GraphicsWTexelBuffer;
    typedef class VulkanRWTexelBuffer GraphicsRWTexelBuffer;

    typedef class VulkanRenderTargetResource GraphicsRenderTargetResource;
    typedef class VulkanCubeImageResource GraphicsCubeImageResource;
    typedef class VulkanImageResource GraphicsImageResource;
}

#endif