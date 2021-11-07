#include "VulkanRI/Resources/VulkanBufferResources.h"

//////////////////////////////////////////////////////////////////////////
//// Shader read buffer
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanRBuffer, VK_OBJECT_TYPE_BUFFER)

VulkanRBuffer::VulkanRBuffer(uint32 bufferStride, uint32 bufferCount /*= 1*/)
    : BaseType()
    , count(bufferCount)
    , stride(bufferStride)
{
    bufferUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
}

VulkanRBuffer::VulkanRBuffer()
    : BaseType()
    , count(1)
    , stride(0)
{
    bufferUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
}

uint64 VulkanRBuffer::getResourceSize() const
{
    return count * stride;
}

uint32 VulkanRBuffer::bufferStride() const
{
    return stride;
}

void VulkanRBuffer::setBufferStride(uint32 newStride)
{
    stride = newStride;
}

uint32 VulkanRBuffer::bufferCount() const
{
    return count;
}

void VulkanRBuffer::setBufferCount(uint32 newCount)
{
    count = newCount;
}

//////////////////////////////////////////////////////////////////////////
//// Shader write buffer
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanWBuffer, VK_OBJECT_TYPE_BUFFER)

VulkanWBuffer::VulkanWBuffer() 
    : BaseType()
    , count(1)
    , stride(0)
{
    bufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
}

VulkanWBuffer::VulkanWBuffer(uint32 bufferStride, uint32 bufferCount /*= 1*/)
    : BaseType()
    , count(bufferCount)
    , stride(bufferStride)
{
    bufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
}

uint64 VulkanWBuffer::getResourceSize() const
{
    return count * stride;
}

uint32 VulkanWBuffer::bufferStride() const
{
    return stride;
}

void VulkanWBuffer::setBufferStride(uint32 newStride)
{
    stride = newStride;
}

uint32 VulkanWBuffer::bufferCount() const
{
    return count;
}

void VulkanWBuffer::setBufferCount(uint32 newCount)
{
    count = newCount;
}

//////////////////////////////////////////////////////////////////////////
//// Shader read & write buffer
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanRWBuffer, VK_OBJECT_TYPE_BUFFER)

VulkanRWBuffer::VulkanRWBuffer()
    : BaseType()
    , count(1)
    , stride(0)
{
    bufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
}

VulkanRWBuffer::VulkanRWBuffer(uint32 bufferStride, uint32 bufferCount /*= 1*/)
    : BaseType()
    , count(bufferCount)
    , stride(bufferStride)
{
    bufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
}

uint64 VulkanRWBuffer::getResourceSize() const
{
    return count * stride;
}

uint32 VulkanRWBuffer::bufferStride() const
{
    return stride;
}

void VulkanRWBuffer::setBufferStride(uint32 newStride)
{
    stride = newStride;
}

uint32 VulkanRWBuffer::bufferCount() const
{
    return count;
}

void VulkanRWBuffer::setBufferCount(uint32 newCount)
{
    count = newCount;
}

//////////////////////////////////////////////////////////////////////////
//// Shader read texel buffer
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanRTexelBuffer, VK_OBJECT_TYPE_BUFFER)

VulkanRTexelBuffer::VulkanRTexelBuffer()
    : BaseType()
    , count(0)
{

}

VulkanRTexelBuffer::VulkanRTexelBuffer(EPixelDataFormat::Type texelFormat, uint32 texelCount /*= 1*/)
    : BaseType()
    , count(texelCount)
{
    dataFormat = texelFormat;
    bufferUsage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
}

uint64 VulkanRTexelBuffer::getResourceSize() const
{
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);

    if (formatInfo)
    {
        return formatInfo->pixelDataSize * count;
    }
    return 0;
}

void VulkanRTexelBuffer::setTexelFormat(EPixelDataFormat::Type newFormat)
{
    dataFormat = newFormat;
}

uint32 VulkanRTexelBuffer::bufferStride() const
{
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);
    return formatInfo ? formatInfo->pixelDataSize : 0;
}

uint32 VulkanRTexelBuffer::bufferCount() const
{
    return count;
}

void VulkanRTexelBuffer::setBufferCount(uint32 newCount)
{
    count = newCount;
}

//////////////////////////////////////////////////////////////////////////
//// Shader write texel buffer
//////////////////////////////////////////////////////////////////////////


DEFINE_VK_GRAPHICS_RESOURCE(VulkanWTexelBuffer, VK_OBJECT_TYPE_BUFFER)

VulkanWTexelBuffer::VulkanWTexelBuffer()
    : BaseType()
    , count(0)
{

}

VulkanWTexelBuffer::VulkanWTexelBuffer(EPixelDataFormat::Type texelFormat, uint32 texelCount /*= 1*/)
    : BaseType()
    , count(texelCount)
{
    dataFormat = texelFormat;
    bufferUsage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
}

uint64 VulkanWTexelBuffer::getResourceSize() const
{
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);

    if (formatInfo)
    {
        return formatInfo->pixelDataSize * count;
    }
    return 0;
}

void VulkanWTexelBuffer::setTexelFormat(EPixelDataFormat::Type newFormat)
{
    dataFormat = newFormat;
}

uint32 VulkanWTexelBuffer::bufferStride() const
{
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);
    return formatInfo ? formatInfo->pixelDataSize : 0;
}

uint32 VulkanWTexelBuffer::bufferCount() const
{
    return count;
}

void VulkanWTexelBuffer::setBufferCount(uint32 newCount)
{
    count = newCount;
}

