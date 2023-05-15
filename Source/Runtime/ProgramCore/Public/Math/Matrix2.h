/*!
 * \file Matrix2.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/CoreMathTypedefs.h"
#include "ProgramCoreExports.h"

GLM_HEADER_INCLUDES_BEGIN

#include <glm/ext/matrix_float2x2.hpp>

GLM_HEADER_INCLUDES_END

class Vector2;

class PROGRAMCORE_EXPORT Matrix2
{
private:
    glm::mat2 value;
    Matrix2(const glm::mat2 &matrix);

public:
    Matrix2() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(Matrix2)

    Matrix2(float allValue);
    Matrix2(float c1x, float c1y, float c2x, float c2y);
    Matrix2(const Vector2 &c1, const Vector2 &c2);
    Matrix2(const Vector2 &scale);

    Matrix2Col &operator[] (uint32 colIndex);
    Matrix2Col operator[] (uint32 colIndex) const;

public:
    Vector2 operator* (const Vector2 &transformingVector) const;
    Matrix2 operator* (const Matrix2 &b) const;
    Matrix2 &operator*= (const Matrix2 &b);

    Matrix2 inverse() const;
    float determinant() const;
    Matrix2 transpose() const;

    // Component wise operations
    Matrix2 operator| (const Matrix2 &b) const;
    Matrix2 &operator|= (const Matrix2 &b);
    Matrix2 operator/ (const Matrix2 &b) const;
    Matrix2 &operator/= (const Matrix2 &b);
    Matrix2 operator- (const Matrix2 &b) const;
    Matrix2 &operator-= (const Matrix2 &b);
    Matrix2 operator+ (const Matrix2 &b) const;
    Matrix2 &operator+= (const Matrix2 &b);
    Matrix2 operator* (float scalar) const;
    Matrix2 &operator*= (float scalar);
    Matrix2 operator/ (float scalar) const;
    Matrix2 &operator/= (float scalar);
    Matrix2 operator- (float scalar) const;
    Matrix2 &operator-= (float scalar);
    Matrix2 operator+ (float scalar) const;
    Matrix2 &operator+= (float scalar);
    Matrix2 operator- () const;

public:
    static const Matrix2 IDENTITY;
};

//////////////////////////////////////////////////////////////////////////
/// Implementations
//////////////////////////////////////////////////////////////////////////

inline Matrix2::Matrix2(const glm::mat2 &matrix)
    : value(matrix)
{}
inline Matrix2::Matrix2(float allValue)
    : value(allValue)
{}
inline Matrix2::Matrix2(float c1x, float c1y, float c2x, float c2y)
    : value(c1x, c1y, c2x, c2y)
{}

inline Matrix2Col &Matrix2::operator[] (uint32 colIndex) { return value[colIndex]; }
inline Matrix2Col Matrix2::operator[] (uint32 colIndex) const { return value[colIndex]; }

inline Matrix2 Matrix2::inverse() const { return glm::inverse(value); }
inline float Matrix2::determinant() const { return glm::determinant(value); }
inline Matrix2 Matrix2::transpose() const { return glm::transpose(value); }

inline Matrix2 Matrix2::operator* (const Matrix2 &b) const { return value * b.value; }
inline Matrix2 &Matrix2::operator*= (const Matrix2 &b)
{
    value *= b.value;
    return *this;
}

inline Matrix2 Matrix2::operator* (float scalar) const { return value * scalar; }
inline Matrix2 &Matrix2::operator*= (float scalar)
{
    value *= scalar;
    return *this;
}

inline Matrix2 Matrix2::operator| (const Matrix2 &b) const { return glm::matrixCompMult(value, b.value); }
inline Matrix2 &Matrix2::operator|= (const Matrix2 &b)
{
    value = glm::matrixCompMult(value, b.value);
    return *this;
}

inline Matrix2 Matrix2::operator/ (const Matrix2 &b) const { return value / b.value; }
inline Matrix2 &Matrix2::operator/= (const Matrix2 &b)
{
    value /= b.value;
    return *this;
}

inline Matrix2 Matrix2::operator/ (float scalar) const { return Matrix2(value / scalar); }
inline Matrix2 &Matrix2::operator/= (float scalar)
{
    value /= scalar;
    return *this;
}

inline Matrix2 Matrix2::operator- (const Matrix2 &b) const { return (value - b.value); }
inline Matrix2 &Matrix2::operator-= (const Matrix2 &b)
{
    value -= b.value;
    return *this;
}

inline Matrix2 Matrix2::operator- (float scalar) const { return (value - scalar); }
inline Matrix2 &Matrix2::operator-= (float scalar)
{
    value -= scalar;
    return *this;
}

inline Matrix2 Matrix2::operator- () const { return -value; }

inline Matrix2 Matrix2::operator+ (const Matrix2 &b) const { return (value + b.value); }
inline Matrix2 &Matrix2::operator+= (const Matrix2 &b)
{
    value += b.value;
    return *this;
}

inline Matrix2 Matrix2::operator+ (float scalar) const { return (value + scalar); }
inline Matrix2 &Matrix2::operator+= (float scalar)
{
    value += scalar;
    return *this;
}