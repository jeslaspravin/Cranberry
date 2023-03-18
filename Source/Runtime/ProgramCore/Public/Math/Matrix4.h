/*!
 * \file Matrix4.h
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

#include <glm/ext/matrix_float4x4.hpp>

GLM_HEADER_INCLUDES_END

class Vector3D;
class Vector4D;

class PROGRAMCORE_EXPORT Matrix4
{
private:
    glm::mat4 value;
    Matrix4(const glm::mat4 &matrix);

public:
    Matrix4();
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
    Matrix4(const Vector3D &c1, const Vector3D &c2, const Vector3D &c3, const Vector3D &c4, float c4w = 1.0f);
    Matrix4(const Vector4D &c1, const Vector4D &c2, const Vector4D &c3, const Vector4D &c4);
    // Scaling matrix should always be applied before rotation or translation to preserve volume
    Matrix4(const Vector3D &scale);
    Matrix4(const Matrix4 &other);
    Matrix4(Matrix4 &&other);
    Matrix4 &operator= (const Matrix4 &other);
    Matrix4 &operator= (Matrix4 &&other);

    Matrix4Col &operator[] (uint32 colIndex);
    Matrix4Col operator[] (uint32 colIndex) const;

public:
    Vector4D operator* (const Vector4D &transformingVector) const;
    Vector3D operator* (const Vector3D &transformingVector) const;
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