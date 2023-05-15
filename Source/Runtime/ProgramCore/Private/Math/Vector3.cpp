/*!
 * \file Vector3.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Vector3.h"
#include "Math/Math.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"

const Vector3 Vector3::RIGHT(0, 1);
const Vector3 Vector3::FWD(1, 0);
const Vector3 Vector3::UP(0, 0, 1);
const Vector3 Vector3::ZERO(0);
const Vector3 Vector3::ONE(1);

Vector3::Vector3(const Vector4 &other)
    : value(other.x(), other.y(), other.z())
{}

Vector3::Vector3(const Vector2 &xy, float z)
    : value(xy.x(), xy.y(), z)
{}

bool Vector3::isSame(const Vector3 &b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon)
           && Math::isEqual(value.z, b.value.z, epsilon);
}

bool Vector3::isFinite() const { return Math::isFinite(value.x) && Math::isFinite(value.y) && Math::isFinite(value.z); }

Vector3 Vector3::safeInverse() const
{
    return Vector3(
        Math::isEqual(value.x, 0.0f) ? 0.0f : 1.0f / value.x, Math::isEqual(value.y, 0.0f) ? 0.0f : 1.0f / value.y,
        Math::isEqual(value.z, 0.0f) ? 0.0f : 1.0f / value.z
    );
}

Vector3 Vector3::safeNormalized(float threshold /*= SMALL_EPSILON*/) const
{
    float sqrLen = sqrlength();
    if (sqrLen < threshold)
    {
        return Vector3::ZERO;
    }
    return Vector3(value * Math::invSqrt(sqrLen));
}