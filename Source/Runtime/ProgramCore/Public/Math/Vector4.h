/*!
 * \file Vector4.h
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
#include "Types/CoreDefines.h"

GLM_HEADER_INCLUDES_BEGIN

#include <glm/ext/vector_float4.hpp>

GLM_HEADER_INCLUDES_BEGIN

class Vector3;

class PROGRAMCORE_EXPORT Vector4
{
private:
    glm::vec4 value;
    // TODO(Jeslas) : To allow vector - matrix product - Remove once using native implemented vectors and
    // matrices
    friend class Matrix4;
    friend Vector4 operator/ (float n, const Vector4 &d);
    friend Vector4 operator- (float n, const Vector4 &d);

public:
    using value_type = glm::vec4::value_type;

public:
    Vector4();
    MAKE_TYPE_DEFAULT_COPY_MOVE(Vector4)
    Vector4(float x, float y, float z, float w);
    explicit Vector4(const Matrix4Col &Vector4);
    explicit Vector4(float allValue);
    Vector4(const Vector3 &xyz, float w);

    float &x();
    float &y();
    float &z();
    float &w();
    float x() const;
    float y() const;
    float z() const;
    float w() const;
    float operator[] (uint32 index) const;
    float &operator[] (uint32 index);

public:
    bool operator== (const Vector4 &b) const;
    float operator| (const Vector4 &b) const;
    // Component wise operations
    Vector4 operator* (const Vector4 &b) const;
    Vector4 &operator*= (const Vector4 &b);
    Vector4 operator/ (const Vector4 &b) const;
    Vector4 &operator/= (const Vector4 &b);
    Vector4 operator- (const Vector4 &b) const;
    Vector4 &operator-= (const Vector4 &b);
    Vector4 operator+ (const Vector4 &b) const;
    Vector4 &operator+= (const Vector4 &b);
    Vector4 operator* (float scalar) const;
    Vector4 &operator*= (float scalar);
    Vector4 operator/ (float scalar) const;
    Vector4 &operator/= (float scalar);
    Vector4 operator- (float scalar) const;
    Vector4 &operator-= (float scalar);
    Vector4 operator+ (float scalar) const;
    Vector4 &operator+= (float scalar);
    Vector4 operator- () const;
    bool isSame(const Vector4 &b, float epsilon = SMALL_EPSILON) const;
    bool isFinite() const;
    Vector4 safeInverse() const;
    Vector4 normalized() const;
    Vector4 safeNormalized(float threshold = SMALL_EPSILON) const;
    float length() const;
    float sqrlength() const;

    Vector4 projectTo(const Vector4 &b) const;
    Vector4 rejectFrom(const Vector4 &b) const;

    //////////////////////////////////////////////////////////////////////////
    //// Static functions
    //////////////////////////////////////////////////////////////////////////
public:
    const static Vector4 ZERO;
    const static Vector4 ONE;

    static float dot(const Vector4 &a, const Vector4 &b);

    static Vector4 clamp(const Vector4 &value, const Vector4 &min, const Vector4 &max);
    static Vector4 min(const Vector4 &a, const Vector4 &b);
    static Vector4 max(const Vector4 &a, const Vector4 &b);
    static Vector4 abs(const Vector4 &value);
    static Vector4 floor(const Vector4 &value);
    static Vector4 ceil(const Vector4 &value);
    static Vector4 round(const Vector4 &value);
    static Vector4 mod(const Vector4 &a, const Vector4 &b);
    static Vector4 mod(const Vector4 &a, float b);
    static Vector4 modf(Vector4 &wholePart, const Vector4 &value);
};

FORCE_INLINE Vector4 operator/ (float n, const Vector4 &d) { return Vector4(n / d.value); }

FORCE_INLINE Vector4 operator- (float n, const Vector4 &d) { return Vector4(n - d.value); }

FORCE_INLINE Vector4 operator* (float n, const Vector4 &d) { return d * n; }

FORCE_INLINE Vector4 operator+ (float n, const Vector4 &d) { return d + n; }