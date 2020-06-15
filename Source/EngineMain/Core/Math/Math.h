#pragma once

#include "CoreMathTypedefs.h"
#include "../Types/CoreDefines.h"

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/trigonometric.hpp>

class Vector3D;
class Vector2D;
class Vector4D;
class Rotation;

class Math
{
private:
    Math() = default;

public:
    template <typename ClampType>
    FORCE_INLINE static ClampType clamp(const ClampType& value, const ClampType& min, const ClampType& max)
    {
        return glm::clamp(value, min, max);
    }

    template <typename Type>
    FORCE_INLINE static Type min(const Type& a, const Type& b)
    {
        return glm::min(a,b);
    }

    template <typename Type>
    FORCE_INLINE static Type max(const Type& a, const Type& b)
    {
        return glm::max(a, b);
    }

    template <typename Type>
    FORCE_INLINE static Type abs(const Type& value)
    {
        return glm::abs(value);
    }

    template <typename Type>
    FORCE_INLINE static Type frac(const Type& value)
    {
        return glm::fract(value);
    }

    template <typename Type>
    FORCE_INLINE static Type floor(const Type& value)
    {
        return glm::floor(value);
    }

    template <typename Type>
    FORCE_INLINE static Type ceil(const Type& value)
    {
        return glm::ceil(value);
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename Type>
    FORCE_INLINE static float pow2(const Type& base)
    {
        return float(glm::pow(base,2));
    }

    template <typename BaseType, typename PowType>
    FORCE_INLINE static float pow(const BaseType& base,const PowType& power)
    {
        return float(glm::pow(base, power));
    }

    template <typename Type>
    FORCE_INLINE static Type exp2(const Type& value)
    {
        return glm::exp2(value);
    }

    template <typename Type>
    FORCE_INLINE static Type log2(const Type& value)
    {
        return glm::log2(value);
    }

    template <typename Type>
    FORCE_INLINE static Type log(const Type& value)
    {
        return glm::log(value);
    }

    template <typename Type>
    FORCE_INLINE static Type exp(const Type& value)
    {
        return glm::exp(value);
    }

    template <typename Type>
    FORCE_INLINE static Type sqrt(const Type& value)
    {
        return glm::sqrt(value);
    }

    template <typename Type>
    FORCE_INLINE static Type invSqrt(const Type& value)
    {
        return glm::inversesqrt(value);
    }

    template <typename Type>
    FORCE_INLINE static Type deg2Rad(const Type& value)
    {
        return glm::radians(value);
    }
    template <typename Type>
    FORCE_INLINE static Type rad2Deg(const Type& value)
    {
        return glm::degrees(value);
    }

    template <typename Type>
    FORCE_INLINE static Type sin(const Type& value)
    {
        return glm::sin(value);
    }

    template <typename Type>
    FORCE_INLINE static Type cos(const Type& value)
    {
        return glm::cos(value);
    }

    template <typename Type>
    FORCE_INLINE static Type tan(const Type& value)
    {
        return glm::tan(value);
    }

    template <typename Type>
    FORCE_INLINE static Type asin(const Type& value)
    {
        return glm::asin(value);
    }

    template <typename Type>
    FORCE_INLINE static Type acos(const Type& value)
    {
        return glm::acos(value);
    }

    template <typename Type>
    FORCE_INLINE static Type atan(const Type& value)
    {
        return glm::atan(value);
    }

    template <typename Type>
    FORCE_INLINE static Type atan(const Type& numerator, const Type& denominator)
    {
        return glm::atan(numerator,denominator);
    }

    // Rotation specializations
    static Rotation deg2Rad(const Rotation& value);
    static Rotation rad2Deg(const Rotation& value);
    static Rotation sin(const Rotation& value);
    static Rotation cos(const Rotation& value);
    static Rotation tan(const Rotation& value);
    static Rotation asin(const Rotation& value);
    static Rotation acos(const Rotation& value);
    static Rotation atan(const Rotation& value);

    // Vector2D specializations
    static Vector2D clamp(const Vector2D& value, const Vector2D& min, const Vector2D& max);
    static Vector2D min(const Vector2D& a, const Vector2D& b);
    static Vector2D max(const Vector2D& a, const Vector2D& b);
    static Vector2D abs(const Vector2D& value);
    static Vector2D floor(const Vector2D& value);
    static Vector2D ceil(const Vector2D& value);
    // Vector3D specializations
    static Vector3D clamp(const Vector3D& value, const Vector3D& min, const Vector3D& max);
    static Vector3D min(const Vector3D& a, const Vector3D& b);
    static Vector3D max(const Vector3D& a, const Vector3D& b);
    static Vector3D abs(const Vector3D& value);
    static Vector3D floor(const Vector3D& value);
    static Vector3D ceil(const Vector3D& value);
    // Vector4D specializations
    static Vector4D clamp(const Vector4D& value, const Vector4D& min, const Vector4D& max);
    static Vector4D min(const Vector4D& a, const Vector4D& b);
    static Vector4D max(const Vector4D& a, const Vector4D& b);
    static Vector4D abs(const Vector4D& value);
    static Vector4D floor(const Vector4D& value);
    static Vector4D ceil(const Vector4D& value);

    //////////////////////////////////////////////////////////////////////////

    template <typename Type>
    FORCE_INLINE static std::enable_if_t<std::is_integral_v<Type>, bool> isEqual(const Type& a, const Type& b, Type epsilon = 0)
    {
        return abs(a - b) <= epsilon;
    }

    template <typename Type>
    FORCE_INLINE static std::enable_if_t<std::is_floating_point_v<Type>, bool> isEqual(const Type& a, const Type& b, Type epsilon = SMALL_EPSILON)
    {
        return abs(a - b) <= epsilon;
    }

    static bool isEqual(const Vector2D& a, const Vector2D& b, float epsilon = SMALL_EPSILON);
    static bool isEqual(const Vector3D& a, const Vector3D& b, float epsilon = SMALL_EPSILON);
    static bool isEqual(const Vector4D& a, const Vector4D& b, float epsilon = SMALL_EPSILON);
    static bool isEqual(const Rotation& a, const Rotation& b, float epsilon = SMALL_EPSILON);
};
