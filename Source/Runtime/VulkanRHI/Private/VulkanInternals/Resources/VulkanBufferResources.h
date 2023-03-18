/*!
 * \file VulkanBufferResources.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "VulkanInternals/Resources/VulkanMemoryResources.h"

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
    uint32 bufferStride() const override;
    void setBufferStride(uint32 newStride) override;
    uint32 bufferCount() const override;
    void setBufferCount(uint32 newCount) override;

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
    uint32 bufferStride() const override;
    void setBufferStride(uint32 newStride) override;
    uint32 bufferCount() const override;
    void setBufferCount(uint32 newCount) override;

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
    uint32 bufferStride() const override;
    void setBufferStride(uint32 newStride) override;
    uint32 bufferCount() const override;
    void setBufferCount(uint32 newCount) override;

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
    void setTexelFormat(EPixelDataFormat::Type newFormat) override;
    uint32 bufferStride() const override;
    uint32 bufferCount() const override;
    void setBufferCount(uint32 newCount) override;

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
    void setTexelFormat(EPixelDataFormat::Type newFormat) override;
    uint32 bufferStride() const override;
    uint32 bufferCount() const override;
    void setBufferCount(uint32 newCount) override;

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
    void setTexelFormat(EPixelDataFormat::Type newFormat) override;
    uint32 bufferStride() const override;
    uint32 bufferCount() const override;
    void setBufferCount(uint32 newCount) override;

    /* Overrides ends */
};

class VulkanVertexBuffer final : public VulkanBufferResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanVertexBuffer, , VulkanBufferResource, )

private:
    uint32 count;
    uint32 stride;

    VulkanVertexBuffer();

public:
    VulkanVertexBuffer(uint32 bufferStride, uint32 bufferCount = 1);

    /* MemoryResource overrides */
    uint64 getResourceSize() const override;
    uint32 bufferStride() const override;
    void setBufferStride(uint32 newStride) override;
    uint32 bufferCount() const override;
    void setBufferCount(uint32 newCount) override;

    /* Overrides ends */
};

class VulkanIndexBuffer final : public VulkanBufferResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanIndexBuffer, , VulkanBufferResource, )

private:
    uint32 count;
    uint32 stride;

    VulkanIndexBuffer();

public:
    VulkanIndexBuffer(uint32 bufferStride, uint32 bufferCount = 1);

    /* MemoryResource overrides */
    uint64 getResourceSize() const override;
    uint32 bufferStride() const override;
    void setBufferStride(uint32 newStride) override;
    uint32 bufferCount() const override;
    void setBufferCount(uint32 newCount) override;

    /* Overrides ends */
};

class VulkanRIndirectBuffer final : public VulkanBufferResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanRIndirectBuffer, , VulkanBufferResource, )

private:
    uint32 count;
    uint32 stride;

    VulkanRIndirectBuffer();

public:
    VulkanRIndirectBuffer(uint32 bufferStride, uint32 bufferCount = 1);

    /* MemoryResource overrides */
    uint64 getResourceSize() const override;
    uint32 bufferStride() const override;
    void setBufferStride(uint32 newStride) override;
    uint32 bufferCount() const override;
    void setBufferCount(uint32 newCount) override;

    /* Overrides ends */
};

class VulkanWIndirectBuffer final : public VulkanBufferResource
{
    DECLARE_VK_GRAPHICS_RESOURCE(VulkanWIndirectBuffer, , VulkanBufferResource, )

private:
    uint32 count;
    uint32 stride;

    VulkanWIndirectBuffer();

public:
    VulkanWIndirectBuffer(uint32 bufferStride, uint32 bufferCount = 1);

    /* MemoryResource overrides */
    uint64 getResourceSize() const override;
    uint32 bufferStride() const override;
    void setBufferStride(uint32 newStride) override;
    uint32 bufferCount() const override;
    void setBufferCount(uint32 newCount) override;

    /* Overrides ends */
};

namespace GraphicsTypes
{

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
} // namespace GraphicsTypes