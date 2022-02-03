/*!
 * \file Vector4D.h
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

#include <glm/ext/vector_float4.hpp>

class Vector3D;

class PROGRAMCORE_EXPORT Vector4D
{
private:
    glm::vec4 value;
    // TODO(Jeslas) : To allow vector - matrix product - Remove once using native implemented vectors and matrices
    friend class Matrix4;
    friend Vector4D operator/(float n, const Vector4D& d);
    friend Vector4D operator-(float n, const Vector4D& d);
public:
    Vector4D();
    Vector4D(const float& x, const float& y, const float& z, const float& w);
    explicit Vector4D(const Matrix4Col& vector4d);
    explicit Vector4D(const float& allValue);
    Vector4D(const Vector3D& xyz, const float& w);
    Vector4D(const Vector4D& other);
    Vector4D(Vector4D&& other);
    Vector4D& operator=(const Vector4D& other);
    Vector4D& operator=(Vector4D&& other);

    float& x();
    float& y();
    float& z();
    float& w();
    float x() const;
    float y() const;
    float z() const;
    float w() const;
    float operator[](uint32 index) const;
    float& operator[](uint32 index);

public:
    bool operator==(const Vector4D& b) const;
    float operator|(const Vector4D& b) const;
    // Component wise operations
    Vector4D operator*(const Vector4D& b) const;
    Vector4D& operator*=(const Vector4D& b);
    Vector4D operator/(const Vector4D& b) const;
    Vector4D& operator/=(const Vector4D& b);
    Vector4D operator-(const Vector4D& b) const;
    Vector4D& operator-=(const Vector4D& b);
    Vector4D operator+(const Vector4D& b) const;
    Vector4D& operator+=(const Vector4D& b);
    Vector4D operator*(const float& scalar) const;
    Vector4D& operator*=(const float& scalar);
    Vector4D operator/(const float& scalar) const;
    Vector4D& operator/=(const float& scalar);
    Vector4D operator-(const float& scalar) const;
    Vector4D& operator-=(const float& scalar);
    Vector4D operator+(const float& scalar) const;
    Vector4D& operator+=(const float& scalar);
    Vector4D operator-() const;
    bool isSame(const Vector4D& b, float epsilon = SMALL_EPSILON) const;
    bool isFinite() const;
    Vector4D normalized() const;
    Vector4D safeNormalize(float threshold = SMALL_EPSILON) const;
    float length() const;
    float sqrlength() const;

    Vector4D projectTo(const Vector4D& b) const;
    Vector4D rejectFrom(const Vector4D& b) const;

    //////////////////////////////////////////////////////////////////////////
    //// Static functions
    //////////////////////////////////////////////////////////////////////////
public:
    const static Vector4D ZERO;
    const static Vector4D ONE;

    static float dot(const Vector4D& a, const Vector4D& b);

    static Vector4D clamp(const Vector4D& value, const Vector4D& min, const Vector4D& max);
    static Vector4D min(const Vector4D& a, const Vector4D& b);
    static Vector4D max(const Vector4D& a, const Vector4D& b);
    static Vector4D abs(const Vector4D& value);
    static Vector4D floor(const Vector4D& value);
    static Vector4D ceil(const Vector4D& value);
    static Vector4D round(const Vector4D& value);
    static Vector4D mod(const Vector4D& a, const Vector4D& b);
    static Vector4D mod(const Vector4D& a, const float& b);
    static Vector4D modf(Vector4D& wholePart, const Vector4D& value);
};

FORCE_INLINE Vector4D operator/(float n, const Vector4D& d)
{
    return Vector4D(n / d.value);
}

FORCE_INLINE Vector4D operator-(float n, const Vector4D& d)
{
    return Vector4D(n - d.value);
}

FORCE_INLINE Vector4D operator*(float n, const Vector4D& d)
{
    return d * n;
}

FORCE_INLINE Vector4D operator+(float n, const Vector4D& d)
{
    return d + n;
}