#pragma once
#include "../Core/String/String.h"

namespace CoreGraphicsTypes
{
    struct EnumTypeInfo
    {
        uint32 value;
        String name;
    };

    namespace ECompareOp
    {
        enum Type
        {
            Never = 0,
            Less = 1,
            Equal = 2,
            EqualOrLess = 3,
            Greater = 4,
            NotEqual = 5,
            EqualOrGreater = 6,
            Always = 7,
        };
    }


    const EnumTypeInfo* getEnumTypeInfo(ECompareOp::Type compareOp);
}


//////////////////////////////////////////////////////////////////////////
// Image and buffer related types
//////////////////////////////////////////////////////////////////////////

namespace EPixelDataFormat
{
    enum Type
    {
        Undefined,
        BGR_U8_Norm,
        BGR_S8_Norm,
        BGR_U8_Scaled,
        BGR_S8_Scaled,
        BGR_UI8,
        BGR_SI8,
        BGR_U8_SRGB,            /* 0 to 255 gives 0.0f - 1.0f per comp in sRGB encoding */
        ABGR_U8_NormPacked,     /* 0 to 255 gives 0.0f - 1.0f per comp */
        ABGR_S8_NormPacked,     /* -127 to 127 gives -1.0f - 1.0f per comp( -128 gets clamped to -127 ) */
        ABGR_U8_ScaledPacked,   /* Just converts the value directly as float 0.0f - 255.0f per comp */
        ABGR_S8_ScaledPacked,   /* Just converts the value directly as float -128.0f - 127.0f per comp */
        ABGR_UI8_Packed,
        ABGR_SI8_Packed,
        ABGR_U8_SrgbPacked,
        BGRA_U8_Norm,
        BGRA_S8_Norm,
        BGRA_U8_Scaled,
        BGRA_S8_Scaled,
        BGRA_UI8,
        BGRA_SI8,
        BGRA_U8_SRGB,           /* 0 to 255 gives 0.0f - 1.0f per comp in sRGB encoding */
        RGBA_U8_Norm,
        RGBA_S8_Norm,
        RGBA_U8_Scaled,
        RGBA_S8_Scaled,
        RGBA_UI8,
        RGBA_SI8,
        RGBA_U8_SRGB,           /* 0 to 255 gives 0.0f - 1.0f per comp in sRGB encoding */
        R_U16_Norm,             /* 0 to 65535 gives 0.0f to 1.0f */
        R_S16_Norm,             /* -32767 to 32767 gives -1.0f - 1.0f per comp( -32768 gets clamped to -32767 )*/
        R_U16_Scaled,
        R_S16_Scaled,
        R_UI16,
        R_SI16,
        R_SF16,
        RG_U16_Norm,
        RG_S16_Norm,
        RG_U16_Scaled,
        RG_S16_Scaled,
        RG_UI16,
        RG_SI16,
        RG_SF16,
        RGB_U16_Norm,
        RGB_S16_Norm,
        RGB_U16_Scaled,
        RGB_S16_Scaled,
        RGB_UI16,
        RGB_SI16,
        RGB_SF16,
        RGBA_U16_Norm,
        RGBA_S16_Norm,
        RGBA_U16_Scaled,
        RGBA_S16_Scaled,
        RGBA_UI16,
        RGBA_SI16,
        RGBA_SF16,
        R_UI32,
        R_SI32,
        R_SF32,
        RG_UI32,
        RG_SI32,
        RG_SF32,
        RGB_UI32,
        RGB_SI32,
        RGB_SF32,
        RGBA_UI32,
        RGBA_SI32,
        RGBA_SF32,
        D_U16_Norm,             /* 0 to 65535 gives 0.0f to 1.0f */
        D24X8_U32_NormPacked,   /* 0 to 16777215 depth gives 0.0f to 1.0f, 8bit not used */
        D_SF32,
        D32S8_SF32_UI8,
        D16S8_U24_DNorm_SInt,   /* 0 to 65535 depth gives 0.0f to 1.0f, 0 - 255 as stencil */
        D24S8_U32_DNorm_SInt,   /* 0 to 16777215 depth gives 0.0f to 1.0f, 0 - 255 as stencil */
        DepthFormatBegin = D_U16_Norm,
        DepthFormatEnd = D24S8_U32_DNorm_SInt,
        StencilDepthBegin = D32S8_SF32_UI8,
        StencilDepthEnd = D24S8_U32_DNorm_SInt
    };

    struct ImageFormatInfo
    {
        uint32 format;
        uint32 pixelDataSize;
        String formatName;
    };

    const ImageFormatInfo* getFormatInfo(EPixelDataFormat::Type dataFormat);
    EPixelDataFormat::Type fromApiFormat(uint32 apiFormat);
    bool isDepthFormat(EPixelDataFormat::Type dataFormat);
    bool isStencilFormat(EPixelDataFormat::Type dataFormat);
}

namespace EPixelSampleCount
{
    enum Type
    {
        SampleCount1 = 0x01,
        SampleCount2 = 0x02,
        SampleCount4 = 0x04,
        SampleCount8 = 0x08,
        SampleCount16 = 0x10,
        SampleCount32 = 0x20,
        SampleCount64 = 0x40,
    };
}

namespace EImageShaderUsage
{
    enum Type
    {
        Sampling = 0x01,
        Writing = 0x02
    };
}

namespace EImageComponentMapping
{
    enum Type
    {
        SameComponent,
        AlwaysOne,
        AlwaysZero,
        R,
        G,
        B,
        A
    };
    struct ComponentMappingInfo
    {
        uint32 mapping;
        String mappingName;
    };

    const ComponentMappingInfo* getComponentMapping(EImageComponentMapping::Type mapping);
}

//////////////////////////////////////////////////////////////////////////
// Sampler types
//////////////////////////////////////////////////////////////////////////

namespace ESamplerFiltering
{
    enum Type
    {
        Nearest = 0,
        Linear = 1,
        Cubic = 2
    };

    struct SamplerFilteringInfo
    {
        uint32 filterTypeValue;
        String filterName;
    };

    const SamplerFilteringInfo* getFilterInfo(ESamplerFiltering::Type dataFormat);
    const SamplerFilteringInfo* getMipFilterInfo(ESamplerFiltering::Type dataFormat);
}

namespace ESamplerTilingMode
{
    enum Type
    {
        Repeat = 0,
        MirroredRepeat = 1,
        EdgeClamp = 2,
        BorderClamp = 3,
        EdgeMirroredClamp = 4
    };

    uint32 getSamplerTiling(ESamplerTilingMode::Type tilingMode);
}