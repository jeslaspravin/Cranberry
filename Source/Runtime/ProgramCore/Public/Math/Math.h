/*!
 * \file Math.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ProgramCoreExports.h"
#include "Math/CoreMathTypedefs.h"
#include "Types/CoreDefines.h"
#include "Types/Templates/TypeTraits.h"

#include <cmath>

GLM_HEADER_INCLUDES_BEGIN

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/trigonometric.hpp>

GLM_HEADER_INCLUDES_END

class Vector3D;
class Vector2D;
class Vector4D;
class Rotation;
class Quat;
namespace std
{
class random_device;
}

template <typename Type>
concept CustomMathTypes = std::disjunction_v<
    std::is_same<Type, Vector2D>, std::is_same<Type, Vector3D>, std::is_same<Type, Vector4D>, std::is_same<Type, Rotation>,
    std::is_same<Type, Quat>>;

// Types that can be a vector in certain coordinate system
template <typename Type>
concept VectorTypes
    = std::disjunction_v<std::is_same<Type, Vector2D>, std::is_same<Type, Vector3D>, std::is_same<Type, Vector4D>, std::is_same<Type, Quat>>;

template <typename Type, typename Enable = void>
class MathHelper;

template <typename Type>
requires (!CustomMathTypes<Type>)
class MathHelper<Type>
{
    friend class Math;

private:
    FORCE_INLINE static Type clamp(const Type &value, const Type &min, const Type &max) { return glm::clamp(value, min, max); }

    FORCE_INLINE static Type min(const Type &a, const Type &b) { return glm::min(a, b); }

    FORCE_INLINE static Type max(const Type &a, const Type &b) { return glm::max(a, b); }

    FORCE_INLINE static Type abs(const Type &value) { return glm::abs(value); }

    FORCE_INLINE static Type frac(const Type &value) { return glm::fract(value); }

    FORCE_INLINE static Type floor(const Type &value) { return glm::floor(value); }

    FORCE_INLINE static Type ceil(const Type &value) { return glm::ceil(value); }

    FORCE_INLINE static Type round(const Type &value) { return glm::round(value); }

    template <typename T>
    FORCE_INLINE static Type mod(const Type &a, const T &b)
    {
        return glm::mod(a, b);
    }

    FORCE_INLINE static Type modf(Type &wholePart, const Type &value) { return glm::modf(value, wholePart); }

    // Non math helpers

    FORCE_INLINE static bool isEqual(const Type &a, const Type &b, Type epsilon) { return abs(a - b) <= epsilon; }

    FORCE_INLINE static bool isFinite(const Type &value) { return IS_FINITE(value); }
};

template <CustomMathTypes Type>
class MathHelper<Type>
{
    friend class Math;

private:
    FORCE_INLINE static Type clamp(const Type &value, const Type &min, const Type &max) { return Type::clamp(value, min, max); }

    FORCE_INLINE static Type min(const Type &a, const Type &b) { return Type::min(a, b); }

    FORCE_INLINE static Type max(const Type &a, const Type &b) { return Type::max(a, b); }

    FORCE_INLINE static Type abs(const Type &value) { return Type::abs(value); }

    FORCE_INLINE static Type frac(const Type &value) { return Type::fract(value); }

    FORCE_INLINE static Type floor(const Type &value) { return Type::floor(value); }

    FORCE_INLINE static Type ceil(const Type &value) { return Type::ceil(value); }

    FORCE_INLINE static Type round(const Type &value) { return Type::round(value); }

    template <typename T>
    FORCE_INLINE static Type mod(const Type &a, const T &b)
    {
        return Type::mod(a, b);
    }

    FORCE_INLINE static Type modf(Type &wholePart, const Type &value) { return Type::modf(value, wholePart); }

    // Non math helpers

    static bool isEqual(const Type &a, const Type &b, float epsilon) { return a.isSame(b, epsilon); }

    FORCE_INLINE static bool isFinite(const Type &value) { return value.isFinite(); }
};

class PROGRAMCORE_EXPORT Math
{
public:
    template <typename ClampType, typename ClampType1, typename ClampType2>
    FORCE_INLINE static ClampType clamp(const ClampType &value, const ClampType1 &min, const ClampType2 &max)
    {
        return clampInternal<ClampType, ClampType1, ClampType2>(value, min, max);
    }

    template <typename Type1, typename Type2, typename T = std::common_type_t<Type1, Type2>>
    FORCE_INLINE static T min(const Type1 &a, const Type2 &b)
    {
        return minInternal<Type1, Type2>(a, b);
    }

    template <typename Type1, typename Type2, typename Type3, typename T = std::common_type_t<Type1, Type2, Type3>>
    FORCE_INLINE static T min(const Type1 &a, const Type2 &b, const Type3 &c)
    {
        return min(min(a, b), c);
    }

    template <typename Type1, typename Type2, typename T = std::common_type_t<Type1, Type2>>
    FORCE_INLINE static T max(const Type1 &a, const Type2 &b)
    {
        return maxInternal<Type1, Type2>(a, b);
    }

    template <typename Type1, typename Type2, typename Type3, typename T = std::common_type_t<Type1, Type2, Type3>>
    FORCE_INLINE static T max(const Type1 &a, const Type2 &b, const Type3 &c)
    {
        return max(max(a, b), c);
    }

    template <typename Type>
    FORCE_INLINE static Type abs(const Type &value)
    {
        return absInternal<Type>(value);
    }

    template <typename Type>
    FORCE_INLINE static Type frac(const Type &value)
    {
        return fracInternal<Type>(value);
    }

    template <typename Type>
    FORCE_INLINE static Type floor(const Type &value)
    {
        return floorInternal<Type>(value);
    }

    template <typename Type>
    FORCE_INLINE static Type ceil(const Type &value)
    {
        return ceilInternal<Type>(value);
    }

    template <typename Type>
    FORCE_INLINE static Type round(const Type &value)
    {
        return roundInternal<Type>(value);
    }

    template <typename Type1, typename Type2, typename T = std::common_type_t<Type1, Type2>>
    FORCE_INLINE static T mod(const Type1 &a, const Type2 &b)
    {
        return modInternal<Type1, Type2>(a, b);
    }

    template <typename Type1, typename Type2, typename T = std::common_type_t<Type1, Type2>>
    FORCE_INLINE static T modf(Type1 &wholePart, const Type2 &value)
    {
        return modfInternal<Type1, Type2>(wholePart, value);
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename Type, typename T = std::common_type_t<Type, uint32>>
    FORCE_INLINE static T pow2(const Type &base)
    {
        return T(glm::pow(base, 2u));
    }

    template <typename BaseType, typename PowType, typename T = std::common_type_t<BaseType, PowType>>
    FORCE_INLINE static T pow(const BaseType &base, const PowType &power)
    {
        return T(glm::pow(base, power));
    }

    template <typename Type>
    FORCE_INLINE static Type exp2(const Type &value)
    {
        return glm::exp2(value);
    }

    template <typename Type, typename T = std::conditional_t<std::is_floating_point_v<Type>, Type, float>>
    FORCE_INLINE static T log2(const Type &value)
    {
        return glm::log2(T(value));
    }

    template <typename Type, typename T = std::conditional_t<std::is_floating_point_v<Type>, Type, float>>
    FORCE_INLINE static T log(const Type &value)
    {
        return glm::log(T(value));
    }

    template <typename Type>
    FORCE_INLINE static Type exp(const Type &value)
    {
        return glm::exp(value);
    }

    template <typename Type>
    FORCE_INLINE static Type sqrt(const Type &value)
    {
        return glm::sqrt(value);
    }

    template <typename Type>
    FORCE_INLINE static Type invSqrt(const Type &value)
    {
        return glm::inversesqrt(value);
    }

    template <typename Type>
    FORCE_INLINE static Type deg2Rad(const Type &value)
    {
        return glm::radians(value);
    }
    template <typename Type>
    FORCE_INLINE static Type rad2Deg(const Type &value)
    {
        return glm::degrees(value);
    }

    template <typename Type>
    FORCE_INLINE static Type sin(const Type &value)
    {
        return glm::sin(value);
    }

    template <typename Type>
    FORCE_INLINE static Type cos(const Type &value)
    {
        return glm::cos(value);
    }

    template <typename Type>
    FORCE_INLINE static Type tan(const Type &value)
    {
        return glm::tan(value);
    }

    template <typename Type>
    FORCE_INLINE static Type asin(const Type &value)
    {
        return glm::asin(value);
    }

    template <typename Type>
    FORCE_INLINE static Type acos(const Type &value)
    {
        return glm::acos(value);
    }

    template <typename Type>
    FORCE_INLINE static Type atan(const Type &value)
    {
        return glm::atan(value);
    }

    template <typename Type1, typename Type2, typename T = std::common_type_t<Type1, Type2>>
    FORCE_INLINE static T atan(const Type1 &numerator, const Type2 &denominator)
    {
        return glm::atan(T(numerator), T(denominator));
    }

    /* Uniform between 0 to 1 */
    static float random();

    template <std::unsigned_integral Type>
    CONST_EXPR static bool isPowOf2(Type value)
    {
        return ((value - 1) & value) == 0;
    }
    // Converts to higher power of two, in 3 gets converted to 4
    template <std::unsigned_integral Type>
    static Type toHigherPowOf2(Type value)
    {
        return Math::pow(2u, Type(Math::ceil(Math::log2(value))));
    }
    // Converts to higher power of two, in 3 gets converted to 4
    template <std::unsigned_integral Type>
    static Type toLowerPowOf2(Type value)
    {
        return Math::pow(2u, Type(Math::floor(Math::log2(value))));
    }
    template <std::unsigned_integral Type>
    CONST_EXPR static Type alignBy2(Type value)
    {
        return (value + 1u) & ~1u;
    }
    // AlignVal has to be a power of 2, Undefined behavior if not power of 2
    template <std::unsigned_integral Type1, std::unsigned_integral Type2, typename Type = std::common_type_t<Type1, Type2>>
    CONST_EXPR static Type alignByUnsafe(Type1 value, Type2 alignVal)
    {
        return ((Type)value + (Type)alignVal - 1) & ~((Type)alignVal - 1);
    }
    template <std::unsigned_integral Type1, std::unsigned_integral Type2, typename Type = std::common_type_t<Type1, Type2>>
    static Type alignBy(Type1 value, Type2 alignVal)
    {
        Type roundedToPow2 = toHigherPowOf2(alignVal);
        return alignByUnsafe(value, roundedToPow2);
    }
    template <std::unsigned_integral Type1, std::unsigned_integral Type2, typename Type = std::common_type_t<Type1, Type2>>
    CONST_EXPR static bool isAligned(Type1 value, Type2 alignVal)
    {
        return ((Type)value & ((Type)alignVal - 1)) == 0;
    }

    template <std::integral Type>
    FORCE_INLINE static bool isEqual(const Type &a, const Type &b, Type epsilon)
    {
        return MathHelper<Type>::isEqual(a, b, epsilon);
    }
    template <std::integral Type>
    FORCE_INLINE static bool isEqual(const Type &a, const Type &b)
    {
        return a == b;
    }

    template <NotAnIntegral Type>
    FORCE_INLINE static bool isEqual(const Type &a, const Type &b, Type epsilon = SMALL_EPSILON)
    {
        return MathHelper<Type>::isEqual(a, b, epsilon);
    }

    template <typename Type>
    FORCE_INLINE static bool isFinite(const Type &value)
    {
        if CONST_EXPR (std::is_integral_v<Type>)
        {
            return true;
        }
        return MathHelper<Type>::isFinite(value);
    }

    // Rotation specializations
    static Rotation deg2Rad(const Rotation &value);
    static Rotation rad2Deg(const Rotation &value);
    static Rotation sin(const Rotation &value);
    static Rotation cos(const Rotation &value);
    static Rotation tan(const Rotation &value);
    static Rotation asin(const Rotation &value);
    static Rotation acos(const Rotation &value);
    static Rotation atan(const Rotation &value);

