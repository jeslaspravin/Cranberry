/*!
 * \file Vector3D.h
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

#include <glm/ext/vector_float3.hpp>

class Vector4D;
class Vector2D;

class PROGRAMCORE_EXPORT Vector3D
{
private:
    glm::vec3 value;
    // TODO(Jeslas) : To allow vector - matrix product - Remove once using native implemented vectors and matrices
    friend class Matrix3;
    friend Vector3D operator/(float n, const Vector3D& d);
    friend Vector3D operator-(float n, const Vector3D& d);
public:
    using value_type = glm::vec3::value_type;
public:
    Vector3D();
    explicit Vector3D(const Matrix3Col& vector3d);
    Vector3D(const float& x, const float& y, const float& z);
    Vector3D(const float& x, const float& y);
    explicit Vector3D(const float& allValue);
    Vector3D(const Vector2D& xy, const float& z);
    Vector3D(const Vector3D& other);
    // Construct from v4d x,y,z
    Vector3D(const Vector4D& other);
    Vector3D(Vector3D&& other);
    Vector3D& operator=(const Vector3D& other);
    Vector3D& operator=(Vector3D&& other);

    float& x();
    float& y();
    float& z();
    float x() const;
    float y() const;
    float z() const;
    float operator[](uint32 index) const;
    float& operator[](uint32 index);

public:
    bool operator==(const Vector3D& b) const;
    float operator|(const Vector3D& b) const;
    Vector3D operator^(const Vector3D& b) const;
    // Component wise operations
    Vector3D operator*(const Vector3D& b) const;
    Vector3D& operator*=(const Vector3D& b);
    Vector3D operator/(const Vector3D& b) const;
    Vector3D& operator/=(const Vector3D& b);
    Vector3D operator-(const Vector3D& b) const;
    Vector3D& operator-=(const Vector3D& b);
    Vector3D operator+(const Vector3D& b) const;
    Vector3D& operator+=(const Vector3D& b);
    Vector3D operator*(const float& scalar) const;
    Vector3D& operator*=(const float& scalar);
    Vector3D operator/(const float& scalar) const;
    Vector3D& operator/=(const float& scalar);
    Vector3D operator-(const float& scalar) const;
    Vector3D& operator-=(const float& scalar);
    Vector3D operator+(const float& scalar) const;
    Vector3D& operator+=(const float& scalar);
    Vector3D operator-() const;
    bool isSame(const Vector3D& b, float epsilon = SMALL_EPSILON) const;
    bool isFinite() const;
    Vector3D normalized() const;
    Vector3D safeNormalize(float threshold = SMALL_EPSILON) const;
    float length() const;
    float sqrlength() const;

    Vector3D projectTo(const Vector3D& b) const;
    Vector3D rejectFrom(const Vector3D& b) const;

    //////////////////////////////////////////////////////////////////////////
    //// Static functions
    //////////////////////////////////////////////////////////////////////////
public:
    const static Vector3D RIGHT;
    const static Vector3D FWD;
    const static Vector3D UP;
    const static Vector3D ZERO;
    const static Vector3D ONE;

    static float dot(const Vector3D& a, const Vector3D& b);
    static Vector3D cross(const Vector3D& a, const Vector3D& b);

    static Vector3D clamp(const Vector3D& value, const Vector3D& min, const Vector3D& max);
    static Vector3D min(const Vector3D& a, const Vector3D& b);
    static Vector3D max(const Vector3D& a, const Vector3D& b);
    static Vector3D abs(const Vector3D& value);
    static Vector3D floor(const Vector3D& value);
    static Vector3D ceil(const Vector3D& value);
    static Vector3D round(const Vector3D& value);
    static Vector3D mod(const Vector3D& a, const Vector3D& b);
    static Vector3D mod(const Vector3D& a, const float& b);
    static Vector3D modf(Vector3D& wholePart, const Vector3D& value);
};

FORCE_INLINE Vector3D operator/(float n, const Vector3D& d)
{
    return Vector3D(n / d.value);
}

FORCE_INLINE Vector3D operator-(float n, const Vector3D& d)
{
    return Vector3D(n - d.value);
}

FORCE_INLINE Vector3D operator*(float n, const Vector3D& d)
{
    return d * n;
}

FORCE_INLINE Vector3D operator+(float n, const Vector3D& d)
{
    return d + n;
}