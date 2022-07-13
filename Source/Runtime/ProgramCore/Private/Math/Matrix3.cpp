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
#include "Math/Vector3D.h"

Matrix3::Matrix3(const glm::mat3 &matrix)
    : value(matrix)
{}

Matrix3::Matrix3()
    : value(0)
{}

Matrix3::Matrix3(float allValue)
    : value(allValue)
{}

Matrix3::Matrix3(const Vector3D &c1, const Vector3D &c2, const Vector3D &c3)
    : value(c1.x(), c1.y(), c1.z(), c2.x(), c2.y(), c2.z(), c3.x(), c3.y(), c3.z())
{}

Matrix3::Matrix3(float c1x, float c1y, float c1z, float c2x, float c2y, float c2z, float c3x, float c3y, float c3z)
    : value(c1x, c1y, c1z, c2x, c2y, c2z, c3x, c3y, c3z)
{}

Matrix3::Matrix3(const Matrix3 &other)
    : value(other.value)
{}

Matrix3::Matrix3(Matrix3 &&other)
    : value(std::move(other.value))
{}

Matrix3::Matrix3(const Vector3D &scale)
    : value(scale.x(), 0, 0, 0, scale.y(), 0, 0, 0, scale.z())
{}

Matrix3 &Matrix3::operator=(const Matrix3 &other)
{
    value = other.value;
    return *this;
}

Matrix3 &Matrix3::operator=(Matrix3 &&other)
{
    value = std::move(other.value);
    return *this;
}

Matrix3Col &Matrix3::operator[](uint32 colIndex) { return value[colIndex]; }

Matrix3Col Matrix3::operator[](uint32 colIndex) const { return value[colIndex]; }

Vector3D Matrix3::operator*(const Vector3D &transformingVector) const { return Vector3D(value * transformingVector.value); }

Matrix3 Matrix3::operator*(const Matrix3 &b) const { return value * b.value; }

Matrix3 &Matrix3::operator*=(const Matrix3 &b)
{
    value *= b.value;
    return *this;
}

Matrix3 Matrix3::operator*(float scalar) const { return value * scalar; }

Matrix3 &Matrix3::operator*=(float scalar)
{
    value *= scalar;
    return *this;
}

Matrix3 Matrix3::inverse() const { return glm::inverse(value); }

float Matrix3::determinant() const { return glm::determinant(value); }

Matrix3 Matrix3::transpose() const { return glm::transpose(value); }

Matrix3 Matrix3::operator|(const Matrix3 &b) const { return glm::matrixCompMult(value, b.value); }

Matrix3 &Matrix3::operator|=(const Matrix3 &b)
{
    value = glm::matrixCompMult(value, b.value);
    return *this;
}

Matrix3 Matrix3::operator/(const Matrix3 &b) const { return value / b.value; }

Matrix3 &Matrix3::operator/=(const Matrix3 &b)
{
    value /= b.value;
    return *this;
}

Matrix3 Matrix3::operator/(float scalar) const { return Matrix3(value / scalar); }

Matrix3 &Matrix3::operator/=(float scalar)
{
    value /= scalar;
    return *this;
}

Matrix3 Matrix3::operator-(const Matrix3 &b) const { return (value - b.value); }

Matrix3 &Matrix3::operator-=(const Matrix3 &b)
{
    value -= b.value;
    return *this;
}

Matrix3 Matrix3::operator-(float scalar) const { return (value - scalar); }

Matrix3 &Matrix3::operator-=(float scalar)
{
    value -= scalar;
    return *this;
}

Matrix3 Matrix3::operator-() const { return -value; }

Matrix3 Matrix3::operator+(const Matrix3 &b) const { return (value + b.value); }

Matrix3 &Matrix3::operator+=(const Matrix3 &b)
{
    value += b.value;
    return *this;
}

Matrix3 Matrix3::operator+(float scalar) const { return (value + scalar); }

Matrix3 &Matrix3::operator+=(float scalar)
{
    value += scalar;
    return *this;
}

const Matrix3 Matrix3::IDENTITY{ Vector3D(1, 0), Vector3D(0, 1), Vector3D(0, 0, 1) };
