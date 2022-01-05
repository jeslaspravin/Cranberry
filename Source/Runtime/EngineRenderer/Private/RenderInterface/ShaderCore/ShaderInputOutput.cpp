/*!
 * \file ShaderInputOutput.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/ShaderCore/ShaderInputOutput.h"
#include "CommonShaderTypes.h"
#include "Math/Math.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Logger/Logger.h"

#include <set>
#include <map>
#include <algorithm>

namespace EShaderInputAttribFormat
{
    Type getInputFormat(const ReflectFieldType& fieldType)
    {
        static const std::map<EReflectBufferPrimitiveType, std::set<Type>> TypeFormatMapper =
        {
            { EReflectBufferPrimitiveType::ReflectPrimitive_bool, { UInt, UInt2, UInt3 ,UInt4 }},
            { EReflectBufferPrimitiveType::ReflectPrimitive_int, { Int, Int2, Int3, Int4 }},
            { EReflectBufferPrimitiveType::ReflectPrimitive_uint, { UInt, UInt2, UInt3 ,UInt4 }},
            { EReflectBufferPrimitiveType::ReflectPrimitive_float, { Float, Float2, Float3, Float4, Matrix2x2, Matrix3x3, Matrix4x4 }},
            { EReflectBufferPrimitiveType::ReflectPrimitive_double, { Double, Double2, Double3 ,Double4 }},
        };

        static const std::map<uint32, std::set<Type>> VecSizeFormatMapper =
        {
            { 1 , { Int, UInt, Float, Double, ShortInt, UShortInt, Byte, UByte }},
            { 2 , { Int2, UInt2, Float2, Double2, ShortInt2, UShortInt2, Byte2, UByte2, Matrix2x2 }},
            { 3 , { Int3, UInt3, Float3, Double3, ShortInt3, UShortInt3, Byte3, UByte3, Matrix3x3 }},
            { 4 , { Int4, UInt4, Float4, Double4, ShortInt4, UShortInt4, Byte4, UByte4, Matrix4x4 }}
        };

        static const std::map<uint32, std::set<Type>> ColumnSizeFormatMapper =
        {
            { 1 , { Int, UInt, Float, Double, ShortInt, UShortInt, Byte, UByte, Int2, UInt2, Float2, Double2
                , ShortInt2, UShortInt2, Byte2, UByte2, Int3, UInt3, Float3, Double3, ShortInt3
                , UShortInt3, Byte3, UByte3, Int4, UInt4, Float4, Double4, ShortInt4, UShortInt4, Byte4, UByte4 }},
            { 2 , { Matrix2x2 }},
            { 3 , { Matrix3x3 }},
            { 4 , { Matrix4x4 }},
        };

        const std::set<Type>& sizeMatches = VecSizeFormatMapper.at(fieldType.vecSize);
        const std::set<Type>& colMatches = ColumnSizeFormatMapper.at(fieldType.colSize);
        const std::set<Type>& typeMatches = TypeFormatMapper.at(fieldType.primitive);

        std::vector<Type> intersections(Math::min(sizeMatches.size(), typeMatches.size()));
        std::vector<Type>::iterator intersectionsEnd = std::set_intersection(typeMatches.cbegin(), typeMatches.cend()
            , sizeMatches.cbegin(), sizeMatches.cend(), intersections.begin());

        std::set<Type> sizeAndTypeMatches(intersections.begin(), intersectionsEnd);
        intersections.resize(Math::min(sizeAndTypeMatches.size(), colMatches.size()));
        intersectionsEnd = std::set_intersection(sizeAndTypeMatches.cbegin(), sizeAndTypeMatches.cend()
            , colMatches.cbegin(), colMatches.cend(), intersections.begin());

        uint64 intersectionCount = intersectionsEnd - intersections.begin();
        debugAssert(intersectionCount == 1);

        return intersections[0];
    }
}
