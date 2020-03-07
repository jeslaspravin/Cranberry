#include "../VulkanGraphicsTypes.h"
#include "../../RenderInterface/Resources/MemoryResources.h"

#include <map>
#include <vulkan_core.h>

#if RENDERAPI_VULKAN
namespace EPixelDataFormat 
{
#define IMG_FORMAT_INFO_PAIR(PixelFormat,RelevantApiFormat,DataSize) { PixelFormat, { RelevantApiFormat, DataSize, #PixelFormat }}
    const std::map<Type, ImageFormatInfo> dataFormatToApiFormat = {
        IMG_FORMAT_INFO_PAIR( Undefined, VK_FORMAT_UNDEFINED, 0),
        IMG_FORMAT_INFO_PAIR( RGBA_U8_Packed, VK_FORMAT_A8B8G8R8_UINT_PACK32, 4 ),
        IMG_FORMAT_INFO_PAIR( RGBA_S8_Packed , VK_FORMAT_A8B8G8R8_SINT_PACK32 , 4 ),
        IMG_FORMAT_INFO_PAIR( RGBA_U8_SrgbPacked , VK_FORMAT_A8B8G8R8_SRGB_PACK32 , 4 ),
        IMG_FORMAT_INFO_PAIR( RGBA_U8_NormPacked , VK_FORMAT_A8B8G8R8_UNORM_PACK32 , 4 ),
        IMG_FORMAT_INFO_PAIR( RGBA_S8_NormPacked , VK_FORMAT_A8B8G8R8_SNORM_PACK32 , 4 ),
        IMG_FORMAT_INFO_PAIR( RGBA_U8_ScaledPacked , VK_FORMAT_A8B8G8R8_USCALED_PACK32 , 4 ),
        IMG_FORMAT_INFO_PAIR( RGBA_S8_ScaledPacked , VK_FORMAT_A8B8G8R8_SSCALED_PACK32 , 4 )
    };
#undef IMG_FORMAT_INFO_PAIR

    const EPixelDataFormat::ImageFormatInfo* getFormatInfo(EPixelDataFormat::Type dataFormat)
    {
        auto itr = dataFormatToApiFormat.find(dataFormat);
        if (itr != dataFormatToApiFormat.end())
        {
            return &itr->second;
        }
        return nullptr;
    }
}

#endif