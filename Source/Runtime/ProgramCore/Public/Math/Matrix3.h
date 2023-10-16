/*!
 * \file Matrix3.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/CoreMathTypedefs.h"
#include "ProgramCoreExports.h"

GLM_HEADER_INCLUDES_BEGIN

#include <glm/ext/matrix_float3x3.hpp>

GLM_HEADER_INCLUDES_END

class Vector3;

class PROGRAMCORE_EXPORT Matrix3
{
private:
    glm::mat3 value;
    Matrix3(const glm::mat3 &matrix);

public:
    Matrix3() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(Matrix3)

    explicit Matrix3(float allValue);
    Matrix3(const Vector3 &c1, const Vector3 &c2, const Vector3 &c3);
    Matrix3(float c1x, float c1y, float c1z, float c2x, float c2y, float c2z, float c3x, float c3y, float c3z);
    Matrix3(const Vector3 &scale);

    Matrix3Col &operator[] (uint32 colIndex);
    Matrix3Col operator[] (uint32 colIndex) const;

public:
    Vector3 operator* (const Vector3 &transformingVector) const;
    Matrix3 operator* (const Matrix3 &b) const;
    Matrix3 &operator*= (const Matrix3 &b);

    Matrix3 inverse() const;
    float determinant() const;
    Matrix3 transpose() const;

    // Component wise operations
    Matrix3 operator| (const Matrix3 &b) const;
    Matrix3 &operator|= (const Matrix3 &b);
    Matrix3 operator/ (const Matrix3 &b) const;
    Matrix3 &operator/= (const Matrix3 &b);
    Matrix3 operator- (const Matrix3 &b) const;
    Matrix3 &operator-= (const Matrix3 &b);
    Matrix3 operator+ (const Matrix3 &b) const;
    Matrix3 &operator+= (const Matrix3 &b);
    Matrix3 operator* (float scalar) const;
    Matrix3 &operator*= (float scalar);
    Matrix3 operator/ (float scalar) const;
    Matrix3 &operator/= (float scalar);
    Matrix3 operator- (float scalar) const;
    Matrix3 &operator-= (float scalar);
    Matrix3 operator+ (float scalar) const;
    Matrix3 &operator+= (float scalar);
    Matrix3 operator- () const;

public:
    static const Matrix3 IDENTITY;
};

//////////////////////////////////////////////////////////////////////////
/// Implementations
//////////////////////////////////////////////////////////////////////////

inline Matrix3::Matrix3(const glm::mat3 &matrix)
    : value(matrix)
{}
inline Matrix3::Matrix3(float allValue)
    : value(allValue)
{}

inline Matrix3Col &Matrix3::operator[] (uint32 colIndex) { return value[colIndex]; }
inline Matrix3Col Matrix3::operator[] (uint32 colIndex) const { return value[colIndex]; }

inline Matrix3 Matrix3::inverse() const { return glm::inverse(value); }
inline float Matrix3::determinant() const { return glm::determinant(value); }
inline Matrix3 Matrix3::transpose() const { return glm::transpose(value); }

inline Matrix3 Matrix3::operator* (const Matrix3 &b) const { return value * b.value; }
inline Matrix3 &Matrix3::operator*= (const Matrix3 &b)
{
    value *= b.value;
    return *this;
}

inline Matrix3 Matrix3::operator* (float scalar) const { return value * scalar; }
inline Matrix3 &Matrix3::operator*= (float scalar)
{
    value *= scalar;
    return *this;
}

inline Matrix3 Matrix3::operator| (const Matrix3 &b) const { return glm::matrixCompMult(value, b.value); }
inline Matrix3 &Matrix3::operator|= (const Matrix3 &b)
{
    value = glm::matrixCompMult(value, b.value);
    return *this;
}

inline Matrix3 Matrix3::operator/ (const Matrix3 &b) const { return value / b.value; }
inline Matrix3 &Matrix3::operator/= (const Matrix3 &b)
{
    value /= b.value;
    return *this;
}

inline Matrix3 Matrix3::operator/ (float scalar) const { return Matrix3(value / scalar); }
inline Matrix3 &Matrix3::operator/= (float scalar)
{
    value /= scalar;
    return *this;
}

inline Matrix3 Matrix3::operator- (const Matrix3 &b) const { return (value - b.value); }
inline Matrix3 &Matrix3::operator-= (const Matrix3 &b)
{
    value -= b.value;
    return *this;
}

inline Matrix3 Matrix3::operator- (float scalar) const { return (value - scalar); }
inline Matrix3 &Matrix3::operator-= (float scalar)
{
    value -= scalar;
    return *this;
}

inline Matrix3 Matrix3::operator- () const { return -value; }

inline Matrix3 Matrix3::operator+ (const Matrix3 &b) const { return (value + b.value); }
inline Matrix3 &Matrix3::operator+= (const Matrix3 &b)
{
    value += b.value;
    return *this;
}

inline Matrix3 Matrix3::operator+ (float scalar) const { return (value + scalar); }
inline Matrix3 &Matrix3::operator+= (float scalar)
{
    value += scalar;
    return *this;
}