/*!
 * \file MathEasing.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */
#pragma once

#include "Math/Math.h"
#include "Types/Platform/PlatformAssertionErrors.h"

class PROGRAMCORE_EXPORT MathEasing
{
private:
    MathEasing() = default;

public:
    template <typename Type>
    static Type lerp(const Type &a, const Type &b, float t)
    {
        return a * (1 - t) + b * t;
    }

    // http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/index.html
    template <VectorTypes Type>
    static Type slerp(const Type &a, const Type &b, float t)
    {
        // Ensure we are operating in unit vectors
        debugAssert(Math::isEqual(a.sqrlength(), 1.0f, SLIGHTLY_SMALL_EPSILON) && Math::isEqual(b.sqrlength(), 1.0f, SLIGHTLY_SMALL_EPSILON));

        float dotVal = Type::dot(a, b);
        // Magic number as LERP vs SLERP are mush less noticeable in such a small delta
        // http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/index.html
        const float DOT_THRESHOLD = 0.9995;
        if (dotVal > DOT_THRESHOLD)
        {
            Type retVal = lerp(a, b, t);
            return retVal.normalized();
        }

        // We do not have to clamp as input is restricted to unit vector
        // dotVal = Math::clamp(dotVal, -1.0f, 1.0f);

        // Planar angle delta between two vectors
        float theta0 = Math::acos(dotVal);
        float theta = theta0 * t;

        // Finding vector(y-axis) perpendicular to 'a'(assuming as x-axis) in plane formed among origin,
        // 'a', 'b' by rejection
        Type yAxis = (b - (a * dotVal)).normalized();

        return a * Math::cos(theta) + yAxis * Math::sin(theta);
    }

    // Following are some easing functions, Each function has an index number corresponding to function
    // in graph https://www.desmos.com/calculator/km86swmxft or here https://easings.net/ Index : 1
    FORCE_INLINE static float quadraticIn(float t) { return t * t; }
    // Index : 2
    FORCE_INLINE static float quadraticOut(float t) { return t * (2.0f - t); }
    // http://www.demofox.org/bezquad1d.html
    // C acts as middle control point in quadratic curve
    // Index : 3
    FORCE_INLINE static float quadraticCurve(float t, float c) { return 2 * c * t * (1 - t) + quadraticIn(t); }
    // Index : 4
    FORCE_INLINE static float cubicIn(float t) { return t * t * t; }
    // Index : 5
    FORCE_INLINE static float cubicOut(float t) { return 1 + cubicIn(t - 1); }
    // Index : 6
    FORCE_INLINE static float quarticIn(float t) { return t * t * t * t; }
    // Index : 7
    FORCE_INLINE static float quarticOut(float t) { return 1 - quarticIn(t - 1); }
    // Index : 8
    FORCE_INLINE static float quinticIn(float t) { return t * t * t * t * t; }
    // Index : 9
    FORCE_INLINE static float quinticOut(float t) { return 1.0f + quinticIn(t - 1); }
    // Index : 10
    FORCE_INLINE static float sineIn(float t) { return 1 - Math::cos(t * 0.5f * PI); }
    // Index : 11
    FORCE_INLINE static float sineOut(float t) { return Math::sin(t * 0.5f * PI); }
    // Index : 12
    FORCE_INLINE static float expIn(float t) { return (t == 0.0f) ? 0.0f : Math::pow(1024, t - 1); }
    // Index : 13
    FORCE_INLINE static float expOut(float t) { return Math::isEqual(t, 1.0f) ? 1.0f : (1 - Math::pow(2, -10 * t)); }
    // Index : 14
    FORCE_INLINE static float circularIn(float t) { return 1.0f - Math::sqrt(1 - t * t); }
    // Index : 15
    FORCE_INLINE static float circularOut(float t)
    {
        t -= 1;
        return Math::sqrt(1 - t * t);
    }
    // Index : 16
    FORCE_INLINE static float elasticIn(float t)
    {
        if (t == 0.0f || Math::isEqual(t, 1.0f))
        {
            return Math::ceil(t);
        }
        return -Math::pow(2.0f, 10.0f * (t - 1)) * Math::sin((t - 1.1f) * PI / 0.2f);
    }
    // Index : 17
    FORCE_INLINE static float elasticOut(float t)
    {
        if (t == 0.0f || Math::isEqual(t, 1.0f))
        {
            return Math::ceil(t);
        }
        return 1.0f + Math::pow(2.0f, -10.0f * t) * Math::sin((t - 0.1f) * PI / 0.2f);
    }

#define BACK_S 1.70158f
#define BACK_S2 2.5949095f;
    // Index : 18
    FORCE_INLINE static float backIn(float t) { return t * t * ((BACK_S + 1.0f) * t - BACK_S); }
    // Index : 19
    FORCE_INLINE static float backOut(float t)
    {
        t -= 1.0f;
        return 1.0f + t * t * ((BACK_S + 1.0f) * t + BACK_S);
    }
#undef BACK_S
#undef BACK_S2

#define BOUNCE_0 7.5625f
#define BOUNCE_1 (1.0f / 2.75f)
#define BOUNCE_C1 0.75f
#define BOUNCE_2 (1.5f * BOUNCE_1)
#define BOUNCE_3 (2.0f * BOUNCE_1)
#define BOUNCE_C2 0.9375f
#define BOUNCE_4 (2.25f * BOUNCE_1)
#define BOUNCE_5 (2.5f * BOUNCE_1)
#define BOUNCE_C3 0.984375f
#define BOUNCE_6 (2.625f * BOUNCE_1)
    // Index : 20
    FORCE_INLINE static float bounceIn(float t) { return 1.0f - bounceOut(1 - t); }
    // Index : 21
    FORCE_INLINE static float bounceOut(float t)
    {
        if (t < BOUNCE_1)
        {
            return BOUNCE_0 * t * t;
        }
        else if (t < BOUNCE_3)
        {
            t -= BOUNCE_2;
            return BOUNCE_0 * t * t + BOUNCE_C1;
        }
        else if (t < BOUNCE_5)
        {
            t -= BOUNCE_4;
            return BOUNCE_0 * t * t + BOUNCE_C2;
        }
        else
        {
            t -= BOUNCE_6;
            return BOUNCE_0 * t * t + BOUNCE_C3;
        }
    }
#undef BOUNCE_0
#undef BOUNCE_1
#undef BOUNCE_C1
#undef BOUNCE_2
#undef BOUNCE_3
#undef BOUNCE_C2
#undef BOUNCE_4
#undef BOUNCE_5
#undef BOUNCE_C1
#undef BOUNCE_6
};