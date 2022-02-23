/*!
 * \file CoreGraphicsTypes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "String/String.h"
#include "EngineRendererExports.h"

namespace CoreGraphicsTypes
{
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
}


//////////////////////////////////////////////////////////////////////////
// Image and buffer related types
//////////////////////////////////////////////////////////////////////////


namespace EImageShaderUsage
{
    enum Type
    {
        Sampling = 0x01,
        Writing = 0x02
    };
}

// Do not change the values without properly going through every referred usage
enum class EPixelComponent : uint8
{
    R = 0,
    G = 1,
    B = 2,
    A = 3
};

namespace EPixelComponentMapping
{
    enum Type : uint8
    {
        SameComponent = 0,
        AlwaysOne,
        AlwaysZero,
        R,
        G,
        B,
        A
    };

    ENGINERENDERER_EXPORT constexpr Type fromImageComponent(EPixelComponent component)
    {
        return Type(uint32(component) + uint32(Type::R));
    }
}

namespace EPixelDataFormat
{
#define MAX_PIXEL_COMP_COUNT 4

    enum Type
    {
        Undefined,
        /* Integral formats */
        BGR_U8_Norm,
        BGR_S8_Norm,
        BGR_U8_Scaled,
        BGR_S8_Scaled,
        BGR_UI8,
        BGR_SI8,
        BGR_U8_SRGB,            /* 0 to 255 gives 0.0f - 1.0f per comp in sRGB encoding */
        BGRA_U8_Norm,
        BGRA_S8_Norm,
        BGRA_U8_Scaled,
        BGRA_S8_Scaled,
        BGRA_UI8,
        BGRA_SI8,
        BGRA_U8_SRGB,           /* 0 to 255 gives 0.0f - 1.0f per comp in sRGB encoding */
        R_U8_Norm,
        R_S8_Norm,
        R_U8_Scaled,
        R_S8_Scaled,
        R_UI8,
        R_SI8,
        R_U8_SRGB,
        RG_U8_Norm,
        RG_S8_Norm,
        RG_U8_Scaled,
        RG_S8_Scaled,
        RG_UI8,
        RG_SI8,
        RG_U8_SRGB,
        RGB_U8_Norm,
        RGB_S8_Norm,
        RGB_U8_Scaled,
        RGB_S8_Scaled,
        RGB_UI8,
        RGB_SI8,
        RGB_U8_SRGB,
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
        RG_U16_Norm,
        RG_S16_Norm,
        RG_U16_Scaled,
        RG_S16_Scaled,
        RG_UI16,
        RG_SI16,
        RGB_U16_Norm,
        RGB_S16_Norm,
        RGB_U16_Scaled,
        RGB_S16_Scaled,
        RGB_UI16,
        RGB_SI16,
        RGBA_U16_Norm,
        RGBA_S16_Norm,
        RGBA_U16_Scaled,
        RGBA_S16_Scaled,
        RGBA_UI16,
        RGBA_SI16,
        R_UI32,
        R_SI32,
        RG_UI32,
        RG_SI32,
        RGB_UI32,
        RGB_SI32,
        RGBA_UI32,
        RGBA_SI32,
        R_UI64,
        R_SI64,
        RG_UI64,
        RG_SI64,
        RGB_UI64,
        RGB_SI64,
        RGBA_UI64,
        RGBA_SI64,
        /* Floating formats */
        R_SF16,
        RG_SF16,
        RGB_SF16,
        RGBA_SF16,
        R_SF32,
        RG_SF32,
        RGB_SF32,
        RGBA_SF32,
        R_SF64,
        RG_SF64,
        RGB_SF64,
        RGBA_SF64,
        ABGR8_U32_NormPacked,     /* 0 to 255 gives 0.0f - 1.0f per comp */
        ABGR8_S32_NormPacked,     /* -127 to 127 gives -1.0f - 1.0f per comp( -128 gets clamped to -127 ) */
        ABGR8_U32_ScaledPacked,   /* Just converts the value directly as float 0.0f - 255.0f per comp */
        ABGR8_S32_ScaledPacked,   /* Just converts the value directly as float -128.0f - 127.0f per comp */
        ABGR8_UI32_Packed,
        ABGR8_SI32_Packed,
        ABGR8_U32_SrgbPacked,
        A2RGB10_U32_NormPacked,
        A2RGB10_S32_NormPacked,
        A2RGB10_U32_ScaledPacked,
        A2RGB10_S32_ScaledPacked,
        A2RGB10_UI32_Packed,
        A2RGB10_SI32_Packed,
        A2BGR10_U32_NormPacked,
        A2BGR10_S32_NormPacked,
        A2BGR10_U32_ScaledPacked,
        A2BGR10_S32_ScaledPacked,
        A2BGR10_UI32_Packed,
        A2BGR10_SI32_Packed,
        /* Depth and stencil formats */
        D24X8_U32_NormPacked,   /* 0 to 16777215 depth gives 0.0f to 1.0f, 8bit not used */
        D_U16_Norm,             /* 0 to 65535 gives 0.0f to 1.0f */
        D_SF32,
        D32S8_SF32_UI8,
        D16S8_U24_DNorm_SInt,   /* 0 to 65535 depth gives 0.0f to 1.0f, 0 - 255 as stencil */
        D24S8_U32_DNorm_SInt,   /* 0 to 16777215 depth gives 0.0f to 1.0f, 0 - 255 as stencil */
        AllFormatEnd,
        FloatFormatBegin = R_SF16,
        FloatFormatEnd = RGBA_SF64,
        PackedFormatBegin = ABGR8_U32_NormPacked,
        PackedFormatEnd = D24X8_U32_NormPacked,
        DepthFormatBegin = D24X8_U32_NormPacked,
        DepthFormatEnd = D24S8_U32_DNorm_SInt,
        StencilDepthBegin = D32S8_SF32_UI8,
        StencilDepthEnd = D24S8_U32_DNorm_SInt
    };

    struct PixelFormatInfo
    {
        // In bytes
        const uint32 pixelDataSize;
        const String formatName;
        // In bits
        const uint8 componentSize[MAX_PIXEL_COMP_COUNT] = { 0,0,0,0 };
        const EPixelComponent componentOrder[MAX_PIXEL_COMP_COUNT] = { EPixelComponent::R,EPixelComponent::G
            ,EPixelComponent::B,EPixelComponent::A };
        const uint32 componentCount = calcCompCount();
        // Packed offsets in bits 0b-7b R comp, 8b-15b G, 16b-23b B, 24b-31b A
        const uint32 componentOffsets = calcOffsets();

        ENGINERENDERER_EXPORT inline constexpr uint8 getOffset(EPixelComponent component) const
        {
            uint32 shift = uint32(component) * 8;
            return uint8((componentOffsets >> shift) & 0x000000FF);
        }

        // Use componentCount over this function
        constexpr uint32 calcCompCount() const
        {
            uint32 compCount = 0;
            for (const uint8& compSize : componentSize)
            {
                compCount += compSize > 0 ? 1 : 0;
            }
            return compCount;
        }
        constexpr uint32 calcOffsets() const
        {
            uint32 offsets = 0;

            for (uint8 idx = 0; idx < componentCount; ++idx)
            {
                uint32 shift = uint32(componentOrder[idx]) * 8;

                uint8 offsetValue = 0;
                for (int32 i = idx - 1; i >= 0; --i)
                {
                    offsetValue += componentSize[uint32(componentOrder[i])];
                }

                offsets |= (offsetValue << shift);
            }

            return offsets;
        }

    };

    ENGINERENDERER_EXPORT const PixelFormatInfo* getFormatInfo(EPixelDataFormat::Type dataFormat);
    ENGINERENDERER_EXPORT bool isDepthFormat(EPixelDataFormat::Type dataFormat);
    ENGINERENDERER_EXPORT bool isStencilFormat(EPixelDataFormat::Type dataFormat);
    ENGINERENDERER_EXPORT bool isFloatingFormat(EPixelDataFormat::Type dataFormat);
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

    ENGINERENDERER_EXPORT String filterName(ESamplerFiltering::Type dataFormat);
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
}