private:
    Math() = default;
    static std::random_device rDevice;

    template <typename ClampType, typename ClampType1, typename ClampType2>
    requires TypeConvertibleTo<ClampType, ClampType1, ClampType2>
    FORCE_INLINE static ClampType clampInternal(const ClampType &value, const ClampType1 &min, const ClampType2 &max)
    {
        return MathHelper<ClampType>::clamp(value, ClampType(min), ClampType(max));
    }

    template <typename Type1, typename Type2, typename T = std::common_type_t<Type1, Type2>>
    FORCE_INLINE static T minInternal(const Type1 &a, const Type2 &b)
    {
        return MathHelper<T>::min(T(a), T(b));
    }

    template <typename Type1, typename Type2, typename T = std::common_type_t<Type1, Type2>>
    FORCE_INLINE static T maxInternal(const Type1 &a, const Type2 &b)
    {
        return MathHelper<T>::max(T(a), T(b));
    }

    template <typename Type>
    FORCE_INLINE static Type absInternal(const Type &value)
    {
        return MathHelper<Type>::abs(value);
    }

    template <typename Type>
    FORCE_INLINE static Type fracInternal(const Type &value)
    {
        return MathHelper<Type>::frac(value);
    }

    template <typename Type>
    FORCE_INLINE static Type floorInternal(const Type &value)
    {
        return MathHelper<Type>::floor(value);
    }

    template <typename Type>
    FORCE_INLINE static Type ceilInternal(const Type &value)
    {
        return MathHelper<Type>::ceil(value);
    }

    template <typename Type>
    FORCE_INLINE static Type roundInternal(const Type &value)
    {
        return MathHelper<Type>::round(value);
    }

    template <typename Type1, typename Type2, typename T = std::common_type_t<Type1, Type2>>
    FORCE_INLINE static Type1 modInternal(const Type1 &a, const Type2 &b)
    {
        return MathHelper<T>::mod(T(a), T(b));
    }

    template <typename Type1, typename Type2, typename T = std::common_type_t<Type1, Type2>>
    FORCE_INLINE static T modfInternal(Type1 &wholePart, const Type2 &value)
    {
        return MathHelper<T>::modf(wholePart, value);
    }
};