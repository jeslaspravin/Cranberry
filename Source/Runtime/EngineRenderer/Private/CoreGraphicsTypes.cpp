/*!
 * \file CoreGraphicsTypes.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include <map>

#include "RenderInterface/CoreGraphicsTypes.h"
#include "RenderInterface/Resources/ShaderResources.h"

// NOTE : Update the Graphics API relevant values as well after changing here
// Vulkan - VulkanGraphicsTypes.cpp

namespace EPixelDataFormat
{
// Except packed format everything else is in byte order 0...N Byte, while packed formats are in order of
// bit N...0 Bit
#define PREPEND_COMP(X) COMBINE(EPixelComponent::, X)
#define MAKE_COMPS(...) MAKE_INITIALIZER(TRANSFORM_ALL(PREPEND_COMP, __VA_ARGS__))
#define IMG_FORMAT_INFO_PAIR(PixelFormat, DataSize, ComponentSize, ...)                                                                        \
    {                                                                                                                                          \
        PixelFormat, { DataSize, TCHAR(#PixelFormat), ComponentSize, __VA_ARGS__ }                                                             \
    }
const std::map<Type, PixelFormatInfo> DATA_FORMAT_TO_API_FORMAT
    = { IMG_FORMAT_INFO_PAIR(Undefined, 0, MAKE_INITIALIZER(0, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(BGR_U8_Norm, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(BGR_S8_Norm, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(BGR_U8_Scaled, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(BGR_S8_Scaled, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(BGR_UI8, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(BGR_SI8, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(BGR_U8_SRGB, 3, MAKE_INITIALIZER(8, 8, 8, 0), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(BGRA_U8_Norm, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(BGRA_S8_Norm, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(BGRA_U8_Scaled, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(BGRA_S8_Scaled, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(BGRA_UI8, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(BGRA_SI8, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(BGRA_U8_SRGB, 4, MAKE_INITIALIZER(8, 8, 8, 8), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(R_U8_Norm, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(R_S8_Norm, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(R_U8_Scaled, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(R_S8_Scaled, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(R_UI8, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(R_SI8, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(R_U8_SRGB, 1, MAKE_INITIALIZER(8, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_U8_Norm, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_S8_Norm, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_U8_Scaled, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_S8_Scaled, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_UI8, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_SI8, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_U8_SRGB, 2, MAKE_INITIALIZER(8, 8, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_U8_Norm, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_S8_Norm, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_U8_Scaled, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_S8_Scaled, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_UI8, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_SI8, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_U8_SRGB, 3, MAKE_INITIALIZER(8, 8, 8, 0)),
        IMG_FORMAT_INFO_PAIR(RGBA_U8_Norm, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(RGBA_S8_Norm, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(RGBA_U8_Scaled, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(RGBA_S8_Scaled, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(RGBA_UI8, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(RGBA_SI8, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(RGBA_U8_SRGB, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(R_U16_Norm, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(R_S16_Norm, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(R_U16_Scaled, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(R_S16_Scaled, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(R_UI16, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(R_SI16, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_U16_Norm, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_S16_Norm, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_U16_Scaled, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_S16_Scaled, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_UI16, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_SI16, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_U16_Norm, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_S16_Norm, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_U16_Scaled, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_S16_Scaled, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_UI16, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_SI16, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
        IMG_FORMAT_INFO_PAIR(RGBA_U16_Norm, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
        IMG_FORMAT_INFO_PAIR(RGBA_S16_Norm, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
        IMG_FORMAT_INFO_PAIR(RGBA_U16_Scaled, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
        IMG_FORMAT_INFO_PAIR(RGBA_S16_Scaled, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
        IMG_FORMAT_INFO_PAIR(RGBA_UI16, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
        IMG_FORMAT_INFO_PAIR(RGBA_SI16, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
        IMG_FORMAT_INFO_PAIR(R_UI32, 4, MAKE_INITIALIZER(32, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(R_SI32, 4, MAKE_INITIALIZER(32, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_UI32, 8, MAKE_INITIALIZER(32, 32, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_SI32, 8, MAKE_INITIALIZER(32, 32, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_UI32, 12, MAKE_INITIALIZER(32, 32, 32, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_SI32, 12, MAKE_INITIALIZER(32, 32, 32, 0)),
        IMG_FORMAT_INFO_PAIR(RGBA_UI32, 16, MAKE_INITIALIZER(32, 32, 32, 32)),
        IMG_FORMAT_INFO_PAIR(RGBA_SI32, 16, MAKE_INITIALIZER(32, 32, 32, 32)),
        IMG_FORMAT_INFO_PAIR(R_UI64, 8, MAKE_INITIALIZER(64, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(R_SI64, 8, MAKE_INITIALIZER(64, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_UI64, 16, MAKE_INITIALIZER(64, 64, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_SI64, 16, MAKE_INITIALIZER(64, 64, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_UI64, 24, MAKE_INITIALIZER(64, 64, 64, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_SI64, 24, MAKE_INITIALIZER(64, 64, 64, 0)),
        IMG_FORMAT_INFO_PAIR(RGBA_UI64, 32, MAKE_INITIALIZER(64, 64, 64, 64)),
        IMG_FORMAT_INFO_PAIR(RGBA_SI64, 32, MAKE_INITIALIZER(64, 64, 64, 64)),
        IMG_FORMAT_INFO_PAIR(R_SF16, 2, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_SF16, 4, MAKE_INITIALIZER(16, 16, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_SF16, 6, MAKE_INITIALIZER(16, 16, 16, 0)),
        IMG_FORMAT_INFO_PAIR(RGBA_SF16, 8, MAKE_INITIALIZER(16, 16, 16, 16)),
        IMG_FORMAT_INFO_PAIR(R_SF32, 4, MAKE_INITIALIZER(32, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_SF32, 8, MAKE_INITIALIZER(32, 32, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_SF32, 12, MAKE_INITIALIZER(32, 32, 32, 0)),
        IMG_FORMAT_INFO_PAIR(RGBA_SF32, 16, MAKE_INITIALIZER(32, 32, 32, 32)),
        IMG_FORMAT_INFO_PAIR(R_SF64, 8, MAKE_INITIALIZER(64, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RG_SF64, 16, MAKE_INITIALIZER(64, 64, 0, 0)),
        IMG_FORMAT_INFO_PAIR(RGB_SF64, 24, MAKE_INITIALIZER(64, 64, 64, 0)),
        IMG_FORMAT_INFO_PAIR(RGBA_SF64, 32, MAKE_INITIALIZER(64, 64, 64, 64)),
        IMG_FORMAT_INFO_PAIR(ABGR8_U32_NormPacked, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(ABGR8_S32_NormPacked, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(ABGR8_U32_ScaledPacked, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(ABGR8_S32_ScaledPacked, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(ABGR8_UI32_Packed, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(ABGR8_SI32_Packed, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(ABGR8_U32_SrgbPacked, 4, MAKE_INITIALIZER(8, 8, 8, 8)),
        IMG_FORMAT_INFO_PAIR(A2RGB10_U32_NormPacked, 4, MAKE_INITIALIZER(10, 10, 10, 2), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(A2RGB10_S32_NormPacked, 4, MAKE_INITIALIZER(10, 10, 10, 2), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(A2RGB10_U32_ScaledPacked, 4, MAKE_INITIALIZER(10, 10, 10, 2), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(A2RGB10_S32_ScaledPacked, 4, MAKE_INITIALIZER(10, 10, 10, 2), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(A2RGB10_UI32_Packed, 4, MAKE_INITIALIZER(10, 10, 10, 2), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(A2RGB10_SI32_Packed, 4, MAKE_INITIALIZER(10, 10, 10, 2), MAKE_COMPS(B, G, R, A)),
        IMG_FORMAT_INFO_PAIR(A2BGR10_U32_NormPacked, 4, MAKE_INITIALIZER(10, 10, 10, 2)),
        IMG_FORMAT_INFO_PAIR(A2BGR10_S32_NormPacked, 4, MAKE_INITIALIZER(10, 10, 10, 2)),
        IMG_FORMAT_INFO_PAIR(A2BGR10_U32_ScaledPacked, 4, MAKE_INITIALIZER(10, 10, 10, 2)),
        IMG_FORMAT_INFO_PAIR(A2BGR10_S32_ScaledPacked, 4, MAKE_INITIALIZER(10, 10, 10, 2)),
        IMG_FORMAT_INFO_PAIR(A2BGR10_UI32_Packed, 4, MAKE_INITIALIZER(10, 10, 10, 2)),
        IMG_FORMAT_INFO_PAIR(A2BGR10_SI32_Packed, 4, MAKE_INITIALIZER(10, 10, 10, 2)),
        IMG_FORMAT_INFO_PAIR(D24X8_U32_NormPacked, 4, MAKE_INITIALIZER(24, 8, 0, 0)),
        IMG_FORMAT_INFO_PAIR(D_U16_Norm, 2, MAKE_INITIALIZER(16, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(D_SF32, 4, MAKE_INITIALIZER(32, 0, 0, 0)),
        IMG_FORMAT_INFO_PAIR(D32S8_SF32_UI8, 5, MAKE_INITIALIZER(32, 8, 0, 0)),
        IMG_FORMAT_INFO_PAIR(D16S8_U24_DNorm_SInt, 3, MAKE_INITIALIZER(16, 8, 0, 0)),
        IMG_FORMAT_INFO_PAIR(D24S8_U32_DNorm_SInt, 4, MAKE_INITIALIZER(24, 8, 0, 0)) };
#undef IMG_FORMAT_INFO_PAIR
#undef MAKE_COMPS
#undef PREPEND_COMP

const PixelFormatInfo *getFormatInfo(EPixelDataFormat::Type dataFormat)
{
    auto itr = DATA_FORMAT_TO_API_FORMAT.find(dataFormat);
    if (itr != DATA_FORMAT_TO_API_FORMAT.end())
    {
        return &itr->second;
    }
    return nullptr;
}
} // namespace EPixelDataFormat

namespace ESamplerFiltering
{
String filterName(Type dataFormat)
{
    switch (dataFormat)
    {
    case ESamplerFiltering::Nearest:
        return TCHAR("Nearest");
        break;
    case ESamplerFiltering::Linear:
        return TCHAR("Linear");
        break;
    case ESamplerFiltering::Cubic:
        return TCHAR("Cubic");
        break;
    default:
        break;
    }
    return {};
}
} // namespace ESamplerFiltering

namespace EShaderStage
{
#define SHADER_STAGE_TO_API_PAIR(ShaderStage, EntryPointName, ShortName)                                                                       \
    {                                                                                                                                          \
        ShaderStage, { TCHAR(#ShaderStage), TCHAR(ShortName), TCHAR(EntryPointName) }                                                          \
    }

const ShaderStageInfo *getShaderStageInfo(EShaderStage::Type shaderStage)
{
    // Using here because this will be accessed while static initialization so it will available always.
    static const std::map<Type, ShaderStageInfo> shaderStageToApiStage
        = { SHADER_STAGE_TO_API_PAIR(Compute, "mainComp", "comp"),           SHADER_STAGE_TO_API_PAIR(Vertex, "mainVS", "vert"),
            SHADER_STAGE_TO_API_PAIR(TessellationControl, "mainTC", "tesc"), SHADER_STAGE_TO_API_PAIR(TessellatonEvaluate, "mainTE", "tese"),
            SHADER_STAGE_TO_API_PAIR(Geometry, "mainGeo", "geom"),           SHADER_STAGE_TO_API_PAIR(Fragment, "mainFS", "frag") };

    auto itr = shaderStageToApiStage.find(shaderStage);
    if (itr != shaderStageToApiStage.end())
    {
        return &itr->second;
    }
    return nullptr;
}
#undef SHADER_STAGE_TO_API_PAIR
} // namespace EShaderStage

//////////////////////////////////////////////////////////////////////////////
/// Engine graphics types -
/// Graphics types that are only relevant to engine and not to graphics api
/////////////////////////////////////////////////////////////////////////////

namespace ERenderPassFormat
{
String toString(Type renderpassFormat)
{
#define CASE_MACRO(Format)                                                                                                                     \
    case ERenderPassFormat::##Format:                                                                                                          \
        return TCHAR(#Format);

    switch (renderpassFormat)
    {
        FOR_EACH_RENDERPASS_FORMAT(CASE_MACRO)
    }
    return {};
}
} // namespace ERenderPassFormat