namespace ESamplerBorderColors
{
    enum Type : uint8
    {
        Transparent = 1,
        White = 2,
        Integer = 4
    };
}

//////////////////////////////////////////////////////////////////////////
// Pipeline types
//////////////////////////////////////////////////////////////////////////

enum class EPolygonDrawMode
{
    Fill = 0,
    Line = 1,
    Point = 2
};

enum class ECullingMode
{
    None = 0,
    FrontFace = 1,
    BackFace = 2,
    Both = 3
};

enum class EStencilOp
{
    KeepOld = 0,
    Zero = 1,
    KeepNew = 2,
    IncrementClamped = 3,
    DecrementClamped = 4,
    Invert = 5,
    IncrementWrap = 6,
    DecrementWrap = 7
};

enum class EBlendOp
{
    Add = 0,// S + D
    Subtract = 1,// S - D
    InvertedSubtract = 2,// D - S
    Min = 3,
    Max = 4
    // TODO(Jeslas) Not very important : support for advanced blending options
};

enum class EBlendFactor
{
    Zero = 0,
    One = 1,
    SrcColor = 2,
    OneMinusSrcColor = 3,
    DstColor = 4,
    OneMinusDstColor = 5,
    SrcAlpha = 6,
    OneMinusSrcAlpha = 7,
    DstAlpha = 8,
    OneMinusDstAlpha = 9,
    ConstColor = 10,
    OneMinusConstColor = 11,
    ConstAlpha = 12,
    OneMinusConstAlpha = 13,
    SrcAlphaSaturate = 14,
    Src1Color = 15,
    OneMinusSrc1Color = 16,
    Src1Alpha = 17,
    OneMinusSrc1Alpha = 18,
};

