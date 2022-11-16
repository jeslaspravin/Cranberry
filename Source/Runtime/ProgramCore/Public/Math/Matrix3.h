/*!
 * \file Matrix3.h
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

#include <glm/ext/matrix_float3x3.hpp>

GLM_HEADER_INCLUDES_END

class Vector3D;

class PROGRAMCORE_EXPORT Matrix3
{
private:
    glm::mat3 value;
    Matrix3(const glm::mat3 &matrix);

public:
    Matrix3();
    explicit Matrix3(float allValue);
    Matrix3(const Vector3D &c1, const Vector3D &c2, const Vector3D &c3);
    Matrix3(float c1x, float c1y, float c1z, float c2x, float c2y, float c2z, float c3x, float c3y, float c3z);
    Matrix3(const Matrix3 &other);
    Matrix3(Matrix3 &&other);
    Matrix3(const Vector3D &scale);
    Matrix3 &operator=(const Matrix3 &other);
    Matrix3 &operator=(Matrix3 &&other);

    Matrix3Col &operator[](uint32 colIndex);
    Matrix3Col operator[](uint32 colIndex) const;

public:
    Vector3D operator*(const Vector3D &transformingVector) const;
    Matrix3 operator*(const Matrix3 &b) const;
    Matrix3 &operator*=(const Matrix3 &b);

    Matrix3 inverse() const;
    float determinant() const;
    Matrix3 transpose() const;

    // Component wise operations
    Matrix3 operator|(const Matrix3 &b) const;
    Matrix3 &operator|=(const Matrix3 &b);
    Matrix3 operator/(const Matrix3 &b) const;
    Matrix3 &operator/=(const Matrix3 &b);
    Matrix3 operator-(const Matrix3 &b) const;
    Matrix3 &operator-=(const Matrix3 &b);
    Matrix3 operator+(const Matrix3 &b) const;
    Matrix3 &operator+=(const Matrix3 &b);
    Matrix3 operator*(float scalar) const;
    Matrix3 &operator*=(float scalar);
    Matrix3 operator/(float scalar) const;
    Matrix3 &operator/=(float scalar);
    Matrix3 operator-(float scalar) const;
    Matrix3 &operator-=(float scalar);
    Matrix3 operator+(float scalar) const;
    Matrix3 &operator+=(float scalar);
    Matrix3 operator-() const;

public:
    static const Matrix3 IDENTITY;
};