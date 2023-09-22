/*!
 * \file Matrix3.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Matrix3.h"
#include "Math/Vector3.h"

const Matrix3 Matrix3::IDENTITY{ Vector3(1, 0), Vector3(0, 1), Vector3(0, 0, 1) };

Matrix3::Matrix3(const Vector3 &c1, const Vector3 &c2, const Vector3 &c3)
    : value(c1.x(), c1.y(), c1.z(), c2.x(), c2.y(), c2.z(), c3.x(), c3.y(), c3.z())
{}

Matrix3::Matrix3(float c1x, float c1y, float c1z, float c2x, float c2y, float c2z, float c3x, float c3y, float c3z)
    : value(c1x, c1y, c1z, c2x, c2y, c2z, c3x, c3y, c3z)
{}

Matrix3::Matrix3(const Vector3 &scale)
    : value(scale.x(), 0, 0, 0, scale.y(), 0, 0, 0, scale.z())
{}

Vector3 Matrix3::operator* (const Vector3 &transformingVector) const { return Vector3(value * transformingVector.value); }