struct DepthState
{
    CoreGraphicsTypes::ECompareOp::Type compareOp = CoreGraphicsTypes::ECompareOp::Greater;
    bool bEnableWrite = true;
    //bool bEnableDepthBounds = false;
};


struct StencilState
{
    // Passed both depth and stencil test
    EStencilOp passOp = EStencilOp::KeepOld;
    // Failed stencil test
    EStencilOp failOp = EStencilOp::KeepOld;
    // Passed stencil test but failed depth test
    EStencilOp depthFailOp = EStencilOp::KeepOld;

    CoreGraphicsTypes::ECompareOp::Type compareOp = CoreGraphicsTypes::ECompareOp::Never;
};

enum class EStencilFaceMode
{
    FrontFace = 1,
    BackFace = 2,
    Both = 3
};

struct AttachmentBlendState
{
    bool bBlendEnable = false;
    EBlendFactor srcColorFactor = EBlendFactor::One;
    EBlendFactor dstColorFactor = EBlendFactor::Zero;
    EBlendOp colorBlendOp = EBlendOp::Add;
    EBlendFactor srcAlphaFactor = EBlendFactor::One;
    EBlendFactor dstAlphaFactor = EBlendFactor::Zero;
    EBlendOp alphaBlendOp = EBlendOp::Add;

    ENGINERENDERER_EXPORT bool usesBlendConstant() const
    {
        return srcColorFactor == EBlendFactor::ConstColor || srcColorFactor == EBlendFactor::OneMinusConstColor
            || srcColorFactor == EBlendFactor::ConstAlpha || srcColorFactor == EBlendFactor::ConstColor
            || dstColorFactor == EBlendFactor::ConstColor || dstColorFactor == EBlendFactor::OneMinusConstColor
            || dstColorFactor == EBlendFactor::ConstAlpha || dstColorFactor == EBlendFactor::ConstColor
            || srcAlphaFactor == EBlendFactor::ConstColor || srcAlphaFactor == EBlendFactor::OneMinusConstColor
            || srcAlphaFactor == EBlendFactor::ConstAlpha || srcAlphaFactor == EBlendFactor::ConstColor
            || dstAlphaFactor == EBlendFactor::ConstColor || dstAlphaFactor == EBlendFactor::OneMinusConstColor
            || dstAlphaFactor == EBlendFactor::ConstAlpha || dstAlphaFactor == EBlendFactor::ConstColor;
    }
};

//////////////////////////////////////////////////////////////////////////
/// Render pass attachment types
//////////////////////////////////////////////////////////////////////////

namespace EAttachmentOp
{
    enum class LoadOp
    {
        DontCare,
        Load,
        Clear
    };

    enum class StoreOp
    {
        DontCare,
        Store
    };
}

#define EPIPELINESTAGES_FOR_EACH_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName) \
    FirstMacroName(Top) \
    MacroName(DrawIndirect) \
    MacroName(VertexInput) \
    MacroName(VertexShaderStage) \
    MacroName(TessellationControlShaderStage) \
    MacroName(TessallationEvalShaderStage) \
    MacroName(GeometryShaderStage) \
    MacroName(FragmentShaderStage) \
    MacroName(EarlyFragTest) \
    MacroName(LateFragTest) \
    MacroName(ColorAttachmentOutput) \
    MacroName(ComputeShaderStage) \
    MacroName(Transfer) \
    MacroName(Bottom) \
    MacroName(Host) \
    MacroName(AllGraphics) \
    LastMacroName(AllCommands)

#define EPIPELINESTAGES_FOR_EACH(MacroName) EPIPELINESTAGES_FOR_EACH_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

namespace EPipelineStages
{
#define EPIPELINESTAGES_FIRST(Stage) Stage## = 0,
#define EPIPELINESTAGES(Stage) Stage##,
#define EPIPELINESTAGES_LAST(Stage) Stage
    enum Type
    {
        EPIPELINESTAGES_FOR_EACH_UNIQUE_FIRST_LAST(EPIPELINESTAGES_FIRST, EPIPELINESTAGES, EPIPELINESTAGES_LAST),
        PipelineStageMax
    };
#undef EPIPELINESTAGES_FIRST
#undef EPIPELINESTAGES
#undef EPIPELINESTAGES_LAST
}

//////////////////////////////////////////////////////////////////////////////
/// Engine graphics types - 
/// Graphics types that are only relevant to engine and not to graphics api
/////////////////////////////////////////////////////////////////////////////

// Moved here to not include FrameBufferTypes.h in places where only this enum is necessary
namespace ERenderPassFormat
{
    enum Type
    {
        Generic,
        Multibuffer,
        Depth,
        PointLightDepth,
        DirectionalLightDepth
    };

    ENGINERENDERER_EXPORT String toString(ERenderPassFormat::Type renderpassFormat);

#define FOR_EACH_RENDERPASS_FORMAT(OpMacro) \
    OpMacro(Generic)                        \
    OpMacro(Multibuffer)                    \
    OpMacro(Depth)                          \
    OpMacro(PointLightDepth)                \
    OpMacro(DirectionalLightDepth)
}