//////////////////////////////////////////////////////////////////////////
//// Shader read & write texel buffer
//////////////////////////////////////////////////////////////////////////


DEFINE_VK_GRAPHICS_RESOURCE(VulkanRWTexelBuffer, VK_OBJECT_TYPE_BUFFER)

VulkanRWTexelBuffer::VulkanRWTexelBuffer()
    : BaseType()
    , count(0)
{

}

VulkanRWTexelBuffer::VulkanRWTexelBuffer(EPixelDataFormat::Type texelFormat, uint32 texelCount /*= 1*/)
    : BaseType()
    , count(texelCount)
{
    dataFormat = texelFormat;
    bufferUsage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
}

uint64 VulkanRWTexelBuffer::getResourceSize() const
{
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);

    if (formatInfo)
    {
        return formatInfo->pixelDataSize * count;
    }
    return 0;
}

void VulkanRWTexelBuffer::setTexelFormat(EPixelDataFormat::Type newFormat)
{
    dataFormat = newFormat;
}

uint32 VulkanRWTexelBuffer::bufferStride() const
{
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);
    return formatInfo ? formatInfo->pixelDataSize : 0;
}

uint32 VulkanRWTexelBuffer::bufferCount() const
{
    return count;
}

void VulkanRWTexelBuffer::setBufferCount(uint32 newCount)
{
    count = newCount;
}

//////////////////////////////////////////////////////////////////////////
//// Vertex buffer
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanVertexBuffer, VK_OBJECT_TYPE_BUFFER);

VulkanVertexBuffer::VulkanVertexBuffer(uint32 bufferStride, uint32 bufferCount /*= 1*/)
    : BaseType()
    , count(bufferCount)
    , stride(bufferStride)
{
    bufferUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
}

VulkanVertexBuffer::VulkanVertexBuffer()
    : BaseType()
    , count(1)
    , stride(0)
{
    bufferUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
}

uint64 VulkanVertexBuffer::getResourceSize() const
{
    return count * stride;
}

uint32 VulkanVertexBuffer::bufferStride() const
{
    return stride;
}

void VulkanVertexBuffer::setBufferStride(uint32 newStride)
{
    stride = newStride;
}

uint32 VulkanVertexBuffer::bufferCount() const
{
    return count;
}

void VulkanVertexBuffer::setBufferCount(uint32 newCount)
{
    count = newCount;
}

//////////////////////////////////////////////////////////////////////////
//// Index buffer
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanIndexBuffer, VK_OBJECT_TYPE_BUFFER);

VulkanIndexBuffer::VulkanIndexBuffer(uint32 bufferStride, uint32 bufferCount /*= 1*/)
    : BaseType()
    , count(bufferCount)
    , stride(bufferStride)
{
    bufferUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
}

VulkanIndexBuffer::VulkanIndexBuffer()
    : BaseType()
    , count(1)
    , stride(0)
{
    bufferUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
}

uint64 VulkanIndexBuffer::getResourceSize() const
{
    return count * stride;
}

uint32 VulkanIndexBuffer::bufferStride() const
{
    return stride;
}

void VulkanIndexBuffer::setBufferStride(uint32 newStride)
{
    stride = newStride;
}

uint32 VulkanIndexBuffer::bufferCount() const
{
    return count;
}

void VulkanIndexBuffer::setBufferCount(uint32 newCount)
{
    count = newCount;
}

//////////////////////////////////////////////////////////////////////////
//// Shader read indirect buffer
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanRIndirectBuffer, VK_OBJECT_TYPE_BUFFER)

VulkanRIndirectBuffer::VulkanRIndirectBuffer(uint32 bufferStride, uint32 bufferCount /*= 1*/)
    : BaseType()
    , count(bufferCount)
    , stride(bufferStride)
{
    bufferUsage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
}

VulkanRIndirectBuffer::VulkanRIndirectBuffer()
    : BaseType()
    , count(1)
    , stride(0)
{
    bufferUsage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
}

uint64 VulkanRIndirectBuffer::getResourceSize() const
{
    return count * stride;
}

uint32 VulkanRIndirectBuffer::bufferStride() const
{
    return stride;
}

void VulkanRIndirectBuffer::setBufferStride(uint32 newStride)
{
    stride = newStride;
}

uint32 VulkanRIndirectBuffer::bufferCount() const
{
    return count;
}

void VulkanRIndirectBuffer::setBufferCount(uint32 newCount)
{
    count = newCount;
}

//////////////////////////////////////////////////////////////////////////
//// Shader write indirect buffer
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanWIndirectBuffer, VK_OBJECT_TYPE_BUFFER)

VulkanWIndirectBuffer::VulkanWIndirectBuffer()
    : BaseType()
    , count(1)
    , stride(0)
{
    bufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
}

VulkanWIndirectBuffer::VulkanWIndirectBuffer(uint32 bufferStride, uint32 bufferCount /*= 1*/)
    : BaseType()
    , count(bufferCount)
    , stride(bufferStride)
{
    bufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
}

uint64 VulkanWIndirectBuffer::getResourceSize() const
{
    return count * stride;
}

uint32 VulkanWIndirectBuffer::bufferStride() const
{
    return stride;
}

void VulkanWIndirectBuffer::setBufferStride(uint32 newStride)
{
    stride = newStride;
}

uint32 VulkanWIndirectBuffer::bufferCount() const
{
    return count;
}

void VulkanWIndirectBuffer::setBufferCount(uint32 newCount)
{
    count = newCount;
}