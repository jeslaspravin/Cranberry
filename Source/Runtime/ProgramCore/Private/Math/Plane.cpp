/*!
 * \file Plane.cpp
 *
 * \author Jeslas
 * \date June 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Plane.h"
#include "Math/Math.h"

const Plane Plane::XZ(Vector3::RIGHT, 0);
const Plane Plane::YZ(Vector3::FWD, 0);
const Plane Plane::XY(Vector3::UP, 0);

bool Plane::isSame(const Plane &b, float epsilon /*= SMALL_EPSILON*/) const { return Math::isEqual(value, b.value, epsilon); }

bool Plane::isFinite() const { return Math::isFinite(value); }

Plane Plane::safeNormalized(float threshold /*= SMALL_EPSILON*/) const
{
    float sqrLen = value.sqrlength3();
    if (sqrLen < threshold)
    {
        return Plane::XY;
    }
    return Plane(value * Math::invSqrt(sqrLen));
}
