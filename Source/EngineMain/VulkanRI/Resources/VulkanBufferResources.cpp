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

VulkanRBuffer::VulkanRBuffer() : count(1), stride(0)
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

VulkanWBuffer::VulkanWBuffer() : count(1), stride(0)
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

VulkanRWBuffer::VulkanRWBuffer() : count(1), stride(0)
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

namespace EPixelDataFormat
{
    const EPixelDataFormat::ImageFormatInfo* getFormatInfo(EPixelDataFormat::Type dataFormat);
}

//////////////////////////////////////////////////////////////////////////
//// Shader read texel buffer
//////////////////////////////////////////////////////////////////////////

DEFINE_VK_GRAPHICS_RESOURCE(VulkanRTexelBuffer, VK_OBJECT_TYPE_BUFFER)

VulkanRTexelBuffer::VulkanRTexelBuffer() : count(0)
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

VulkanWTexelBuffer::VulkanWTexelBuffer() : count(0)
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

VulkanRWTexelBuffer::VulkanRWTexelBuffer() : count(0)
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