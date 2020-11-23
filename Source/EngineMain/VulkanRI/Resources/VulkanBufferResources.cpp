#include "VulkanBufferResources.h"

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

int32 VulkanRBuffer::bufferStride() const 
{
    return stride;
}

void VulkanRBuffer::setBufferStride(int32 newStride)
{
    stride = newStride;
}

int32 VulkanRBuffer::bufferCount() const
{
    return count;
}

void VulkanRBuffer::setBufferCount(int32 newCount)
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

int32 VulkanWBuffer::bufferStride() const
{
    return stride;
}

void VulkanWBuffer::setBufferStride(int32 newStride)
{
    stride = newStride;
}

int32 VulkanWBuffer::bufferCount() const
{
    return count;
}

void VulkanWBuffer::setBufferCount(int32 newCount)
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

int32 VulkanRWBuffer::bufferStride() const
{
    return stride;
}

void VulkanRWBuffer::setBufferStride(int32 newStride)
{
    stride = newStride;
}

int32 VulkanRWBuffer::bufferCount() const
{
    return count;
}

void VulkanRWBuffer::setBufferCount(int32 newCount)
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

int32 VulkanRTexelBuffer::bufferStride() const
{
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);
    return formatInfo ? formatInfo->pixelDataSize : 0;
}

int32 VulkanRTexelBuffer::bufferCount() const
{
    return count;
}

void VulkanRTexelBuffer::setBufferCount(int32 newCount)
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

int32 VulkanWTexelBuffer::bufferStride() const
{
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);
    return formatInfo ? formatInfo->pixelDataSize : 0;
}

int32 VulkanWTexelBuffer::bufferCount() const
{
    return count;
}

void VulkanWTexelBuffer::setBufferCount(int32 newCount)
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

int32 VulkanRWTexelBuffer::bufferStride() const
{
    const EPixelDataFormat::PixelFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);
    return formatInfo ? formatInfo->pixelDataSize : 0;
}

int32 VulkanRWTexelBuffer::bufferCount() const
{
    return count;
}

void VulkanRWTexelBuffer::setBufferCount(int32 newCount)
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

int32 VulkanVertexBuffer::bufferStride() const
{
    return stride;
}

void VulkanVertexBuffer::setBufferStride(int32 newStride)
{
    stride = newStride;
}

int32 VulkanVertexBuffer::bufferCount() const
{
    return count;
}

void VulkanVertexBuffer::setBufferCount(int32 newCount)
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

int32 VulkanIndexBuffer::bufferStride() const
{
    return stride;
}

void VulkanIndexBuffer::setBufferStride(int32 newStride)
{
    stride = newStride;
}

int32 VulkanIndexBuffer::bufferCount() const
{
    return count;
}

void VulkanIndexBuffer::setBufferCount(int32 newCount)
{
    count = newCount;
}