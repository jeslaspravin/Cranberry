#include "../VulkanGraphicsTypes.h"
#include "../../RenderInterface/CoreGraphicsTypes.h"

#include <map>
#include <vulkan_core.h>

#if RENDERAPI_VULKAN

namespace CoreGraphicsTypes
{

#define ENUM_TYPE_INFO_PAIR(EnumTypeValue,RelevantApiFormat) { EnumTypeValue, { RelevantApiFormat, #EnumTypeValue }}
    const std::map<ECompareOp::Type, EnumTypeInfo> COMPARE_OP_TO_API_FILTER = {
        ENUM_TYPE_INFO_PAIR(ECompareOp::Never,VK_COMPARE_OP_NEVER),
        ENUM_TYPE_INFO_PAIR(ECompareOp::Less,VK_COMPARE_OP_LESS),
        ENUM_TYPE_INFO_PAIR(ECompareOp::Equal,VK_COMPARE_OP_EQUAL),
        ENUM_TYPE_INFO_PAIR(ECompareOp::EqualOrLess,VK_COMPARE_OP_LESS_OR_EQUAL),
        ENUM_TYPE_INFO_PAIR(ECompareOp::Greater,VK_COMPARE_OP_GREATER),
        ENUM_TYPE_INFO_PAIR(ECompareOp::NotEqual ,VK_COMPARE_OP_NOT_EQUAL),
        ENUM_TYPE_INFO_PAIR(ECompareOp::EqualOrGreater ,VK_COMPARE_OP_GREATER_OR_EQUAL),
        ENUM_TYPE_INFO_PAIR(ECompareOp::Always ,VK_COMPARE_OP_ALWAYS)
    };

    const EnumTypeInfo* getEnumTypeInfo(ECompareOp::Type compareOp)
    {
        auto itr = COMPARE_OP_TO_API_FILTER.find(compareOp);
        if (itr != COMPARE_OP_TO_API_FILTER.end())
        {
            return &itr->second;
        }
        return nullptr;
    }

#undef  ENUM_TYPE_INFO_PAIR

}

namespace EPixelDataFormat 
{
#define IMG_FORMAT_INFO_PAIR(PixelFormat,RelevantApiFormat,DataSize) { PixelFormat, { RelevantApiFormat, DataSize, #PixelFormat }}
    const std::map<Type, ImageFormatInfo> DATA_FORMAT_TO_API_FORMAT = {
        IMG_FORMAT_INFO_PAIR( Undefined, VK_FORMAT_UNDEFINED, 0),
        IMG_FORMAT_INFO_PAIR( ABGR_UI8_Packed, VK_FORMAT_A8B8G8R8_UINT_PACK32, 4 ),
        IMG_FORMAT_INFO_PAIR( ABGR_SI8_Packed , VK_FORMAT_A8B8G8R8_SINT_PACK32 , 4 ),
        IMG_FORMAT_INFO_PAIR( ABGR_UI8_SrgbPacked , VK_FORMAT_A8B8G8R8_SRGB_PACK32 , 4 ),
        IMG_FORMAT_INFO_PAIR( ABGR_U8_NormPacked , VK_FORMAT_A8B8G8R8_UNORM_PACK32 , 4 ),
        IMG_FORMAT_INFO_PAIR( ABGR_S8_NormPacked , VK_FORMAT_A8B8G8R8_SNORM_PACK32 , 4 ),
        IMG_FORMAT_INFO_PAIR( ABGR_U8_ScaledPacked , VK_FORMAT_A8B8G8R8_USCALED_PACK32 , 4 ),
        IMG_FORMAT_INFO_PAIR( ABGR_S8_ScaledPacked , VK_FORMAT_A8B8G8R8_SSCALED_PACK32 , 4),
        IMG_FORMAT_INFO_PAIR( BGRA_U8_Norm , VK_FORMAT_B8G8R8A8_UNORM , 4),
        IMG_FORMAT_INFO_PAIR( BGRA_S8_Norm , VK_FORMAT_B8G8R8A8_SNORM , 4),
        IMG_FORMAT_INFO_PAIR( BGRA_U8_Scaled , VK_FORMAT_B8G8R8A8_USCALED , 4),
        IMG_FORMAT_INFO_PAIR( BGRA_S8_Scaled , VK_FORMAT_B8G8R8A8_SSCALED , 4),
        IMG_FORMAT_INFO_PAIR( R_UI32 , VK_FORMAT_R32_UINT , 4),
        IMG_FORMAT_INFO_PAIR( R_SI32 , VK_FORMAT_R32_SINT , 4),
        IMG_FORMAT_INFO_PAIR( R_SF32 , VK_FORMAT_R32_SFLOAT , 4),
        IMG_FORMAT_INFO_PAIR( D_U16_Norm , VK_FORMAT_D16_UNORM , 2),
        IMG_FORMAT_INFO_PAIR( D24X8_U32_NormPacked, VK_FORMAT_X8_D24_UNORM_PACK32, 4),
        IMG_FORMAT_INFO_PAIR( D_SF32, VK_FORMAT_D32_SFLOAT, 4),
        IMG_FORMAT_INFO_PAIR( D32S8_SF32_UI8, VK_FORMAT_D32_SFLOAT_S8_UINT, 5),
        IMG_FORMAT_INFO_PAIR( D16S8_U24_DNorm_SInt, VK_FORMAT_D16_UNORM_S8_UINT, 3),
        IMG_FORMAT_INFO_PAIR( D24S8_U32_DNorm_SInt, VK_FORMAT_D24_UNORM_S8_UINT, 4)
    };
#undef IMG_FORMAT_INFO_PAIR

