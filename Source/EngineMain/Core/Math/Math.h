#pragma once

#include "CoreMathTypedefs.h"
#include "../Types/CoreDefines.h"

#include <glm/common.hpp>
#include <glm/exponential.hpp>

class Vector3D;
class Vector2D;

class Math
{
private:
    Math() = default;

public:
    template <typename ClampType>
    FORCEINLINE static ClampType clamp(const ClampType& value, const ClampType& min, const ClampType& max)
    {
        return glm::clamp(value, min, max);
    }

    template <typename Type>
    FORCEINLINE static Type min(const Type& a, const Type& b)
    {
        return glm::min(a,b);
    }

    template <typename Type>
    FORCEINLINE static Type max(const Type& a, const Type& b)
    {
        return glm::max(a, b);
    }

    template <typename Type>
    FORCEINLINE static Type abs(const Type& value)
    {
        return glm::abs(value);
    }

    template <typename Type>
    FORCEINLINE static Type floor(const Type& value)
    {
        return glm::floor(value);
    }

    template <typename Type>
    FORCEINLINE static Type ceil(const Type& value)
    {
        return glm::ceil(value);
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename Type>
    FORCEINLINE static Type pow2(const Type& base)
    {
        return glm::pow(base,2);
    }

    template <typename BaseType, typename PowType>
    FORCEINLINE static BaseType pow(const BaseType& base,const PowType& power)
    {
        return glm::pow(base, power);
    }

    template <typename Type>
    FORCEINLINE static Type exp2(const Type& value)
    {
        return glm::exp2(value);
    }

    template <typename Type>
    FORCEINLINE static Type log2(const Type& value)
    {
        return glm::log2(value);
    }

    template <typename Type>
    FORCEINLINE static Type log(const Type& value)
    {
        return glm::log(value);
    }

    template <typename Type>
    FORCEINLINE static Type exp(const Type& value)
    {
        return glm::exp(value);
    }

    template <typename Type>
    FORCEINLINE static Type sqrt(const Type& value)
    {
        return glm::sqrt(value);
    }

    template <typename Type>
    FORCEINLINE static Type invSqrt(const Type& value)
    {
        return glm::inversesqrt(value);
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename Type>
    FORCEINLINE static std::enable_if_t<std::is_integral_v<Type>, bool> isEqual(const Type& a, const Type& b, Type epsilon = 0)
    {
        return abs(a - b) <= epsilon;
    }

    template <typename Type>
    FORCEINLINE static std::enable_if_t<std::is_floating_point_v<Type>, bool> isEqual(const Type& a, const Type& b, Type epsilon = SMALL_EPSILON)
    {
        return abs(a - b) <= epsilon;
    }

    static bool isEqual(const Vector3D& a, const Vector3D& b, float epsilon = SMALL_EPSILON);
    static bool isEqual(const Vector2D& a, const Vector2D& b, float epsilon = SMALL_EPSILON);
};
