/*!
 * \file VulkanGraphicsTypes.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include <map>

#include "RenderInterface/CoreGraphicsTypes.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/Resources/ShaderResources.h"
#include "Types/CoreDefines.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "VulkanInternals/VulkanGraphicsTypes.h"

namespace EngineToVulkanAPI
{
#define COMPARE_OP_INFO_PAIR(EnumTypeValue, RelevantApiFormat) RelevantApiFormat
VkCompareOp vulkanCompareOp(CoreGraphicsTypes::ECompareOp::Type compareOp)
{
    static const VkCompareOp ENGINE_TO_VK_COMPARE_OP[] = { COMPARE_OP_INFO_PAIR(ECompareOp::Never, VK_COMPARE_OP_NEVER),
                                                           COMPARE_OP_INFO_PAIR(ECompareOp::Less, VK_COMPARE_OP_LESS),
                                                           COMPARE_OP_INFO_PAIR(ECompareOp::Equal, VK_COMPARE_OP_EQUAL),
                                                           COMPARE_OP_INFO_PAIR(ECompareOp::EqualOrLess, VK_COMPARE_OP_LESS_OR_EQUAL),
                                                           COMPARE_OP_INFO_PAIR(ECompareOp::Greater, VK_COMPARE_OP_GREATER),
                                                           COMPARE_OP_INFO_PAIR(ECompareOp::NotEqual, VK_COMPARE_OP_NOT_EQUAL),
                                                           COMPARE_OP_INFO_PAIR(ECompareOp::EqualOrGreater, VK_COMPARE_OP_GREATER_OR_EQUAL),
                                                           COMPARE_OP_INFO_PAIR(ECompareOp::Always, VK_COMPARE_OP_ALWAYS) };
    static_assert(
        ARRAY_LENGTH(ENGINE_TO_VK_COMPARE_OP) == CoreGraphicsTypes::ECompareOp::MaxCount,
        "Mismatch CoreGraphicsTypes::ECompareOp between vulkan and interface"
    );
    return ENGINE_TO_VK_COMPARE_OP[uint32(compareOp)];
}

#undef COMPARE_OP_INFO_PAIR

// Except packed format everything else is in byte order 0...N Byte, while packed formats are in order of
// bit N...0 Bit
#define PREPEND_COMP(X)
#define MAKE_COMPS(...)
#define IMG_FORMAT_INFO_PAIR(PixelFormat, RelevantApiFormat, DataSize, ComponentSize, ...)                                                     \
    {                                                                                                                                          \
        EPixelDataFormat::##PixelFormat, RelevantApiFormat                                                                                     \
    }
// clang-format off
const std::map<EPixelDataFormat::Type, VkFormat> PIXEL_DATA_FORMAT_TO_API_FORMAT = {
    IMG_FORMAT_INFO_PAIR(Undefined, VK_FORMAT_UNDEFINED, 0, MAKE_INITIALIZER(0, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(ABGR8_UI32_Packed, VK_FORMAT_A8B8G8R8_UINT_PACK32, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(A2RGB10_UI32_Packed, VK_FORMAT_A2R10G10B10_UINT_PACK32, 4, MAKE_INITIALIZER(10, 10, 10, 2), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(A2BGR10_UI32_Packed, VK_FORMAT_A2B10G10R10_UINT_PACK32, 4, MAKE_INITIALIZER(10, 10, 10, 2)),
    IMG_FORMAT_INFO_PAIR(R_UI8, VK_FORMAT_R8_UINT, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_UI8, VK_FORMAT_R8G8_UINT, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_UI8, VK_FORMAT_R8G8B8_UINT, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_UI8, VK_FORMAT_R8G8B8A8_UINT, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(R_UI16, VK_FORMAT_R16_UINT, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_UI16, VK_FORMAT_R16G16_UINT, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_UI16, VK_FORMAT_R16G16B16_UINT, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_UI16, VK_FORMAT_R16G16B16A16_UINT, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
    IMG_FORMAT_INFO_PAIR(R_UI32, VK_FORMAT_R32_UINT, 4, MAKE_INITIALIZER(32, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_UI32, VK_FORMAT_R32G32_UINT, 8, MAKE_INITIALIZER(32, 32, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_UI32, VK_FORMAT_R32G32B32_UINT, 12, MAKE_INITIALIZER(32, 32, 32, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_UI32, VK_FORMAT_R32G32B32A32_UINT, 16, MAKE_INITIALIZER(32, 32, 32, 32)),
    IMG_FORMAT_INFO_PAIR(R_UI64, VK_FORMAT_R64_UINT, 8, MAKE_INITIALIZER(64, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_UI64, VK_FORMAT_R64G64_UINT, 16, MAKE_INITIALIZER(64, 64, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_UI64, VK_FORMAT_R64G64B64_UINT, 24, MAKE_INITIALIZER(64, 64, 64, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_UI64, VK_FORMAT_R64G64B64A64_UINT, 32, MAKE_INITIALIZER(64, 64, 64, 64)),
    IMG_FORMAT_INFO_PAIR(BGR_UI8, VK_FORMAT_B8G8R8_UINT, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(BGRA_UI8, VK_FORMAT_B8G8R8A8_UINT, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(ABGR8_SI32_Packed, VK_FORMAT_A8B8G8R8_SINT_PACK32, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(A2RGB10_SI32_Packed, VK_FORMAT_A2R10G10B10_SINT_PACK32, 4, MAKE_INITIALIZER(10, 10, 10, 2), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(A2BGR10_SI32_Packed, VK_FORMAT_A2B10G10R10_SINT_PACK32, 4, MAKE_INITIALIZER(10, 10, 10, 2)),
    IMG_FORMAT_INFO_PAIR(R_SI8, VK_FORMAT_R8_SINT, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_SI8, VK_FORMAT_R8G8_SINT, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_SI8, VK_FORMAT_R8G8B8_SINT, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_SI8, VK_FORMAT_R8G8B8A8_SINT, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(R_SI16, VK_FORMAT_R16_SINT, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_SI16, VK_FORMAT_R16G16_SINT, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_SI16, VK_FORMAT_R16G16B16_SINT, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_SI16, VK_FORMAT_R16G16B16A16_SINT, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
    IMG_FORMAT_INFO_PAIR(R_SI32, VK_FORMAT_R32_SINT, 4, MAKE_INITIALIZER(32, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_SI32, VK_FORMAT_R32G32_SINT, 8, MAKE_INITIALIZER(32, 32, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_SI32, VK_FORMAT_R32G32B32_SINT, 12, MAKE_INITIALIZER(32, 32, 32, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_SI32, VK_FORMAT_R32G32B32A32_SINT, 16, MAKE_INITIALIZER(32, 32, 32, 32)),
    IMG_FORMAT_INFO_PAIR(R_SI64, VK_FORMAT_R64_SINT, 8, MAKE_INITIALIZER(64, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_SI64, VK_FORMAT_R64G64_SINT, 16, MAKE_INITIALIZER(64, 64, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_SI64, VK_FORMAT_R64G64B64_SINT, 24, MAKE_INITIALIZER(64, 64, 64, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_SI64, VK_FORMAT_R64G64B64A64_SINT, 32, MAKE_INITIALIZER(64, 64, 64, 64)),
    IMG_FORMAT_INFO_PAIR(BGR_SI8, VK_FORMAT_B8G8R8_SINT, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(BGRA_SI8, VK_FORMAT_B8G8R8A8_SINT, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(ABGR8_U32_NormPacked, VK_FORMAT_A8B8G8R8_UNORM_PACK32, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(A2RGB10_U32_NormPacked, VK_FORMAT_A2R10G10B10_UNORM_PACK32, 4, MAKE_INITIALIZER(10, 10, 10, 2), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(A2BGR10_U32_NormPacked, VK_FORMAT_A2B10G10R10_UNORM_PACK32, 4, MAKE_INITIALIZER(10, 10, 10, 2)),
    IMG_FORMAT_INFO_PAIR(R_U8_Norm, VK_FORMAT_R8_UNORM, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_U8_Norm, VK_FORMAT_R8G8_UNORM, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_U8_Norm, VK_FORMAT_R8G8B8_UNORM, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_U8_Norm, VK_FORMAT_R8G8B8A8_UNORM, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(R_U16_Norm, VK_FORMAT_R16_UNORM, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_U16_Norm, VK_FORMAT_R16G16_UNORM, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_U16_Norm, VK_FORMAT_R16G16B16_UNORM, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_U16_Norm, VK_FORMAT_R16G16B16A16_UNORM, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
    IMG_FORMAT_INFO_PAIR(BGR_U8_Norm, VK_FORMAT_B8G8R8_UNORM, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(BGRA_U8_Norm, VK_FORMAT_B8G8R8A8_UNORM, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(BGR_U8_SRGB, VK_FORMAT_B8G8R8_SRGB, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(BGRA_U8_SRGB, VK_FORMAT_B8G8R8A8_SRGB, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(ABGR8_U32_SrgbPacked, VK_FORMAT_A8B8G8R8_SRGB_PACK32, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(R_U8_SRGB, VK_FORMAT_R8_SRGB, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_U8_SRGB, VK_FORMAT_R8G8_SRGB, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_U8_SRGB, VK_FORMAT_R8G8B8_SRGB, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_U8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(ABGR8_S32_NormPacked, VK_FORMAT_A8B8G8R8_SNORM_PACK32, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(A2RGB10_S32_NormPacked, VK_FORMAT_A2R10G10B10_SNORM_PACK32, 4, MAKE_INITIALIZER(10, 10, 10, 2), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(A2BGR10_S32_NormPacked, VK_FORMAT_A2B10G10R10_SNORM_PACK32, 4, MAKE_INITIALIZER(10, 10, 10, 2)),
    IMG_FORMAT_INFO_PAIR(R_S8_Norm, VK_FORMAT_R8_SNORM, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_S8_Norm, VK_FORMAT_R8G8_SNORM, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_S8_Norm, VK_FORMAT_R8G8B8_SNORM, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_S8_Norm, VK_FORMAT_R8G8B8A8_SNORM, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(R_S16_Norm, VK_FORMAT_R16_SNORM, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_S16_Norm, VK_FORMAT_R16G16_SNORM, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_S16_Norm, VK_FORMAT_R16G16B16_SNORM, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_S16_Norm, VK_FORMAT_R16G16B16A16_SNORM, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
    IMG_FORMAT_INFO_PAIR(BGR_S8_Norm, VK_FORMAT_B8G8R8_SNORM, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(BGRA_S8_Norm, VK_FORMAT_B8G8R8A8_SNORM, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(ABGR8_U32_ScaledPacked, VK_FORMAT_A8B8G8R8_USCALED_PACK32, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(A2RGB10_U32_ScaledPacked, VK_FORMAT_A2R10G10B10_USCALED_PACK32, 4, MAKE_INITIALIZER(10, 10, 10, 2), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(A2BGR10_U32_ScaledPacked, VK_FORMAT_A2B10G10R10_USCALED_PACK32, 4, MAKE_INITIALIZER(10, 10, 10, 2)),
    IMG_FORMAT_INFO_PAIR(R_U8_Scaled, VK_FORMAT_R8_USCALED, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_U8_Scaled, VK_FORMAT_R8G8_USCALED, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_U8_Scaled, VK_FORMAT_R8G8B8_USCALED, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_U8_Scaled, VK_FORMAT_R8G8B8A8_USCALED, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(R_U16_Scaled, VK_FORMAT_R16_USCALED, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_U16_Scaled, VK_FORMAT_R16G16_USCALED, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_U16_Scaled, VK_FORMAT_R16G16B16_USCALED, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_U16_Scaled, VK_FORMAT_R16G16B16A16_USCALED, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
    IMG_FORMAT_INFO_PAIR(BGR_U8_Scaled, VK_FORMAT_B8G8R8_USCALED, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(BGRA_U8_Scaled, VK_FORMAT_B8G8R8A8_USCALED, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(ABGR8_S32_ScaledPacked, VK_FORMAT_A8B8G8R8_SSCALED_PACK32, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(A2RGB10_S32_ScaledPacked, VK_FORMAT_A2R10G10B10_SSCALED_PACK32, 4, MAKE_INITIALIZER(10, 10, 10, 2), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(A2BGR10_S32_ScaledPacked, VK_FORMAT_A2B10G10R10_SSCALED_PACK32, 4, MAKE_INITIALIZER(10, 10, 10, 2)),
    IMG_FORMAT_INFO_PAIR(R_S8_Scaled, VK_FORMAT_R8_SSCALED, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_S8_Scaled, VK_FORMAT_R8G8_SSCALED, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_S8_Scaled, VK_FORMAT_R8G8B8_SSCALED, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_S8_Scaled, VK_FORMAT_R8G8B8A8_SSCALED, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
    IMG_FORMAT_INFO_PAIR(R_S16_Scaled, VK_FORMAT_R16_SSCALED, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_S16_Scaled, VK_FORMAT_R16G16_SSCALED, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_S16_Scaled, VK_FORMAT_R16G16B16_SSCALED, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_S16_Scaled, VK_FORMAT_R16G16B16A16_SSCALED, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
    IMG_FORMAT_INFO_PAIR(BGR_S8_Scaled, VK_FORMAT_B8G8R8_SSCALED, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(BGRA_S8_Scaled, VK_FORMAT_B8G8R8A8_SSCALED, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
    IMG_FORMAT_INFO_PAIR(R_SF16, VK_FORMAT_R16_SFLOAT, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_SF16, VK_FORMAT_R16G16_SFLOAT, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_SF16, VK_FORMAT_R16G16B16_SFLOAT, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_SF16, VK_FORMAT_R16G16B16A16_SFLOAT, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
    IMG_FORMAT_INFO_PAIR(R_SF32, VK_FORMAT_R32_SFLOAT, 4, MAKE_INITIALIZER(32, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_SF32, VK_FORMAT_R32G32_SFLOAT, 8, MAKE_INITIALIZER(32, 32, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_SF32, VK_FORMAT_R32G32B32_SFLOAT, 12, MAKE_INITIALIZER(32, 32, 32, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_SF32, VK_FORMAT_R32G32B32A32_SFLOAT, 16, MAKE_INITIALIZER(32, 32, 32, 32)),
    IMG_FORMAT_INFO_PAIR(R_SF64, VK_FORMAT_R64_SFLOAT, 8, MAKE_INITIALIZER(64, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RG_SF64, VK_FORMAT_R64G64_SFLOAT, 16, MAKE_INITIALIZER(64, 64, 0, 0)),
    IMG_FORMAT_INFO_PAIR(RGB_SF64, VK_FORMAT_R64G64B64_SFLOAT, 24, MAKE_INITIALIZER(64, 64, 64, 0)),
    IMG_FORMAT_INFO_PAIR(RGBA_SF64, VK_FORMAT_R64G64B64A64_SFLOAT, 32, MAKE_INITIALIZER(64, 64, 64, 64)),
    IMG_FORMAT_INFO_PAIR(D24X8_U32_NormPacked, VK_FORMAT_X8_D24_UNORM_PACK32, 4, MAKE_INITIALIZER(24, 8, 0, 0)),
    IMG_FORMAT_INFO_PAIR(D_U16_Norm, VK_FORMAT_D16_UNORM, 2, MAKE_INITIALIZER(16, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(D_SF32, VK_FORMAT_D32_SFLOAT, 4, MAKE_INITIALIZER(32, 0, 0, 0)),
    IMG_FORMAT_INFO_PAIR(D32S8_SF32_UI8, VK_FORMAT_D32_SFLOAT_S8_UINT, 5, MAKE_INITIALIZER(32, 8, 0, 0)),
    IMG_FORMAT_INFO_PAIR(D16S8_U24_DNorm_SInt, VK_FORMAT_D16_UNORM_S8_UINT, 3, MAKE_INITIALIZER(16, 8, 0, 0)),
    IMG_FORMAT_INFO_PAIR(D24S8_U32_DNorm_SInt, VK_FORMAT_D24_UNORM_S8_UINT, 4, MAKE_INITIALIZER(24, 8, 0, 0))
};
// clang-format on
#undef IMG_FORMAT_INFO_PAIR
#undef MAKE_COMPS
#undef PREPEND_COMP
static_assert(EPixelDataFormat::Type::AllFormatEnd == 120, "Mismatch EPixelDataFormat between vulkan and interface");

VkFormat vulkanDataFormat(EPixelDataFormat::Type dataFormat)
{
    auto itr = PIXEL_DATA_FORMAT_TO_API_FORMAT.find(dataFormat);
    if (itr != PIXEL_DATA_FORMAT_TO_API_FORMAT.end())
    {
        return itr->second;
    }
    debugAssert(!"Data format to Vulkan data format not found");
    return VkFormat::VK_FORMAT_MAX_ENUM;
}

EPixelDataFormat::Type vulkanToEngineDataFormat(VkFormat dataFormat)
{
    for (const std::pair<const EPixelDataFormat::Type, VkFormat> &formatPair : PIXEL_DATA_FORMAT_TO_API_FORMAT)
    {
        if (formatPair.second == dataFormat)
        {
            return formatPair.first;
        }
    }
    return EPixelDataFormat::Undefined;
}

struct FilterInfoData
{
    VkFilter filter;
    VkSamplerMipmapMode mipFilter;
};

#define SAMPLE_FILTER_INFO_PAIR(FilteringType, RelevantApiFormat, RelevantMipApiFormat)                                                        \
    {                                                                                                                                          \
        RelevantApiFormat, RelevantMipApiFormat                                                                                                \
    }
const FilterInfoData FILTER_TYPE_TO_API_FILTER[] = { SAMPLE_FILTER_INFO_PAIR(Nearest, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST),
                                                     SAMPLE_FILTER_INFO_PAIR(Linear, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR),
                                                     SAMPLE_FILTER_INFO_PAIR(Cubic, VK_FILTER_CUBIC_IMG, VK_SAMPLER_MIPMAP_MODE_LINEAR) };
#undef SAMPLE_FILTER_INFO_PAIR
VkFilter vulkanFilter(ESamplerFiltering::Type filter) { return FILTER_TYPE_TO_API_FILTER[uint32(filter)].filter; }

VkSamplerMipmapMode vulkanSamplerMipFilter(ESamplerFiltering::Type filter) { return FILTER_TYPE_TO_API_FILTER[uint32(filter)].mipFilter; }

VkSamplerAddressMode vulkanSamplerAddressing(ESamplerTilingMode::Type tilingMode) { return VkSamplerAddressMode(tilingMode); }

#define COMP_MAP_INFO_PAIR(ComponentMappingValue, RelevantApiFormat)                                                                           \
    {                                                                                                                                          \
        EPixelComponentMapping::##ComponentMappingValue, RelevantApiFormat                                                                     \
    }
const std::map<EPixelComponentMapping::Type, VkComponentSwizzle> COMP_MAPPING_TO_API_COMP_SWIZZLE
    = { COMP_MAP_INFO_PAIR(SameComponent, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY),
        COMP_MAP_INFO_PAIR(AlwaysOne, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_ONE),
        COMP_MAP_INFO_PAIR(AlwaysZero, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_ZERO),
        COMP_MAP_INFO_PAIR(R, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_R),
        COMP_MAP_INFO_PAIR(G, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_G),
        COMP_MAP_INFO_PAIR(B, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_B),
        COMP_MAP_INFO_PAIR(A, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_A) };
#undef COMP_MAP_INFO_PAIR
VkComponentSwizzle vulkanComponentSwizzle(EPixelComponentMapping::Type mapping)
{
    return COMP_MAPPING_TO_API_COMP_SWIZZLE.find(mapping)->second;
}

#define SHADER_STAGE_TO_API_PAIR(ShaderStage, RelevantApiStage, EntryPointName, ShortName) RelevantApiStage
VkShaderStageFlagBits vulkanShaderStage(EShaderStage::Type shaderStage)
{
    // Using here because this will be accessed while static initialization so it will available always.
    static const VkShaderStageFlagBits SHADER_STAGE_TO_API_STAGE[]
        = { SHADER_STAGE_TO_API_PAIR(Compute, VK_SHADER_STAGE_COMPUTE_BIT, "mainComp", "comp"),
            SHADER_STAGE_TO_API_PAIR(Vertex, VK_SHADER_STAGE_VERTEX_BIT, "mainVS", "vert"),
            SHADER_STAGE_TO_API_PAIR(TessellationControl, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "mainTC", "tesc"),
            SHADER_STAGE_TO_API_PAIR(TessellatonEvaluate, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "mainTE", "tese"),
            SHADER_STAGE_TO_API_PAIR(Geometry, VK_SHADER_STAGE_GEOMETRY_BIT, "mainGeo", "geom"),
            SHADER_STAGE_TO_API_PAIR(Fragment, VK_SHADER_STAGE_FRAGMENT_BIT, "mainFS", "frag") };
    return SHADER_STAGE_TO_API_STAGE[shaderStage];
}
#undef SHADER_STAGE_TO_API_PAIR
VkShaderStageFlags vulkanShaderStageFlags(uint32 shaderStages)
{
    VkShaderStageFlags retFlags = 0;
    uint32 stageMask = 1;
    for (uint32 i = 0; i < EShaderStage::ShaderStageMax; ++i)
    {
        retFlags |= BIT_SET(shaderStages, stageMask) ? vulkanShaderStage(EShaderStage::Type(i)) : 0;
        stageMask <<= 1;
    }
    return retFlags;
}

#define PIPELINE_STAGE_TO_API_PAIR(PipelineStage, RelevantApiStage) RelevantApiStage
VkPipelineStageFlagBits2 vulkanPipelineStage(EPipelineStages::Type pipelineStage)
{
    static const VkPipelineStageFlagBits2 PIPELINE_STAGE_TO_API_STAGE[] = {
        PIPELINE_STAGE_TO_API_PAIR(Top, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(DrawIndirect, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(VertexInput, VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(VertexShaderStage, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(TessellationControlShaderStage, VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(TessallationEvalShaderStage, VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(GeometryShaderStage, VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(FragmentShaderStage, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(EarlyFragTest, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(LateFragTest, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(ColorAttachmentOutput, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(ComputeShaderStage, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(Transfer, VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(Bottom, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(Host, VK_PIPELINE_STAGE_2_HOST_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(AllGraphics, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR),
        PIPELINE_STAGE_TO_API_PAIR(AllCommands, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR),
    };
    return PIPELINE_STAGE_TO_API_STAGE[pipelineStage];
}
#undef PIPELINE_STAGE_TO_API_PAIR

VkPipelineStageFlags2 vulkanPipelineStageFlags(uint64 pipelineStages)
{
    VkPipelineStageFlags2 retFlags = 0;
    uint64 stageMask = 1;
    for (uint32 i = 0; i < EPipelineStages::PipelineStageMax; ++i)
    {
        retFlags |= BIT_SET(pipelineStages, stageMask) ? vulkanPipelineStage(EPipelineStages::Type(i)) : 0;
        stageMask <<= 1;
    }
    return retFlags;
}

VkPipelineStageFlags2 shaderToPipelineStageFlags(VkShaderStageFlags shaderStageFlags)
{
    static VkShaderStageFlagBits shaderStages[] = { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_ALL_GRAPHICS,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_ALL,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_TASK_BIT_NV,
                                                    VkShaderStageFlagBits::VK_SHADER_STAGE_MESH_BIT_NV };

    if (shaderStageFlags == 0)
    {
        return 0;
    }
    VkShaderStageFlags temp = shaderStageFlags;

    VkPipelineStageFlags2 pipelineStageFlags = 0;
    for (VkShaderStageFlags shaderStage : shaderStages)
    {
        if (BIT_SET(shaderStageFlags, shaderStage))
        {
            switch (shaderStage)
            {
            case VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT:
                pipelineStageFlags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                break;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                pipelineStageFlags |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT;
                break;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                pipelineStageFlags |= VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT;
                break;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT:
                pipelineStageFlags |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;
                break;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT:
                pipelineStageFlags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
                break;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT:
                pipelineStageFlags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                break;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_ALL_GRAPHICS:
                pipelineStageFlags |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                break;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_ALL:
                pipelineStageFlags |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                break;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR:
            case VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
            case VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
            case VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR:
            case VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
            case VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR:
                pipelineStageFlags |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
                break;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_TASK_BIT_NV:
                pipelineStageFlags |= VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV;
                break;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_MESH_BIT_NV:
                pipelineStageFlags |= VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV;
                break;
            }

            temp &= ~shaderStage;
            if (temp == 0)
            {
                break;
            }
        }
    }
    return pipelineStageFlags;
}

VkShaderStageFlags pipelineToShaderStageFlags(VkPipelineStageFlags2 pipelineStageFlags)
{
    VkPipelineStageFlagBits2 pipelineStages[] = { VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                                                  VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,
                                                  VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,
                                                  VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,
                                                  VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                                  VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                                  VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                                                  VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                  VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
                                                  VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV,
                                                  VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV };
    if (pipelineStageFlags == 0)
    {
        return 0;
    }

    VkPipelineStageFlags2 temp = pipelineStageFlags;

    VkShaderStageFlags shaderStageFlags = 0;
    for (VkPipelineStageFlagBits2 pipelineStage : pipelineStages)
    {
        if (BIT_SET(pipelineStageFlags, pipelineStage))
        {
            switch (pipelineStage)
            {
            case VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT:
                shaderStageFlags |= VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT:
                shaderStageFlags |= VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                break;
            case VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT:
                shaderStageFlags |= VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                break;
            case VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT:
                shaderStageFlags |= VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT:
                shaderStageFlags |= VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT:
                shaderStageFlags |= VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            case VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT:
                shaderStageFlags |= VkShaderStageFlagBits::VK_SHADER_STAGE_ALL_GRAPHICS;
                break;
            case VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT:
                shaderStageFlags |= VkShaderStageFlagBits::VK_SHADER_STAGE_ALL;
                break;
            case VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR:
                shaderStageFlags
                    |= (VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR
                        | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR
                        | VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR
                        | VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR);
                break;
            case VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV:
                shaderStageFlags |= VkShaderStageFlagBits::VK_SHADER_STAGE_TASK_BIT_NV;
                break;
            case VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV:
                shaderStageFlags |= VkShaderStageFlagBits::VK_SHADER_STAGE_MESH_BIT_NV;
                break;
            }

            temp &= ~pipelineStage;
            if (temp == 0)
            {
                break;
            }
        }
    }
    return shaderStageFlags;
}

VkPipelineStageFlags2 pipelinesSupportedPerQueue(VkQueueFlags queueFlags)
{
    // clang-format off
    static const std::pair<VkQueueFlags, VkPipelineStageFlags2> QUEUE_TO_PIPELINESTAGES[] = { 
        { VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_HOST_BIT | VK_PIPELINE_STAGE_2_RESOLVE_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT | VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT | VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT | VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT | VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT },
        { VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_2_HOST_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT },
        { VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT }
    };
    // clang-format on

    VkPipelineStageFlagBits2 outStages = VK_PIPELINE_STAGE_2_NONE;
    for (uint32 i = 0; i < ARRAY_LENGTH(QUEUE_TO_PIPELINESTAGES); ++i)
    {
        if (BIT_SET(queueFlags, QUEUE_TO_PIPELINESTAGES[i].first))
        {
            outStages |= QUEUE_TO_PIPELINESTAGES[i].second;
        }
    }
    return outStages;
}

VkAccessFlags2 accessMaskPerQueue(VkQueueFlags queueFlags)
{
    // clang-format off
    static const std::pair<VkQueueFlags, VkAccessFlags2> QUEUE_TO_ACCESSMASK[] = { 
        { VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT | VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT | VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_HOST_READ_BIT | VK_ACCESS_2_HOST_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT | VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT | VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT | VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT | VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT | VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR },
        { VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT | VK_ACCESS_2_HOST_READ_BIT | VK_ACCESS_2_HOST_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT },
        { VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_HOST_READ_BIT | VK_ACCESS_2_HOST_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR }
    };
    // clang-format on

    VkAccessFlags2 outStages = VK_ACCESS_2_NONE;
    for (uint32 i = 0; i < ARRAY_LENGTH(QUEUE_TO_ACCESSMASK); ++i)
    {
        if (BIT_SET(queueFlags, QUEUE_TO_ACCESSMASK[i].first))
        {
            outStages |= QUEUE_TO_ACCESSMASK[i].second;
        }
    }
    return outStages;
}

VkAccessFlags2 accessMaskForStages(VkPipelineStageFlags2 pipelineStages)
{
    // clang-format off
    static const std::pair<VkPipelineStageFlagBits2, VkAccessFlags2> STAGE_TO_ACCESSMASK[] = { 
        { VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT                      , VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT         | VK_ACCESS_2_INDEX_READ_BIT                            | VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT                 | VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT                      | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT   | VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT | VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT | VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT | VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT },
        { VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT                      , VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT         | VK_ACCESS_2_INDEX_READ_BIT                            | VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT                 | VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT                      | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT   | VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT | VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT | VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT | VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT | VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR },
        { VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT                      , VK_ACCESS_2_TRANSFER_WRITE_BIT                | VK_ACCESS_2_TRANSFER_WRITE_BIT                        },
        { VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT                     , VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT         | VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT   },
        { VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT            , VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT         },
        { VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT                      , VK_ACCESS_2_INDEX_READ_BIT                    | VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT                 },
        { VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT                       , VK_ACCESS_2_INDEX_READ_BIT },
        { VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT              , VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT        },
        { VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT           , VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT         | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT                | VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT },
        { VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT               , VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT        },
        { VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT                     , VK_ACCESS_2_UNIFORM_READ_BIT                  | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT  | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR   | VK_ACCESS_2_SHADER_READ_BIT           | VK_ACCESS_2_SHADER_WRITE_BIT          },
        { VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT       , VK_ACCESS_2_UNIFORM_READ_BIT                  | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT  | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR   | VK_ACCESS_2_SHADER_READ_BIT           | VK_ACCESS_2_SHADER_WRITE_BIT          },
        { VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT    , VK_ACCESS_2_UNIFORM_READ_BIT                  | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT  | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR   | VK_ACCESS_2_SHADER_READ_BIT           | VK_ACCESS_2_SHADER_WRITE_BIT          },
        { VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT                   , VK_ACCESS_2_UNIFORM_READ_BIT                  | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT  | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR   | VK_ACCESS_2_SHADER_READ_BIT           | VK_ACCESS_2_SHADER_WRITE_BIT          },
        { VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT                   , VK_ACCESS_2_UNIFORM_READ_BIT                  | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT  | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR   | VK_ACCESS_2_SHADER_READ_BIT           | VK_ACCESS_2_SHADER_WRITE_BIT          |  VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT },
        { VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT                    , VK_ACCESS_2_UNIFORM_READ_BIT                  | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT  | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR   | VK_ACCESS_2_SHADER_READ_BIT           | VK_ACCESS_2_SHADER_WRITE_BIT          },
        { VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT                    , VK_ACCESS_2_UNIFORM_READ_BIT                  | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT  | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR   | VK_ACCESS_2_SHADER_READ_BIT           | VK_ACCESS_2_SHADER_WRITE_BIT          },
        { VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR            , VK_ACCESS_2_UNIFORM_READ_BIT                  | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_READ_BIT                   | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT  | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR   | VK_ACCESS_2_SHADER_READ_BIT           | VK_ACCESS_2_SHADER_WRITE_BIT          },
        { VK_PIPELINE_STAGE_2_COPY_BIT                              , VK_ACCESS_2_TRANSFER_READ_BIT                 | VK_ACCESS_2_TRANSFER_WRITE_BIT                        },
        { VK_PIPELINE_STAGE_2_BLIT_BIT                              , VK_ACCESS_2_TRANSFER_READ_BIT                 | VK_ACCESS_2_TRANSFER_WRITE_BIT                        },
        { VK_PIPELINE_STAGE_2_RESOLVE_BIT                           , VK_ACCESS_2_TRANSFER_READ_BIT                 | VK_ACCESS_2_TRANSFER_WRITE_BIT                        },
        { VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT         , VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT},
        { VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT      , VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT },
        { VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT            , VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT  | VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT   | VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT  },
        { VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR  , VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT         | VK_ACCESS_2_SHADER_READ_BIT                           | VK_ACCESS_2_TRANSFER_READ_BIT                         | VK_ACCESS_2_TRANSFER_WRITE_BIT        | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR   | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR },
        { VK_PIPELINE_STAGE_2_HOST_BIT                              , VK_ACCESS_2_HOST_READ_BIT                     | VK_ACCESS_2_HOST_WRITE_BIT                            }
    };
    // clang-format on

    VkAccessFlags2 outStages = VK_ACCESS_2_NONE;
    for (uint32 i = 0; i < ARRAY_LENGTH(STAGE_TO_ACCESSMASK); ++i)
    {
        if (BIT_SET(pipelineStages, STAGE_TO_ACCESSMASK[i].first))
        {
            outStages |= STAGE_TO_ACCESSMASK[i].second;
        }
    }
    return outStages;
}

VkPrimitiveTopology vulkanPrimitiveTopology(EPrimitiveTopology::Type inputAssembly)
{
    switch (inputAssembly)
    {
    case EPrimitiveTopology::Triangle:
        return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case EPrimitiveTopology::Line:
        return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case EPrimitiveTopology::Point:
        return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
    return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
}

VkAttachmentLoadOp vulkanLoadOp(EAttachmentOp::LoadOp loadOp)
{
    switch (loadOp)
    {
    case EAttachmentOp::LoadOp::DontCare:
        return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    case EAttachmentOp::LoadOp::Load:
        return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
    case EAttachmentOp::LoadOp::Clear:
        return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
    }

    return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

VkAttachmentStoreOp vulkanStoreOp(EAttachmentOp::StoreOp storeOp)
{
    switch (storeOp)
    {
    case EAttachmentOp::StoreOp::DontCare:
        return VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
    case EAttachmentOp::StoreOp::Store:
        return VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    }
    return VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
}
} // namespace EngineToVulkanAPI