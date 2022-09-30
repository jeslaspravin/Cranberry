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

#include <glm/ext/matrix_float2x2.hpp>

class Vector2D;

class PROGRAMCORE_EXPORT Matrix2
{
private:
    glm::mat2 value;
    Matrix2(const glm::mat2 &matrix);

public:
    Matrix2();
    Matrix2(float allValue);
    Matrix2(float c1x, float c1y, float c2x, float c2y);
    Matrix2(const Vector2D &c1, const Vector2D &c2);
    Matrix2(const Vector2D &scale);
    Matrix2(const Matrix2 &other);
    Matrix2(Matrix2 &&other);
    Matrix2 &operator=(const Matrix2 &other);
    Matrix2 &operator=(Matrix2 &&other);

    Matrix2Col &operator[](uint32 colIndex);
    Matrix2Col operator[](uint32 colIndex) const;

public:
    Vector2D operator*(const Vector2D &transformingVector) const;
    Matrix2 operator*(const Matrix2 &b) const;
    Matrix2 &operator*=(const Matrix2 &b);

    Matrix2 inverse() const;
    float determinant() const;
    Matrix2 transpose() const;

    // Component wise operations
    Matrix2 operator|(const Matrix2 &b) const;
    Matrix2 &operator|=(const Matrix2 &b);
    Matrix2 operator/(const Matrix2 &b) const;
    Matrix2 &operator/=(const Matrix2 &b);
    Matrix2 operator-(const Matrix2 &b) const;
    Matrix2 &operator-=(const Matrix2 &b);
    Matrix2 operator+(const Matrix2 &b) const;
    Matrix2 &operator+=(const Matrix2 &b);
    Matrix2 operator*(float scalar) const;
    Matrix2 &operator*=(float scalar);
    Matrix2 operator/(float scalar) const;
    Matrix2 &operator/=(float scalar);
    Matrix2 operator-(float scalar) const;
    Matrix2 &operator-=(float scalar);
    Matrix2 operator+(float scalar) const;
    Matrix2 &operator+=(float scalar);
    Matrix2 operator-() const;

public:
    static const Matrix2 IDENTITY;
};