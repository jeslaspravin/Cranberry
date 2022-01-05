/*!
 * \file Vector2D.h
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
#include "Types/CoreDefines.h"
#include "ProgramCoreExports.h"

#include <glm/ext/vector_float2.hpp>

class Vector3D;

class PROGRAMCORE_EXPORT Vector2D
{
private:
    glm::vec2 value;
    // TODO(Jeslas) : To allow vector - matrix product - Remove once using native implemented vectors and matrices
    friend class Matrix2;
    friend Vector2D operator/(float n, const Vector2D& d);
    friend Vector2D operator-(float n, const Vector2D& d);
public:
    Vector2D();
    explicit Vector2D(const Matrix2Col& vector2d);
    Vector2D(const float& x, const float& y);
    explicit Vector2D(const float& allValue);
    Vector2D(const Vector2D& other);
    Vector2D(const Vector3D& other);
    Vector2D(Vector2D&& other);
    Vector2D& operator=(const Vector2D& other);
    Vector2D& operator=(Vector2D&& other);

    float& x();
    float& y();
    float x() const;
    float y() const;
    float operator[](uint32 index) const;
    float& operator[](uint32 index);

public:
    bool operator==(const Vector2D& b) const;
    float operator|(const Vector2D& b) const;
    float operator^(const Vector2D& b) const;
    // Component wise operations
    Vector2D operator*(const Vector2D& b) const;
    Vector2D& operator*=(const Vector2D& b);
    Vector2D operator/(const Vector2D& b) const;
    Vector2D& operator/=(const Vector2D& b);
    Vector2D operator-(const Vector2D& b) const;
    Vector2D& operator-=(const Vector2D& b);
    Vector2D operator+(const Vector2D& b) const;
    Vector2D& operator+=(const Vector2D& b);
    Vector2D operator*(const float& scalar) const;
    Vector2D& operator*=(const float& scalar);
    Vector2D operator/(const float& scalar) const;
    Vector2D& operator/=(const float& scalar);
    Vector2D operator-(const float& scalar) const;
    Vector2D& operator-=(const float& scalar);
    Vector2D operator+(const float& scalar) const;
    Vector2D& operator+=(const float& scalar);
    Vector2D operator-() const;


    bool isSame(const Vector2D& b, float epsilon = SMALL_EPSILON) const;
    Vector2D normalized() const;
    Vector2D safeNormalize(float threshold = SMALL_EPSILON) const;
    float length() const;
    float sqrlength() const;

    Vector2D projectTo(const Vector2D& b) const;
    Vector2D rejectFrom(const Vector2D& b) const;

//////////////////////////////////////////////////////////////////////////
//// Static functions
//////////////////////////////////////////////////////////////////////////
public:
    const static Vector2D RIGHT;
    const static Vector2D FWD;
    const static Vector2D ZERO;
    const static Vector2D ONE;

    static float dot(const Vector2D& a, const Vector2D& b);
    static float cross(const Vector2D& a, const Vector2D& b);

    static Vector2D clamp(const Vector2D& value, const Vector2D& min, const Vector2D& max);
    static Vector2D min(const Vector2D& a, const Vector2D& b);
    static Vector2D max(const Vector2D& a, const Vector2D& b);
    static Vector2D abs(const Vector2D& value);
    static Vector2D floor(const Vector2D& value);
    static Vector2D ceil(const Vector2D& value);
    static Vector2D round(const Vector2D& value);
    static Vector2D mod(const Vector2D& a, const Vector2D& b);
    static Vector2D mod(const Vector2D& a, const float& b);
    static Vector2D modf(Vector2D& wholePart, const Vector2D& value);
};

FORCE_INLINE Vector2D operator/(float n, const Vector2D& d)
{
    return Vector2D(n / d.value);
}

FORCE_INLINE Vector2D operator-(float n, const Vector2D& d)
{
    return Vector2D(n - d.value);
}

FORCE_INLINE Vector2D operator*(float n, const Vector2D& d)
{
    return d * n;
}

FORCE_INLINE Vector2D operator+(float n, const Vector2D& d)
{
    return d + n;
}
