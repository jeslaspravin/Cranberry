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
    const EPixelDataFormat::ImageFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);

    if (formatInfo)
    {
        return formatInfo->pixelDataSize * count;
    }
    return 0;
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
    const EPixelDataFormat::ImageFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);

    if (formatInfo)
    {
        return formatInfo->pixelDataSize * count;
    }
    return 0;
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
    const EPixelDataFormat::ImageFormatInfo* formatInfo = EPixelDataFormat::getFormatInfo(dataFormat);

    if (formatInfo)
    {
        return formatInfo->pixelDataSize * count;
    }
    return 0;
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