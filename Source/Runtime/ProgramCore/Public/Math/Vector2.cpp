/*!
 * \file Vector2.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Vector2.h"
#include "Math/Math.h"
#include "Math/Vector3.h"

const Vector2 Vector2::RIGHT(0, 1);
const Vector2 Vector2::FWD(1, 0);
const Vector2 Vector2::ZERO(0);
const Vector2 Vector2::ONE(1);

Vector2::Vector2(const Vector3 &other)
    : value(other.x(), other.y())
{}

bool Vector2::isSame(const Vector2 &b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon);
}

bool Vector2::isFinite() const { return Math::isFinite(value.x) && Math::isFinite(value.y); }
bool Vector2::isNan() const { return Math::isNan(value.x) || Math::isNan(value.y); }

Vector2 Vector2::safeInverse() const
{
    return Vector2(Math::isEqual(value.x, 0.0f) ? 0.0f : 1.0f / value.x, Math::isEqual(value.y, 0.0f) ? 0.0f : 1.0f / value.y);
}

Vector2 Vector2::safeNormalized(float threshold /*= SMALL_EPSILON*/) const
{
    float sqrLen = sqrlength();
    if (sqrLen < threshold)
    {
        return Vector2::ZERO;
    }
    return Vector2(value * Math::invSqrt(sqrLen));
}