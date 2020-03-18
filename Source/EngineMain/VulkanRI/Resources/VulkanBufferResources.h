#pragma once

#include "../VulkanInternals/Resources/VulkanMemoryResources.h"

class VulkanRBuffer final : public VulkanBufferResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanRBuffer, , VulkanBufferResource, )

private:
    uint32 count;
    uint32 stride;

    VulkanRBuffer();
public:
    VulkanRBuffer(uint32 bufferStride, uint32 bufferCount = 1);

    /* MemoryResource overrides */
    uint64 getResourceSize() const override;

    /* Overrides ends */
};

class VulkanWBuffer final : public VulkanBufferResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanWBuffer, , VulkanBufferResource, )

private:
    uint32 count;
    uint32 stride;

    VulkanWBuffer();
public:
    VulkanWBuffer(uint32 bufferStride, uint32 bufferCount = 1);

    /* MemoryResource overrides */
    uint64 getResourceSize() const override;

    /* Overrides ends */
};

class VulkanRWBuffer final : public VulkanBufferResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanRWBuffer, , VulkanBufferResource, )

private:
    uint32 count;
    uint32 stride;

    VulkanRWBuffer();
public:
    VulkanRWBuffer(uint32 bufferStride, uint32 bufferCount = 1);

    /* MemoryResource overrides */
    uint64 getResourceSize() const override;

    /* Overrides ends */
};

class VulkanRTexelBuffer final : public VulkanBufferResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanRTexelBuffer, , VulkanBufferResource, )
private:
    uint32 count;

    VulkanRTexelBuffer();

public:
    VulkanRTexelBuffer(EPixelDataFormat::Type texelFormat, uint32 texelCount = 1);

    /* MemoryResource overrides */
    uint64 getResourceSize() const override;

    /* Overrides ends */
};

class VulkanWTexelBuffer final : public VulkanBufferResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanWTexelBuffer, , VulkanBufferResource, )
private:
    uint32 count;

    VulkanWTexelBuffer();

public:
    VulkanWTexelBuffer(EPixelDataFormat::Type texelFormat, uint32 texelCount = 1);

    /* MemoryResource overrides */
    uint64 getResourceSize() const override;

    /* Overrides ends */
};

class VulkanRWTexelBuffer final : public VulkanBufferResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanRWTexelBuffer, , VulkanBufferResource, )
private:
    uint32 count;

    VulkanRWTexelBuffer();

public:
    VulkanRWTexelBuffer(EPixelDataFormat::Type texelFormat, uint32 texelCount = 1);

    /* MemoryResource overrides */
    uint64 getResourceSize() const override;

    /* Overrides ends */
};

namespace GraphicsTypes {

    typedef VulkanRBuffer GraphicsRBuffer;
    typedef VulkanWBuffer GraphicsWBuffer;
    typedef VulkanRWBuffer GraphicsRWBuffer;

    typedef VulkanRTexelBuffer GraphicsRTexelBuffer;
    typedef VulkanWTexelBuffer GraphicsWTexelBuffer;
    typedef VulkanRWTexelBuffer GraphicsRWTexelBuffer;
}