/*!
 * \file Matrix2.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Matrix2.h"
#include "Math/Vector2D.h"

Matrix2::Matrix2(const glm::mat2 &matrix)
    : value(matrix)
{}

Matrix2::Matrix2()
    : value(0)
{}

Matrix2::Matrix2(float allValue)
    : value(allValue)
{}

Matrix2::Matrix2(float c1x, float c1y, float c2x, float c2y)
    : value(c1x, c1y, c2x, c2y)
{}

Matrix2::Matrix2(const Vector2D &c1, const Vector2D &c2)
    : value(c1.x(), c1.y(), c2.x(), c2.y())
{}

Matrix2::Matrix2(const Matrix2 &other)
    : value(other.value)
{}

Matrix2::Matrix2(Matrix2 &&other)
    : value(std::move(other.value))
{}

Matrix2::Matrix2(const Vector2D &scale)
    : value(scale.x(), 0, 0, scale.y())
{}

Matrix2 &Matrix2::operator= (const Matrix2 &other)
{
    value = other.value;
    return *this;
}

Matrix2 &Matrix2::operator= (Matrix2 &&other)
{
    value = std::move(other.value);
    return *this;
}

Matrix2Col &Matrix2::operator[] (uint32 colIndex) { return value[colIndex]; }

Matrix2Col Matrix2::operator[] (uint32 colIndex) const { return value[colIndex]; }

Vector2D Matrix2::operator* (const Vector2D &transformingVector) const { return Vector2D(value * transformingVector.value); }

Matrix2 Matrix2::operator* (const Matrix2 &b) const { return value * b.value; }

Matrix2 &Matrix2::operator*= (const Matrix2 &b)
{
    value *= b.value;
    return *this;
}

Matrix2 Matrix2::inverse() const { return glm::inverse(value); }

float Matrix2::determinant() const { return glm::determinant(value); }

Matrix2 Matrix2::transpose() const { return glm::transpose(value); }

Matrix2 Matrix2::operator* (float scalar) const { return value * scalar; }

Matrix2 &Matrix2::operator*= (float scalar)
{
    value *= scalar;
    return *this;
}

Matrix2 Matrix2::operator| (const Matrix2 &b) const { return glm::matrixCompMult(value, b.value); }

Matrix2 &Matrix2::operator|= (const Matrix2 &b)
{
    value = glm::matrixCompMult(value, b.value);
    return *this;
}

Matrix2 Matrix2::operator/ (const Matrix2 &b) const { return value / b.value; }

Matrix2 &Matrix2::operator/= (const Matrix2 &b)
{
    value /= b.value;
    return *this;
}

Matrix2 Matrix2::operator/ (float scalar) const { return Matrix2(value / scalar); }

Matrix2 &Matrix2::operator/= (float scalar)
{
    value /= scalar;
    return *this;
}

Matrix2 Matrix2::operator- (const Matrix2 &b) const { return (value - b.value); }

Matrix2 &Matrix2::operator-= (const Matrix2 &b)
{
    value -= b.value;
    return *this;
}

Matrix2 Matrix2::operator- (float scalar) const { return (value - scalar); }

Matrix2 &Matrix2::operator-= (float scalar)
{
    value -= scalar;
    return *this;
}

Matrix2 Matrix2::operator- () const { return -value; }

Matrix2 Matrix2::operator+ (const Matrix2 &b) const { return (value + b.value); }

Matrix2 &Matrix2::operator+= (const Matrix2 &b)
{
    value += b.value;
    return *this;
}

Matrix2 Matrix2::operator+ (float scalar) const { return (value + scalar); }

Matrix2 &Matrix2::operator+= (float scalar)
{
    value += scalar;
    return *this;
}

const Matrix2 Matrix2::IDENTITY{ 1, 0, 0, 1 };
