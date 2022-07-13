/*!
 * \file Matrix4.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Matrix4.h"
#include "Math/Vector3D.h"
#include "Math/Vector4D.h"

Matrix4::Matrix4(const glm::mat4 &matrix)
    : value(matrix)
{}

Matrix4::Matrix4()
    : value(0)
{}

Matrix4::Matrix4(float allValue)
    : value(allValue)
{}

Matrix4::Matrix4(const Vector3D &c1, const Vector3D &c2, const Vector3D &c3, const Vector3D &c4, float c4w /*=1.0f*/)
    : value(c1.x(), c1.y(), c1.z(), 0, c2.x(), c2.y(), c2.z(), 0, c3.x(), c3.y(), c3.z(), 0, c4.x(), c4.y(), c4.z(), c4w)
{}

Matrix4::Matrix4(const Matrix4 &other)
    : value(other.value)
{}

Matrix4::Matrix4(Matrix4 &&other)
    : value(std::move(other.value))
{}

Matrix4::Matrix4(const Vector4D &c1, const Vector4D &c2, const Vector4D &c3, const Vector4D &c4)
    : value(c1.x(), c1.y(), c1.z(), c1.w(), c2.x(), c2.y(), c2.z(), c2.w(), c3.x(), c3.y(), c3.z(), c3.w(), c4.x(), c4.y(), c4.z(), c4.w())
{}

Matrix4::Matrix4(
    float c1x, float c1y, float c1z, float c1w, float c2x, float c2y, float c2z, float c2w, float c3x, float c3y, float c3z, float c3w,
    float c4x, float c4y, float c4z, float c4w
)
    : value(c1x, c1y, c1z, c1w, c2x, c2y, c2z, c2w, c3x, c3y, c3z, c3w, c4x, c4y, c4z, c4w)
{}

Matrix4::Matrix4(const Vector3D &scale)
    : value(scale.x(), 0, 0, 0, 0, scale.y(), 0, 0, 0, 0, scale.z(), 0, 0, 0, 0, 1)
{}

Matrix4 &Matrix4::operator=(const Matrix4 &other)
{
    value = other.value;
    return *this;
}

Matrix4 &Matrix4::operator=(Matrix4 &&other)
{
    value = std::move(other.value);
    return *this;
}

Matrix4Col &Matrix4::operator[](uint32 colIndex) { return value[colIndex]; }

Matrix4Col Matrix4::operator[](uint32 colIndex) const { return value[colIndex]; }

Vector4D Matrix4::operator*(const Vector4D &transformingVector) const { return Vector4D(value * transformingVector.value); }

Vector3D Matrix4::operator*(const Vector3D &transformingVector) const
{
    Vector4D vector4D(transformingVector.x(), transformingVector.y(), transformingVector.z(), 1.0f);
    vector4D = (*this) * vector4D;
    return Vector3D(vector4D.x(), vector4D.y(), vector4D.z()) / vector4D.w();
}

Matrix4 Matrix4::operator*(const Matrix4 &b) const { return value * b.value; }

Matrix4 &Matrix4::operator*=(const Matrix4 &b)
{
    value *= b.value;
    return *this;
}

Matrix4 Matrix4::operator*(float scalar) const { return value * scalar; }

Matrix4 &Matrix4::operator*=(float scalar)
{
    value *= scalar;
    return *this;
}

Matrix4 Matrix4::inverse() const { return glm::inverse(value); }

float Matrix4::determinant() const { return glm::determinant(value); }

Matrix4 Matrix4::transpose() const { return glm::transpose(value); }

Matrix4 Matrix4::operator|(const Matrix4 &b) const { return glm::matrixCompMult(value, b.value); }

Matrix4 &Matrix4::operator|=(const Matrix4 &b)
{
    value = glm::matrixCompMult(value, b.value);
    return *this;
}

Matrix4 Matrix4::operator/(const Matrix4 &b) const { return value / b.value; }

Matrix4 &Matrix4::operator/=(const Matrix4 &b)
{
    value /= b.value;
    return *this;
}

Matrix4 Matrix4::operator/(float scalar) const { return Matrix4(value / scalar); }

Matrix4 &Matrix4::operator/=(float scalar)
{
    value /= scalar;
    return *this;
}

Matrix4 Matrix4::operator-(const Matrix4 &b) const { return (value - b.value); }

Matrix4 &Matrix4::operator-=(const Matrix4 &b)
{
    value -= b.value;
    return *this;
}

Matrix4 Matrix4::operator-(float scalar) const { return (value - scalar); }

Matrix4 &Matrix4::operator-=(float scalar)
{
    value -= scalar;
    return *this;
}

Matrix4 Matrix4::operator+(const Matrix4 &b) const { return (value + b.value); }

Matrix4 &Matrix4::operator+=(const Matrix4 &b)
{
    value += b.value;
    return *this;
}

Matrix4 Matrix4::operator+(float scalar) const { return (value + scalar); }

Matrix4 &Matrix4::operator+=(float scalar)
{
    value += scalar;
    return *this;
}

const Matrix4 Matrix4::IDENTITY{ Vector3D(1, 0), Vector3D(0, 1), Vector3D(0, 0, 1), Vector3D(0) };
