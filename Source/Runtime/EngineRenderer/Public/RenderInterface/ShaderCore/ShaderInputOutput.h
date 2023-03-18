/*!
 * \file ShaderInputOutput.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "RenderInterface/CoreGraphicsTypes.h"

struct ReflectFieldType;

namespace EShaderInputFrequency
{
enum Type
{
    PerVertex = 0,
    PerInstance = 1
};
} // namespace EShaderInputFrequency

// Half float not supported yet
namespace EShaderInputAttribFormat
{
enum Type : uint32
{
    Undefined = EPixelDataFormat::Undefined,
    Double = EPixelDataFormat::R_SF64,
    Double2 = EPixelDataFormat::RG_SF64,
    Double3 = EPixelDataFormat::RGB_SF64,
    Double4 = EPixelDataFormat::RGBA_SF64,
    Float = EPixelDataFormat::R_SF32,
    Float2 = EPixelDataFormat::RG_SF32,
    Float3 = EPixelDataFormat::RGB_SF32,
    Float4 = EPixelDataFormat::RGBA_SF32,
    Int = EPixelDataFormat::R_SI32,
    Int2 = EPixelDataFormat::RG_SI32,
    Int3 = EPixelDataFormat::RGB_SI32,
    Int4 = EPixelDataFormat::RGBA_SI32,
    UInt = EPixelDataFormat::R_UI32,
    UInt2 = EPixelDataFormat::RG_UI32,
    UInt3 = EPixelDataFormat::RGB_UI32,
    UInt4 = EPixelDataFormat::RGBA_UI32,
    ShortInt = EPixelDataFormat::R_SI16,
    ShortInt2 = EPixelDataFormat::RG_SI16,
    ShortInt3 = EPixelDataFormat::RGB_SI16,
    ShortInt4 = EPixelDataFormat::RGBA_SI16,
    UShortInt = EPixelDataFormat::R_UI16,
    UShortInt2 = EPixelDataFormat::RG_UI16,
    UShortInt3 = EPixelDataFormat::RGB_UI16,
    UShortInt4 = EPixelDataFormat::RGBA_UI16,
    Byte = EPixelDataFormat::R_SI8,
    Byte2 = EPixelDataFormat::RG_SI8,
    Byte3 = EPixelDataFormat::RGB_SI8,
    Byte4 = EPixelDataFormat::RGBA_SI8,
    UByte = EPixelDataFormat::R_UI8,
    UByte2 = EPixelDataFormat::RG_UI8,
    UByte3 = EPixelDataFormat::RGB_UI8,
    UByte4 = EPixelDataFormat::RGBA_UI8,
    // Additional formats for packed color values
    UInt4Norm = EPixelDataFormat::RGBA_U8_Norm,
    Matrix2x2 = EPixelDataFormat::AllFormatEnd,
    Matrix3x3,
    Matrix4x4
};

EShaderInputAttribFormat::Type getInputFormat(const ReflectFieldType &fieldType);
} // namespace EShaderInputAttribFormat