/*!
 * \file Vector4.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Vector4.h"
#include "Math/Math.h"
#include "Math/Vector3.h"

const Vector4 Vector4::ZERO(0);
const Vector4 Vector4::ONE(1);

Vector4::Vector4(const Vector3 &xyz, float w)
    : value(xyz.x(), xyz.y(), xyz.z(), w)
{}

bool Vector4::isSame(const Vector4 &b, float epsilon /*= SMALL_EPSILON*/) const
{
    return Math::isEqual(value.x, b.value.x, epsilon) && Math::isEqual(value.y, b.value.y, epsilon)
           && Math::isEqual(value.z, b.value.z, epsilon);
}

bool Vector4::isFinite() const
{
    return Math::isFinite(value.x) && Math::isFinite(value.y) && Math::isFinite(value.z) && Math::isFinite(value.w);
}
bool Vector4::isNan() const { return Math::isNan(value.x) || Math::isNan(value.y) || Math::isNan(value.z) || Math::isNan(value.w); }

Vector4 Vector4::safeInverse() const
{
    return Vector4(
        Math::isEqual(value.x, 0.0f) ? 0.0f : 1.0f / value.x, Math::isEqual(value.y, 0.0f) ? 0.0f : 1.0f / value.y,
        Math::isEqual(value.z, 0.0f) ? 0.0f : 1.0f / value.z, Math::isEqual(value.w, 0.0f) ? 0.0f : 1.0f / value.w
    );
}

Vector4 Vector4::safeNormalized(float threshold /*= SMALL_EPSILON*/) const
{
    float sqrLen = sqrlength();
    if (sqrLen < threshold)
    {
        return Vector4::ZERO;
    }
    return Vector4(value * Math::invSqrt(sqrLen));
}