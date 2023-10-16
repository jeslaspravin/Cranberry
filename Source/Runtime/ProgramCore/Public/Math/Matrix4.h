/*!
 * \file Matrix4.h
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

#include <glm/ext/matrix_float4x4.hpp>

GLM_HEADER_INCLUDES_END

class Vector3;
class Vector4;

class PROGRAMCORE_EXPORT Matrix4
{
private:
    glm::mat4 value{ 0 };
    Matrix4(const glm::mat4 &matrix);

public:
    Matrix4() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(Matrix4)

    explicit Matrix4(float allValue);

    //  Matrix arrangement
    //  0-3B    4-7B    8-11B   12-15B
    //  r0(c0)  r0(c1)  r0(c2)  r0(c3)
    //  r1(c0)  r1(c1)  r1(c2)  r1(c3)
    //  r2(c0)  r2(c1)  r2(c2)  r2(c3)
    //  r3(c0)  r3(c1)  r3(c2)  r3(c3)
    Matrix4(
        float c1x, float c1y, float c1z, float c1w, float c2x, float c2y, float c2z, float c2w, float c3x, float c3y, float c3z, float c3w,
        float c4x, float c4y, float c4z, float c4w
    );
    Matrix4(const Vector3 &c1, const Vector3 &c2, const Vector3 &c3, const Vector3 &c4, float c4w = 1.0f);
    Matrix4(const Vector4 &c1, const Vector4 &c2, const Vector4 &c3, const Vector4 &c4);
    // Scaling matrix should always be applied before rotation or translation to preserve volume
    Matrix4(const Vector3 &scale);

    Matrix4Col &operator[] (uint32 colIndex);
    Matrix4Col operator[] (uint32 colIndex) const;

public:
    Vector4 operator* (const Vector4 &transformingVector) const;
    Vector3 operator* (const Vector3 &transformingVector) const;
    Matrix4 operator* (const Matrix4 &b) const;
    Matrix4 &operator*= (const Matrix4 &b);

    Matrix4 inverse() const;
    float determinant() const;
    Matrix4 transpose() const;

    // Component wise operations
    Matrix4 operator| (const Matrix4 &b) const;
    Matrix4 &operator|= (const Matrix4 &b);
    Matrix4 operator/ (const Matrix4 &b) const;
    Matrix4 &operator/= (const Matrix4 &b);
    Matrix4 operator- (const Matrix4 &b) const;
    Matrix4 &operator-= (const Matrix4 &b);
    Matrix4 operator+ (const Matrix4 &b) const;
    Matrix4 &operator+= (const Matrix4 &b);
    Matrix4 operator* (float scalar) const;
    Matrix4 &operator*= (float scalar);
    Matrix4 operator/ (float scalar) const;
    Matrix4 &operator/= (float scalar);
    Matrix4 operator- (float scalar) const;
    Matrix4 &operator-= (float scalar);
    Matrix4 operator+ (float scalar) const;
    Matrix4 &operator+= (float scalar);

public:
    static const Matrix4 IDENTITY;
};

//////////////////////////////////////////////////////////////////////////
/// Implementations
//////////////////////////////////////////////////////////////////////////

inline Matrix4::Matrix4(const glm::mat4 &matrix)
    : value(matrix)
{}
inline Matrix4::Matrix4(float allValue)
    : value(allValue)
{}

inline Matrix4Col &Matrix4::operator[] (uint32 colIndex) { return value[colIndex]; }
inline Matrix4Col Matrix4::operator[] (uint32 colIndex) const { return value[colIndex]; }

inline Matrix4 Matrix4::inverse() const { return glm::inverse(value); }
inline float Matrix4::determinant() const { return glm::determinant(value); }
inline Matrix4 Matrix4::transpose() const { return glm::transpose(value); }

inline Matrix4 Matrix4::operator* (const Matrix4 &b) const { return value * b.value; }
inline Matrix4 &Matrix4::operator*= (const Matrix4 &b)
{
    value *= b.value;
    return *this;
}

inline Matrix4 Matrix4::operator* (float scalar) const { return value * scalar; }
inline Matrix4 &Matrix4::operator*= (float scalar)
{
    value *= scalar;
    return *this;
}

inline Matrix4 Matrix4::operator| (const Matrix4 &b) const { return glm::matrixCompMult(value, b.value); }
inline Matrix4 &Matrix4::operator|= (const Matrix4 &b)
{
    value = glm::matrixCompMult(value, b.value);
    return *this;
}

inline Matrix4 Matrix4::operator/ (const Matrix4 &b) const { return value / b.value; }
inline Matrix4 &Matrix4::operator/= (const Matrix4 &b)
{
    value /= b.value;
    return *this;
}

inline Matrix4 Matrix4::operator/ (float scalar) const { return Matrix4(value / scalar); }
inline Matrix4 &Matrix4::operator/= (float scalar)
{
    value /= scalar;
    return *this;
}

inline Matrix4 Matrix4::operator- (const Matrix4 &b) const { return (value - b.value); }
inline Matrix4 &Matrix4::operator-= (const Matrix4 &b)
{
    value -= b.value;
    return *this;
}

inline Matrix4 Matrix4::operator- (float scalar) const { return (value - scalar); }
inline Matrix4 &Matrix4::operator-= (float scalar)
{
    value -= scalar;
    return *this;
}

inline Matrix4 Matrix4::operator+ (const Matrix4 &b) const { return (value + b.value); }
inline Matrix4 &Matrix4::operator+= (const Matrix4 &b)
{
    value += b.value;
    return *this;
}

inline Matrix4 Matrix4::operator+ (float scalar) const { return (value + scalar); }
inline Matrix4 &Matrix4::operator+= (float scalar)
{
    value += scalar;
    return *this;
}