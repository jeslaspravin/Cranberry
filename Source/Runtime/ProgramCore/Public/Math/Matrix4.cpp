/*!
 * \file Matrix4.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

const Matrix4 Matrix4::IDENTITY{ Vector3(1, 0), Vector3(0, 1), Vector3(0, 0, 1), Vector3(0) };

Matrix4::Matrix4(const Vector3 &c1, const Vector3 &c2, const Vector3 &c3, const Vector3 &c4, float c4w /*=1.0f*/)
    : value(c1.x(), c1.y(), c1.z(), 0, c2.x(), c2.y(), c2.z(), 0, c3.x(), c3.y(), c3.z(), 0, c4.x(), c4.y(), c4.z(), c4w)
{}

Matrix4::Matrix4(const Vector4 &c1, const Vector4 &c2, const Vector4 &c3, const Vector4 &c4)
    : value(c1.x(), c1.y(), c1.z(), c1.w(), c2.x(), c2.y(), c2.z(), c2.w(), c3.x(), c3.y(), c3.z(), c3.w(), c4.x(), c4.y(), c4.z(), c4.w())
{}

Matrix4::Matrix4(
    float c1x, float c1y, float c1z, float c1w, float c2x, float c2y, float c2z, float c2w, float c3x, float c3y, float c3z, float c3w,
    float c4x, float c4y, float c4z, float c4w
)
    : value(c1x, c1y, c1z, c1w, c2x, c2y, c2z, c2w, c3x, c3y, c3z, c3w, c4x, c4y, c4z, c4w)
{}

Matrix4::Matrix4(const Vector3 &scale)
    : value(scale.x(), 0, 0, 0, 0, scale.y(), 0, 0, 0, 0, scale.z(), 0, 0, 0, 0, 1)
{}

Vector4 Matrix4::operator* (const Vector4 &transformingVector) const { return Vector4(value * transformingVector.value); }

Vector3 Matrix4::operator* (const Vector3 &transformingVector) const
{
    Vector4 Vector4(transformingVector.x(), transformingVector.y(), transformingVector.z(), 1.0f);
    Vector4 = (*this) * Vector4;
    return Vector3(Vector4.x(), Vector4.y(), Vector4.z()) / Vector4.w();
}