    const ImageFormatInfo* getFormatInfo(EPixelDataFormat::Type dataFormat)
    {
        auto itr = DATA_FORMAT_TO_API_FORMAT.find(dataFormat);
        if (itr != DATA_FORMAT_TO_API_FORMAT.end())
        {
            return &itr->second;
        }
        return nullptr;
    }
    EPixelDataFormat::Type fromApiFormat(uint32 apiFormat)
    {
        for (const std::pair<Type, ImageFormatInfo>& formatPair : DATA_FORMAT_TO_API_FORMAT)
        {
            if (formatPair.second.format == apiFormat)
            {
                return formatPair.first;
            }
        }
        return Undefined;
    }
}

namespace ESamplerFiltering
{
    struct FilterInfoData
    {
        SamplerFilteringInfo filteringInfo;
        SamplerFilteringInfo mipFilteringInfo;
    };

#define SAMPLE_FILTER_INFO_PAIR(FilteringType,RelevantApiFormat, RelevantMipApiFormat) { FilteringType, \
    {{ RelevantApiFormat, STRINGIFY(##FilteringType##Filter) } , { RelevantMipApiFormat , STRINGIFY(Mip##FilteringType##Filter)}}}
    const std::map<Type, FilterInfoData> FILTER_TYPE_TO_API_FILTER = {
        SAMPLE_FILTER_INFO_PAIR(Nearest, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST),
        SAMPLE_FILTER_INFO_PAIR(Linear, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR),
        SAMPLE_FILTER_INFO_PAIR(Cubic, VK_FILTER_CUBIC_IMG, VK_SAMPLER_MIPMAP_MODE_NEAREST)
    };
#undef SAMPLE_FILTER_INFO_PAIR

    const SamplerFilteringInfo* getFilterInfo(ESamplerFiltering::Type dataFormat)
    {
        auto itr = FILTER_TYPE_TO_API_FILTER.find(dataFormat);
        if (itr != FILTER_TYPE_TO_API_FILTER.end())
        {
            return &itr->second.filteringInfo;
        }
        return nullptr;
    }

    const SamplerFilteringInfo* getMipFilterInfo(ESamplerFiltering::Type dataFormat)
    {
        auto itr = FILTER_TYPE_TO_API_FILTER.find(dataFormat);
        if (itr != FILTER_TYPE_TO_API_FILTER.end())
        {
            return &itr->second.mipFilteringInfo;
        }
        return nullptr;
    }
}

namespace ESamplerTilingMode
{
    uint32 getSamplerTiling(ESamplerTilingMode::Type tilingMode)
    {
        return uint32(tilingMode);
    }
}

namespace EImageComponentMapping
{
#define COMP_MAP_INFO_PAIR(ComponentMappingValue,RelevantApiFormat) { ComponentMappingValue, { RelevantApiFormat, #ComponentMappingValue }}
    const std::map<Type, ComponentMappingInfo> COMP_MAPPING_TO_API_COMP_SWIZZLE = {
        COMP_MAP_INFO_PAIR(EImageComponentMapping::SameComponent, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY),
        COMP_MAP_INFO_PAIR(EImageComponentMapping::AlwaysOne, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_ONE),
        COMP_MAP_INFO_PAIR(EImageComponentMapping::AlwaysZero, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_ZERO),
        COMP_MAP_INFO_PAIR(EImageComponentMapping::R, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_R),
        COMP_MAP_INFO_PAIR(EImageComponentMapping::G, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_G),
        COMP_MAP_INFO_PAIR(EImageComponentMapping::B, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_B),
        COMP_MAP_INFO_PAIR(EImageComponentMapping::A, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_A)
    };
#undef COMP_MAP_INFO_PAIR
    const ComponentMappingInfo* getComponentMapping(EImageComponentMapping::Type mapping)
    {
        return &COMP_MAPPING_TO_API_COMP_SWIZZLE.find(mapping)->second;
    }
}

#endif